import argparse
import os
import sys
from pathlib import Path

def floor_pow2(num: int) -> int:
	if num < 1: raise ValueError(num)
	return 1 << (num.bit_length() - 1)

def ceil_pow2(num: int) -> int:
	if num < 1: raise ValueError(num)
	return 1 << (num - 1).bit_length()

def pad_rom(rom: bytes) -> bytes:
	size = len(rom)
	chunk1_size = floor_pow2(size)
	chunk2_size = size - chunk1_size

	if chunk2_size == 0:
		return bytes(rom)

	output = bytearray(rom)
	padded_chunk2_size = ceil_pow2(chunk2_size)
	output += b'\0' * (padded_chunk2_size - chunk2_size)
	mirror_count = chunk1_size // padded_chunk2_size - 1
	output += output[chunk1_size:] * mirror_count

	return bytes(output)

def calc_checksum(rom: bytes) -> int:
	return sum(rom) % 65536

def calc_complement(checksum: int) -> int:
	return 0xffff ^ checksum

def patch_rom(rom: bytes, header_addr: int, checksum: int, complement: int) -> bytes:
	rom = bytearray(rom)

	rom[header_addr + 0xdc] = (complement >> 0) & 0xff
	rom[header_addr + 0xdd] = (complement >> 8) & 0xff
	rom[header_addr + 0xde] = (checksum >> 0) & 0xff
	rom[header_addr + 0xdf] = (checksum >> 8) & 0xff

	return bytes(rom)

def mapping_header_addr(mapping_type: str) -> int:
	if mapping_type == 'lorom':
		return 0x7f00
	elif mapping_type == 'hirom':
		return 0xff00
	elif mapping_type == 'exhirom':
		return 0x40ff00
	else:
		raise ValueError(f'Unsupported ROM mapping ({mapping_type})')

def fix_rom(rom: bytes, header_addr: int) -> bytes:
	rom = patch_rom(rom, header_addr, 0x0000, 0xffff)
	padded_rom = pad_rom(rom)

	checksum = calc_checksum(padded_rom)
	complement = calc_complement(checksum)

	return patch_rom(rom, header_addr, checksum, complement)

def main():
	parser = argparse.ArgumentParser(
		description='SFC ROM checksum patching utility.',
		add_help=False)

	parser.add_argument('-h', '--help', action='help',
		help='Show this help message and exit')

	parser.add_argument(
		'input', metavar='INPUT',
		type=Path,
		help='Input file (ROM)')

	parser.add_argument(
		'-o', '--output', metavar='OUTPUT',
		type=Path,
		help='Output file')

	mapping_args = parser.add_mutually_exclusive_group(required=True)

	mapping_args.add_argument(
		'--lorom', action='store_true',
		help='Input file uses LoROM mapping (header location at 0x007fxx)')

	mapping_args.add_argument(
		'--hirom', action='store_true',
		help='Input file uses HiROM mapping (header location at 0x00ffxx)')

	mapping_args.add_argument(
		'--exhirom', action='store_true',
		help='Input file uses LoROM mapping (header location at 0x40ffxx)')

	parser.add_argument(
		'--pad', action='store_true',
		help='Pad the output ROM size to the next power of 2 (minimum 64KB)')

	logging_args = parser.add_mutually_exclusive_group()

	logging_args.add_argument(
		'-q', '--quiet', action='store_true',
		help='Suppress info logging')

	cmd_args = parser.parse_args()

	output_file_path = cmd_args.output or cmd_args.input

	def print(*args, **kwargs):
		if cmd_args.quiet or False:
			return
		__builtins__.print(*args, **kwargs)

	with open(cmd_args.input, 'rb') as input_file:
		if cmd_args.lorom:
			mapping_type = 'lorom'
		elif cmd_args.hirom:
			mapping_type = 'hirom'
		elif cmd_args.exhirom:
			mapping_type = 'exhirom'

		header_addr = mapping_header_addr(mapping_type)

		rom = input_file.read()
		if cmd_args.pad:
			valid_size = max(64 * 1024, ceil_pow2(len(rom)))
			if len(rom) < valid_size:
				rom = rom + b'\0' * (valid_size - len(rom))

		old_checksum = int.from_bytes(rom[header_addr + 0xde:header_addr + 0xdf + 1], 'little')
		old_complement = int.from_bytes(rom[header_addr + 0xdc:header_addr + 0xdd + 1], 'little')

		rom = fix_rom(rom, header_addr)
		new_checksum = calc_checksum(pad_rom(rom))
		new_complement = calc_complement(new_checksum)

		print(f'Updated checksum:   0x{old_checksum:04x} -> 0x{new_checksum:04x}' + (' (identical)' if old_checksum == new_checksum else ''))
		print(f'Updated complement: 0x{old_complement:04x} -> 0x{new_complement:04x}' + (' (identical)' if old_complement == new_complement else ''))

		with open(output_file_path, 'wb') as output_file:
			output_file.write(rom)
			print(f'Wrote output file to {Path(output_file_path).absolute()}')

if __name__ == '__main__':
	try:
		main()
	except Exception as e:
		if (os.getenv('DEBUG') or '').lower() in ['1', 'true']:
			raise
		print(f'Error: {e}', file=sys.stderr)
		sys.exit(1)