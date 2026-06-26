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
    
    parser.add_argument('--sheetwidth', metavar='sheetwidth', type=int, default=128, help='Tile data sheet width')
    parser.add_argument('--framewidth', metavar='framewidth', type=int, default=16, help='Sprite frame width')
    parser.add_argument('--frameheight', metavar='frameheight', type=int, default=16, help='Sprite frame height')
    
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

    sheetwidth_bytes = (cmd_args.sheetwidth >> 3) * tile_size
    framewidth_bytes = (cmd_args.framewidth >> 3) * tile_size

    if cmd_args.wide == True:
        tile_size *= 2

    tile_stride = sheetwidth_bytes - framewidth_bytes
    tile_rows = cmd_args.frameheight >> 3

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
    lookup_file += "const uint16_t data_" + input_file_name.removesuffix(".bin") + "_lut[] = { \n\t"

    output_file_path_str = str(output_file_path).removesuffix(".bin.dd")
    output_file_path = output_file_path_str + "_lut.h"

    read_x = 0
    read_y = 0

    with open(cmd_args.input, 'rb') as input_file:

        current_byte = input_file.tell()

        while True:
            tile = input_file.read(tile_size)

            if len(tile) == 0:
                break

            # add the current tile
            if tile in dictionary_file:
                offset = dictionary_file.find(tile)
                lookup_file += str(offset)
                lookup_file += ", "

            read_x += tile_size

            if (read_x >= framewidth_bytes):
                # Go down an entire row
                read_x = 0
                read_y += 1
                
                if (read_y >= tile_rows):
                    # Go to a new frame
                    read_y = 0
                    current_byte += framewidth_bytes

                    lookup_file += "\n\t"

                    if ((current_byte % sheetwidth_bytes) == 0):
                        # Edge of the frame overrun, go down
                        i = 1
                        
                        while (i < tile_rows):
                            current_byte += sheetwidth_bytes
                            i += 1

                    input_file.seek(current_byte, os.SEEK_SET)

                    current_byte = input_file.tell()
                else:
                    input_file.seek(tile_stride, os.SEEK_CUR)

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