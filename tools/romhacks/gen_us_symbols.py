#!/usr/bin/env python3
"""
gen_us_symbols.py  -  Writes the address -> name table Torch uses to export scripts from a US ROM.

Level and behavior scripts in a ROM point at code: behaviors, the native functions behaviors call, the functions
level scripts call and the global level scripts they jump to. Torch writes those as names Ghostship resolves at load
time (docs/romhacks/SCRIPTS.md), using this table. It's built by walking the vanilla US ROM side by side with the
decomp sources Ghostship compiles, so every address pairs with the name of the same thing:

    python3 tools/romhacks/gen_us_symbols.py baserom.us.z64 assets/symbols/us.yml
"""

import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rom_manager import LEVEL_FOLDERS, US_ENTRY_SCRIPT  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BEHAVIOR_SEGMENT = 0x219E00

# Behavior command sizes in words, by opcode.
BEHAVIOR_OPS = [
    ("BEGIN", 1), ("DELAY", 1), ("CALL", 2), ("RETURN", 1), ("GOTO", 2), ("BEGIN_REPEAT", 1), ("END_REPEAT", 1),
    ("END_REPEAT_CONTINUE", 1), ("BEGIN_LOOP", 1), ("END_LOOP", 1), ("BREAK", 1), ("BREAK_UNUSED", 1),
    ("CALL_NATIVE", 2), ("ADD_FLOAT", 1), ("SET_FLOAT", 1), ("ADD_INT", 1), ("SET_INT", 1), ("OR_INT", 1),
    ("BIT_CLEAR", 1), ("SET_INT_RAND_RSHIFT", 2), ("SET_RANDOM_FLOAT", 2), ("SET_RANDOM_INT", 2),
    ("ADD_RANDOM_FLOAT", 2), ("ADD_INT_RAND_RSHIFT", 2), ("CMD_NOP_1", 1), ("CMD_NOP_2", 1), ("CMD_NOP_3", 1),
    ("SET_MODEL", 1), ("SPAWN_CHILD", 3), ("DEACTIVATE", 1), ("DROP_TO_FLOOR", 1), ("SUM_FLOAT", 1), ("SUM_INT", 1),
    ("BILLBOARD", 1), ("HIDE", 1), ("SET_HITBOX", 2), ("CMD_NOP_4", 1), ("DELAY_VAR", 1),
    ("BEGIN_REPEAT_UNUSED", 1), ("LOAD_ANIMATIONS", 2), ("ANIMATE", 1), ("SPAWN_CHILD_WITH_PARAM", 3),
    ("LOAD_COLLISION_DATA", 2), ("SET_HITBOX_WITH_OFFSET", 3), ("SPAWN_OBJ", 3), ("SET_HOME", 1),
    ("SET_HURTBOX", 2), ("SET_INTERACT_TYPE", 2), ("SET_OBJ_PHYSICS", 5), ("SET_INTERACT_SUBTYPE", 2), ("SCALE", 1),
    ("PARENT_BIT_CLEAR", 2), ("ANIMATE_TEXTURE", 1), ("DISABLE_RENDERING", 1), ("SET_INT_UNUSED", 2),
    ("SPAWN_WATER_DROPLET", 2),
]
BEHAVIOR_SIZES = dict(BEHAVIOR_OPS)
OBJ_LISTS = ["OBJ_LIST_PLAYER", "OBJ_LIST_UNUSED_1", "OBJ_LIST_DESTRUCTIVE", "OBJ_LIST_UNUSED_3",
             "OBJ_LIST_GENACTOR", "OBJ_LIST_PUSHABLE", "OBJ_LIST_LEVEL", "OBJ_LIST_UNUSED_7", "OBJ_LIST_DEFAULT",
             "OBJ_LIST_SURFACE", "OBJ_LIST_POLELIKE", "OBJ_LIST_SPAWNER", "OBJ_LIST_UNIMPORTANT"]


