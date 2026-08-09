import os
import re
import sys


def clean_symbol_name(label):
    # Extract human-readable variable symbol from linker labels
    m = re.search(r'(?:bss|data)\.(?:far|near)\.([^.\s]+)', label)
    if m:
        return m.group(1)
    m2 = re.search(r'([^\s()]+)\.o\(([^)]+)\)', label)
    if m2:
        objfile, sec = m2.groups()
        return f"{sec} ({objfile})"
    return label


def categorize_ram_item(label, name):
    lbl_lower = label.lower()
    name_lower = name.lower()

    if any(k in name_lower or k in lbl_lower for k in ['obj_', 'routines_player', 'routines_enemy', 'routines_boss', 'hittest']):
        return 'Object Engine & Hitboxes'
    elif any(k in name_lower or k in lbl_lower for k in ['spr_', 'shadow_oam', 'metaspr']):
        return 'Sprite Engine & OAM'
    elif any(k in name_lower or k in lbl_lower for k in ['cgram', 'hdma_', 'gfx_', 'pal_']):
        return 'Graphics & HDMA Buffers'
    elif any(k in name_lower or k in lbl_lower for k in ['map_', 'level_', 'bg_scroll']):
        return 'Maps & Level Buffers'
    elif any(k in name_lower or k in lbl_lower for k in ['ui_', 'subscreen_', 'vwf_']):
        return 'UI & Text Engine'
    elif any(k in name_lower or k in lbl_lower for k in ['snd_', 'audio']):
        return 'Sound & Audio Engine'
    else:
        return 'System & Other RAM'


