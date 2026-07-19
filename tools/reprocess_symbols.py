import argparse
import os
import sys
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(
    description='Fix symbol file',
    add_help=False)
	
    parser.add_argument('-h', '--help', action='help',
		help='Show this help message and exit')
	
    parser.add_argument(
		'input', metavar='INPUT',
		type=Path,
		help='Input file (.sym)')
	
    cmd_args = parser.parse_args()
	
    output_file_path = cmd_args.input # Writing to same file
	
    with open(cmd_args.input, 'r', encoding="utf-8") as input_file:
        text = input_file.read()
		
        text = text.replace(':', ' ')
		
        with open(output_file_path, 'w', encoding="utf-8") as output_file:
            output_file.write(text)

if __name__ == '__main__':
	try:
		main()
	except Exception as e:
		if (os.getenv('DEBUG') or '').lower() in ['1', 'true']:
			raise
		print(f'Error: {e}', file=sys.stderr)
		sys.exit(1)