def u32(rom, a):
    return struct.unpack_from(">I", rom, a)[0]


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def active_lines(text, defines=("VERSION_US",)):
    """Keeps the lines a US build compiles."""
    out, stack = [], []
    for line in text.splitlines():
        s = line.strip()
        m = re.match(r"#\s*(ifdef|ifndef)\s+(\w+)", s)
        if m:
            cond = (m.group(2) in defines) == (m.group(1) == "ifdef")
            stack.append([cond, cond])
            continue
        m = re.match(r"#\s*(el)?if\s+(.*)", s)
        if m:
            names = re.findall(r"defined\s*\(?\s*(\w+)", m.group(2))
            expr = m.group(2)
            for n in names:
                expr = re.sub(r"defined\s*\(?\s*" + n + r"\s*\)?", str(n in defines), expr)
            expr = expr.replace("||", " or ").replace("&&", " and ").replace("!", " not ")
            try:
                cond = bool(eval(expr))
            except Exception:
                cond = False
            if m.group(1):
                stack[-1][0] = cond and not stack[-1][1]
                stack[-1][1] = stack[-1][1] or cond
            else:
                stack.append([cond, cond])
            continue
        if re.match(r"#\s*else", s):
            stack[-1][0] = not stack[-1][1]
            continue
        if re.match(r"#\s*endif", s):
            stack.pop()
            continue
        if all(c[0] for c in stack):
            out.append(line)
    return "\n".join(out)


def behavior_symbols(rom):
    """Behaviors and the native functions they call, from segment 0x13 next to data/behavior_data.c."""
    text = active_lines(open(os.path.join(ROOT, "data", "behavior_data.c"), encoding="utf-8").read())
    text = strip_comments(text)
    symbols = {}
    off = 0

    def begins_at(o, obj_list, cmds):
        a = BEHAVIOR_SEGMENT + o
        if rom[a] != 0 or rom[a + 1] != obj_list or rom[a + 2 : a + 4] != b"\0\0":
            return False
        p = a + 4
        for name, _ in cmds[1:4]:
            op = rom[p]
            if op >= len(BEHAVIOR_OPS) or BEHAVIOR_OPS[op][0] != name:
                return False
            p += BEHAVIOR_SIZES[name] * 4
        return True

    for m in re.finditer(r"const\s+BehaviorScript\s+(\w+)\s*\[\s*\]\s*=\s*\{(.*?)\};", text, re.S):
        name, body = m.groups()
        cmds = [(c, args) for c, args in re.findall(r"\b([A-Z_][A-Z0-9_]*)\s*\(([^()]*(?:\([^()]*\)[^()]*)*)\)", body)
                if c in BEHAVIOR_SIZES]
        b = re.match(r"\s*BEGIN\s*\(\s*(\w+)\s*\)", body)
        if b and b.group(1) in OBJ_LISTS:
            lst = OBJ_LISTS.index(b.group(1))
            if not begins_at(off, lst, cmds):
                # Some behaviors sit after padding the decomp doesn't spell out; find the real start nearby.
                for d in sorted(range(-32, 129, 4), key=abs):
                    if begins_at(off + d, lst, cmds):
                        off += d
                        break
                else:
                    raise SystemExit(f"lost track of the behavior segment at {name}")
        symbols[0x13000000 + off] = name
        p = BEHAVIOR_SEGMENT + off
        for c, args in cmds:
            if c == "CALL_NATIVE":
                symbols[u32(rom, p + 4)] = args.strip()
            p += BEHAVIOR_SIZES[c] * 4
        off = p - BEHAVIOR_SEGMENT
    return symbols


LEVEL_CMD = re.compile(r"\b(CALL|CALL_LOOP|JUMP_LINK)\s*\(([^()]*)\)")