def parse_mapfile(map_path, verbose=False):
    if not os.path.exists(map_path):
        return False

    with open(map_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    in_section_mapping = False
    sections = []
    current_sec = None
    stack_bytes = 1024  # Default 1 KB stack

    for line in lines:
        # Check for STACKLEN definition (e.g. "  0x000400 STACKLEN: global abs, size 0")
        m_stack = re.search(r'0x([0-9a-fA-F]+)\s+(?:STACKLEN|___stacklen)', line)
        if m_stack:
            stack_bytes = int(m_stack.group(1), 16)

        if 'Section mapping' in line:
            in_section_mapping = True
            continue
        if not in_section_mapping:
            continue

        # Header line: "  00000000 xrom  (size 8000, allocated 7fff)"
        m = re.match(r'^\s*([0-9a-fA-F]+)\s+(\S+)\s+\(size\s+([0-9a-fA-F]+)(?:,\s+allocated\s+([0-9a-fA-F]+))?\)', line)
        if m:
            offset, sec_name, size, allocated = m.groups()
            sections.append({
                'name': sec_name,
                'offset': int(offset, 16),
                'size': int(size, 16),
                'allocated': int(allocated, 16) if allocated else int(size, 16),
                'items': []
            })
            current_sec = sections[-1]
            continue

        # Item line: "           00c00000 - 00c00009 INITEXIT(.ctors)"
        m_item = re.match(r'^\s*([0-9a-fA-F]{6,8})\s*-\s*([0-9a-fA-F]{6,8})\s+(.*)', line)
        if m_item and current_sec:
            start_addr, end_addr, label = m_item.groups()
            current_sec['items'].append({
                'start': int(start_addr, 16),
                'end': int(end_addr, 16),
                'label': label.strip()
            })

    if not sections:
        return False

    rom_sections = {'xrom', 'nrom', 'header', 'vectors', 'from', 'hrom'}
    ram_sections = {'ndata', 'nbss', 'fdata', 'fbss', 'hdata', 'hbss', 'zpage'}

    total_rom_capacity = 512 * 1024  # 512 KB HiROM target
    num_banks = 8
    bank_size = 64 * 1024

    # Track usage per bank (0x00 to 0x07, mapped from addresses 0xC00000-0xC7FFFF)
    bank_max_addr = [0] * num_banks
    bank_min_addr = [0xffffffff] * num_banks
    bank_used_bytes = [0] * num_banks

    category_sizes = {
        'Code / Logic': 0,
        'Graphics (Tiles/Palettes)': 0,
        'Audio (Engine/Samples/Seq)': 0,
        'Maps / Level Data': 0,
        'System / Header / Vectors': 0,
        'Other / Uncategorized': 0,
    }

    total_rom_used = 0

    for sec in sections:
        if sec['name'] in rom_sections:
            sec_used = 0
            for item in sec['items']:
                item_size = item['end'] - item['start']
                sec_used += item_size
                label = item['label']

                # Categorize item
                if 'header' in label or 'vectors' in label or 'INITEXIT' in label or 'startup' in label:
                    category_sizes['System / Header / Vectors'] += item_size
                elif 'bindata.sprtiles' in label or 'bindata.bgtiles' in label or 'bindata.uitiles' in label or 'bindata.palette' in label or 'bindata.vwf' in label or 'const_pal' in label:
                    category_sizes['Graphics (Tiles/Palettes)'] += item_size
                elif 'bindata.overviewmaps' in label or 'data_map_' in label or 'MapSystem' in label:
                    category_sizes['Maps / Level Data'] += item_size
                elif 'audio' in label or 'SoundInterface' in label or 'data_seq_' in label or 'data_snd_' in label:
                    category_sizes['Audio (Engine/Samples/Seq)'] += item_size
                elif '_text.' in label or 'text.far' in label or 'text.near' in label:
                    category_sizes['Code / Logic'] += item_size
                else:
                    category_sizes['Other / Uncategorized'] += item_size

                # Map 24-bit SNES address to bank index
                # ROM addresses in HiROM start at 0xC00000
                addr_start = item['start']
                addr_end = item['end']

                start_bank = ((addr_start >> 16) & 0x7f)
                if start_bank >= 0x40:
                    start_bank -= 0x40  # 0xC0 -> 0, 0xC1 -> 1, etc.

                end_bank = (((addr_end - 1) >> 16) & 0x7f) if addr_end > addr_start else start_bank
                if end_bank >= 0x40:
                    end_bank -= 0x40

                for b in range(start_bank, end_bank + 1):
                    if 0 <= b < num_banks:
                        b_start = max(addr_start, 0xc00000 + b * bank_size)
                        b_end = min(addr_end, 0xc00000 + (b + 1) * bank_size)
                        if b_end > b_start:
                            b_bytes = b_end - b_start
                            bank_used_bytes[b] += b_bytes
                            if b_start < bank_min_addr[b]:
                                bank_min_addr[b] = b_start
                            if b_end > bank_max_addr[b]:
                                bank_max_addr[b] = b_end

            total_rom_used += sec_used

    # --- Process RAM Sections ---
    ram_item_list = []
    ram_categories = {
        'Object Engine & Hitboxes': 0,
        'Sprite Engine & OAM': 0,
        'Graphics & HDMA Buffers': 0,
        'Maps & Level Buffers': 0,
        'UI & Text Engine': 0,
        'Sound & Audio Engine': 0,
        'System & Other RAM': 0,
    }

    dp_capacity = 256  # Direct Page (0x0000 - 0x00FF)
    raw_near_capacity = 8192 - 256  # 0x0100 to 0x1FFF = 7936 bytes (7.75 KB)
    usable_near_capacity = max(0, raw_near_capacity - stack_bytes)
    bank7e_wram_capacity = 56 * 1024  # 56 KB (57,344 bytes) for 0x7E2000 - 0x7EFFFF

    dp_used_bytes = 0
    dp_max_end = 0
    near_max_end = 0
    bank7e_max_end = 0
    bank7f_max_end = 0

    section_ram_summary = {}

    for sec in sections:
        if sec['name'] in ram_sections:
            sec_name = sec['name']
            sec_size = sec['size']
            section_ram_summary[sec_name] = {
                'size': sec_size,
                'offset': sec['offset']
            }

            if sec_name == 'zpage':
                dp_used_bytes = max(dp_used_bytes, sec_size)

            for item in sec['items']:
                item_size = item['end'] - item['start']
                start_addr = item['start']
                end_addr = item['end']
                label = item['label']
                sym_name = clean_symbol_name(label)
                category = categorize_ram_item(label, sym_name)
                ram_categories[category] += item_size

                if sec_name == 'zpage' or (start_addr < 0x100 and sec_name in {'ndata', 'nbss'}):
                    region = 'Direct Page'
                    dp_max_end = max(dp_max_end, end_addr)
                elif sec_name in {'ndata', 'nbss'} or (0x100 <= start_addr < 0x2000):
                    region = 'Near WRAM'
                    near_max_end = max(near_max_end, end_addr)
                elif (0x7e0000 <= start_addr < 0x7f0000) or sec_name in {'fdata', 'fbss'}:
                    region = 'Far WRAM (7E)'
                    bank7e_max_end = max(bank7e_max_end, end_addr)
                elif start_addr >= 0x7f0000 or sec_name in {'hdata', 'hbss'}:
                    region = 'Huge WRAM (7F)'
                    bank7f_max_end = max(bank7f_max_end, end_addr)
                else:
                    region = f"RAM ({sec_name})"

                ram_item_list.append({
                    'name': sym_name,
                    'size': item_size,
                    'start': start_addr,
                    'end': end_addr,
                    'label': label,
                    'section': sec_name,
                    'region': region
                })

    if dp_max_end > 0:
        dp_used_bytes = max(dp_used_bytes, dp_max_end)
    elif 'zpage' in section_ram_summary:
        dp_used_bytes = section_ram_summary['zpage']['size']

    near_used_bytes = max(0, near_max_end - 0x0100) if near_max_end >= 0x0100 else 0
    if near_used_bytes == 0 and ('ndata' in section_ram_summary or 'nbss' in section_ram_summary):
        near_used_bytes = section_ram_summary.get('ndata', {}).get('size', 0) + \
                            section_ram_summary.get('nbss', {}).get('size', 0)

    bank7e_used_bytes = max(0, bank7e_max_end - 0x7e2000) if bank7e_max_end >= 0x7e2000 else 0
    if bank7e_used_bytes == 0 and ('fdata' in section_ram_summary or 'fbss' in section_ram_summary):
        bank7e_used_bytes = section_ram_summary.get('fdata', {}).get('size', 0) + \
                            section_ram_summary.get('fbss', {}).get('size', 0)

    # --- ROM Display Output ---
    print(f"--- ROM Size Analysis (via Linker Mapfile: {os.path.basename(map_path)}) ---")
    print(f"Total True ROM Used : {total_rom_used:6d} bytes / {total_rom_capacity:6d} bytes ({total_rom_used / 1024:.2f} KB / {total_rom_capacity // 1024} KB)")
    print(f"Total Free ROM Space: {total_rom_capacity - total_rom_used:6d} bytes ({(total_rom_capacity - total_rom_used) / 1024:.2f} KB free, {(total_rom_used / total_rom_capacity) * 100:.1f}% capacity used)")

    print("\n--- Per-Bank ROM Usage Breakdown ---")
    global_max_free = 0
    global_max_free_bank = 0

    for b in range(num_banks):
        used = bank_used_bytes[b]
        free = bank_size - used
        if free > global_max_free:
            global_max_free = free
            global_max_free_bank = b

        bank_addr_str = f"0x{0xc0 + b:02X}"
        if verbose:
            print(f"Bank {b} ({bank_addr_str}): Used {used:5d} / {bank_size} bytes ({used / 1024:5.2f} KB) | Free: {free:5d} bytes ({free / 1024:5.2f} KB)")
        else:
            pct = (used / bank_size) * 100
            print(f"  Bank {b} ({bank_addr_str}): Used {used:5d} bytes ({pct:5.1f}%) | Free: {free:5d} bytes at end")

    print(f"\nLargest contiguous free bank: Bank {global_max_free_bank} (0x{0xc0 + global_max_free_bank:02X}) with {global_max_free} bytes ({global_max_free / 1024:.2f} KB) free")

    if verbose:
        print("\n--- ROM Asset & Code Category Breakdown ---")
        for cat, sz in category_sizes.items():
            if sz > 0:
                print(f"  {cat:<30s}: {sz:6d} bytes ({sz / 1024:6.2f} KB)")

    # --- RAM Display Output ---
    print("\n--- Direct Page & Work RAM Size Analysis ---")

    dp_pct = (dp_used_bytes / dp_capacity) * 100 if dp_capacity else 0
    dp_free = dp_capacity - dp_used_bytes
    print("Direct Page (DP / Bank $00 0x0000-0x00FF):")
    print(f"  Statically Allocated : {dp_used_bytes:6d} / {dp_capacity:6d} bytes ({dp_used_bytes / 1024:.2f} KB / {dp_capacity / 1024:.2f} KB)")
    print(f"  Free DP Space        : {dp_free:6d} bytes ({dp_pct:.1f}% used, {100.0 - dp_pct:.1f}% free)")

    print(f"\nStack Allocation (Linker STACKLEN):")
    print(f"  Allocated Stack      : {stack_bytes:6d} bytes ({stack_bytes / 1024:.2f} KB)")

    near_pct = (near_used_bytes / usable_near_capacity) * 100 if usable_near_capacity else 0
    near_free = usable_near_capacity - near_used_bytes
    print("\nNear Work RAM (Bank $00 Low 8KB 0x0100-0x1FFF minus Stack):")
    print(f"  High-Water Mark      : 0x{near_max_end:04X} ({near_used_bytes:5d} / {usable_near_capacity:5d} bytes | {near_used_bytes / 1024:.2f} KB / {usable_near_capacity / 1024:.2f} KB usable)")
    print(f"  Free Near WRAM       : {near_free:5d} bytes ({near_pct:.1f}% used, {100.0 - near_pct:.1f}% free)")

    b7e_pct = (bank7e_used_bytes / bank7e_wram_capacity) * 100 if bank7e_wram_capacity else 0
    b7e_free = bank7e_wram_capacity - bank7e_used_bytes
    print("\nWork RAM Bank $7E (56 KB Target Budget 0x7E2000-0x7EFFFF):")
    print(f"  High-Water Mark      : 0x{bank7e_max_end if bank7e_max_end >= 0x7e2000 else 0x7e2000:06X} ({bank7e_used_bytes:5d} / {bank7e_wram_capacity:5d} bytes | {bank7e_used_bytes / 1024:.2f} KB / {bank7e_wram_capacity // 1024:.2f} KB)")
    print(f"  Free Bank $7E WRAM   : {b7e_free:5d} bytes ({b7e_pct:.1f}% used, {100.0 - b7e_pct:.1f}% free)")

    if bank7f_max_end >= 0x7f0000:
        b7f_used = bank7f_max_end - 0x7f0000
        print("\nExtended Work RAM Bank $7F:")
        print(f"  High-Water Mark      : 0x{bank7f_max_end:06X} ({b7f_used:5d} / 65536 bytes | {b7f_used / 1024:.2f} KB)")

    # Sort RAM items by size descending
    sorted_ram_items = sorted(ram_item_list, key=lambda x: x['size'], reverse=True)

    top_count = 10 if not verbose else 15
    if sorted_ram_items:
        print(f"\n--- Top {min(top_count, len(sorted_ram_items))} Statically Allocated RAM Variables ---")
        for idx, item in enumerate(sorted_ram_items[:top_count], 1):
            addr_str = f"0x{item['start']:06X}-0x{item['end']:06X}" if item['start'] >= 0x10000 else f"0x{item['start']:04X}-0x{item['end']:04X}"
            print(f"  {idx:2d}. {item['name']:<32s}: {item['size']:5d} bytes ({item['size'] / 1024:5.2f} KB) [{item['region']} {addr_str}]")

    if verbose:
        print("\n--- RAM Section Breakdown ---")
        for sec_name, sinfo in section_ram_summary.items():
            print(f"  Section {sec_name:<8s}: {sinfo['size']:5d} bytes (Offset: 0x{sinfo['offset']:06X})")

        print("\n--- RAM Subsystem Category Breakdown ---")
        for cat, sz in ram_categories.items():
            if sz > 0:
                print(f"  {cat:<30s}: {sz:5d} bytes ({sz / 1024:5.2f} KB)")

    return True


def analyze_rom_fallback(rom_path, verbose=False):
    if not os.path.exists(rom_path):
        print(f"Error: ROM file '{rom_path}' not found.")
        return

    rom_size = os.path.getsize(rom_path)
    with open(rom_path, 'rb') as f:
        rom_data = f.read()

    block_size = 64 * 1024
    num_blocks = len(rom_data) // block_size

    total_used_zeros = 0
    bank_details = []
    global_max_free = 0
    global_max_free_bank = 0

    for i in range(num_blocks):
        block = rom_data[i * block_size : (i + 1) * block_size]

        # Count contiguous 0x00 bytes from the end of the block
        zeros_at_end = 0
        for byte in reversed(block):
            if byte == 0x00:
                zeros_at_end += 1
            else:
                break

        used_bytes = block_size - zeros_at_end
        total_used_zeros += used_bytes

        # Find largest contiguous block of 0x00 anywhere in the bank
        max_free_run = 0
        current_run = 0
        for byte in block:
            if byte == 0x00:
                current_run += 1
            else:
                if current_run > max_free_run:
                    max_free_run = current_run
                current_run = 0
        if current_run > max_free_run:
            max_free_run = current_run

        if max_free_run > global_max_free:
            global_max_free = max_free_run
            global_max_free_bank = i

        bank_details.append((i, zeros_at_end, used_bytes, max_free_run))

    print("[Note: Using 0x00 byte scan fallback. Compile with linker mapfile for exact ROM and RAM metrics.]")
    if verbose:
        print(f"ROM File: {rom_path}")
        print(f"ROM Size: {rom_size} bytes ({rom_size // 1024} KB)")
        print("\n--- Counting Contiguous 0x00 padding at the end of each 64KB bank ---")
        for bank_id, zeros_at_end, used_bytes, max_free_run in bank_details:
            print(f"Bank {bank_id} (0x{bank_id:02X}):")
            print(f"  Contiguous 0x00 at end: {zeros_at_end:5d} bytes (Used: {used_bytes:5d} bytes)")
            print(f"  Largest contiguous free block: {max_free_run:5d} bytes")
        print(f"\nTotal ROM used: {total_used_zeros} bytes / {rom_size} bytes")
    else:
        print(f"ROM Used: {total_used_zeros} / {rom_size} bytes")
        print(f"Largest contiguous free block within a 64KB bank: {global_max_free} bytes (Bank {global_max_free_bank} / 0x{global_max_free_bank:02X})")


def main():
    verbose = '--verbose' in sys.argv or '-v' in sys.argv
    args = [arg for arg in sys.argv[1:] if not arg.startswith('-')]

    target_path = args[0] if args else 'mapfile_debug'

    # Check candidate mapfiles
    candidates = []
    if target_path:
        if target_path.endswith('.sfc'):
            map_name = 'mapfile_debug' if 'debug' in target_path else 'mapfile'
            candidates.append(map_name)
        elif target_path != 'mapfile_debug':
            candidates.append(target_path)
    candidates.extend(['mapfile_debug', 'mapfile'])

    for cand in candidates:
        if os.path.exists(cand) and parse_mapfile(cand, verbose):
            return

    # Fallback to SFC if mapfile not available
    rom_target = target_path if (target_path and target_path.endswith('.sfc')) else 'main_debug.sfc'
    if not os.path.exists(rom_target) and os.path.exists('main.sfc'):
        rom_target = 'main.sfc'

    analyze_rom_fallback(rom_target, verbose)


if __name__ == '__main__':
    main()


