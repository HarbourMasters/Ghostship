import sys
import os
import re
import argparse

def amalgamate(file_path, include_dirs, processed, out_file):
    abs_path = os.path.abspath(file_path)
    
    # Inclusion guard: prevents circular dependencies AND acts as a global #pragma once
    if abs_path in processed:
        return
    processed.add(abs_path)

    try:
        with open(abs_path, 'r', encoding='utf-8') as f:
            for line in f:
                match = re.match(r'^\s*#\s*include\s+([<"])([^>"]+)[>"]', line)
                if match:
                    delimiter = match.group(1)
                    header_name = match.group(2)
                    header_path = None
                    
                    search_paths = list(include_dirs)
                    if delimiter == '"':
                        local_dir = os.path.dirname(abs_path)
                        search_paths.insert(0, local_dir)
                    
                    for path in search_paths:
                        potential_path = os.path.join(path, header_name)
                        if os.path.exists(potential_path):
                            header_path = potential_path
                            break
                    
                    if header_path:
                        out_file.write(f"/* --- Start: {header_name} --- */\n")
                        amalgamate(header_path, include_dirs, processed, out_file)
                        out_file.write(f"/* --- End: {header_name} --- */\n")
                    else:
                        out_file.write(line) 
                else:
                    out_file.write(line)
    except Exception as e:
        print(f"Warning: Could not process {abs_path}: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate a Unity Build amalgamation.")
    parser.add_argument('--out', required=True, help="The single output .c file")
    parser.add_argument('--includes', nargs='*', default=[], help="Include directories")
    parser.add_argument('--srcs', nargs='+', required=True, help="Input .c files")
    
    args = parser.parse_args()

    # The processed set is maintained globally across ALL input files
    global_processed = set()
    
    with open(args.out, 'w', encoding='utf-8') as out_file:
        for src_file in args.srcs:
            out_file.write(f"\n/* ========================================= */\n")
            out_file.write(f"/* === Start of Source: {os.path.basename(src_file)} === */\n")
            out_file.write(f"/* ========================================= */\n\n")
            
            amalgamate(src_file, args.includes, global_processed, out_file)