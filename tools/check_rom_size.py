import os
import sys

def analyze_rom(rom_path, verbose=False):
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

if __name__ == '__main__':
    verbose = '--verbose' in sys.argv or '-v' in sys.argv
    rom_path = next((argument for argument in sys.argv[1:] if not argument.startswith('-')), 'main.sfc')
    analyze_rom(rom_path, verbose)
