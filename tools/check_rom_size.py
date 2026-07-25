import os
import re
import sys

def parse_mapfile(map_path, verbose=False):
    if not os.path.exists(map_path):
        return False

    with open(map_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    in_section_mapping = False
    sections = []
    current_sec = None

    for line in lines:
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

        print("\n--- RAM Allocation Summary ---")
        for sec in sections:
            if sec['name'] in ram_sections:
                print(f"  Section {sec['name']:<8s}: Allocated {sec['allocated']:5d} bytes (0x{sec['offset']:06X})")

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

    print(f"[Note: Using 0x00 byte scan fallback. Compile with linker mapfile for exact metrics.]")
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
    if target_path and target_path != 'mapfile_debug':
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

