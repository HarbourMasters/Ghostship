#!/usr/bin/env python3
"""
rom_manager.py  -  Helpers for porting SM64 romhacks made with ROM Manager.

ROM Manager hacks start from an sm64extend ROM (every MIO0 block decompressed past 8 MB) and keep each level in its
own block: the level script in segment 0x19, the area geometry in segment 0x0E, and texture banks in segment 0x09
stored as uncompressed "fake" MIO0 blocks. This tool rebuilds the ROM a hack was made from and writes the Torch
YAMLs for the levels it changed.

    # The extended base ROM a ROM Manager patch is made against (sm64extend with its defaults)
    python3 tools/romhacks/rom_manager.py extend baserom.us.z64 sm64.ext.z64

    # Apply a PPF3 patch
    python3 tools/romhacks/rom_manager.py ppf sm64.ext.z64 hack.ppf hack.z64

    # Torch YAMLs for every level the hack moved into its own block, with its scripts
    python3 tools/romhacks/rom_manager.py yamls hack.z64 tools/romhacks/<name>/yamls --yaml-root yamls --scripts \
        --base-rom baserom.us.z64 --symbols assets/symbols/us.yml --symbols tools/romhacks/<name>/symbols/<name>.yml

    # The hack's recipe package
    python3 tools/romhacks/rom_manager.py package tools/romhacks/<name> romhacks/<name>.o2r

See docs/romhacks/DESIGN.md.
"""

import argparse
import os
import re
import struct
import zlib
import sys
from collections import OrderedDict

# Level numbers to the decomp's level folders (levels/level_defines.h).
LEVEL_FOLDERS = {
    4: "bbh", 5: "ccm", 6: "castle_inside", 7: "hmc", 8: "ssl", 9: "bob", 10: "sl", 11: "wdw", 12: "jrb", 13: "thi",
    14: "ttc", 15: "rr", 16: "castle_grounds", 17: "bitdw", 18: "vcutm", 19: "bitfs", 20: "sa", 21: "bits",
    22: "lll", 23: "ddd", 24: "wf", 25: "ending", 26: "castle_courtyard", 27: "pss", 28: "cotmc", 29: "totwc",
    30: "bowser_1", 31: "wmotr", 33: "bowser_2", 34: "bowser_3", 36: "ttm",
}

US_ENTRY_SCRIPT = 0x108A10

EXT_IN_START = 0x000D0000
EXT_OUT_START = 0x00800000
EXT_SIZE = 64 * 1024 * 1024
EXT_PADDING = 32 * 1024
MIO0_HEADER_LEN = 16


