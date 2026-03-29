"""
strip_mtl_paths.py

Strips the prefix  C:/Projects/2026/ForwardWeGo/  from all texture paths
inside every .mtl file found in the current directory.

Before:  map_Kd C:/Projects/2026/ForwardWeGo/assets/textures/PolygonScifi_01_C.png
After:   map_Kd assets/textures/PolygonScifi_01_C.png

A .bak backup of each modified file is written alongside the original.
"""

import os
import re

PREFIX = "C:/Projects/2026/ForwardWeGo/"

def strip_prefix_from_mtl(filepath: str) -> int:
    """
    Remove PREFIX from every occurrence in the file.
    Returns the number of replacements made.
    """
    with open(filepath, "r", encoding="utf-8", errors="replace") as f:
        original = f.read()

    updated, count = re.subn(re.escape(PREFIX), "", original)

    if count:
        # Write a backup first
        backup = filepath + ".bak"
        with open(backup, "w", encoding="utf-8") as f:
            f.write(original)

        with open(filepath, "w", encoding="utf-8") as f:
            f.write(updated)

    return count


def main():
    cwd = os.getcwd()
    mtl_files = [f for f in os.listdir(cwd) if f.lower().endswith(".mtl")]

    if not mtl_files:
        print("No .mtl files found in the current directory.")
        return

    total_files_changed = 0
    total_replacements  = 0

    for filename in sorted(mtl_files):
        filepath = os.path.join(cwd, filename)
        count = strip_prefix_from_mtl(filepath)

        if count:
            print(f"  [modified]  {filename}  ({count} replacement{'s' if count != 1 else ''})")
            total_files_changed += 1
            total_replacements  += count
        else:
            print(f"  [unchanged] {filename}")

    print()
    print(f"Done — {total_files_changed} file(s) modified, "
          f"{total_replacements} replacement(s) total.")
    print("Backups written as <filename>.mtl.bak")


if __name__ == "__main__":
    main()
