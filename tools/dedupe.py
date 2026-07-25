import argparse
import os
import re
import sys
from pathlib import Path

def generate_tiles(input_bytes: bytes, sheetwidth: int, framewidth: int, frameheight: int, tile_size_single: int, wide: bool):
    """Yield tile slice byte chunks in 2D sprite frame order."""
    if sheetwidth <= 0 or framewidth <= 0 or frameheight <= 0:
        raise ValueError("Sheet width, frame width, and frame height must be positive integers.")
    if sheetwidth % 8 != 0 or framewidth % 8 != 0 or frameheight % 8 != 0:
        raise ValueError("Sheet width, frame width, and frame height must be multiples of 8.")
    if sheetwidth % framewidth != 0:
        raise ValueError(f"Sheet width ({sheetwidth}) must be divisible by frame width ({framewidth}).")

    if wide:
        if framewidth % 16 != 0:
            raise ValueError(f"Frame width ({framewidth}) must be divisible by 16 in wide mode.")
        tile_slice_size = tile_size_single * 2
        step_tiles_x = 2
    else:
        tile_slice_size = tile_size_single
        step_tiles_x = 1

    sheet_tiles_x = sheetwidth // 8
    frame_tiles_x = framewidth // 8
    frame_tiles_y = frameheight // 8
    frames_per_row = sheetwidth // framewidth

    total_bytes = len(input_bytes)
    sheet_row_bytes = sheet_tiles_x * tile_size_single
    frame_rank_bytes = sheet_row_bytes * frame_tiles_y

    offset = 0
    while offset < total_bytes:
        rank_start = offset
        for frame_x_idx in range(frames_per_row):
            frame_start = rank_start + (frame_x_idx * frame_tiles_x * tile_size_single)
            if frame_start >= total_bytes:
                break

            for row_in_frame in range(frame_tiles_y):
                row_start = frame_start + (row_in_frame * sheet_row_bytes)
                if row_start >= total_bytes:
                    break

                col_in_frame = 0
                while col_in_frame < frame_tiles_x:
                    tile_offset = row_start + (col_in_frame * tile_size_single)
                    if tile_offset >= total_bytes:
                        break
                    tile = input_bytes[tile_offset:tile_offset + tile_slice_size]
                    if len(tile) == tile_slice_size:
                        yield tile
                    col_in_frame += step_tiles_x

        offset += frame_rank_bytes

def main():
    parser = argparse.ArgumentParser(
        description='Tile dictionary and lookup table builder')

    parser.add_argument(
        'input', metavar='INPUT',
        type=Path,
        help='Input file (raw tile data)')

    parser.add_argument(
        '-o', '--output', metavar='OUTPUT',
        type=Path,
        help='Output file (deduplicated tile data; lookup table will have same name with _lut.h appended)')

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

    input_file_path = Path(cmd_args.input)
    output_file_path = Path(cmd_args.output) if cmd_args.output else input_file_path

    tile_size_single = 32
    if cmd_args.bpp2:
        tile_size_single = 16
    elif cmd_args.bpp8:
        tile_size_single = 64

    with open(input_file_path, 'rb') as f:
        input_data = f.read()

    tile_slice_size = tile_size_single * 2 if cmd_args.wide else tile_size_single

    if len(input_data) % tile_slice_size != 0:
        print(f"Warning: Input file size ({len(input_data)} bytes) is not an exact multiple of tile slice size ({tile_slice_size} bytes).", file=sys.stderr)

    tile_to_offset = {}
    dictionary_bytes = bytearray()

    # Phase 1: Build dictionary in linear sheet order (preserving original tile discovery order)
    for offset in range(0, len(input_data), tile_slice_size):
        tile = input_data[offset:offset + tile_slice_size]
        if len(tile) == tile_slice_size:
            if tile not in tile_to_offset:
                tile_to_offset[tile] = len(dictionary_bytes)
                dictionary_bytes += tile

    with open(output_file_path, 'wb') as f:
        f.write(dictionary_bytes)

    out_str = str(output_file_path)
    if out_str.endswith(".bin.dd"):
        lut_header_path = Path(out_str[:-7] + "_lut.h")
    elif out_str.endswith(".dd"):
        lut_header_path = Path(out_str[:-3] + "_lut.h")
    elif out_str.endswith(".bin"):
        lut_header_path = Path(out_str[:-4] + "_lut.h")
    else:
        lut_header_path = output_file_path.with_suffix('').with_name(output_file_path.stem + "_lut.h")

    raw_identifier = input_file_path.name
    if raw_identifier.endswith(".bin"):
        raw_identifier = raw_identifier[:-4]
    c_identifier = re.sub(r'[^a-zA-Z0-9_]', '_', raw_identifier)

    # Phase 2: Traversal in 2D frame grid order to build LUT lines
    tiles_2d = list(generate_tiles(
        input_data,
        cmd_args.sheetwidth,
        cmd_args.framewidth,
        cmd_args.frameheight,
        tile_size_single,
        cmd_args.wide
    ))

    frame_tiles_x = cmd_args.framewidth // 8
    frame_tiles_y = cmd_args.frameheight // 8
    slices_per_frame = (frame_tiles_x // 2) * frame_tiles_y if cmd_args.wide else frame_tiles_x * frame_tiles_y

    lut_lines = []
    current_frame = []
    for idx, tile in enumerate(tiles_2d):
        offset = tile_to_offset[tile]
        current_frame.append(str(offset))
        if (idx + 1) % slices_per_frame == 0:
            lut_lines.append("\t" + ", ".join(current_frame) + ", ")
            current_frame = []

    if current_frame:
        lut_lines.append("\t" + ", ".join(current_frame) + ", ")

    lut_content = f"const uint16_t data_{c_identifier}_lut[] = {{ \n"
    if lut_lines:
        lut_content += "\n".join(lut_lines) + "\n"
    lut_content += "};\n"

    with open(lut_header_path, 'w') as f:
        f.write(lut_content)

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        if (os.getenv('DEBUG') or '').lower() in ['1', 'true']:
            raise
        print(f'Error: {e}', file=sys.stderr)
        sys.exit(1)