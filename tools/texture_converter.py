#!/usr/bin/env python3

import sys
import os
import os.path
import glob
import re
from pathlib import Path
# ALIGNED8 static const Texture chain_ball_seg6_texture_06020AE8[]
prefixes = [
    "ALIGNED8 static const u8", "static const u8", "ALIGNED8 static u8", "ALIGNED8 u8", "ALIGNED8 const u8", "u8",
    "ALIGNED8 static const Texture", "static const Texture", "ALIGNED8 static Texture", "ALIGNED8 Texture", "ALIGNED8 const Texture", "Texture",
]

def parse_model(file, lines, overwrite):
    searchInclude = False
    searchLineIndex = 0
    searchLineOriginal = ""
    needsReplace = False
    includePath = ""
    for lineIndex, uline in enumerate(lines, 0):
        line = uline.strip()
        if not searchInclude:
            for prefix in prefixes:
                if line.rstrip().startswith(prefix) and "[] = {" in line:
                    searchInclude = True
                    searchLineIndex = lineIndex
                    searchLineOriginal = line
        else:
            if line.startswith("#include"):
                includePath = line.replace("#include \"", "\"__OTR__").replace(".inc.c", "").replace(".rgba16", "").replace(".rgba32", "").replace(".ia16", "").replace(".ia1", "").replace(".ia4", "").replace(".ia8", "")
            elif line.endswith("};") and includePath != "":
                lines[searchLineIndex] = searchLineOriginal.replace("[] = {", f"[] = {includePath};")
                lines[searchLineIndex + 1] = ""
                lines[searchLineIndex + 2] = "\n"
                searchInclude = False
                searchLineIndex = 0
                searchLineOriginal = ""
                needsReplace = True
                includePath = ""
            else:
                searchInclude = False
    nfile = os.path.abspath(file) if overwrite else os.path.abspath(file.replace("\\", "/").replace("sm64-cc02/", "sm64-cc02/overwriten/"))
    npath = os.path.dirname(nfile)

    if needsReplace:
        if not os.path.isdir(npath):
            os.makedirs(npath)
        f = open(nfile, "w")
        f.writelines(lines)
        f.close()

if not len(sys.argv) > 1:
    path = f"{str(Path(__file__).resolve().parents[1])}/**/*.c"

    print(f"Converting repo textures this may take a long time")

    for file in glob.iglob(path, recursive=True):
        try:
            if not "\\build" in file:
                parse_model(file, open(file, 'r').readlines(), False)
        except Exception as err:
            print(f"Error on {file}")
            print(err)
else:
    file = os.path.abspath(sys.argv[1])
    try:
        if os.path.isfile(file):
            print(f"Converting {file} textures")
            parse_model(file, open(file, 'r').readlines(), True)
        else:
            print(f"Invalid Path")
    except Exception as err:
        print(f"Error on {file}")
        print(err)