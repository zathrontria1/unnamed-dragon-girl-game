import argparse
import os
import sys
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(
		description='Tile dictionary and lookup table builder',
		add_help=False)
	
    parser.add_argument('-h', '--help', action='help',
		help='Show this help message and exit')
	
    parser.add_argument(
		'input', metavar='INPUT',
		type=Path,
		help='Input file (raw tile data)')
	
    parser.add_argument(
		'-o', '--output', metavar='OUTPUT',
		type=Path,
		help='Output file (deduplicated tile data; lookup table will have same name with .lut appended)')
    
    parser.add_argument(
		'-w', '--wide', 
        action='store_true',
		help='Use 16x8 slices (dictionary is made of 2-tile pairs; faster to copy, but reduces efficiency)')
    
    bitdepth_args = parser.add_mutually_exclusive_group(required=True)

    bitdepth_args.add_argument(
		'--bpp2', action='store_true',
		help='Input tile data is 2bpp (16 bytes per tile)')
    
    bitdepth_args.add_argument(
		'--bpp4', action='store_true',
		help='Input tile data is 4bpp (32 bytes per tile)')
    
    bitdepth_args.add_argument(
		'--bpp8', action='store_true',
		help='Input tile data is 8bpp (64 bytes per tile)')
    
    cmd_args = parser.parse_args()

    def print(*args, **kwargs):
        __builtins__.print(*args, **kwargs)
    
    output_file_path = cmd_args.output or cmd_args.input

    # Create the dictionary
    tile_size = 32

    if cmd_args.bpp2:
        tile_size = 16
    elif cmd_args.bpp8:
        tile_size = 64

    if cmd_args.wide == True:
        tile_size *= 2

    dictionary_file = bytearray()

    with open(cmd_args.input, 'rb') as input_file:
        while True:
            tile = input_file.read(tile_size)

            if len(tile) == 0:
                break

            if len(dictionary_file) == 0:
                dictionary_file += tile
                continue
            if tile not in dictionary_file: 
                dictionary_file += tile

    with open(output_file_path, 'wb') as output_file:
        output_file.write(dictionary_file)

    input_file.close()
    output_file.close()

    # Create the lookup table
    input_file_path = str(cmd_args.input)

    input_file_name = str(os.path.basename(input_file_path))

    lookup_file = str()
    lookup_file += "const uint16_t data_" + input_file_name.rstrip(".bin") + "_lut[] = { \n\t"

    output_file_path_str = str(output_file_path).rstrip(".bin.dd")
    output_file_path = output_file_path_str + "_lut.h"

    with open(cmd_args.input, 'rb') as input_file:
        while True:
            current_byte = input_file.tell()

            tile = input_file.read(tile_size)

            if len(tile) == 0:
                break

            if tile in dictionary_file:
                offset = dictionary_file.find(tile)
                lookup_file += str(offset)
                lookup_file += ", "

            if cmd_args.wide == False:
                tile = input_file.read(tile_size)

                if len(tile) == 0:
                    break

                if tile in dictionary_file:
                    offset = dictionary_file.find(tile)
                    lookup_file += str(offset)
                    lookup_file += ", "

                input_file.seek((tile_size * 14), os.SEEK_CUR)
            else:
                input_file.seek((tile_size * 7), os.SEEK_CUR)

            tile = input_file.read(tile_size)

            if len(tile) == 0:
                break

            if tile in dictionary_file:
                offset = dictionary_file.find(tile)
                lookup_file += str(offset)
                lookup_file += ", "

            if cmd_args.wide == False:
                tile = input_file.read(tile_size)

                if len(tile) == 0:
                    break

                if tile in dictionary_file:
                    offset = dictionary_file.find(tile)
                    lookup_file += str(offset)
                    lookup_file += ", \n\t"

                current_byte += tile_size * 2

                if current_byte % (tile_size * 16) == 0:
                    current_byte += (tile_size * 16)

                input_file.seek(current_byte, os.SEEK_SET)
            else:
                lookup_file += "\n\t"
                current_byte += tile_size

                if current_byte % (tile_size * 8) == 0:
                    current_byte += (tile_size * 8)

                input_file.seek(current_byte, os.SEEK_SET)

    lookup_file += "};\n"

    with open(output_file_path, 'w') as output_file:
        output_file.write(lookup_file)

if __name__ == '__main__':
	try:
		main()
	except Exception as e:
		if (os.getenv('DEBUG') or '').lower() in ['1', 'true']:
			raise
		print(f'Error: {e}', file=sys.stderr)
		sys.exit(1)