def level_symbols(rom):
    """Functions level scripts call and the global scripts they jump to, from each level next to its script.c."""
    symbols = {}
    levels = {}
    seen = set()

    def walk(pc, segments, level_num, depth):
        if depth > 16 or (pc, level_num) in seen:
            return
        seen.add((pc, level_num))
        for _ in range(8192):
            cmd, length = rom[pc], rom[pc + 1]
            if length == 0:
                return
            if cmd in (0x00, 0x01):
                seg = rom[pc + 3]
                start, end, entry = struct.unpack_from(">III", rom, pc + 4)
                segs = dict(segments)
                segs[seg] = start
                target = segs[entry >> 24] + (entry & 0xFFFFFF) if entry >> 24 in segs else None
                if level_num in LEVEL_FOLDERS and level_num not in levels and target is not None:
                    levels[level_num] = (target, segs)
                elif target is not None:
                    walk(target, segs, level_num, depth + 1)
                if cmd == 0x01:
                    return
            elif cmd in (0x17, 0x18, 0x1A):
                segments = dict(segments)
                segments[rom[pc + 3]] = u32(rom, pc + 4)
            elif cmd in (0x05, 0x06):
                t = u32(rom, pc + 4)
                if t >> 24 in segments:
                    walk(segments[t >> 24] + (t & 0xFFFFFF), segments, level_num, depth + 1)
                if cmd == 0x05:
                    return
            elif cmd == 0x0C and rom[pc + 2] == 2:
                val = struct.unpack_from(">i", rom, pc + 4)[0]
                t = u32(rom, pc + 8)
                if t >> 24 in segments:
                    walk(segments[t >> 24] + (t & 0xFFFFFF), segments, val, depth + 1)
            if cmd in (0x02, 0x07):
                return
            pc += length

    walk(US_ENTRY_SCRIPT, {0x10: US_ENTRY_SCRIPT}, None, 0)

    for num, (start, _) in sorted(levels.items()):
        folder = LEVEL_FOLDERS[num]
        path = os.path.join(ROOT, "levels", folder, "script.c")
        if not os.path.exists(path):
            continue
        text = strip_comments(open(path, encoding="utf-8").read())
        m = re.search(r"LevelScript\s+level_" + folder + r"_entry\s*\[\s*\]\s*=\s*\{(.*?)\};", text, re.S)
        if not m:
            continue
        c_cmds = [(c, [a.strip() for a in args.split(",")]) for c, args in LEVEL_CMD.findall(m.group(1))]
        rom_cmds = []
        pc = start
        for _ in range(4096):
            cmd, length = rom[pc], rom[pc + 1]
            if length == 0:
                break
            if cmd in (0x11, 0x12):
                rom_cmds.append(("CALL" if cmd == 0x11 else "CALL_LOOP", u32(rom, pc + 4)))
            elif cmd == 0x06:
                rom_cmds.append(("JUMP_LINK", u32(rom, pc + 4)))
            if cmd in (0x02, 0x07):
                break
            pc += length
        if [c for c, _ in c_cmds] != [c for c, _ in rom_cmds]:
            print(f"  ! {folder}: the ROM's calls and jumps don't line up with script.c, skipped", file=sys.stderr)
            continue
        for (c, args), (_, addr) in zip(c_cmds, rom_cmds):
            name = args[-1]
            if c == "JUMP_LINK" and not name.startswith("script_func_global"):
                continue
            symbols[addr] = name
    return symbols


def main():
    rom = open(sys.argv[1], "rb").read()
    symbols = behavior_symbols(rom)
    symbols.update(level_symbols(rom))
    with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as f:
        f.write("# Generated by tools/romhacks/gen_us_symbols.py from the US ROM and the decomp sources.\n")
        f.write("# Address -> name of the code level and behavior scripts point at; see docs/romhacks/SCRIPTS.md.\n")
        for addr, name in sorted(symbols.items()):
            f.write(f"0x{addr:08X}: {name}\n")
    print(f"{len(symbols)} symbols")


if __name__ == "__main__":
    main()