def mio0_decode(buf, off):
    if buf[off : off + 4] != b"MIO0":
        return None, 0
    dest_size, comp_off, uncomp_off = struct.unpack_from(">III", buf, off + 4)
    out = bytearray()
    bit = comp = uncomp = 0
    while len(out) < dest_size:
        if buf[off + MIO0_HEADER_LEN + bit // 8] & (0x80 >> (bit % 8)):
            out.append(buf[off + uncomp_off + uncomp])
            uncomp += 1
        else:
            v0, v1 = buf[off + comp_off + comp], buf[off + comp_off + comp + 1]
            comp += 2
            dist = ((v0 & 0xF) << 8) + v1 + 1
            for _ in range((v0 >> 4) + 3):
                out.append(out[-dist])
        bit += 1
    return bytes(out), uncomp_off + uncomp


def _la2int(b, lui, addiu):
    hi = struct.unpack_from(">H", b, lui + 2)[0]
    lo = struct.unpack_from(">H", b, addiu + 2)[0]
    if lo & 0x8000:
        hi = (hi - 1) & 0xFFFF
    return (hi << 16) | lo


def extend_rom(rom):
    """Byte-identical to `sm64extend` run with its default options."""
    if len(rom) > 8 * 1024 * 1024:
        raise SystemExit("ROM is already extended")
    in_len = len(rom)
    out = bytearray(rom) + bytearray(b"\x01" * (EXT_SIZE - in_len))

    blocks = OrderedDict()
    for a in range(EXT_IN_START, in_len, 16):
        if rom[a : a + 4] == b"MIO0":
            blocks[a] = {"old_end": 0, "cmd": 0, "asm": 0, "a1": 0, "new": 0, "new_end": 0}

    for a in range(EXT_IN_START, in_len, 4):
        if rom[a] in (0x18, 0x1A) and rom[a + 1] == 0x0C and rom[a + 2] == 0:
            b = blocks.get(struct.unpack_from(">I", rom, a + 4)[0])
            if b:
                b["cmd"] = rom[a]
                b["old_end"] = struct.unpack_from(">I", rom, a + 8)[0]

    # lui a1 / lui a2 / addiu a2 / [addiu a1 | jal] pairs in code that load MIO0 blocks directly.
    for a in range(0, EXT_IN_START, 4):
        if rom[a] & 0xFC == 0x3C and rom[a + 4] & 0xFC == 0x3C and rom[a + 8] & 0xFC == 0x24:
            a1 = 0xC if rom[a + 0xC] & 0xFC == 0x24 else (0x10 if rom[a + 0x10] & 0xFC == 0x24 else 0)
            if a1 and rom[a + 1] & 0x1F == rom[a + a1 + 1] & 0x1F and rom[a + 5] & 0x1F == rom[a + 9] & 0x1F:
                b = blocks.get(_la2int(rom, a, a + a1))
                if b:
                    b.update(cmd=0xFF, asm=a, a1=a1, new_end=_la2int(rom, a + 4, a + 8))

    out_addr = EXT_OUT_START
    for old, b in blocks.items():
        data, end = mio0_decode(rom, old)
        if data is None:
            continue
        length = len(data)
        if b["cmd"] in (0x1A, 0xFF):
            # Texture banks and code-loaded blocks stay MIO0, with every byte stored uncompressed.
            move = MIO0_HEADER_LEN + (length + 7) // 8 + 2 + 2
            block = b"MIO0" + struct.pack(">III", length, move - 2, move)
            block += b"\xFF" * (move - 2 - MIO0_HEADER_LEN) + b"\0\0" + data
            out[out_addr : out_addr + len(block)] = block
            length += move
        else:
            out[out_addr : out_addr + length] = data
            if b["cmd"] == 0x18:
                b["cmd"] = 0x17
        if b["old_end"] == 0:
            b["old_end"] = old + end
        b["new"], b["new_end"] = out_addr, out_addr + length
        out_addr += length + EXT_PADDING

    for a in range(EXT_IN_START, in_len, 4):
        if out[a] in (0x17, 0x18, 0x1A) and out[a + 1] == 0x0C and out[a + 2] < 2:
            b = blocks.get(struct.unpack_from(">I", out, a + 4)[0])
            if b:
                struct.pack_into(">II", out, a + 4, b["new"], b["new_end"])
                out[a] = b["cmd"]

    for b in blocks.values():
        if b["cmd"] == 0xFF:
            a = b["asm"]
            for val, hi_at, lo_at in ((b["new"], a + 2, a + b["a1"] + 2), (b["new_end"], a + 6, a + 0xA)):
                lo, hi = val & 0xFFFF, (val >> 16) & 0xFFFF
                if lo & 0x8000:
                    hi = (hi + 1) & 0xFFFF
                struct.pack_into(">H", out, hi_at, hi)
                struct.pack_into(">H", out, lo_at, lo)

    struct.pack_into(">II", out, 0x10, *n64_checksum(out))
    return out


def n64_checksum(rom):
    """CIC-NUS-6102 header checksum."""
    m = 0xFFFFFFFF
    t1 = t2 = t3 = t4 = t5 = t6 = 0xF8CA4DDC
    for i in range(0x1000, 0x101000, 4):
        d = struct.unpack_from(">I", rom, i)[0]
        if (t6 + d) & m < t6:
            t4 = (t4 + 1) & m
        t6 = (t6 + d) & m
        t3 ^= d
        s = d & 0x1F
        r = ((d << s) | (d >> (32 - s))) & m if s else d
        t5 = (t5 + r) & m
        t2 = t2 ^ r if t2 > d else t2 ^ t6 ^ d
        t1 = (t1 + (t5 ^ d)) & m
    return t6 ^ t4 ^ t3, t5 ^ t2 ^ t1


def _vcdiff_code_table():
    """RFC 3284's default instruction table: (type1, size1, mode1, type2, size2, mode2), types 0 NOOP 1 ADD 2 RUN
    3 COPY."""
    table = [(2, 0, 0, 0, 0, 0), (1, 0, 0, 0, 0, 0)]
    table += [(1, size, 0, 0, 0, 0) for size in range(1, 18)]
    for mode in range(9):
        table.append((3, 0, mode, 0, 0, 0))
        table += [(3, size, mode, 0, 0, 0) for size in range(4, 19)]
    for mode in range(6):
        for add in range(1, 5):
            table += [(1, add, 0, 3, size, mode) for size in range(4, 7)]
    for mode in range(6, 9):
        for add in range(1, 5):
            table.append((1, add, 0, 3, 4, mode))
    table += [(3, 4, mode, 1, 1, 0) for mode in range(9)]
    return table


def apply_vcdiff(rom, delta):
    """An xdelta3 / VCDIFF patch without secondary compression."""
    if delta[:3] != b"\xd6\xc3\xc4":
        raise SystemExit("Not a VCDIFF (xdelta3) patch")
    pos = 5

    def varint():
        nonlocal pos
        value = 0
        while True:
            b = delta[pos]
            pos += 1
            value = (value << 7) | (b & 0x7F)
            if not b & 0x80:
                return value

    indicator = delta[4]
    if indicator & 1:
        raise SystemExit("xdelta patches with secondary compression aren't supported (make it with -S none)")
    if indicator & 2:
        raise SystemExit("VCDIFF custom code tables aren't supported")
    if indicator & 4:
        app_header = varint()
        pos += app_header
    table = _vcdiff_code_table()
    out = bytearray()
    while pos < len(delta):
        win = delta[pos]
        pos += 1
        source, source_len = b"", 0
        if win & 3:
            source_len, start = varint(), varint()
            source = rom[start:start + source_len] if win & 1 else bytes(out[start:start + source_len])
        varint()  # delta encoding length
        target_len = varint()
        if delta[pos] != 0:
            raise SystemExit("xdelta patches with compressed sections aren't supported")
        pos += 1
        data_len, inst_len, addr_len = varint(), varint(), varint()
        checksum = None
        if win & 4:
            checksum = struct.unpack_from(">I", delta, pos)[0]
            pos += 4
        data, insts, addrs = pos, pos + data_len, pos + data_len + inst_len
        pos = addrs + addr_len
        target = bytearray()
        near, near_slot, same = [0] * 4, 0, [0] * (3 * 256)
        d, i, a = data, insts, addrs

        def read(at):
            value = 0
            while True:
                b = delta[at]
                at += 1
                value = (value << 7) | (b & 0x7F)
                if not b & 0x80:
                    return value, at

        while i < addrs:
            code = table[delta[i]]
            i += 1
            for kind, size, mode in ((code[0], code[1], code[2]), (code[3], code[4], code[5])):
                if kind == 0:
                    continue
                if size == 0:
                    size, i = read(i)
                if kind == 1:
                    target += delta[d:d + size]
                    d += size
                elif kind == 2:
                    target += delta[d:d + 1] * size
                    d += 1
                else:
                    here = source_len + len(target)
                    if mode == 0:
                        addr, a = read(a)
                    elif mode == 1:
                        v, a = read(a)
                        addr = here - v
                    elif mode < 6:
                        v, a = read(a)
                        addr = near[mode - 2] + v
                    else:
                        addr = same[(mode - 6) * 256 + delta[a]]
                        a += 1
                    near[near_slot] = addr
                    near_slot = (near_slot + 1) % 4
                    same[addr % (3 * 256)] = addr
                    for k in range(size):
                        at = addr + k
                        target.append(source[at] if at < source_len else target[at - source_len])
        if len(target) != target_len or (checksum is not None and zlib.adler32(bytes(target)) != checksum):
            raise SystemExit("VCDIFF window doesn't match its checksum: wrong source ROM?")
        out += target
    return out


def apply_patch(rom, patch):
    """PPF3, IPS, BPS or VCDIFF (xdelta3), by its header."""
    if patch[:5] == b"PPF30":
        return apply_ppf(rom, patch)
    if patch[:3] == b"\xd6\xc3\xc4":
        return apply_vcdiff(rom, patch)
    raise SystemExit("Unsupported patch format (rom_manager.py handles PPF3 and xdelta; Torch also takes IPS and BPS)")


def apply_ppf(rom, ppf):
    if ppf[:5] != b"PPF30" or ppf[5] != 2:
        raise SystemExit("Only PPF 3.0 patches are supported")
    blockcheck, undo = ppf[0x39], ppf[0x3A]
    pos, end = 0x3C + (1024 if blockcheck else 0), len(ppf)
    if ppf[-4:] == b".DIZ":
        end -= struct.unpack_from("<I", ppf, end - 8)[0] + 38
    out = bytearray(rom)
    while pos < end:
        off = struct.unpack_from("<Q", ppf, pos)[0]
        n = ppf[pos + 8]
        if off + n > len(out):
            out.extend(b"\0" * (off + n - len(out)))
        out[off : off + n] = ppf[pos + 9 : pos + 9 + n]
        pos += 9 + n * (2 if undo else 1)
    return out


class Level:
    def __init__(self, num, seg, script_rom):
        self.num = num
        self.folder = LEVEL_FOLDERS[num]
        self.seg = seg  # the segment its script block is in: 0x19 when moved, 0x0E when edited in place
        self.script_rom = script_rom
        self.segments = {}  # seg -> (rom, compressed)
        self.areas = []  # (index, geo segaddr)
        self.terrain = []  # (area index, collision segaddr)
        self.macros = []  # macro object list segaddrs
        self.scripts = []  # scripts in its own block the level jumps to, besides its entry
        self.behaviors = set()  # behavior segaddrs its objects use
        self.models = []  # (command, segaddr): LOAD_MODEL_FROM_GEO (0x22) and LOAD_MODEL_FROM_DL (0x21)


def _u32(rom, a):
    return struct.unpack_from(">I", rom, a)[0]


def _resolve(segments, addr):
    seg = addr >> 24
    if seg not in segments:
        return None
    return segments[seg] + (addr & 0xFFFFFF)


def _segment_content(rom, start, end):
    """A loaded segment's data: decompressed, after sm64extend's fake MIO0 header, or as it is."""
    if rom[start : start + 4] == b"MIO0":
        if rom[start + 16] == 0xFF:
            return bytes(rom[start + _u32(rom, start + 12) : start + 16 + _u32(rom, start + 4)])
        data, _ = mio0_decode(rom, start)
        return bytes(data)
    return bytes(rom[start:end])


def _level_edited(rom, base_rom, start, end, entry):
    """Whether a level whose script stayed where vanilla has it was changed: its block (script and geo layouts),
    ignoring where its segments are loaded from, or the data it loads."""
    a, b = bytearray(rom[start:end]), bytearray(base_rom[start:end])
    pc = entry
    while start <= pc < end:
        cmd, length = rom[pc], rom[pc + 1]
        loads = (0x17, 0x18, 0x1A)  # extended ROMs load what was compressed with 0x17
        if length == 0 or cmd in (0x02, 0x07) or (base_rom[pc] != cmd and not {base_rom[pc], cmd} <= set(loads)):
            break
        if cmd in loads:
            if _segment_content(rom, _u32(rom, pc + 4), _u32(rom, pc + 8)) != \
                    _segment_content(base_rom, _u32(base_rom, pc + 4), _u32(base_rom, pc + 8)):
                return True
            a[pc - start : pc - start + 12] = b[pc - start : pc - start + 12]
        pc += length
    return a != b


def find_levels(rom, base_rom=None):
    """Follows the main script into the level dispatch and returns every level whose script was moved by the hack,
    and with the base ROM, the levels it edited where vanilla has them."""
    found = {}
    seen = set()

    def walk(pc, segments, level_num, depth):
        if depth > 16 or (pc, level_num) in seen:
            return
        seen.add((pc, level_num))
        for _ in range(8192):
            if pc + 4 > len(rom):
                return
            cmd, length = rom[pc], rom[pc + 1]
            if length == 0:
                return
            if cmd in (0x00, 0x01):
                seg = rom[pc + 3]
                start, end, entry = struct.unpack_from(">III", rom, pc + 4)
                segs = dict(segments)
                segs[seg] = start
                target = _resolve(segs, entry)
                if seg == 0x19 and level_num in LEVEL_FOLDERS and level_num not in found:
                    # The segments the global script loaded (common actor banks) are the level's too.
                    found[level_num] = (seg, start, target, dict(segments))
                elif seg == 0x0E and level_num in LEVEL_FOLDERS and level_num not in found and base_rom is not None \
                        and target is not None and _level_edited(rom, base_rom, start, end, target):
                    found[level_num] = (seg, start, target, dict(segments))
                elif target is not None:
                    walk(target, segs, level_num, depth + 1)
                if cmd == 0x01:
                    return
            elif cmd in (0x17, 0x18, 0x1A):
                segments = dict(segments)
                segments[rom[pc + 3]] = _u32(rom, pc + 4)
            elif cmd in (0x05, 0x06):
                target = _resolve(segments, _u32(rom, pc + 4))
                if target is not None:
                    walk(target, segments, level_num, depth + 1)
                if cmd == 0x05:
                    return
            elif cmd == 0x0C:
                op, val = rom[pc + 2], struct.unpack_from(">i", rom, pc + 4)[0]
                target = _resolve(segments, _u32(rom, pc + 8))
                if target is not None and op == 2:
                    walk(target, segments, val, depth + 1)
            if cmd in (0x02, 0x07):
                return
            pc += length

    walk(US_ENTRY_SCRIPT, {0x10: US_ENTRY_SCRIPT}, None, 0)
    return found


def _segment_data(rom, start):
    """Where a loaded segment's data is: sm64extend's uncompressed MIO0 blocks keep it after their header."""
    if rom[start : start + 4] == b"MIO0":
        return (start + _u32(rom, start + 12), rom[start + 16] != 0xFF)
    return (start, False)


def read_level(rom, num, script_seg, base, script_rom, global_segments):
    lvl = Level(num, script_seg, script_rom)
    for seg, start in global_segments.items():
        if seg not in (0x10, 0x14):
            lvl.segments[seg] = _segment_data(rom, start)
    lvl.segments[script_seg] = (base, False)
    area = None
    visited = set()

    def walk(pc):
        nonlocal area
        if pc in visited:
            return
        visited.add(pc)
        for _ in range(8192):
            cmd, length = rom[pc], rom[pc + 1]
            if length == 0:
                return
            if cmd in (0x17, 0x18, 0x1A):
                seg, start = rom[pc + 3], _u32(rom, pc + 4)
                lvl.segments[seg] = _segment_data(rom, start)
            elif cmd in (0x05, 0x06):
                # The level's own scripts and the global ones it jumps to (0x15), which load the shared actor
                # models: both are extracted with this level, so its models come from the hack's banks.
                target = _u32(rom, pc + 4)
                if target >> 24 in (script_seg, 0x15) and target >> 24 in lvl.segments:
                    if target not in lvl.scripts:
                        lvl.scripts.append(target)
                    walk(lvl.segments[target >> 24][0] + (target & 0xFFFFFF))
                if cmd == 0x05:
                    return
            elif cmd in (0x21, 0x22):
                lvl.models.append((cmd, _u32(rom, pc + 4)))
            elif cmd == 0x24:
                lvl.behaviors.add(_u32(rom, pc + 20))
            elif cmd == 0x25:
                lvl.behaviors.add(_u32(rom, pc + 8))
            elif cmd == 0x1F:
                area = rom[pc + 2]
                lvl.areas.append((area, _u32(rom, pc + 4)))
            elif cmd == 0x2E:
                lvl.terrain.append((area, _u32(rom, pc + 4)))
            elif cmd == 0x39 and _u32(rom, pc + 4) not in lvl.macros:
                lvl.macros.append(_u32(rom, pc + 4))
            if cmd in (0x02, 0x07):
                return
            pc += length

    walk(script_rom)
    return lvl


# Geo command sizes; None means variable.
GEO_SIZES = {
    0x00: 8, 0x01: 4, 0x02: 8, 0x03: 4, 0x04: 4, 0x05: 4, 0x06: 4, 0x07: 4, 0x08: 12, 0x09: 4, 0x0A: None,
    0x0B: 4, 0x0C: 4, 0x0D: 8, 0x0E: 8, 0x0F: 20, 0x10: None, 0x11: None, 0x12: None, 0x13: None, 0x14: None,
    0x15: 8, 0x16: 8, 0x17: 4, 0x18: 8, 0x19: 8, 0x1A: 8, 0x1B: 4, 0x1C: 12, 0x1D: None, 0x1E: 8, 0x1F: 16,
    0x20: 4,
}


class Resolver:
    def __init__(self, rom, segments):
        self.rom = rom
        self.segments = segments

    def rom_addr(self, segaddr):
        seg = segaddr >> 24
        if seg not in self.segments:
            return None
        return self.segments[seg][0] + (segaddr & 0xFFFFFF)


def geo_display_lists(res, geo_addr, out, seen):
    """Collects the display lists a geo layout draws, following its branches. False if it doesn't parse."""
    pc = res.rom_addr(geo_addr)
    if geo_addr in seen:
        return True
    if pc is None:
        return False
    seen.add(geo_addr)
    rom = res.rom
    for _ in range(4096):
        cmd = rom[pc]
        if cmd in (0x01, 0x03):
            return True
        if cmd in (0x00, 0x02):
            if not geo_display_lists(res, _u32(rom, pc + 4), out, seen):
                return False
            if cmd == 0x02 and rom[pc + 1] == 0:
                return True
            pc += 8
            continue
        b1 = rom[pc + 1]
        if cmd == 0x0A:
            size = 12 if b1 else 8
        elif cmd == 0x10:
            fmt = (b1 >> 4) & 7
            size = {0: 16, 1: 8, 2: 8, 3: 8}.get(fmt, 8) + (4 if b1 & 0x80 else 0)
            if b1 & 0x80:
                out.add(_u32(rom, pc + size - 4))
        elif cmd in (0x11, 0x12, 0x14):
            size = 8 + (4 if b1 & 0x80 else 0)
            if b1 & 0x80:
                out.add(_u32(rom, pc + 8))
        elif cmd == 0x13:
            size = 12
            out.add(_u32(rom, pc + 8))
        elif cmd == 0x15:
            size = 8
            out.add(_u32(rom, pc + 4))
        elif cmd == 0x1D:
            size = 8 + (4 if b1 & 0x80 else 0)
            if b1 & 0x80:
                out.add(_u32(rom, pc + 8))
        else:
            size = GEO_SIZES.get(cmd)
            if size is None:
                return False
        pc += size
    return False


FMT_NAMES = {0: "RGBA", 1: "YUV", 2: "CI", 3: "IA", 4: "I"}
SIZ_BITS = {0: 4, 1: 8, 2: 16, 3: 32}


# Every command an F3D display list can hold: DMA commands, immediate RSP commands and RDP commands.
F3D_OPCODES = set(range(0x00, 0x08)) | set(range(0xB1, 0xC1)) | set(range(0xE4, 0x100))


class RdpState:
    """What the display lists set so far. It carries over into the lists they call, as on the RDP."""

    def __init__(self):
        self.timg = None  # (addr, fmt, siz)
        self.tiles = {}  # tile -> (fmt, siz)
        self.sizes = {}  # tile -> (width, height)
        self.pending = None  # (addr, bytes) loaded into TMEM, waiting for the render tile's size


def dl_textures(res, dl_addr, textures, lights, seen, problems, state=None, stack=()):
    """Walks an F3D display list and records the textures it loads, with format and dimensions, and its lights.
    Lists that don't look like real display lists (in another bank's data, say) are reported in `problems`.
    `seen` gets the address of every list walked."""
    if dl_addr in stack:
        problems.add(f"display list {dl_addr:08X} calls itself")
        return
    st = state if state is not None else RdpState()
    # Lists are walked every time they're called: what a shared list loads depends on what its caller set (blinking
    # eyes, animated frames), and the tile state it sets carries on into the caller.
    seen.add(dl_addr)
    stack = stack + (dl_addr,)
    if dl_addr % 8:
        problems.add(f"display list {dl_addr:08X} isn't aligned")
        return
    pc = res.rom_addr(dl_addr)
    if pc is None:
        problems.add(f"display list {dl_addr:08X} is in an unmapped segment")
        return
    rom = res.rom

    def commit(tile):
        if st.pending is None or tile not in st.sizes:
            return
        addr, size = st.pending
        st.pending = None
        width, height = st.sizes[tile]
        fmt, siz = st.tiles.get(tile, (st.timg[1], st.timg[2]) if st.timg else (0, 2))
        name = FMT_NAMES.get(fmt, "RGBA") + str(SIZ_BITS[siz])
        if name.startswith("CI"):
            problems.add(f"CI texture {addr:08X} needs its palette wired up by hand")
        if name not in ("RGBA16", "RGBA32", "IA4", "IA8", "IA16", "I4", "I8", "CI4", "CI8"):
            problems.add(f"texture {addr:08X} has unsupported format {name}")
            return
        prev = textures.get(addr)
        if not prev or prev["size"] < size:
            textures[addr] = {"format": name, "width": width, "height": height, "size": size}

    for _ in range(65536):
        if pc + 8 > len(rom):
            problems.add(f"display list {dl_addr:08X} runs off the ROM")
            return
        w0, w1 = struct.unpack_from(">II", rom, pc)
        op = w0 >> 24
        if op not in F3D_OPCODES:
            problems.add(f"display list {dl_addr:08X} has an invalid command {op:02X}")
            return
        if op in (0xB8, 0xFD, 0x04, 0xBF, 0xB1):
            # Material setup is over: anything still loaded draws with the render tile as set.
            commit(0)
        if op == 0xB8:
            return
        if op == 0x06:
            dl_textures(res, w1, textures, lights, seen, problems, st, stack)
            if (w0 >> 16) & 0xFF == 1:
                return
        elif op == 0x04 and (((w0 >> 20) & 0xF) + 1) * 16 != (w0 & 0xFFFF):
            problems.add(f"display list {dl_addr:08X} has a malformed vertex load")
            return
        elif op == 0xBF and any(b % 10 or b >= 160 for b in ((w1 >> 16) & 0xFF, (w1 >> 8) & 0xFF, w1 & 0xFF)):
            problems.add(f"display list {dl_addr:08X} has a malformed triangle")
            return
        elif op == 0x03 and (w0 >> 16) & 0xFF in (0x86, 0x88, 0x8A, 0x8C, 0x8E, 0x90, 0x92, 0x94):
            lights.add(w1)
        elif op == 0xFD:
            st.timg = (w1, (w0 >> 21) & 7, (w0 >> 19) & 3) if w1 else None
        elif op == 0xF5:
            st.tiles[(w1 >> 24) & 7] = ((w0 >> 21) & 7, (w0 >> 19) & 3)
        elif op == 0xF3 and st.timg:
            texels = ((w1 >> 12) & 0xFFF) + 1
            st.pending = (st.timg[0], texels * SIZ_BITS[st.timg[2]] // 8)
        elif op == 0xF4 and st.timg:
            uls, ult = (w0 >> 12) & 0xFFF, w0 & 0xFFF
            lrs, lrt = (w1 >> 12) & 0xFFF, w1 & 0xFFF
            texels = (((lrs - uls) >> 2) + 1) * (((lrt - ult) >> 2) + 1)
            st.pending = (st.timg[0], texels * SIZ_BITS[st.timg[2]] // 8)
        elif op == 0xF0 and st.timg:
            count = ((w1 >> 14) & 0x3FF) + 1
            textures.setdefault(st.timg[0], {"format": "TLUT", "width": count, "height": 1, "size": count * 2})
            st.pending = None
        elif op == 0xF2:
            tile = (w1 >> 24) & 7
            if tile != 7:
                uls, ult = (w0 >> 12) & 0xFFF, w0 & 0xFFF
                lrs, lrt = (w1 >> 12) & 0xFFF, w1 & 0xFFF
                st.sizes[tile] = (((lrs - uls) >> 2) + 1, ((lrt - ult) >> 2) + 1)
                commit(tile)
        pc += 8


# Behavior command sizes in words, by opcode, and where their pointers are.
BEHAVIOR_SIZES = [1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 3, 1, 1, 1, 1,
                  1, 1, 2, 1, 1, 1, 2, 1, 3, 2, 3, 3, 1, 2, 2, 5, 2, 1, 2, 1, 1, 2, 2]
BEHAVIOR_END = {0x03, 0x04, 0x09, 0x0A, 0x0B, 0x1D}  # RETURN, GOTO, END_LOOP, BREAK, BREAK_UNUSED, DEACTIVATE
BEHAVIOR_JUMPS = {0x02: 4, 0x04: 4}  # CALL, GOTO
BEHAVIOR_SPAWNS = {0x1C: 8, 0x29: 8, 0x2C: 8}  # SPAWN_CHILD, SPAWN_CHILD_WITH_PARAM, SPAWN_OBJ
BEHAVIOR_NATIVE = 0x0C
BEHAVIOR_ANIMATIONS = 0x27
BEHAVIOR_COLLISION = 0x2A
VANILLA_BEHAVIORS = 0x219E00  # segment 0x13 in the US ROM, which sm64extend leaves in place


def read_symbols(path):
    symbols = {}
    for line in open(path, encoding="utf-8"):
        line = line.split("#", 1)[0].strip()
        if ":" in line:
            addr, name = line.split(":", 1)
            symbols[int(addr, 0)] = name.strip()
    return symbols


def walk_behavior(res, addr):
    """The pointers in one behavior script: (kind, segaddr) with kind behavior, native, collision or animations."""
    pc = res.rom_addr(addr)
    out = []
    if pc is None:
        return None
    rom = res.rom
    for _ in range(1024):
        op = rom[pc]
        if op >= len(BEHAVIOR_SIZES):
            return None
        if op in BEHAVIOR_JUMPS:
            out.append(("behavior", _u32(rom, pc + BEHAVIOR_JUMPS[op])))
        elif op in BEHAVIOR_SPAWNS:
            out.append(("behavior", _u32(rom, pc + BEHAVIOR_SPAWNS[op])))
        elif op == BEHAVIOR_NATIVE:
            out.append(("native", _u32(rom, pc + 4)))
        elif op == BEHAVIOR_COLLISION:
            out.append(("collision", _u32(rom, pc + 4)))
        elif op == BEHAVIOR_ANIMATIONS:
            out.append(("animations", _u32(rom, pc + 4)))
        if op in BEHAVIOR_END:
            return out
        pc += BEHAVIOR_SIZES[op] * 4
    return None


def animation_table(res, addr):
    """The animations a LOAD_ANIMATIONS table points at. The ROM doesn't store the count, so this reads pointers
    while they lead to something shaped like an animation header in the same segment."""
    seg = addr >> 24
    pc = res.rom_addr(addr)
    anims = []
    if pc is None:
        return anims
    rom = res.rom
    for i in range(64):
        ptr = _u32(rom, pc + i * 4)
        hdr = res.rom_addr(ptr) if ptr >> 24 == seg else None
        if hdr is None or hdr + 0x18 > len(rom):
            break
        bones = struct.unpack_from(">h", rom, hdr + 0x0A)[0]
        values, index = _u32(rom, hdr + 0x0C), _u32(rom, hdr + 0x10)
        # Torch reads the values up to the index table, so the values come first, in the same segment.
        if values >> 24 != seg or index >> 24 != seg or not 0 < index - values < 0x40000 or not 0 < bones < 64:
            break
        if res.rom_addr(index) is None or res.rom_addr(index) + (bones + 1) * 12 > len(rom):
            break
        anims.append(ptr)
    return anims


# The intro (title logo) keeps the vanilla script, which the game runs compiled; a hack changes the logo through the
# geo layouts the script draws, in segment 0x14 next to the script, and the model they draw from segment 0x07.
INTRO_SCRIPT = 0x269EA0
INTRO_GEOS = {"intro_geo_0002D0": 0x140002D0}


def intro_segments(rom):
    """Segment 0x07 as the hack's intro script loads it, or None if it loads none."""
    pc = INTRO_SCRIPT
    for _ in range(64):
        cmd, length = rom[pc], rom[pc + 1]
        if length == 0 or cmd in (0x02, 0x07):
            return None
        if cmd in (0x17, 0x18, 0x1A) and rom[pc + 3] == 0x07:
            return {0x14: (INTRO_SCRIPT, False), 0x07: _segment_data(rom, _u32(rom, pc + 4))}
        pc += length
    return None


# What the intro's compiled code draws from segment 0x07 (geo_intro_super_mario_64_logo), by its vanilla name: the logo
# and its scale tables. The hack's versions shadow vanilla's at the same paths.
INTRO_LOGO_DL = ("intro_seg7_dl_0700B3A0", 0x0700B3A0)
INTRO_LOGO_TABLES = (("intro_seg7_table_0700C790", 0x0700C790, 60), ("intro_seg7_table_0700C880", 0x0700C880, 48))


def intro_yaml(rom, base_rom, out_root, problems):
    """The hack's title logo, under the intro's vanilla resource names so it shadows vanilla's."""
    segments = intro_segments(rom)
    if segments is None or segments[0x07][1]:
        return
    res = Resolver(rom, segments)
    seg_list = [(seg, start) for seg, (start, _) in sorted(segments.items())]
    written = 0

    # Geo layouts next to the script: the hack can drop or swap what they call.
    geo_nodes = OrderedDict()
    for name, geo in INTRO_GEOS.items():
        a = res.rom_addr(geo)
        if rom[a:a + 0x90] != base_rom[a:a + 0x90]:
            geo_nodes[name] = {"type": "SM64:GEO_LAYOUT", "offset": geo, "symbol": name}
    if geo_nodes:
        write_yaml(os.path.join(out_root, "levels", "intro", "geo.yml"), {"segments": seg_list}, geo_nodes)
        written += len(geo_nodes)

    # The logo model, when the segment it's in isn't vanilla's.
    vanilla_model, _ = mio0_decode(base_rom, 0x26A3A0)
    model_start = segments[0x07][0]
    if rom[model_start:model_start + len(vanilla_model)] == vanilla_model:
        return
    name, dl = INTRO_LOGO_DL
    textures, lights = {}, set()
    dl_problems = set()
    dl_textures(res, dl, textures, lights, set(), dl_problems)
    if dl_problems:
        problems.update(dl_problems)
        return
    nodes = OrderedDict()
    nodes[name] = {"type": "GFX", "offset": dl, "symbol": name}
    for table, addr, count in INTRO_LOGO_TABLES:
        nodes[table] = {"type": "ARRAY", "offset": addr, "array_type": "f32", "count": count, "symbol": table}
    for addr, tex in sorted(textures.items()):
        tname = f"intro_tex_{addr:08X}"
        nodes[tname] = {"type": "TEXTURE", "offset": addr, "width": tex["width"], "height": tex["height"],
                        "format": tex["format"], "size": tex["size"], "symbol": tname}
    for addr in sorted(lights):
        if addr - 8 not in lights:
            nodes[f"intro_lights_{addr:08X}"] = {"type": "LIGHTS", "offset": addr, "size": 24,
                                                  "symbol": f"intro_lights_{addr:08X}"}
    write_yaml(os.path.join(out_root, "levels", "intro.yml"), {"segments": seg_list}, nodes)
    print(f"intro              geo layouts={written} logo textures={len(textures)}")


# Music: the sequence bank and the bank sets (which instrument banks each sequence plays with). Hacks move them and
# repoint the code that loads them, so they're found through that code.
US_SEQUENCES = 0x7B0860
US_BANK_SETS = 0x7CC620


def code_pointers(base_rom, target):
    """Where the vanilla code builds `target` with lui + addiu/ori, as (lui, low) instruction offsets."""
    out = []
    hi = (target >> 16) + (1 if target & 0x8000 else 0)
    for a in range(0x1000, 0x100000, 4):
        w = _u32(base_rom, a)
        if w >> 26 == 0x0F and (w & 0xFFFF) == hi:
            rt = (w >> 16) & 31
            for b in range(a + 4, a + 0x40, 4):
                w2 = _u32(base_rom, b)
                if w2 >> 26 in (0x09, 0x0D) and ((w2 >> 21) & 31) == rt and (w2 & 0xFFFF) == target & 0xFFFF:
                    out.append((a, b))
    return out


def read_pointer(rom, pairs):
    """The address the same instructions build in the hack."""
    values = set()
    for a, b in pairs:
        hi = struct.unpack_from(">H", rom, a + 2)[0]
        lo = struct.unpack_from(">h", rom, b + 2)[0] if rom[b] >> 2 == 0x09 else struct.unpack_from(">H", rom, b + 2)[0]
        values.add(((hi << 16) + lo) & 0xFFFFFFFF)
    return values.pop() if len(values) == 1 else None


def yaml_names(path, key_field="id"):
    """`id -> node name` from a vanilla audio YAML."""
    names, current = {}, None
    for line in open(path, encoding="utf-8"):
        if line and not line[0].isspace() and line.rstrip().endswith(":") and not line.startswith(":"):
            current = line.strip()[:-1]
        elif current and line.strip().startswith(f"{key_field}:"):
            names[int(line.split(":", 1)[1].strip(), 0)] = current
    return names


def audio_yaml(rom, base_rom, out_root, base_yamls, problems):
    """sound/sequences/<name> for every sequence: vanilla ids keep their names, so they replace vanilla's music."""
    seq_addr = read_pointer(rom, code_pointers(base_rom, US_SEQUENCES))
    sets_addr = read_pointer(rom, code_pointers(base_rom, US_BANK_SETS))
    if seq_addr is None or sets_addr is None:
        problems.add("couldn't find where the code loads the sequences from")
        return
    if seq_addr == US_SEQUENCES and sets_addr == US_BANK_SETS and \
            rom[US_SEQUENCES:US_BANK_SETS] == base_rom[US_SEQUENCES:US_BANK_SETS]:
        return
    seq_names = yaml_names(os.path.join(base_yamls, "sound", "sequences.yml"))
    bank_names = yaml_names(os.path.join(base_yamls, "sound", "banks.yml"))
    _, count = struct.unpack_from(">HH", rom, seq_addr)
    offsets = struct.unpack_from(f">{count}H", rom, sets_addr)
    path = os.path.join(out_root, "sound", "sequences.yml")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", newline="\n") as f:
        f.write("# Generated by tools/romhacks/rom_manager.py. Do not edit by hand.\n")
        for i in range(count):
            off, size = struct.unpack_from(">II", rom, seq_addr + 4 + i * 8)
            n = rom[sets_addr + offsets[i]]
            # The bank set lists a sequence's banks last to first.
            banks = list(rom[sets_addr + offsets[i] + 1:sets_addr + offsets[i] + 1 + n])[::-1]
            missing = [b for b in banks if b not in bank_names]
            if missing:
                problems.add(f"sequence {i} uses instrument banks {missing} the base game doesn't have")
                continue
            f.write(f"\n{seq_names.get(i, f'seq_{i:02X}')}:\n")
            f.write("  type: NAUDIO:V0:SEQUENCE\n")
            f.write(f"  id: {i}\n  offset: 0x{seq_addr + off:X}\n  size: {size}\n  banks:\n")
            for b in banks:
                f.write(f"    - sound/banks/{bank_names[b]}\n")
    print(f"music              sequences={count} at 0x{seq_addr:X}, bank sets at 0x{sets_addr:X}")


# Text: segment 2 holds the dialogs, course and act names. Hacks edit the strings and repoint the tables to them.
US_SEGMENT_2 = 0x108A40


def yaml_nodes(path):
    """`name -> offset` from a vanilla YAML, in file order."""
    nodes, current = OrderedDict(), None
    for line in open(path, encoding="utf-8"):
        if line and not line[0].isspace() and line.rstrip().endswith(":") and not line.startswith(":"):
            current = line.strip()[:-1]
        elif current and line.strip().startswith("offset:"):
            nodes[current] = int(line.split(":", 1)[1].strip(), 0)
    return nodes


def text_yaml(rom, raw_base, out_root, base_yamls, problems):
    """texts/<dialogs|courses|acts>/<name>, under the vanilla names, when the hack changed segment 2."""
    vanilla, _ = mio0_decode(raw_base, US_SEGMENT_2)
    # The font textures at the start are rarely touched, so they find the hack's copy of the segment.
    start = rom.find(vanilla[:256], 0x800000)
    if start < 0 or rom.find(vanilla[:256], start + 1) >= 0:
        problems.add("couldn't find the hack's text segment")
        return
    if rom[start:start + len(vanilla)] == vanilla:
        return
    seg = [(0x02, start)]
    written = {}
    for kind, typ in (("dialogs", "SM64:DIALOG"), ("courses", "SM64:TEXT"), ("acts", "SM64:TEXT")):
        names = yaml_nodes(os.path.join(base_yamls, "texts", f"{kind}.yml"))
        nodes = OrderedDict()
        if kind == "dialogs":
            # The table of dialog entries stays where it was; each entry points at the hack's string.
            for name, off in names.items():
                nodes[name] = {"type": typ, "offset": 0x02000000 + off}
        else:
            # Names are read through the hack's table, which sits where vanilla's did.
            first = next(iter(names.values()))
            table = vanilla.find(struct.pack(">I", 0x02000000 + first))
            if table < 0:
                problems.add(f"couldn't find the {kind} table")
                continue
            for i, name in enumerate(names):
                ptr = _u32(rom, start + table + i * 4)
                if ptr >> 24 != 0x02:
                    problems.add(f"{kind} {name} points at {ptr:08X}, outside the text segment")
                    continue
                nodes[name] = {"type": typ, "offset": ptr}
        write_yaml(os.path.join(out_root, "texts", f"{kind}.yml"), {"segments": seg}, nodes)
        written[kind] = len(nodes)
    print("text               " + " ".join(f"{k}={v}" for k, v in written.items()))


# Actors the main script loads for every level, which the compiled game draws through their vanilla names: Mario
# (models 0x04, geo layouts 0x17) and the common actors (0x03, 0x16). Hacks edit them in place, so changed nodes
# keep their vanilla names and shadow vanilla's. Group banks come from the ROM with the levels that load them.
ACTOR_BANKS = {0x04: 0x114750, 0x03: 0x201410, 0x17: 0x1279B0, 0x16: 0x218DA0}  # segment -> US ROM offset
ACTOR_MODELS = (0x03, 0x04)  # compressed in the US ROM, node offsets relative to the bank
ACTOR_GEO_ENDS = {0x17: 0x12A7E0, 0x16: 0x219E00}  # where the raw geo layout banks end, at the next block


def yaml_file_nodes(path):
    """A vanilla YAML's config text and its nodes, as (name, field lines, offset, type), fields kept as written."""
    config, nodes, current = [], [], None
    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if not line[0].isspace():
            current = None if line.startswith(":config:") else [line.strip()[:-1], [], None, None]
            if current:
                nodes.append(current)
        elif current is None:
            config.append(line)
        else:
            key, _, value = line.strip().partition(":")
            if key == "offset" and line.startswith("  offset:"):
                current[2] = int(value.strip(), 0)
            elif key == "type" and line.startswith("  type:"):
                current[3] = value.strip()
            current[1].append(line)
    return "\n".join(config), [tuple(n) for n in nodes if n[2] is not None]


def yaml_bank(config):
    """The actor bank segment a vanilla YAML's config maps, if it's one of ACTOR_BANKS."""
    for seg, start in re.findall(r"\[\s*(0x[0-9A-Fa-f]+)\s*,\s*(0x[0-9A-Fa-f]+)\s*\]", config):
        if ACTOR_BANKS.get(int(seg, 16)) == int(start, 16):
            return int(seg, 16)
    return None


def actor_yaml(rom, raw_base, global_segments, out_root, base_yamls, yaml_root, problems):
    """actors/... for each node of the shared actor banks the hack changed, under its vanilla file and name."""
    hack = {seg: _segment_data(rom, global_segments[seg])[0] for seg in ACTOR_BANKS if seg in global_segments}
    res = Resolver(rom, {seg: (start, False) for seg, start in hack.items()})
    files = {}  # path relative to base_yamls -> (segment, [(name, lines, segaddr, type)])
    for root, _, names in os.walk(os.path.join(base_yamls, "actors")):
        for fname in sorted(names):
            path = os.path.join(root, fname)
            config, nodes = yaml_file_nodes(path)
            seg = yaml_bank(config)
            if seg is None or seg not in hack:
                continue
            rel = os.path.relpath(path, base_yamls).replace(os.sep, "/")
            files[rel] = (seg, [(n, lines, (seg << 24) | off if off < 0x1000000 else off, t)
                                for n, lines, off, t in nodes])
    if not files:
        return
    vanilla = {}
    for seg, off in ACTOR_BANKS.items():
        vanilla[seg] = mio0_decode(raw_base, off)[0] if seg in ACTOR_MODELS else raw_base[off:ACTOR_GEO_ENDS[seg]]
    by_addr = {}  # segaddr -> (file, node)
    for rel, (seg, nodes) in files.items():
        for node in nodes:
            by_addr.setdefault(node[2], (rel, node))
    ends = {}
    for seg in ACTOR_BANKS:
        addrs = sorted(a for a in by_addr if a >> 24 == seg)
        for a, b in zip(addrs, addrs[1:] + [(seg << 24) + len(vanilla[seg])]):
            ends[a] = b

    def changed(addr):
        seg, off, end = addr >> 24, addr & 0xFFFFFF, ends[addr] & 0xFFFFFF
        return rom[hack[seg] + off:hack[seg] + end] != vanilla[seg][off:end]

    out = {}  # rel -> OrderedDict(name -> field lines)
    new_nodes = {}  # rel -> OrderedDict(name -> node)

    def include(rel, name, lines, addr):
        fixed = [f"  offset: {hexs(addr)}" if l.startswith("  offset:") else l for l in lines]
        out.setdefault(rel, OrderedDict())[name] = fixed

    def visible(rel, target):
        # Geo layout files see the model files, as in vanilla; a file always sees itself.
        return target == rel or (files[rel][0] not in ACTOR_MODELS and files[target][0] in ACTOR_MODELS)

    for rel, (seg, nodes) in files.items():
        for name, lines, addr, typ in nodes:
            if addr not in ends or not changed(addr):
                continue
            if typ in ("TEXTURE", "LIGHTS", "VTX"):
                include(rel, name, lines, addr)
                continue
            if typ not in ("GFX", "SM64:GEO_LAYOUT"):
                problems.add(f"actor {name} ({typ}) changed, kept vanilla's")
                continue
            dls, bad, walked, textures, lights = set(), set(), set(), {}, set()
            if typ == "GFX":
                dls.add(addr)
            elif not geo_display_lists(res, addr, dls, set()):
                bad.add("geo")
            for dl in sorted(dls - {0}):
                dl_textures(res, dl, textures, lights, walked, bad)
            if bad:
                problems.add(f"actor {name} changed into something that doesn't draw, kept vanilla's")
                continue
            include(rel, name, lines, addr)
            stem = rel[:-4].replace("/", "_")
            for ref in sorted(walked | set(textures) | {l for l in lights if l - 8 not in lights}):
                known = by_addr.get(ref)
                if known and visible(rel, known[0]):
                    include(known[0], known[1][0], known[1][1], ref)
                elif ref in textures:
                    t = textures[ref]
                    new_nodes.setdefault(rel, OrderedDict())[f"{stem}_tex_{ref:08X}"] = {
                        "type": "TEXTURE", "offset": ref, "width": t["width"], "height": t["height"],
                        "format": t["format"], "size": t["size"], "symbol": f"{stem}_tex_{ref:08X}"}
                elif ref in lights:
                    new_nodes.setdefault(rel, OrderedDict())[f"{stem}_lights_{ref:08X}"] = {
                        "type": "LIGHTS", "offset": ref, "size": 24, "symbol": f"{stem}_lights_{ref:08X}"}
    models = [f"{yaml_root}/{rel}" for rel in sorted(out) if files[rel][0] in ACTOR_MODELS]
    segments = sorted(hack.items())
    for rel in sorted(set(out) | set(new_nodes)):
        path = os.path.join(out_root, rel)
        externals = [] if files[rel][0] in ACTOR_MODELS else models
        write_yaml(path, {"segments": segments, "external_files": externals}, new_nodes.get(rel, OrderedDict()))
        with open(path, "a", newline="\n") as f:
            for name, lines in out.get(rel, {}).items():
                f.write(f"\n{name}:\n" + "\n".join(lines) + "\n")
    print(f"actors             files={len(set(out) | set(new_nodes))} "
          f"nodes={sum(len(v) for v in out.values()) + sum(len(v) for v in new_nodes.values())}")


# ROM Manager water boxes: the level's geo layout draws quad collections 0x5000 to 0x5003, which the hack's code
# finds in the level script segment. Each is a list of {water box id, quad array} ending at id -1.
ROMHACK_WATER_ID = 0x5000
ROMHACK_WATER = 0x19001800
ROMHACK_WATER_STRIDE = 0x50


def water_boxes(rom, lvl):
    """(segaddr, entries) of each water box collection, without its terminator: hacks leave garbage in the
    terminator's pointer, or skip it, and the port adds one."""
    res = Resolver(rom, lvl.segments)
    out = []
    if lvl.seg != 0x19:
        return out
    for n in range(4):
        addr = ROMHACK_WATER + n * ROMHACK_WATER_STRIDE
        pc = res.rom_addr(addr)
        count = 0
        for i in range(ROMHACK_WATER_STRIDE // 8):
            box, _, quads = struct.unpack_from(">hhI", rom, pc + i * 8)
            if box == -1:
                count = i
                break
            arr = res.rom_addr(quads) if quads >> 24 == 0x19 else None
            if arr is None or not 0 < struct.unpack_from(">h", rom, arr)[0] <= 64:
                count = i
                break
        out.append((addr, count))
    return out


# The hack's N64 RAM image, for code ports that read the tables and patches a hack made to the game's code
# (src/port/romhack/RamImage.h): the code and engine segments, and every block its code DMAs into expansion RAM.
RAM_SEGMENTS = [(0x80246000, 0x1000, 0xF5580), (0x80378800, 0xF5580, 0x108A10)]  # RAM, ROM start, ROM end (US)
DMA_READ = 0x80278504  # dma_read(dest, srcStart, srcEnd)


def _code_ram(rom_off):
    for ram, start, end in RAM_SEGMENTS:
        if start <= rom_off < end:
            return ram + rom_off - start
    return None


def dma_blocks(rom, base_rom):
    """(RAM, ROM start, ROM end) for each dma_read call the hack added with constant arguments into expansion RAM."""
    blocks = set()
    jal = 0x0C000000 | ((DMA_READ & 0x0FFFFFFF) >> 2)
    for _, start, end in RAM_SEGMENTS:
        for pc in range(start, end, 4):
            if _u32(rom, pc) != jal or rom[pc - 16:pc + 8] == base_rom[pc - 16:pc + 8]:
                continue
            regs = {}
            # lui/ori pairs for $a0-$a2 in the instructions before the call and its delay slot.
            for q in list(range(pc - 32, pc, 4)) + [pc + 4]:
                w = _u32(rom, q)
                op, rs, rt, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
                if op == 0x0F and rt in (4, 5, 6):
                    regs[rt] = imm << 16
                elif op == 0x0D and rt in (4, 5, 6) and rs == rt and rt in regs:
                    regs[rt] |= imm
            dest, src, src_end = regs.get(4), regs.get(5), regs.get(6)
            if dest is not None and src is not None and src_end is not None and \
                    0x80400000 <= dest < 0x80800000 and src < src_end <= len(rom):
                blocks.add((dest, src, src_end))
    return sorted(blocks)


def ram_yaml(rom, base_rom, out_root):
    """romhack/ram_<address> blobs, when the hack changed the game's code."""
    regions = [(ram, start, end) for ram, start, end in RAM_SEGMENTS]
    blocks = dma_blocks(rom, base_rom)
    if not blocks and all(rom[a:b] == base_rom[a:b] for _, a, b in RAM_SEGMENTS):
        return
    regions += blocks
    if len(regions) > 6:
        raise SystemExit(f"the hack loads {len(blocks)} blocks of its own; the RAM image has room for 4")
    nodes = OrderedDict()
    segments = []
    for i, (ram, start, end) in enumerate(regions):
        seg = 0x1F - i  # segments SM64 never uses (it stops at 0x19)
        segments.append((seg, start))
        nodes[f"ram_{ram:08X}"] = {"type": "BLOB", "offset": seg << 24, "size": end - start,
                                   "symbol": f"ram_{ram:08X}"}
    write_yaml(os.path.join(out_root, "romhack.yml"), {"segments": segments}, nodes)
    print("ram image          " + " ".join(f"{ram:08X}+{end - start:X}" for ram, start, end in regions))


SKYBOX_TILE_SIZE = 0x800  # 32x32 RGBA16
SKYBOX_PTRLIST = 0x20000  # where the 80 tile pointers follow the tiles
SKYBOX_PTRS = 80


def level_skybox(rom, lvl):
    """The level's skybox segment and its tile pointers, if segment 0x0A holds one laid out like vanilla's."""
    if 0x0A not in lvl.segments:
        return None
    start, compressed = lvl.segments[0x0A]
    if compressed or start + SKYBOX_PTRLIST + SKYBOX_PTRS * 4 > len(rom):
        return None
    ptrs = struct.unpack_from(f">{SKYBOX_PTRS}I", rom, start + SKYBOX_PTRLIST)
    if not all(p >> 24 == 0x0A and (p & 0xFFFFFF) % SKYBOX_TILE_SIZE == 0 and (p & 0xFFFFFF) < SKYBOX_PTRLIST
               for p in ptrs):
        return None
    return start, ptrs


def hexs(v):
    return f"0x{v:X}"


def write_yaml(path, config, nodes):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", newline="\n") as f:
        f.write("# Generated by tools/romhacks/rom_manager.py. Do not edit by hand.\n")
        f.write(":config:\n")
        f.write("  autopads: false\n")
        if config.get("directory"):
            f.write(f"  directory: {config['directory']}\n")
        if config.get("external_files"):
            f.write("  external_files:\n")
            for ext in config["external_files"]:
                f.write(f'    - "{ext}"\n')
        f.write("  segments:\n")
        for seg, rom in config["segments"]:
            f.write(f"    - [ {hexs(seg)}, {hexs(rom)} ]\n")
        for name, node in nodes.items():
            f.write(f"\n{name}:\n")
            for k, v in node.items():
                f.write(f"  {k}: {hexs(v) if k == 'offset' else v}\n")


def load_extra(path):
    """Extra assets: `<folder>: [{name, type, offset}]`, e.g. collision a custom behavior loads."""
    extra = {}
    if not path:
        return extra
    folder = None
    for raw in open(path, encoding="utf-8"):
        line = raw.split("#", 1)[0].rstrip()
        if not line.strip():
            continue
        if not line.startswith(" "):
            folder = line.rstrip(":").strip()
            extra[folder] = []
        else:
            fields = dict(p.strip().split(":", 1) for p in line.strip().lstrip("- ").strip("{}").split(","))
            extra[folder].append({k.strip(): v.strip() for k, v in fields.items()})
    return extra


def cmd_yamls(args):
    rom = open(args.rom, "rb").read()
    raw_base = open(args.base_rom, "rb").read() if args.base_rom else None
    base_rom = extend_rom(raw_base) if raw_base else None
    if args.scripts and base_rom is None:
        raise SystemExit("--scripts needs --base-rom, to tell the behaviors the hack rewrote from vanilla ones")
    symbols = {}
    for path in args.symbols or ["assets/symbols/us.yml"]:
        symbols.update(read_symbols(path))
    extra = load_extra(args.extra)
    levels = find_levels(rom, raw_base)
    banks = {}  # rom offset of a seg 9 bank -> textures
    skyboxes = {}  # level folder -> (rom offset of its segment 0x0A, tile pointers)
    out_root = args.outdir
    summary = []

    for num, (script_seg, base, script, global_segments) in sorted(levels.items()):
        lvl = read_level(rom, num, script_seg, base, script, global_segments)
        folder = lvl.folder
        res = Resolver(rom, lvl.segments)
        nodes = OrderedDict()
        problems = set()
        textures = {}
        dls = set()
        for area, geo in lvl.areas:
            rom_addr = res.rom_addr(geo)
            name = f"Geo_{folder}_{area}_0x{rom_addr:x}"
            nodes[name] = {"type": "SM64:GEO_LAYOUT", "offset": geo, "symbol": name}
            geo_display_lists(res, geo, dls, set())
        for area, col in lvl.terrain:
            name = f"col_{folder}_{area}_0x{col:x}"
            nodes[name] = {"type": "SM64:COLLISION", "offset": col, "symbol": name}
        for addr in lvl.macros:
            name = f"{folder}_macro_{addr:08X}"
            nodes[name] = {"type": "SM64:MACRO", "offset": addr, "symbol": name}
        for index, (addr, count) in enumerate(water_boxes(rom, lvl)):
            if count:
                name = f"{folder}_water_{ROMHACK_WATER_ID + index:04X}"
                nodes[name] = {"type": "SM64:MOVTEX_QUAD", "count": count, "offset": addr, "symbol": name}
        for item in extra.get(folder, []):
            nodes[item["name"]] = {"type": item["type"], "offset": int(item["offset"], 0), "symbol": item["name"]}
            if item["type"] == "SM64:GEO_LAYOUT":
                geo_display_lists(res, int(item["offset"], 0), dls, set())

        behaviors = OrderedDict()  # rewritten behaviors to extract, into the level's script file
        collision = set()  # collision the extracted behaviors load
        if args.scripts:
            taken = {n["offset"] for n in nodes.values()}

            def add(name, node):
                if node["offset"] not in taken:
                    taken.add(node["offset"])
                    nodes[name] = node

            # Behaviors the hack rewrote in place, or new ones inside the vanilla behavior segment, come from the
            # ROM; untouched vanilla behaviors stay names the game has compiled in.
            starts = sorted(a for a in symbols if a >> 24 == 0x13)

            def rewritten(addr):
                if addr not in symbols:
                    return True
                nxt = next((a for a in starts if a > addr), addr + 0x40)
                a, b = VANILLA_BEHAVIORS + (addr & 0xFFFFFF), VANILLA_BEHAVIORS + (nxt & 0xFFFFFF)
                return rom[a:b] != base_rom[a:b]

            queue = sorted(lvl.behaviors)
            while queue:
                beh = queue.pop(0)
                if beh in behaviors or beh >> 24 != 0x13 or not rewritten(beh):
                    continue
                ptrs = walk_behavior(res, beh)
                if ptrs is None:
                    problems.add(f"behavior {beh:08X} doesn't parse")
                    continue
                behaviors[beh] = f"{folder}_bhv_{beh:08X}"
                for kind, ptr in ptrs:
                    if kind == "behavior":
                        queue.append(ptr)
                    elif kind == "native" and ptr not in symbols:
                        problems.add(f"behavior {beh:08X} calls {ptr:08X}, which isn't a known function")
                    elif kind == "collision" and ptr:
                        collision.add(ptr)
                        add(f"{folder}_col_{ptr:08X}", {"type": "SM64:COLLISION", "offset": ptr,
                                                        "symbol": f"{folder}_col_{ptr:08X}"})
                    elif kind == "animations" and ptr:
                        anims = animation_table(res, ptr)
                        if not anims:
                            problems.add(f"behavior {beh:08X} loads animations at {ptr:08X} that don't parse")
                            continue
                        for anim in anims:
                            add(f"{folder}_anim_{anim:08X}", {"type": "SM64:ANIM", "offset": anim,
                                                              "symbol": f"{folder}_anim_{anim:08X}"})
                        add(f"{folder}_anims_{ptr:08X}", {"type": "ASSET_ARRAY", "assetType": "Animation",
                                                          "factoryType": "SM64:ANIM", "count": len(anims),
                                                          "offset": ptr, "symbol": f"{folder}_anims_{ptr:08X}"})

            # Models the level loads, itself or through the global scripts it jumps to.
            for cmd, ptr in lvl.models:
                if ptr >> 24 not in lvl.segments:
                    continue
                if cmd == 0x22:
                    found = set()
                    bad, walked = set(), set()
                    if geo_display_lists(res, ptr, found, set()):
                        for d in found - {0}:
                            dl_textures(res, d, {}, set(), walked, bad)
                    else:
                        bad.add("geo")
                    if bad or walked & collision:
                        # Usually a vanilla model whose display lists were in a bank this level doesn't load,
                        # so they run into garbage or into collision an object here really uses.
                        problems.add(f"model geo {ptr:08X} doesn't point at a valid model here, left out")
                        continue
                    add(f"{folder}_geo_{ptr:08X}", {"type": "SM64:GEO_LAYOUT", "offset": ptr,
                                                    "symbol": f"{folder}_geo_{ptr:08X}"})
                    dls |= found
                else:
                    add(f"{folder}_dl_{ptr:08X}", {"type": "GFX", "offset": ptr, "symbol": f"{folder}_dl_{ptr:08X}"})
                    dls.add(ptr)

        seen = set()
        lights = set()
        dls.discard(0)
        for dl in sorted(dls):
            dl_textures(res, dl, textures, lights, seen, problems)
        for name in [n for n, node in nodes.items() if node["type"] == "SM64:COLLISION" and node["offset"] in seen]:
            # A behavior pointing into a bank this level doesn't have, where a display list happens to be.
            problems.add(f"collision {nodes[name]['offset']:08X} is a display list in this level, left out")
            del nodes[name]
        for addr in sorted(lights):
            # Torch looks a light up at its address, then 8 bytes before it, so one node covers a light pair.
            if addr - 8 in lights:
                continue
            name = f"{folder}_lights_{addr:08X}"
            nodes[name] = {"type": "LIGHTS", "offset": addr, "size": 24, "symbol": name}

        segments = [(lvl.seg, lvl.segments[lvl.seg][0])]
        for n in nodes.values():
            if n["offset"] >> 24 not in lvl.segments:
                problems.add(f"{n['symbol']} is in segment {n['offset'] >> 24:02X}, which this level doesn't load")
        # Everything the level loads, so models and collision can point into the actor banks. Texture banks get
        # their own files, and compressed segments can't be mapped.
        for seg, (start, compressed) in sorted(lvl.segments.items()):
            if seg not in (lvl.seg, 0x09, 0x0A) and not compressed:
                segments.append((seg, start))
        externals = []
        for addr, tex in sorted(textures.items()):
            seg = addr >> 24
            if seg in (0x0E, 0x19) or (seg != 0x09 and seg in dict(segments)):
                name = f"{folder}_tex_{addr:08X}"
                nodes[name] = {
                    "type": "TEXTURE", "offset": addr, "width": tex["width"], "height": tex["height"],
                    "format": tex["format"], "size": tex["size"], "symbol": name,
                }
            elif seg == 0x09 and 0x09 in lvl.segments:
                base, compressed = lvl.segments[0x09]
                if compressed:
                    problems.add(f"segment 9 bank at {base:08X} is really compressed")
                    continue
                banks.setdefault(base, {})[addr] = tex
                ext = f"{args.yaml_root}/textures/bank_{base:08X}.yml"
                if ext not in externals:
                    externals.append(ext)
            else:
                problems.add(f"texture {addr:08X} is in segment {seg:02X}, which isn't extracted")

        write_yaml(os.path.join(out_root, "levels", f"{folder}.yml"),
                   {"segments": segments, "external_files": externals}, nodes)
        if args.scripts:
            # The level script itself, at scripts/levels/<folder> so it replaces the vanilla level, and the scripts
            # it jumps to. Torch writes behaviors and functions by name, from the game's symbols file.
            base = lvl.segments[lvl.seg][0]
            scripts = OrderedDict()
            scripts[folder] = {"type": "SM64:LEVEL_SCRIPT", "offset": (lvl.seg << 24) + lvl.script_rom - base,
                               "symbol": f"level_{folder}_entry"}
            for addr in lvl.scripts:
                name = f"{folder}_script_{addr:08X}"
                scripts[name] = {"type": "SM64:LEVEL_SCRIPT", "offset": addr, "symbol": name}
            for addr, name in behaviors.items():
                scripts[name] = {"type": "SM64:BEHAVIOR_SCRIPT", "offset": addr, "symbol": name}
            write_yaml(os.path.join(out_root, "scripts", f"{folder}.yml"),
                       {"segments": segments, "directory": "scripts/levels",
                        "external_files": [f"{args.yaml_root}/levels/{folder}.yml"] + externals}, scripts)
        sky = level_skybox(rom, lvl)
        if sky:
            skyboxes[folder] = sky
        summary.append((folder, len(lvl.areas), len(dls), len(textures), sorted(problems)))

    intro_problems = set()
    if base_rom is not None:
        intro_yaml(rom, base_rom, out_root, intro_problems)
        audio_yaml(rom, base_rom, out_root, args.base_yamls, intro_problems)
        text_yaml(rom, raw_base, out_root, args.base_yamls, intro_problems)
        ram_yaml(rom, base_rom, out_root)
        if levels:
            actor_yaml(rom, raw_base, next(iter(levels.values()))[3], out_root, args.base_yamls, args.yaml_root,
                       intro_problems)
    for p in sorted(intro_problems):
        print(f"    ! intro: {p}")

    for base, textures in sorted(banks.items()):
        nodes = OrderedDict()
        for addr, tex in sorted(textures.items()):
            name = f"bank_{base:08X}_tex_{addr & 0xFFFFFF:06X}"
            nodes[name] = {
                "type": "TEXTURE", "offset": addr, "width": tex["width"], "height": tex["height"],
                "format": tex["format"], "size": tex["size"], "symbol": name,
            }
        write_yaml(os.path.join(out_root, "textures", f"bank_{base:08X}.yml"), {"segments": [(0x09, base)]}, nodes)

    # A level's own skybox: textures/skyboxes/levels/<folder>/ptrlist, which the game uses over the vanilla skybox
    # its geo layout names. Levels that load the same skybox share its tiles.
    for start in sorted({sky[0] for sky in skyboxes.values()}):
        tiles = sorted({p for sky in skyboxes.values() if sky[0] == start for p in sky[1]})
        nodes = OrderedDict()
        for addr in tiles:
            name = f"sky_{start:08X}_{addr & 0xFFFFFF:05X}"
            nodes[name] = {"type": "TEXTURE", "offset": addr, "width": 32, "height": 32, "format": "RGBA16",
                           "size": SKYBOX_TILE_SIZE, "symbol": name}
        write_yaml(os.path.join(out_root, "textures", "skyboxes", f"sky_{start:08X}.yml"),
                   {"segments": [(0x0A, start)]}, nodes)
    for folder, (start, _) in sorted(skyboxes.items()):
        nodes = OrderedDict(ptrlist={"type": "ASSET_ARRAY", "assetType": "Texture", "factoryType": "TEXTURE",
                                     "count": SKYBOX_PTRS, "offset": 0x0A000000 + SKYBOX_PTRLIST,
                                     "symbol": f"{folder}_skybox_ptrlist"})
        write_yaml(os.path.join(out_root, "textures", "skyboxes", "levels", f"{folder}.yml"),
                   {"segments": [(0x0A, start)],
                    "external_files": [f"{args.yaml_root}/textures/skyboxes/sky_{start:08X}.yml"]}, nodes)

    for folder, areas, dls, texs, problems in summary:
        print(f"{folder:18} areas={areas} display_lists={dls} textures={texs}")
        for p in problems:
            print(f"    ! {p}")
    print(f"{len(banks)} texture banks, {len(set(s[0] for s in skyboxes.values()))} skyboxes for {len(skyboxes)} levels")


def config_entries(text):
    """`key -> (raw text, parsed)` for each top-level entry of a config.yml, the raw text kept as written."""
    import yaml
    entries, key, lines = OrderedDict(), None, []
    for line in text.replace("\r\n", "\n").split("\n"):
        top = line and not line[0].isspace() and not line.startswith("#") and ":" in line
        if top:
            if key is not None:
                entries[key] = "\n".join(lines).rstrip() + "\n"
            key, lines = line.split(":", 1)[0].strip(), [line]
        elif key is not None:
            lines.append(line)
    if key is not None:
        entries[key] = "\n".join(lines).rstrip() + "\n"
    return OrderedDict((k, (raw, (yaml.safe_load(raw) or {}).get(k))) for k, raw in entries.items())


REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def build_mod(folder, out):
    """The hack's C mod (src/, include/, manifest.json in its folder), built like the mod template's CMake does: each
    source merged with the headers it includes into dist/, which build.gen lists."""
    import shutil
    import subprocess
    includes = [os.path.join(folder, "include"), os.path.join(folder, "src"), REPO_ROOT,
                os.path.join(REPO_ROOT, "include"), os.path.join(REPO_ROOT, "src"),
                os.path.join(REPO_ROOT, "libultraship", "src"), os.path.join(REPO_ROOT, "libultraship", "include"),
                os.path.join(REPO_ROOT, "libultraship", "include", "libultraship")]
    dist = []
    for dirpath, _, names in os.walk(os.path.join(folder, "src")):
        for name in sorted(n for n in names if n.endswith(".c") and not n.endswith(".inc.c")):
            src = os.path.join(dirpath, name)
            rel = os.path.relpath(src, os.path.join(folder, "src")).replace(os.sep, "/")
            target = os.path.join(out, "dist", rel)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            subprocess.run([sys.executable, os.path.join(REPO_ROOT, "tools", "merge.py"), "--out", target,
                            "--includes", *includes, "--assets-dir", out, "--src", src],
                           check=True, stdout=subprocess.DEVNULL)
            dist.append("dist/" + rel)
    for dirpath, _, names in os.walk(os.path.join(folder, "include")):
        for name in names:
            src = os.path.join(dirpath, name)
            target = os.path.join(out, "include", os.path.relpath(src, os.path.join(folder, "include")))
            os.makedirs(os.path.dirname(target), exist_ok=True)
            shutil.copyfile(src, target)
    open(os.path.join(out, "build.gen"), "w", newline="\n").write("\n".join(dist) + "\n")
    subprocess.run([sys.executable, os.path.join(REPO_ROOT, "tools", "mod_manifest.py"),
                    "--api", os.path.join(REPO_ROOT, "src", "port", "api", "api.h"),
                    "--src", os.path.join(folder, "manifest.json"), "--out", os.path.join(out, "manifest.json")],
                   check=True)


def cmd_package(args):
    """A recipe package from a hack's folder: its recipe.yml as config.yml, the files the recipe names, and its C mod
    built into code/. Laid out so the package's root is Torch's source directory: the folder's own paths stay as they
    are, shared files from the repo (assets/...) keep their repo paths. Nothing in it comes from the ROM."""
    import tempfile
    import zipfile
    folder = os.path.abspath(args.folder)
    entries = config_entries(open(os.path.join(folder, "recipe.yml"), encoding="utf-8").read())
    match = [(k, raw, e) for k, (raw, e) in entries.items() if isinstance(e, dict) and "romhack" in e]
    if len(match) != 1:
        raise SystemExit(f"{folder}/recipe.yml has {len(match)} romhack entries, not 1")
    key, raw, entry = match[0]

    def resolve(path):
        # The hack's own files, or a shared one from the repo.
        own = os.path.join(folder, path)
        return own if os.path.exists(own) else os.path.join(REPO_ROOT, path)

    files = []  # (disk path, path in the package)
    for name in [entry["path"]] + ([entry["romhack"]["files"]] if entry["romhack"].get("files") else []):
        top = resolve(name)
        for dirpath, _, names in os.walk(top):
            for n in sorted(names):
                f = os.path.join(dirpath, n)
                files.append((f, (name + "/" + os.path.relpath(f, top)).replace(os.sep, "/")))
    symbols = entry.get("symbols") or []
    for name in [symbols] if isinstance(symbols, str) else symbols:
        files.append((resolve(name), name))

    # The hack's patch, when it's here: an entry for the ROM it was made for, whose preprocess jobs rebuild the hack
    # (extend, patch) and continue with its recipe. Without it the package only takes the patched ROM.
    base = entry["romhack"].get("base") or {}
    patch = base.get("patch")
    patch_path = next((p for p in (os.path.join(folder, patch or ""), os.path.join(REPO_ROOT, patch or ""))
                       if patch and os.path.isfile(p)), None)
    if patch and not patch_path:
        print(f"note: {patch} isn't next to the recipe or at the repo root; the package needs the patched ROM")
    if patch_path:
        files.append((patch_path, "patch/" + os.path.basename(patch_path)))
        jobs = {}
        if base.get("extend"):
            jobs["extend"] = {"method": "sm64extend"}
        jobs["patch"] = {"method": "patch", "file": "patch/" + os.path.basename(patch_path), "target": key,
                         "restart": True}
        import yaml
        raw += "\n" + yaml.safe_dump({base["sha1"]: {"preprocess": jobs}}, sort_keys=False)

    with tempfile.TemporaryDirectory() as code:
        if os.path.exists(os.path.join(folder, "manifest.json")):
            build_mod(folder, code)
            raw = raw.replace("\n  romhack:\n", "\n  romhack:\n    code: code\n", 1)
            for dirpath, _, names in os.walk(code):
                for n in names:
                    f = os.path.join(dirpath, n)
                    files.append((f, "code/" + os.path.relpath(f, code).replace(os.sep, "/")))
        os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
        with zipfile.ZipFile(args.out, "w", zipfile.ZIP_DEFLATED) as z:
            z.writestr("config.yml", raw)
            for f, arc in files:
                z.write(f, arc)
    print(f"{args.out}: {entry['name']} ({key}), {len(files)} files")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("package", help="pack a hack's folder (recipe.yml, YAMLs, symbols, C mod) into a recipe o2r")
    p.add_argument("folder", help="the hack's folder, e.g. tools/romhacks/<name>")
    p.add_argument("out", help="the package to write, e.g. romhacks/<name>.o2r")

    p = sub.add_parser("extend", help="extend an 8 MB SM64 US ROM like sm64extend does with its defaults")
    p.add_argument("rom")
    p.add_argument("out")

    p = sub.add_parser("patch", help="apply a PPF3 or xdelta (VCDIFF) patch")
    p.add_argument("rom")
    p.add_argument("patch")
    p.add_argument("out")

    p = sub.add_parser("ppf", help="apply a PPF3 patch")
    p.add_argument("rom")
    p.add_argument("patch")
    p.add_argument("out")

    p = sub.add_parser("yamls", help="write Torch YAMLs for the levels a ROM Manager hack moved")
    p.add_argument("rom")
    p.add_argument("outdir", help="e.g. tools/romhacks/<name>/yamls")
    p.add_argument("--yaml-root", help="outdir as config.yml's path: sees it (default: outdir)")
    p.add_argument("--extra", help="extra assets per level folder, e.g. collision for custom behaviors")
    p.add_argument("--scripts", action="store_true",
                   help="also extract each level's script to scripts/levels/<folder>, with the models it loads and "
                        "the behaviors the hack rewrote (docs/romhacks/SCRIPTS.md)")
    p.add_argument("--base-yamls", default="assets/ymls/us", help="the base game's YAMLs, for vanilla asset names")
    p.add_argument("--base-rom", help="the unpatched 8 MB ROM, to tell behaviors the hack rewrote from vanilla ones")
    p.add_argument("--symbols", action="append",
                   help="the symbols files config.yml lists, in order (default: assets/symbols/us.yml)")

    args = parser.parse_args()
    if args.cmd == "extend":
        open(args.out, "wb").write(extend_rom(open(args.rom, "rb").read()))
    elif args.cmd == "patch":
        open(args.out, "wb").write(apply_patch(open(args.rom, "rb").read(), open(args.patch, "rb").read()))
    elif args.cmd == "ppf":
        open(args.out, "wb").write(apply_ppf(open(args.rom, "rb").read(), open(args.patch, "rb").read()))
    elif args.cmd == "yamls":
        args.yaml_root = (args.yaml_root or args.outdir).replace("\\", "/").rstrip("/")
        cmd_yamls(args)
    elif args.cmd == "package":
        cmd_package(args)


if __name__ == "__main__":
    main()
