import argparse
import os
import sys
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(
        description='Fix symbol file formatting for Mesen',
        add_help=False
    )
    
    parser.add_argument(
        '-h', '--help', action='help',
        help='Show this help message and exit'
    )
    
    parser.add_argument(
        'input', metavar='INPUT',
        type=Path,
        help='Input file (.sym)'
    )
    
    cmd_args = parser.parse_args()
    
    if not cmd_args.input.is_file():
        print(f"Error: Symbol file not found: {cmd_args.input}", file=sys.stderr)
        sys.exit(1)
        
    with open(cmd_args.input, 'r', encoding="utf-8") as f:
        lines = f.readlines()

    # Replace only the first colon per line (address:symbol separator)
    # to avoid corrupting colons inside symbol names or comments.
    processed_lines = [line.replace(':', ' ', 1) for line in lines]
    
    # Write to a temporary file first, then perform atomic replace
    temp_path = cmd_args.input.with_suffix('.tmp')
    with open(temp_path, 'w', encoding="utf-8", newline='') as f:
        f.writelines(processed_lines)
        
    temp_path.replace(cmd_args.input)

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        if (os.getenv('DEBUG') or '').lower() in ['1', 'true']:
            raise
        print(f'Error: {e}', file=sys.stderr)
        sys.exit(1)