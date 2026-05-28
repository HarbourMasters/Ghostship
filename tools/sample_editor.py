#!/usr/bin/env python3
"""
Ghostship Sample Editor — browse, export, and replace SM64 audio samples.

  python sample_editor.py [gui] [archive.o2r]
  python sample_editor.py list    <archive>
  python sample_editor.py export  <archive> <sample_path> <out.wav>
  python sample_editor.py replace <archive> <sample_path> <audio.mp3> [--mod path]
"""

from __future__ import annotations

import argparse
import array
import io
import json
import os
import struct
import subprocess
import sys
import tempfile
import threading
import tkinter as tk
import wave
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from tkinter import filedialog, messagebox, scrolledtext, ttk
from typing import Dict, List, Optional, Tuple

# ── Resource types ────────────────────────────────────────────────────────────
SAMPLE_RES_TYPE = 0x41554643   # AUFC
BANK_RES_TYPE   = 0x42414E4B   # BANK
SEQ_RES_TYPE    = 0x53455143   # SEQC
OTR_HEADER_SIZE = 64
AUDIO_REF_HZ    = 32000
ADPCM_FRAME_BYTES   = 9
ADPCM_FRAME_SAMPLES = 16
LOOP_INFINITE       = 0xFFFFFFFF

# ── SM64 sequence names ───────────────────────────────────────────────────────
SEQ_NAMES = {
    0: "SFX",                      1: "CUTSCENE_COLLECT_STAR",
    2: "MENU_TITLE_SCREEN",        3: "LEVEL_GRASS",
    4: "LEVEL_INSIDE_CASTLE",      5: "LEVEL_WATER",
    6: "LEVEL_HOT",                7: "BOSS_KOOPA",
    8: "LEVEL_SNOW",               9: "LEVEL_SLIDE",
    10: "LEVEL_SPOOKY",            11: "EVENT_PIRANHA_PLANT",
    12: "LEVEL_UNDERGROUND",       13: "MENU_STAR_SELECT",
    14: "EVENT_POWERUP",           15: "EVENT_METAL_CAP",
    16: "EVENT_KOOPA_MESSAGE",     17: "LEVEL_KOOPA_ROAD",
    18: "EVENT_HIGH_SCORE",        19: "EVENT_MERRY_GO_ROUND",
    20: "EVENT_RACE",              21: "CUTSCENE_STAR_SPAWN",
    22: "EVENT_BOSS",              23: "CUTSCENE_COLLECT_KEY",
    24: "EVENT_ENDLESS_STAIRS",    25: "LEVEL_BOSS_KOOPA_FINAL",
    26: "EVENT_CUTSCENE_CREDITS",  27: "EVENT_SOLVE_PUZZLE",
    28: "EVENT_TOAD_MESSAGE",      29: "EVENT_PEACH_MESSAGE",
    30: "EVENT_CUTSCENE_INTRO",    31: "EVENT_CUTSCENE_VICTORY",
    32: "EVENT_CUTSCENE_ENDING",   33: "MENU_FILE_SELECT",
    34: "EVENT_CUTSCENE_LAKITU",
}

CODEC_LABELS = ["All", "ADPCM", "MP3", "OGG", "WAV", "FLAC"]

# Codec/medium name → int maps (mirrors AudioContext in Torch)
CODEC_NAMES  = {"ADPCM": 0, "S8": 1, "SKIP": 2, "HALF": 3, "ADPCM_HALF": 4, "S16": 5}
MEDIUM_NAMES = {"RAM": 0, "UNK": 1, "CART": 2, "DISK": 3}
CODEC_IDS    = {v: k for k, v in CODEC_NAMES.items()}

# ── Catppuccin Mocha ──────────────────────────────────────────────────────────
BG      = "#1e1e2e"
BG2     = "#181825"
BG3     = "#313244"
FG      = "#cdd6f4"
FG_DIM  = "#6c7086"
ACCENT  = "#89b4fa"
GREEN   = "#a6e3a1"
RED     = "#f38ba8"
MAUVE   = "#cba6f7"
SURFACE = "#585b70"
OVERLAY = "#45475a"


# ─────────────────────────────────────────────────────────────────────────────
# Binary reader
# ─────────────────────────────────────────────────────────────────────────────

class BinaryReader:
    def __init__(self, data: bytes):
        self._d = memoryview(data)
        self._p = 0

    def read(self, n: int) -> bytes:
        b = bytes(self._d[self._p:self._p + n])
        self._p += n
        return b

    def _a(self, n: int) -> int:
        p = self._p; self._p += n; return p

    def u8(self)  -> int:   return struct.unpack_from("B",  self._d, self._a(1))[0]
    def u16(self) -> int:   return struct.unpack_from("<H", self._d, self._a(2))[0]
    def i16(self) -> int:   return struct.unpack_from("<h", self._d, self._a(2))[0]
    def u32(self) -> int:   return struct.unpack_from("<I", self._d, self._a(4))[0]
    def i32(self) -> int:   return struct.unpack_from("<i", self._d, self._a(4))[0]
    def f32(self) -> float: return struct.unpack_from("<f", self._d, self._a(4))[0]

    def string(self) -> str:
        n = self.i32()
        return self.read(max(0, n)).decode("utf-8", errors="replace") if n > 0 else ""


def _bswap16(x: int) -> int:
    x &= 0xFFFF
    return ((x << 8) & 0xFF00) | ((x >> 8) & 0x00FF)

def _otr_type(raw: bytes) -> int:
    return struct.unpack_from("<I", raw, 4)[0] if len(raw) >= 8 else 0

def _is_xml(raw: bytes) -> bool:
    return len(raw) > OTR_HEADER_SIZE and raw[OTR_HEADER_SIZE:OTR_HEADER_SIZE+1] == b"<"

def _read_zip(path: str, name: str) -> Optional[bytes]:
    try:
        with zipfile.ZipFile(path) as z: return z.read(name)
    except Exception: return None


# ─────────────────────────────────────────────────────────────────────────────
# Data structures
# ─────────────────────────────────────────────────────────────────────────────

@dataclass
class AdpcmLoop:
    start: int = 0; end: int = 0; count: int = 0
    state: List[int] = field(default_factory=list)

@dataclass
class AdpcmBook:
    order: int = 0; npredictors: int = 0
    book:  List[int] = field(default_factory=list)

@dataclass
class SampleEntry:
    path:        str       = ""
    loop:        AdpcmLoop = field(default_factory=AdpcmLoop)
    book:        AdpcmBook = field(default_factory=AdpcmBook)
    sample_size: int       = 0
    sample_data: bytes     = b""
    codec:       int       = 0   # 0=ADPCM, 1=S8, 2=SKIP, 5=S16 (CODEC_* from Torch)
    medium:      int       = 0   # 0=RAM, 1=UNK, 2=CART, 3=DISK
    # Cross-ref from bank scan
    bank_path:   str   = ""
    inst_index:  int   = -1
    slot:        str   = ""
    tuning:      float = 0.0
    # Custom format (mp3/ogg/wav)
    is_custom:   bool  = False
    custom_fmt:  str   = ""
    audio_path:  str   = ""
    # Cached PCM
    _pcm:      Optional[array.array] = field(default=None, repr=False)
    _pcm_rate: int                   = field(default=0,    repr=False)

    def codec_str(self) -> str:
        if self.is_custom: return self.custom_fmt.upper()
        return CODEC_IDS.get(self.codec, f"CODEC{self.codec}")

    def has_loop(self) -> bool:
        return self.loop.end > 0 or self.loop.count != 0

    def has_book(self) -> bool:
        return bool(self.book.book)

    def pcm_rate(self) -> int:
        if self._pcm_rate: return self._pcm_rate
        return max(1, int(self.tuning * AUDIO_REF_HZ)) if self.tuning > 0 else AUDIO_REF_HZ

    def estimated_duration(self) -> float:
        if self.is_custom: return 0.0
        if self.codec == 5:  # CODEC_S16: raw int16, 2 bytes per sample
            return (self.sample_size // 2) / self.pcm_rate()
        n_samples = (self.sample_size // ADPCM_FRAME_BYTES) * ADPCM_FRAME_SAMPLES
        return n_samples / self.pcm_rate()

@dataclass
class InstrumentEntry:
    release_rate: int = 0; normal_range_lo: int = 0; normal_range_hi: int = 127
    lo_sample:  Optional[str] = None; lo_tuning:  float = 0.0
    med_sample: Optional[str] = None; med_tuning: float = 0.0
    hi_sample:  Optional[str] = None; hi_tuning:  float = 0.0

@dataclass
class DrumEntry:
    release_rate: int = 0; pan: int = 64
    sample_path: str = ""; tuning: float = 0.0

@dataclass
class BankEntry:
    bank_id: int = 0
    instruments: List[Optional[InstrumentEntry]] = field(default_factory=list)
    drums:       List[DrumEntry]                 = field(default_factory=list)

@dataclass
class SequenceEntry:
    path: str = ""; seq_id: int = -1
    banks: List[str] = field(default_factory=list)
    m64_data: bytes = b""


# ─────────────────────────────────────────────────────────────────────────────
# Binary parsers
# ─────────────────────────────────────────────────────────────────────────────

def _parse_sample_bin(payload: bytes) -> Optional[SampleEntry]:
    try:
        r = BinaryReader(payload); e = SampleEntry()
        e.loop.start = r.u32(); e.loop.end = r.u32()
        e.loop.count = r.u32(); r.i32()
        for _ in range(r.u32()): e.loop.state.append(r.i16())
        e.book.order = r.i32(); e.book.npredictors = r.i32()
        for _ in range(r.u32()): e.book.book.append(r.i16())
        sz = r.i32(); e.sample_size = max(0, sz); e.sample_data = r.read(e.sample_size)
        return e
    except Exception: return None

def _parse_sample_xml(raw: bytes) -> Optional[SampleEntry]:
    try:
        import xml.etree.ElementTree as ET
        root = ET.fromstring(raw[OTR_HEADER_SIZE:].decode("utf-8", errors="replace"))
        e = SampleEntry()

        # v1 Torch XML attributes (Version, Codec, Medium, Tuning, etc.)
        codec_s = root.get("Codec")
        if codec_s: e.codec = CODEC_NAMES.get(codec_s, 0)
        medium_s = root.get("Medium")
        if medium_s: e.medium = MEDIUM_NAMES.get(medium_s, 0)
        tuning_s = root.get("Tuning")
        if tuning_s:
            try: e.tuning = float(tuning_s)
            except ValueError: pass

        # Custom-format mod replacement (mp3/wav/ogg/flac)
        cf = root.get("CustomFormat")
        if cf:
            e.is_custom = True; e.custom_fmt = cf; e.audio_path = root.get("Path", "")

        lp = root.find("ADPCMLoop")
        if lp is not None:
            e.loop.start = int(lp.get("Start", 0))
            e.loop.end   = int(lp.get("End",   0))
            e.loop.count = int(lp.get("Count", 0))
            # Predictor state — only present when count != 0 (v1 Torch format)
            for pred in lp.findall("Predictor"):
                try: e.loop.state.append(int(pred.get("State", 0)))
                except ValueError: pass

        if not cf:
            bk = root.find("ADPCMBook")
            if bk is not None:
                e.book.order = int(bk.get("Order", 0)); e.book.npredictors = int(bk.get("Npredictors", 0))
                for b in bk.findall("Book"): e.book.book.append(int(b.get("Page", 0)))
            e.audio_path = root.get("Path", "")
        return e
    except Exception: return None

def _parse_bank_bin(payload: bytes) -> Optional[BankEntry]:
    try:
        r = BinaryReader(payload); bank = BankEntry()
        bank.bank_id = r.u32()
        for _ in range(r.u32()):
            if not r.u8(): bank.instruments.append(None); continue
            inst = InstrumentEntry()
            inst.release_rate = r.u8(); inst.normal_range_lo = r.u8(); inst.normal_range_hi = r.u8()
            for _ in range(r.u32()): _bswap16(r.u16()); _bswap16(r.u16())
            sf = r.u32()
            if sf & 1: inst.lo_sample  = r.string(); inst.lo_tuning  = r.f32()
            if sf & 2: inst.med_sample = r.string(); inst.med_tuning = r.f32()
            if sf & 4: inst.hi_sample  = r.string(); inst.hi_tuning  = r.f32()
            bank.instruments.append(inst)
        for _ in range(r.u32()):
            d = DrumEntry(); d.release_rate = r.u8(); d.pan = r.u8()
            for _ in range(r.u32()): _bswap16(r.u16()); _bswap16(r.u16())
            d.sample_path = r.string(); d.tuning = r.f32(); bank.drums.append(d)
        return bank
    except Exception: return None

def _parse_seq_bin(payload: bytes) -> Optional[SequenceEntry]:
    try:
        r = BinaryReader(payload); s = SequenceEntry()
        s.seq_id = r.u32()
        for _ in range(r.u32()): s.banks.append(r.string())
        s.m64_data = r.read(r.u32())
        return s
    except Exception: return None

def _parse_seq_xml(raw: bytes, zf: "zipfile.ZipFile") -> Optional[SequenceEntry]:
    try:
        import xml.etree.ElementTree as ET
        root = ET.fromstring(raw[OTR_HEADER_SIZE:].decode("utf-8", errors="replace"))
        s = SequenceEntry()
        s.seq_id = int(root.get("ID", -1))
        banks_root = root.find("Banks")
        if banks_root is not None:
            for b in banks_root:
                bp = b.get("Path", "")
                if bp: s.banks.append(bp)
        m64_path = root.get("Path", "")
        if m64_path:
            try:
                m64_raw = zf.read(m64_path)
                s.m64_data = m64_raw[OTR_HEADER_SIZE:] if len(m64_raw) > OTR_HEADER_SIZE else m64_raw
            except Exception: pass
        return s
    except Exception: return None


# ─────────────────────────────────────────────────────────────────────────────
# N64 VADPCM decoder — ported from aifc_decode.c via Starship tools
# ─────────────────────────────────────────────────────────────────────────────

def _build_coef_table(order: int, npred: int, flat: List[int]) -> List:
    table = []
    for pred in range(npred):
        entry = [[0] * (order + 8) for _ in range(8)]
        base  = pred * order * 8
        for j in range(order):
            for k in range(8): entry[k][j] = flat[base + j * 8 + k]
        for k in range(1, 8): entry[k][order] = entry[k-1][order-1]
        entry[0][order] = 1 << 11
        for k in range(1, 8):
            for j in range(8):
                entry[j][k + order] = 0 if j < k else entry[j - k][order]
        table.append(entry)
    return table

def _decode_frame(frame: bytes, state: List[int], order: int, table: List) -> None:
    h = frame[0]; scale = 1 << (h >> 4); pred = (h & 0xF) % max(1, len(table))
    ix = [0] * 16
    for i in range(0, 16, 2):
        c = frame[1 + i // 2]; ix[i] = c >> 4; ix[i+1] = c & 0xF
    for i in range(16):
        if ix[i] >= 8: ix[i] -= 16
        ix[i] *= scale
    entry = table[pred]; inv = [0] * (order + 8)
    for half in range(2):
        for i in range(order): inv[i] = state[(8 if half else 16) - order + i]
        for i in range(8):
            ind = half * 8 + i; inv[order + i] = ix[ind]
            dp = sum(entry[i][x] * inv[x] for x in range(order + i))
            state[ind] = dp // 2048 + ix[ind]

def vadpcm_decode(raw: bytes, book: AdpcmBook) -> array.array:
    if not book.book or not book.npredictors: return array.array("h")
    table = _build_coef_table(book.order, book.npredictors, book.book)
    state = [0] * 16; out = array.array("h")
    for f in range(len(raw) // ADPCM_FRAME_BYTES):
        _decode_frame(raw[f*9:(f+1)*9], state, book.order, table)
        for v in state: out.append(max(-32768, min(32767, v)))
    return out


# ─────────────────────────────────────────────────────────────────────────────
# PCM / audio helpers
# ─────────────────────────────────────────────────────────────────────────────

def _get_pcm(entry: SampleEntry, archive: str) -> Tuple[array.array, int]:
    if entry._pcm is not None: return entry._pcm, entry._pcm_rate

    # ── Custom format (mp3/wav/ogg/flac) ─────────────────────────────────────
    # Mirrors AudioSampleXMLFactoryV0::ReadResource custom-format branch:
    # load from archive, decode, then set loop bounds to [0, frame_count] so
    # the loop editor always reflects the real audio length.
    if entry.is_custom:
        raw = _read_zip(archive, entry.audio_path)
        if raw:
            data = raw[OTR_HEADER_SIZE:] if len(raw) > OTR_HEADER_SIZE else raw
            pcm, rate = _ffmpeg_decode(data)
            if pcm:
                entry.loop.start = 0
                entry.loop.end   = len(pcm)
                entry.loop.count = 0
            entry._pcm = pcm; entry._pcm_rate = rate
            return pcm, rate
        return array.array("h"), AUDIO_REF_HZ

    # ── ADPCM/S16: sample_data may need to be loaded from the companion file ──
    # Mirrors AudioSampleXMLFactoryV0::ReadResource ADPCM branch: XML entries
    # store raw bytes in a separate "_data" path in the archive rather than
    # embedding them inline (v1 Torch format).
    if not entry.sample_data and entry.audio_path:
        raw = _read_zip(archive, entry.audio_path)
        if raw:
            # companion _data files may carry an OTR header prefix — strip it
            data = raw[OTR_HEADER_SIZE:] if len(raw) > OTR_HEADER_SIZE else raw
            entry.sample_data = data
            entry.sample_size  = len(data)

    if not entry.sample_data:
        return array.array("h"), AUDIO_REF_HZ

    rate = entry.pcm_rate()

    # ── CODEC_S16: raw signed 16-bit PCM, no ADPCM decode needed ─────────────
    # Mirrors FromMp3 / Mp3DecoderWorker: sampleAddr is already int16 frames.
    if entry.codec == 5:
        pcm = array.array("h"); pcm.frombytes(entry.sample_data)
        entry._pcm = pcm; entry._pcm_rate = rate
        return pcm, rate

    # ── CODEC_ADPCM: N64 VADPCM decode ───────────────────────────────────────
    if not entry.book.book:
        return array.array("h"), AUDIO_REF_HZ
    pcm = vadpcm_decode(entry.sample_data, entry.book)
    entry._pcm = pcm; entry._pcm_rate = rate
    return pcm, rate

def _ffmpeg_decode(data: bytes, target_rate: int = 0) -> Tuple[array.array, int]:
    try:
        # Probe native sample rate first
        probe = subprocess.run(
            ["ffprobe", "-v", "quiet", "-select_streams", "a:0",
             "-show_entries", "stream=sample_rate", "-of", "csv=p=0", "pipe:0"],
            input=data, capture_output=True, timeout=10)
        rate = int(probe.stdout.strip()) if probe.returncode == 0 and probe.stdout.strip().isdigit() else AUDIO_REF_HZ
        if target_rate: rate = target_rate
        p = subprocess.run(
            ["ffmpeg", "-v", "quiet", "-i", "pipe:0", "-f", "s16le", "-ac", "1", "-ar", str(rate), "pipe:1"],
            input=data, capture_output=True, timeout=60)
        if p.returncode: return array.array("h"), rate
        out = array.array("h"); out.frombytes(p.stdout)
        return out, rate
    except Exception: return array.array("h"), AUDIO_REF_HZ

def _pcm_to_wav(pcm: array.array, rate: int) -> bytes:
    buf = io.BytesIO()
    with wave.open(buf, "wb") as wf:
        wf.setnchannels(1); wf.setsampwidth(2); wf.setframerate(rate)
        wf.writeframes(pcm.tobytes())
    return buf.getvalue()

def _play_wav(wav_bytes: bytes):
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        f.write(wav_bytes); tmp = f.name
    def _run():
        try:
            cmd = ["afplay", tmp] if sys.platform == "darwin" else ["ffplay", "-nodisp", "-autoexit", tmp]
            subprocess.run(cmd, capture_output=True, timeout=120)
        finally:
            try: os.unlink(tmp)
            except: pass
    threading.Thread(target=_run, daemon=True).start()


# ─────────────────────────────────────────────────────────────────────────────
# Archive scan
# ─────────────────────────────────────────────────────────────────────────────

def _xml_root_tag(raw: bytes) -> str:
    try:
        import xml.etree.ElementTree as ET
        return ET.fromstring(raw[OTR_HEADER_SIZE:].decode("utf-8", errors="replace")).tag
    except Exception:
        return ""

def scan_archive(path: str) -> Tuple[Dict[str, SampleEntry], Dict[str, BankEntry], Dict[str, SequenceEntry]]:
    samples: Dict[str, SampleEntry]   = {}
    banks:   Dict[str, BankEntry]     = {}
    seqs:    Dict[str, SequenceEntry] = {}
    try:
        with zipfile.ZipFile(path) as zf:
            for name in zf.namelist():
                try: raw = zf.read(name)
                except: continue
                if len(raw) < OTR_HEADER_SIZE: continue
                rt = _otr_type(raw); pl = raw[OTR_HEADER_SIZE:]

                # Fallback: infer type from XML root tag when header type is unrecognized
                if rt not in (SAMPLE_RES_TYPE, BANK_RES_TYPE, SEQ_RES_TYPE) and _is_xml(raw):
                    tag = _xml_root_tag(raw)
                    if tag in ("Sample", "AudioSample"):   rt = SAMPLE_RES_TYPE
                    elif tag in ("Sequence", "AudioSequence"): rt = SEQ_RES_TYPE

                if rt == SAMPLE_RES_TYPE:
                    e = _parse_sample_xml(raw) if _is_xml(raw) else _parse_sample_bin(pl)
                    if e: e.path = name; samples[name] = e
                elif rt == BANK_RES_TYPE:
                    if not _is_xml(raw):
                        b = _parse_bank_bin(pl)
                        if b: banks[name] = b
                elif rt == SEQ_RES_TYPE:
                    if _is_xml(raw):
                        s = _parse_seq_xml(raw, zf)
                    else:
                        s = _parse_seq_bin(pl)
                    if s: s.path = name; seqs[name] = s
    except: pass

    for bp, bank in banks.items():
        for i, inst in enumerate(bank.instruments):
            if not inst: continue
            for slot, sname, tuning in [("lo", inst.lo_sample, inst.lo_tuning),
                                         ("normal", inst.med_sample, inst.med_tuning),
                                         ("hi", inst.hi_sample, inst.hi_tuning)]:
                if sname and sname in samples and not samples[sname].bank_path:
                    s = samples[sname]; s.bank_path = bp; s.inst_index = i
                    s.slot = slot; s.tuning = tuning
        for j, drum in enumerate(bank.drums):
            if drum.sample_path and drum.sample_path in samples:
                s = samples[drum.sample_path]
                if not s.bank_path:
                    s.bank_path = bp; s.inst_index = j
                    s.slot = f"drum_{j}"; s.tuning = drum.tuning
    return samples, banks, seqs


# ─────────────────────────────────────────────────────────────────────────────
# Mod archive / replacement helpers
# ─────────────────────────────────────────────────────────────────────────────

def _make_xml(entry: SampleEntry, audio_arch_path: str, fmt: str) -> str:
    lp = entry.loop
    return (f'<Sample CustomFormat="{fmt}" Path="{audio_arch_path}">\n'
            f'  <ADPCMLoop Start="{lp.start}" End="{lp.end}" Count="{lp.count}"/>\n'
            f'</Sample>\n')

def _make_seq_xml(seq: SequenceEntry, audio_arch_path: str, fmt: str) -> str:
    return f'<AudioSequence ID="{seq.seq_id}" CustomFormat="{fmt}" Path="{audio_arch_path}"/>\n'

def _otr_hdr(res_type: int, n: int) -> bytes:
    h = bytearray(OTR_HEADER_SIZE); struct.pack_into("<I", h, 4, res_type); struct.pack_into("<I", h, 8, n); return bytes(h)

def _upsert_zip(mod_path: str, entries: Dict[str, bytes]):
    existing: Dict[str, bytes] = {}
    if os.path.exists(mod_path):
        try:
            with zipfile.ZipFile(mod_path) as zf:
                for n in zf.namelist(): existing[n] = zf.read(n)
        except: pass
    existing.update(entries)
    tmp = mod_path + ".tmp"
    with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as zf:
        for n, d in existing.items(): zf.writestr(n, d)
    os.replace(tmp, mod_path)

def do_replace(entry: SampleEntry, src_path: str, archive_path: str, mod_path: str = "") -> str:
    fmt = Path(src_path).suffix.lstrip(".").lower()
    if fmt not in ("mp3", "ogg", "wav", "flac", "aiff"): raise ValueError(f"Unsupported: {fmt}")
    if not mod_path:
        d = os.path.join(os.path.dirname(archive_path), "mods"); os.makedirs(d, exist_ok=True)
        mod_path = os.path.join(d, "custom_audio.o2r")
    ap   = str(Path(entry.path).with_suffix(f".{fmt}"))
    xb   = _make_xml(entry, ap, fmt).encode()
    with open(src_path, "rb") as f: ab = f.read()
    _upsert_zip(mod_path, {entry.path: xb, ap: ab})
    return mod_path

def do_replace_seq(seq: SequenceEntry, src_path: str, archive_path: str, mod_path: str = "") -> str:
    fmt = Path(src_path).suffix.lstrip(".").lower()
    if fmt not in ("mp3", "ogg", "wav", "flac", "aiff"): raise ValueError(f"Unsupported: {fmt}")
    if not mod_path:
        d = os.path.join(os.path.dirname(archive_path), "mods"); os.makedirs(d, exist_ok=True)
        mod_path = os.path.join(d, "custom_audio.o2r")
    ap = f"sound/sequences/{seq.seq_id:02d}.{fmt}"
    xb = _make_seq_xml(seq, ap, fmt).encode()
    with open(src_path, "rb") as f: ab = f.read()
    _upsert_zip(mod_path, {seq.path: xb, ap: ab})
    return mod_path

def export_wav(entry: SampleEntry, out_path: str, archive: str):
    pcm, rate = _get_pcm(entry, archive)
    if not pcm: raise RuntimeError("Could not decode sample")
    with open(out_path, "wb") as f: f.write(_pcm_to_wav(pcm, rate))


# ─────────────────────────────────────────────────────────────────────────────
# Alias store
# ─────────────────────────────────────────────────────────────────────────────

class AliasStore:
    def __init__(self, path: str):
        self._path = path; self._d: Dict[str, str] = {}; self._load()
    def _load(self):
        try:
            with open(self._path, "r", encoding="utf-8") as f: self._d = json.load(f)
        except: self._d = {}
    def save(self):
        os.makedirs(os.path.dirname(self._path), exist_ok=True)
        with open(self._path, "w", encoding="utf-8") as f: json.dump(self._d, f, indent=2)
    def get(self, p: str) -> str: return self._d.get(p, "")
    def set(self, p: str, a: str):
        if a: self._d[p] = a
        elif p in self._d: del self._d[p]
        self.save()
    def all(self) -> Dict[str, str]: return dict(self._d)


# ─────────────────────────────────────────────────────────────────────────────
# Basic SM64 M64 hex dump
# ─────────────────────────────────────────────────────────────────────────────

def _hexdump(data: bytes) -> str:
    if not data: return "(empty)"
    lines = [f"Sequence data: {len(data)} bytes (0x{len(data):04X})\n"]
    for row in range(0, len(data), 16):
        chunk = data[row:row+16]
        hex_  = " ".join(f"{b:02X}" for b in chunk).ljust(47)
        asc   = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        lines.append(f"  {row:04X}  {hex_}  {asc}")
    return "\n".join(lines)


# ─────────────────────────────────────────────────────────────────────────────
# Formatting helpers
# ─────────────────────────────────────────────────────────────────────────────

def _fmt_size(n: int) -> str:
    if n >= 1024*1024: return f"{n/1048576:.1f} MB"
    if n >= 1024:      return f"{n/1024:.1f} KB"
    return f"{n} B"

def _fmt_dur(s: float) -> str:
    if s <= 0: return "—"
    if s >= 60: return f"{int(s//60)}m {s%60:.1f}s"
    return f"{s:.3f}s"


# ─────────────────────────────────────────────────────────────────────────────
# Waveform / loop editor
# ─────────────────────────────────────────────────────────────────────────────

class _WaveformEditor(tk.Toplevel):
    H = 240

    def __init__(self, parent, entry: SampleEntry, archive: str, on_save=None):
        super().__init__(parent)
        self.title(f"Loop Editor — {Path(entry.path).name}")
        self.configure(bg=BG); self.resizable(True, True); self.minsize(640, 380)
        self._e = entry; self._arch = archive; self._on_save = on_save
        self._pcm: array.array = array.array("h"); self._rate = AUDIO_REF_HZ
        self._zs = 0.0; self._ze = 1.0
        self._ls = entry.loop.start; self._le = entry.loop.end; self._lc = entry.loop.count
        self._drag = None; self._pan_o = None
        self._build(); self._load()

    def _build(self):
        self._cv = tk.Canvas(self, bg=BG2, height=self.H, highlightthickness=0, cursor="crosshair")
        self._cv.pack(fill=tk.BOTH, expand=True, padx=8, pady=(8, 0))
        self._inf = tk.Label(self, text="Loading…", bg=BG, fg=FG_DIM, font=("monospace", 9), anchor="w")
        self._inf.pack(fill=tk.X, padx=8)
        cf = tk.Frame(self, bg=BG, pady=4); cf.pack(fill=tk.X, padx=8)
        self._vs = tk.StringVar(value=str(self._ls)); self._ve = tk.StringVar(value=str(self._le))
        self._vc = tk.StringVar(value=str(self._lc))
        for ci, (lbl, var) in enumerate([("Loop start", self._vs), ("Loop end", self._ve), ("Count", self._vc)]):
            tk.Label(cf, text=lbl, bg=BG, fg=FG).grid(row=0, column=ci*2, sticky="w", padx=(0 if ci==0 else 10, 2))
            ttk.Entry(cf, textvariable=var, width=12).grid(row=0, column=ci*2+1, padx=2)
        bf = tk.Frame(self, bg=BG, pady=4); bf.pack(fill=tk.X, padx=8, pady=(0, 8))
        for txt, cmd in [("▶  Attack", self._play_atk), ("▶  Loop", self._play_loop),
                         ("Snap start", self._snap_s), ("Snap end", self._snap_e),
                         ("Save", self._save), ("Close", self.destroy)]:
            _btn(bf, txt, cmd).pack(side=tk.LEFT, padx=3)
        self._cv.bind("<ButtonPress-1>",   lambda e: setattr(self, "_drag", self._nearest(e.x)))
        self._cv.bind("<B1-Motion>",       self._drag_move)
        self._cv.bind("<ButtonRelease-1>", lambda e: setattr(self, "_drag", None))
        self._cv.bind("<ButtonPress-2>",   lambda e: setattr(self, "_pan_o", (e.x, self._zs, self._ze)))
        self._cv.bind("<B2-Motion>",       self._pan_mv)
        self._cv.bind("<ButtonRelease-2>", lambda e: setattr(self, "_pan_o", None))
        self._cv.bind("<Control-MouseWheel>", lambda e: self._zoom(e.x, 1 if e.delta > 0 else -1))
        self._cv.bind("<Control-Button-4>",   lambda e: self._zoom(e.x, +1))
        self._cv.bind("<Control-Button-5>",   lambda e: self._zoom(e.x, -1))
        self._cv.bind("<Configure>",          lambda e: self._draw())

    def _load(self):
        def w():
            pcm, rate = _get_pcm(self._e, self._arch)
            self.after(0, lambda: self._loaded(pcm, rate))
        threading.Thread(target=w, daemon=True).start()

    def _loaded(self, pcm, rate):
        self._pcm = pcm; self._rate = rate
        if self._le == 0 and pcm: self._le = len(pcm) - 1; self._ve.set(str(self._le))
        self._draw(); self._upd_info()

    def _draw(self):
        c = self._cv; c.delete("all")
        W = c.winfo_width() or 800; H = self.H
        c.create_rectangle(0, 0, W, H, fill=BG2, outline="")
        n = len(self._pcm)
        if not n:
            c.create_text(W//2, H//2, text="Loading…", fill=FG, font=("monospace", 14)); return
        vs = int(self._zs * n); ve = max(vs+1, int(self._ze * n))
        sl = self._pcm[vs:ve]; mid = H // 2
        for x in range(W):
            i0 = int(x * len(sl) / W); i1 = max(i0+1, int((x+1)*len(sl)/W))
            chunk = sl[i0:i1]; pk = max(abs(v) for v in chunk) if chunk else 0
            h = int(pk / 32768.0 * mid * 0.95)
            c.create_line(x, mid-h, x, mid+h, fill=ACCENT, width=1)
        c.create_line(0, mid, W, mid, fill=OVERLAY, dash=(2,4))
        sx = self._s2x(self._ls); ex = self._s2x(self._le)
        c.create_rectangle(max(0,sx), 0, min(W,ex), H, fill="#89b4fa18", outline="")
        for samp, color, lbl in [(self._ls, ACCENT, "S"), (self._le, GREEN, "E")]:
            x = self._s2x(samp)
            if -5 <= x <= W+5:
                c.create_line(x, 0, x, H, fill=color, width=2)
                c.create_text(x+4, 10, text=lbl, fill=color, anchor="w", font=("monospace", 9, "bold"))

    def _s2x(self, s: int) -> int:
        W = self._cv.winfo_width() or 800; n = len(self._pcm) or 1
        vs = self._zs*n; ve = self._ze*n
        return int((s - vs) / max(1, ve-vs) * W)

    def _x2s(self, x: int) -> int:
        W = self._cv.winfo_width() or 800; n = len(self._pcm) or 1
        return int(self._zs*n + x/max(1,W) * (self._ze-self._zs)*n)

    def _nearest(self, x: int) -> Optional[str]:
        if abs(x - self._s2x(self._ls)) < 12: return "s"
        if abs(x - self._s2x(self._le)) < 12: return "e"
        return None

    def _drag_move(self, event):
        if not self._drag or not self._pcm: return
        sp = max(0, min(len(self._pcm)-1, self._x2s(event.x)))
        if self._drag == "s": self._ls = sp; self._vs.set(str(sp))
        else: self._le = sp; self._ve.set(str(sp))
        self._draw(); self._upd_info()

    def _pan_mv(self, event):
        if not self._pan_o: return
        ox, os_, oe_ = self._pan_o
        W = self._cv.winfo_width() or 800
        d = (ox - event.x) / max(1,W) * (oe_-os_)
        ns = max(0.0, os_+d); ne = min(1.0, oe_+d)
        if ne-ns > 0.001: self._zs, self._ze = ns, ne; self._draw()

    def _zoom(self, cx_px: int, direction: int):
        W = self._cv.winfo_width() or 800; cx = cx_px / max(1, W)
        sp = self._ze - self._zs; f = 0.75 if direction > 0 else 1.33
        ns = max(0.001, min(1.0, sp*f)); piv = self._zs + cx*sp
        zs = max(0.0, piv - cx*ns); self._zs, self._ze = zs, min(1.0, zs+ns); self._draw()

    def _snap(self, s: int, r: int=200) -> int:
        n = len(self._pcm)
        if not n: return s
        lo, hi = max(0, s-r), min(n-1, s+r)
        best, bd = s, abs(self._pcm[s]) if 0 <= s < n else 999999
        for i in range(lo, hi+1):
            v = abs(self._pcm[i])
            if v < bd: bd = v; best = i
        return best

    def _play_atk(self):
        if not self._pcm: return
        seg = self._pcm[:max(1, self._ls)]; _play_wav(_pcm_to_wav(seg, self._rate))
    def _play_loop(self):
        if not self._pcm: return
        seg = self._pcm[max(0,self._ls):min(len(self._pcm),self._le)]
        if seg: _play_wav(_pcm_to_wav(seg, self._rate))
    def _snap_s(self):
        try: s = int(self._vs.get())
        except: s = self._ls
        self._ls = self._snap(s); self._vs.set(str(self._ls)); self._draw()
    def _snap_e(self):
        try: e = int(self._ve.get())
        except: e = self._le
        self._le = self._snap(e); self._ve.set(str(self._le)); self._draw()
    def _save(self):
        try: self._e.loop.start = int(self._vs.get()); self._e.loop.end = int(self._ve.get()); self._e.loop.count = int(self._vc.get())
        except: messagebox.showerror("Error", "Invalid loop values", parent=self); return
        if self._on_save: self._on_save(self._e)
        self.destroy()
    def _upd_info(self):
        n = len(self._pcm); r = self._rate
        self._inf.config(text=f"{n} smp  {r} Hz  {_fmt_dur(n/max(1,r))}  "
                              f"| loop {self._ls}–{self._le} ({_fmt_dur(self._ls/max(1,r))}–{_fmt_dur(self._le/max(1,r))})")


# ─────────────────────────────────────────────────────────────────────────────
# Tk theme
# ─────────────────────────────────────────────────────────────────────────────

def _apply_theme(root: tk.Misc):
    s = ttk.Style(root); s.theme_use("clam")
    s.configure("TNotebook",        background=BG,  borderwidth=0)
    s.configure("TNotebook.Tab",    background=BG2, foreground=FG, padding=(10,4), focuscolor=BG)
    s.map("TNotebook.Tab",          background=[("selected", BG3)])
    s.configure("TFrame",           background=BG)
    s.configure("TPanedwindow",     background=BG)
    s.configure("Treeview",         background=BG2, foreground=FG, fieldbackground=BG2, rowheight=20)
    s.configure("Treeview.Heading", background=BG3, foreground=FG, relief="flat")
    s.map("Treeview",               background=[("selected", BG3)], foreground=[("selected", ACCENT)])
    s.configure("TScrollbar",       background=BG3, troughcolor=BG2, arrowcolor=FG)
    s.configure("TEntry",           fieldbackground=BG3, foreground=FG, insertcolor=FG)
    s.configure("TCombobox",        fieldbackground=BG3, foreground=FG, selectbackground=BG3,
                selectforeground=ACCENT, arrowcolor=FG)
    s.map("TCombobox",              fieldbackground=[("readonly", BG3)])
    s.configure("Dark.TButton",     background=BG3, foreground=FG, padding=(8,3),
                borderwidth=1, relief="flat", focusthickness=0)
    s.map("Dark.TButton",           background=[("active", SURFACE), ("pressed", BG2)],
                                    foreground=[("active", FG)])
    s.configure("Accent.TButton",   background=ACCENT, foreground=BG, padding=(10,3),
                borderwidth=0, relief="flat", focusthickness=0)
    s.map("Accent.TButton",         background=[("active", "#74a8e8"), ("pressed", "#5f90cc")])
    s.configure("TLabel",           background=BG, foreground=FG)
    s.configure("Dim.TLabel",       background=BG, foreground=FG_DIM)
    s.configure("Head.TLabel",      background=BG, foreground=FG, font=("TkDefaultFont", 11, "bold"))

def _btn(parent, text, cmd, style="Dark.TButton", **kw) -> ttk.Button:
    return ttk.Button(parent, text=text, command=cmd, style=style, **kw)


# ─────────────────────────────────────────────────────────────────────────────
# Main application
# ─────────────────────────────────────────────────────────────────────────────

class App(tk.Tk):
    _TITLE = "Ghostship Sample Editor"
    _FT    = [("OTR/O2R archive", "*.o2r *.otr"), ("All files", "*")]

    def __init__(self, archive: str = ""):
        super().__init__()
        self.title(self._TITLE)
        self.configure(bg=BG)
        self.geometry("1380x860")

        # State
        self._archive_path   = ""
        self._samples:   Dict[str, SampleEntry]   = {}
        self._banks:     Dict[str, BankEntry]     = {}
        self._sequences: Dict[str, SequenceEntry] = {}
        self._sel_entry: Optional[SampleEntry]    = None
        self._sel_seq:   Optional[SequenceEntry]  = None
        self._node_map:    Dict[str, dict]         = {}  # iid → {entry, slot}
        self._iids_by_path: Dict[str, List[str]]  = {}
        self._assignments:     Dict[str, str] = {}  # sample_path → local audio path
        self._seq_assignments: Dict[str, str] = {}  # seq_path    → local audio path
        self._outdir  = ""
        self._leaf_seq = 0  # monotonic counter for unique tree leaf IIDs

        cfg = os.path.join(os.path.expanduser("~"), ".config", "ghostship_se")
        self._aliases = AliasStore(os.path.join(cfg, "aliases.json"))

        _apply_theme(self)
        self._build_menu()
        self._build_ui()

        if archive:
            self.after(100, lambda: self._load(archive))

    # ── Menu ─────────────────────────────────────────────────────────────────

    def _build_menu(self):
        kw = dict(bg=BG2, fg=FG, activebackground=BG3, activeforeground=FG)
        mb = tk.Menu(self, tearoff=False, **kw); self.config(menu=mb)
        fm = tk.Menu(mb, tearoff=False, **kw); mb.add_cascade(label="File", menu=fm)
        fm.add_command(label="Open Archive…",  command=self._browse,      accelerator="Ctrl+O")
        fm.add_separator()
        fm.add_command(label="Open Project…",  command=self._open_project)
        fm.add_command(label="Save Project…",  command=self._save_project)
        fm.add_separator()
        fm.add_command(label="Quit",           command=self.quit,          accelerator="Ctrl+Q")
        em = tk.Menu(mb, tearoff=False, **kw); mb.add_cascade(label="Edit", menu=em)
        em.add_command(label="Edit Aliases…",  command=self._alias_editor)
        em.add_command(label="Export Aliases…",command=self._export_aliases)
        self.bind("<Control-o>", lambda _: self._browse())
        self.bind("<Control-q>", lambda _: self.quit())

    # ── Top-level UI ─────────────────────────────────────────────────────────

    def _build_ui(self):
        self._build_toolbar()
        self._build_filterbar()
        self._nb = ttk.Notebook(self)
        self._nb.pack(fill=tk.BOTH, expand=True, padx=4, pady=(0, 4))
        self._tab_s = ttk.Frame(self._nb); self._tab_q = ttk.Frame(self._nb)
        self._nb.add(self._tab_s, text="Samples")
        self._nb.add(self._tab_q, text="Sequences")
        self._build_samples_tab()
        self._build_sequences_tab()

    def _build_toolbar(self):
        tb = tk.Frame(self, bg=BG, pady=4); tb.pack(fill=tk.X, padx=6, pady=(4, 0))

        ttk.Label(tb, text="Archive", style="Dim.TLabel").pack(side=tk.LEFT, padx=(0, 3))
        self._arch_var = tk.StringVar()
        ttk.Entry(tb, textvariable=self._arch_var, width=42).pack(side=tk.LEFT)
        _btn(tb, "Browse…", self._browse).pack(side=tk.LEFT, padx=4)
        _btn(tb, "Load", self._do_load, style="Accent.TButton").pack(side=tk.LEFT, padx=(0, 8))

        ttk.Separator(tb, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=4)

        ttk.Label(tb, text="Output", style="Dim.TLabel").pack(side=tk.LEFT, padx=(0, 3))
        self._out_var = tk.StringVar()
        ttk.Entry(tb, textvariable=self._out_var, width=30).pack(side=tk.LEFT)
        _btn(tb, "…", self._browse_out).pack(side=tk.LEFT, padx=4)

        ttk.Separator(tb, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=4)

        _btn(tb, "Open Project", self._open_project).pack(side=tk.LEFT, padx=3)
        _btn(tb, "Save Project", self._save_project).pack(side=tk.LEFT, padx=3)

        ttk.Separator(tb, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=6)

        self._write_all_var = tk.StringVar(value="Write All  (0)")
        self._btn_write = _btn(tb, "", self._write_all, style="Accent.TButton")
        self._btn_write.configure(textvariable=self._write_all_var)
        self._btn_write.pack(side=tk.LEFT, padx=3)
        self._btn_write.state(["disabled"])

    def _build_filterbar(self):
        fb = tk.Frame(self, bg=BG, pady=2); fb.pack(fill=tk.X, padx=6)

        ttk.Label(fb, text="Filter", style="Dim.TLabel").pack(side=tk.LEFT, padx=(0, 3))
        self._filt_var = tk.StringVar()
        ttk.Entry(fb, textvariable=self._filt_var, width=22).pack(side=tk.LEFT)
        self._filt_var.trace_add("write", lambda *_: self._refresh_tree())

        ttk.Label(fb, text="Codec", style="Dim.TLabel").pack(side=tk.LEFT, padx=(10, 3))
        self._codec_var = tk.StringVar(value="All")
        cb = ttk.Combobox(fb, textvariable=self._codec_var, values=CODEC_LABELS,
                          state="readonly", width=10)
        cb.pack(side=tk.LEFT); cb.bind("<<ComboboxSelected>>", lambda _: self._refresh_tree())

        ttk.Label(fb, text="Bank", style="Dim.TLabel").pack(side=tk.LEFT, padx=(10, 3))
        self._bank_var = tk.StringVar(value="All")
        self._bank_cb = ttk.Combobox(fb, textvariable=self._bank_var, values=["All"],
                                     state="readonly", width=18)
        self._bank_cb.pack(side=tk.LEFT)
        self._bank_cb.bind("<<ComboboxSelected>>", lambda _: self._refresh_tree())

        _btn(fb, "Scan Folder…", self._scan_folder).pack(side=tk.LEFT, padx=(10, 0))

        self._status_var = tk.StringVar(value="No archive loaded.")
        ttk.Label(fb, textvariable=self._status_var, style="Dim.TLabel",
                  font=("TkDefaultFont", 9)).pack(side=tk.RIGHT, padx=6)

    # ── Samples tab ──────────────────────────────────────────────────────────

    def _build_samples_tab(self):
        right = tk.Frame(self._tab_s, bg=BG, width=340)
        right.pack(side=tk.RIGHT, fill=tk.Y)
        right.pack_propagate(False)
        ttk.Separator(self._tab_s, orient=tk.VERTICAL).pack(side=tk.RIGHT, fill=tk.Y)
        left = tk.Frame(self._tab_s, bg=BG)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self._build_tree(left)
        self._build_detail(right)

    def _build_tree(self, parent):
        cols = ("file", "codec", "size", "dur", "loop", "book", "slot")
        self._tree = ttk.Treeview(parent, columns=cols, show="tree headings",
                                   selectmode="browse")
        self._tree.heading("#0",    text="Sample")
        self._tree.heading("file",  text="Replace With")
        self._tree.heading("codec", text="Codec")
        self._tree.heading("size",  text="Size")
        self._tree.heading("dur",   text="Duration")
        self._tree.heading("loop",  text="Loop")
        self._tree.heading("book",  text="Book")
        self._tree.heading("slot",  text="Pitch")
        self._tree.column("#0",    width=280, stretch=True)
        self._tree.column("file",  width=140, stretch=True)
        self._tree.column("codec", width=70,  stretch=False, anchor="center")
        self._tree.column("size",  width=70,  stretch=False, anchor="e")
        self._tree.column("dur",   width=72,  stretch=False, anchor="e")
        self._tree.column("loop",  width=42,  stretch=False, anchor="center")
        self._tree.column("book",  width=42,  stretch=False, anchor="center")
        self._tree.column("slot",  width=60,  stretch=False, anchor="center")

        self._tree.tag_configure("bank",   foreground=FG,    font=("TkDefaultFont", 10, "bold"))
        self._tree.tag_configure("inst",   foreground=FG_DIM)
        self._tree.tag_configure("custom", foreground=MAUVE)
        self._tree.tag_configure("leaf",   foreground=FG)

        vsb = ttk.Scrollbar(parent, orient=tk.VERTICAL,   command=self._tree.yview)
        hsb = ttk.Scrollbar(parent, orient=tk.HORIZONTAL, command=self._tree.xview)
        self._tree.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
        parent.rowconfigure(0, weight=1)
        parent.columnconfigure(0, weight=1)
        self._tree.grid(row=0, column=0, sticky="nsew")
        vsb.grid(row=0, column=1, sticky="ns")
        hsb.grid(row=1, column=0, sticky="ew")
        self._tree.bind("<<TreeviewSelect>>", self._on_sel)
        self._tree.bind("<Double-1>",         lambda _: self._play())

    def _build_detail(self, parent):
        sf = tk.Frame(parent, bg=BG); sf.pack(fill=tk.BOTH, expand=True, padx=8, pady=6)

        ttk.Label(sf, text="Sample Details", style="Head.TLabel").pack(fill=tk.X, pady=(0, 6))

        # Info fields
        self._det_vars: Dict[str, tk.StringVar] = {}
        for key, label in [("Path", "Path"), ("Tuning", "Tuning"), ("Rate", "Rate"),
                            ("Banks", "Banks"), ("Slot", "Slot"), ("Codec", "Codec"),
                            ("Size", "Size"), ("Duration", "Duration"),
                            ("Loop", "Loop"), ("Book", "Book")]:
            row = tk.Frame(sf, bg=BG); row.pack(fill=tk.X, pady=1)
            ttk.Label(row, text=label, style="Dim.TLabel",
                      font=("TkDefaultFont", 9), width=9, anchor="e").pack(side=tk.LEFT, padx=(0, 6))
            v = tk.StringVar(value="—"); self._det_vars[key] = v
            ttk.Label(row, textvariable=v, foreground=FG, background=BG,
                      font=("TkDefaultFont", 9), anchor="w",
                      wraplength=230, justify="left").pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Alias
        al = tk.Frame(sf, bg=BG); al.pack(fill=tk.X, pady=(4, 2))
        ttk.Label(al, text="Alias", style="Dim.TLabel",
                  font=("TkDefaultFont", 9), width=9, anchor="e").pack(side=tk.LEFT, padx=(0, 6))
        self._alias_var = tk.StringVar()
        ttk.Entry(al, textvariable=self._alias_var, width=20).pack(side=tk.LEFT, fill=tk.X, expand=True)
        self._alias_var.trace_add("write", self._on_alias_change)

        ttk.Separator(sf, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=6)

        # Action buttons
        self._btn_play  = _btn(sf, "▶  Play",             self._play)
        self._btn_loop  = _btn(sf, "◈  Loop Editor…",     self._open_loop_editor)
        self._btn_exp   = _btn(sf, "↓  Export WAV…",      self._export_wav)
        self._btn_asgn  = _btn(sf, "↔  Assign Audio…",    self._assign_audio)
        self._btn_clr   = _btn(sf, "✕  Clear Assigned",   self._clear_assigned)
        self._btn_expal = _btn(sf, "⬆  Export Aliases…",  self._export_aliases)

        for b in (self._btn_play, self._btn_loop, self._btn_exp,
                  self._btn_asgn, self._btn_clr, self._btn_expal):
            b.pack(fill=tk.X, pady=1)
            b.state(["disabled"])

        ttk.Separator(sf, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=6)

        # Mini waveform
        ttk.Label(sf, text="Waveform preview", style="Dim.TLabel",
                  font=("TkDefaultFont", 8)).pack()
        self._mini = tk.Canvas(sf, bg=BG2, height=80,
                               highlightthickness=1, highlightbackground=BG3)
        self._mini.pack(fill=tk.X, pady=(2, 0))

    # ── Sequences tab ─────────────────────────────────────────────────────────

    def _build_sequences_tab(self):
        right = tk.Frame(self._tab_q, bg=BG, width=380)
        right.pack(side=tk.RIGHT, fill=tk.Y)
        right.pack_propagate(False)
        ttk.Separator(self._tab_q, orient=tk.VERTICAL).pack(side=tk.RIGHT, fill=tk.Y)
        left = tk.Frame(self._tab_q, bg=BG)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self._build_seq_list(left)
        self._build_seq_detail(right)

    def _build_seq_list(self, parent):
        # Filter bar — row 0 in grid
        fb = tk.Frame(parent, bg=BG, pady=3)
        fb.grid(row=0, column=0, columnspan=2, sticky="ew", padx=4)
        ttk.Label(fb, text="Filter", style="Dim.TLabel").pack(side=tk.LEFT, padx=(0, 3))
        self._sfilt = tk.StringVar()
        ttk.Entry(fb, textvariable=self._sfilt, width=24).pack(side=tk.LEFT)
        self._sfilt.trace_add("write", lambda *_: self._refresh_seq())
        self._scount_var = tk.StringVar(value="")
        ttk.Label(fb, textvariable=self._scount_var, style="Dim.TLabel",
                  font=("TkDefaultFont", 9)).pack(side=tk.RIGHT)

        cols = ("id", "name", "file", "banks", "size")
        self._seq_tree = ttk.Treeview(parent, columns=cols, show="headings", selectmode="browse")
        self._seq_tree.heading("id",    text="ID")
        self._seq_tree.heading("name",  text="Sequence")
        self._seq_tree.heading("file",  text="Replace With")
        self._seq_tree.heading("banks", text="Banks")
        self._seq_tree.heading("size",  text="M64 size")
        self._seq_tree.column("id",    width=40,  stretch=False)
        self._seq_tree.column("name",  width=200, stretch=True)
        self._seq_tree.column("file",  width=140, stretch=True)
        self._seq_tree.column("banks", width=240, stretch=True)
        self._seq_tree.column("size",  width=80,  stretch=False, anchor="e")
        self._seq_tree.tag_configure("assigned", foreground=MAUVE)
        vsb = ttk.Scrollbar(parent, orient=tk.VERTICAL,   command=self._seq_tree.yview)
        hsb = ttk.Scrollbar(parent, orient=tk.HORIZONTAL, command=self._seq_tree.xview)
        self._seq_tree.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
        # filter bar is row 0 (packed above), tree grid starts at row 1
        parent.rowconfigure(1, weight=1)
        parent.columnconfigure(0, weight=1)
        self._seq_tree.grid(row=1, column=0, sticky="nsew", padx=(4,0), pady=(0,4))
        vsb.grid(row=1, column=1, sticky="ns",              pady=(0,4))
        hsb.grid(row=2, column=0, sticky="ew",              padx=(4,0))
        self._seq_tree.bind("<<TreeviewSelect>>", self._on_seq_sel)

    def _build_seq_detail(self, parent):
        sf = tk.Frame(parent, bg=BG); sf.pack(fill=tk.BOTH, expand=True, padx=8, pady=6)

        ttk.Label(sf, text="Sequence Details", style="Head.TLabel").pack(fill=tk.X, pady=(0,6))

        self._seq_det_vars: Dict[str, tk.StringVar] = {}
        for key, label in [("Path","Path"), ("Seq ID","Seq ID"), ("Banks","Banks"), ("Size","Size")]:
            row = tk.Frame(sf, bg=BG); row.pack(fill=tk.X, pady=1)
            ttk.Label(row, text=label, style="Dim.TLabel",
                      font=("TkDefaultFont",9), width=8, anchor="e").pack(side=tk.LEFT, padx=(0,6))
            v = tk.StringVar(value="—"); self._seq_det_vars[key] = v
            ttk.Label(row, textvariable=v, foreground=FG, background=BG,
                      font=("TkDefaultFont",9), anchor="w",
                      wraplength=260, justify="left").pack(side=tk.LEFT, fill=tk.X, expand=True)

        ttk.Separator(sf, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=6)

        self._seq_btn_play = _btn(sf, "▶  Play",        self._seq_play)
        self._seq_btn_exp  = _btn(sf, "↓  Export .m64", self._seq_export)
        self._seq_btn_asgn = _btn(sf, "↔  Assign Audio…", self._seq_assign)
        self._seq_btn_clr  = _btn(sf, "✕  Clear Assigned", self._seq_clear)
        for b in (self._seq_btn_play, self._seq_btn_exp,
                  self._seq_btn_asgn, self._seq_btn_clr):
            b.pack(fill=tk.X, pady=1); b.state(["disabled"])

        ttk.Separator(sf, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=6)

        dh = tk.Frame(sf, bg=BG); dh.pack(fill=tk.X)
        ttk.Label(dh, text="Hex dump", style="Dim.TLabel",
                  font=("TkDefaultFont",9,"bold")).pack(side=tk.LEFT)
        _btn(dh, "Dump",  self._seq_dump).pack(side=tk.RIGHT, padx=2)
        _btn(dh, "Clear", self._seq_clear_txt).pack(side=tk.RIGHT)

        self._seq_txt = tk.Text(sf, bg=BG2, fg=FG, font=("monospace", 8),
                                state="disabled", wrap="none",
                                highlightthickness=0, relief="flat",
                                insertbackground=FG, selectbackground=BG3)
        sv = ttk.Scrollbar(sf, orient=tk.VERTICAL,   command=self._seq_txt.yview)
        sh = ttk.Scrollbar(sf, orient=tk.HORIZONTAL, command=self._seq_txt.xview)
        self._seq_txt.configure(yscrollcommand=sv.set, xscrollcommand=sh.set)
        sv.pack(side=tk.RIGHT, fill=tk.Y); sh.pack(side=tk.BOTTOM, fill=tk.X)
        self._seq_txt.pack(fill=tk.BOTH, expand=True, pady=(3, 0))

    # ── Archive loading ───────────────────────────────────────────────────────

    def _browse(self):
        p = filedialog.askopenfilename(title="Open Archive", filetypes=self._FT, parent=self)
        if p: self._arch_var.set(p); self._load(p)

    def _browse_out(self):
        d = filedialog.askdirectory(title="Output Directory", parent=self)
        if d: self._out_var.set(d); self._outdir = d

    def _do_load(self):
        p = self._arch_var.get().strip()
        if p: self._load(p)

    def _load(self, path: str):
        self._archive_path = path
        self._arch_var.set(path)
        if not self._outdir: self._out_var.set(os.path.dirname(path))
        self.title(f"{self._TITLE} — {Path(path).name}")
        self._status_var.set("Scanning…"); self.update_idletasks()
        threading.Thread(target=lambda: self._scan_worker(path), daemon=True).start()

    def _scan_worker(self, path: str):
        s, b, q = scan_archive(path)
        self.after(0, lambda: self._on_loaded(s, b, q))

    def _on_loaded(self, samples, banks, seqs):
        self._samples = samples; self._banks = banks; self._sequences = seqs
        # Update Bank combobox
        bank_names = ["All"] + sorted(Path(p).stem for p in banks)
        self._bank_cb["values"] = bank_names
        self._refresh_tree()
        self._refresh_seq()
        self._status_var.set(
            f"{len(samples)} samples · {len(banks)} banks · {len(seqs)} sequences"
        )

    # ── Sample tree ───────────────────────────────────────────────────────────

    def _match(self, entry: SampleEntry) -> bool:
        filt  = self._filt_var.get().lower()
        codec = self._codec_var.get()
        if filt and filt not in entry.path.lower() and filt not in self._aliases.get(entry.path).lower():
            return False
        if codec != "All":
            if codec == "ADPCM" and entry.is_custom: return False
            if codec != "ADPCM" and (not entry.is_custom or entry.custom_fmt.upper() != codec): return False
        return True

    def _leaf_vals(self, entry: SampleEntry, slot: str) -> tuple:
        asgn = self._assignments.get(entry.path, "")
        return (
            Path(asgn).name if asgn else "",
            entry.codec_str(),
            _fmt_size(entry.sample_size) if not entry.is_custom else "—",
            _fmt_dur(entry.estimated_duration()) if not entry.is_custom else "~",
            "✓" if entry.has_loop() else "",
            "✓" if entry.has_book() else "",
            slot.upper() if slot else "—",
        )

    def _insert_leaf(self, parent_iid: str, entry: SampleEntry, slot: str) -> str:
        alias  = self._aliases.get(entry.path)
        name   = alias if alias else Path(entry.path).name
        lbl    = f"  {name}  [{slot}]" if slot else f"  {name}"
        tags   = ("custom",) if entry.is_custom else ("leaf",)
        self._leaf_seq += 1
        iid    = f"leaf:{self._leaf_seq}"
        self._tree.insert(parent_iid, tk.END, iid=iid, text=lbl,
                          values=self._leaf_vals(entry, slot), tags=tags)
        info   = {"entry": entry, "slot": slot}
        self._node_map[iid] = info
        self._iids_by_path.setdefault(entry.path, []).append(iid)
        return iid

    def _refresh_tree(self):
        self._tree.delete(*self._tree.get_children())
        self._node_map.clear(); self._iids_by_path.clear()
        self._leaf_seq = 0
        bank_filt = self._bank_var.get()
        shown = 0

        for bank_path, bank in sorted(self._banks.items()):
            bstem = Path(bank_path).stem
            if bank_filt != "All" and bstem != bank_filt: continue

            bank_iid  = f"bank:{bank_path}"; has_any = False

            for i, inst in enumerate(bank.instruments):
                if inst is None: continue
                inst_leaves = []
                for slot, sname, tuning in [("lo",     inst.lo_sample,  inst.lo_tuning),
                                             ("normal", inst.med_sample, inst.med_tuning),
                                             ("hi",     inst.hi_sample,  inst.hi_tuning)]:
                    if not sname or sname not in self._samples: continue
                    e = self._samples[sname]
                    if not self._match(e): continue
                    inst_leaves.append((e, slot))

                if inst_leaves:
                    if not has_any:
                        self._tree.insert("", tk.END, iid=bank_iid,
                                         text=f"Bank  {bstem}", open=True, tags=("bank",))
                        has_any = True
                    rng = f"{inst.normal_range_lo}–{inst.normal_range_hi}"
                    iid = f"inst:{bank_path}:{i}"
                    self._tree.insert(bank_iid, tk.END, iid=iid, open=True,
                                     text=f"  Inst {i}  ({rng})", tags=("inst",))
                    for e, slot in inst_leaves:
                        self._insert_leaf(iid, e, slot); shown += 1

            drum_leaves = []
            for j, drum in enumerate(bank.drums):
                if not drum.sample_path or drum.sample_path not in self._samples: continue
                e = self._samples[drum.sample_path]
                if not self._match(e): continue
                drum_leaves.append((e, f"drum_{j}"))

            if drum_leaves:
                if not has_any:
                    self._tree.insert("", tk.END, iid=bank_iid,
                                     text=f"Bank  {bstem}", open=True, tags=("bank",))
                    has_any = True
                diid = f"drums:{bank_path}"
                self._tree.insert(bank_iid, tk.END, iid=diid, open=True,
                                 text=f"  Drums ({len(drum_leaves)})", tags=("inst",))
                for e, slot in drum_leaves:
                    self._insert_leaf(diid, e, slot); shown += 1

        # Unassigned samples (only shown when no bank filter is active)
        if bank_filt == "All":
            un = [(p, e) for p, e in sorted(self._samples.items())
                   if not e.bank_path and self._match(e)]
            if un:
                self._tree.insert("", tk.END, iid="__un__",
                                 text=f"Unassigned ({len(un)})", open=False, tags=("bank",))
                for p, e in un:
                    self._insert_leaf("__un__", e, "—"); shown += 1

        total = len(self._samples)
        self._status_var.set(f"{shown} shown / {total} total")

    # ── Sample selection ──────────────────────────────────────────────────────

    def _on_sel(self, _event):
        sel = self._tree.selection()
        if not sel: return
        iid = sel[0]
        info = self._node_map.get(iid)
        if not info: self._sel_entry = None; return
        self._sel_entry = info["entry"]
        self._update_detail(info["entry"], info["slot"])

    def _update_detail(self, entry: SampleEntry, slot: str):
        rate  = entry.pcm_rate()
        dur   = entry.estimated_duration()
        lp    = entry.loop
        bk    = entry.book

        self._det_vars["Path"].set(entry.path)
        self._det_vars["Tuning"].set(f"{entry.tuning:.6f}" if entry.tuning else "—")
        self._det_vars["Rate"].set(f"~{rate:,} Hz")
        self._det_vars["Banks"].set(Path(entry.bank_path).stem if entry.bank_path else "—")
        self._det_vars["Slot"].set(slot if slot else "—")
        self._det_vars["Codec"].set(entry.codec_str())
        self._det_vars["Size"].set(_fmt_size(entry.sample_size) if not entry.is_custom else "—")
        self._det_vars["Duration"].set(_fmt_dur(dur) if not entry.is_custom else "~")
        if entry.has_loop():
            cnt = "∞" if lp.count == LOOP_INFINITE else str(lp.count)
            self._det_vars["Loop"].set(f"{lp.start} – {lp.end}  ×{cnt}")
        else:
            self._det_vars["Loop"].set("none")
        if entry.has_book():
            self._det_vars["Book"].set(f"order={bk.order}  npred={bk.npredictors}  coefs={len(bk.book)}")
        else:
            self._det_vars["Book"].set("none")

        self._alias_var.set(self._aliases.get(entry.path))

        # Enable buttons
        can_play = entry.is_custom or (entry.has_book() and entry.sample_data)
        for b in (self._btn_play, self._btn_exp, self._btn_asgn, self._btn_expal):
            b.state(["!disabled"])
        self._btn_loop.state(["!disabled"] if can_play and entry.has_loop() else ["disabled"])
        self._btn_clr.state(["!disabled"] if entry.path in self._assignments else ["disabled"])

        self._draw_mini(entry)

    def _draw_mini(self, entry: SampleEntry):
        c = self._mini; c.delete("all")
        W = c.winfo_width() or 290; H = 80
        c.create_rectangle(0, 0, W, H, fill=BG2, outline="")
        pcm = entry._pcm
        if not pcm:
            c.create_text(W//2, H//2, text="press Play to preview", fill=FG_DIM, font=("TkDefaultFont",9))
            return
        n = len(pcm); mid = H // 2
        for x in range(W):
            i = int(x * n / max(1, W)); v = abs(pcm[i]) if i < n else 0
            h = int(v / 32768.0 * mid * 0.9)
            c.create_line(x, mid-h, x, mid+h, fill=ACCENT)

    # ── Sample actions ────────────────────────────────────────────────────────

    def _on_alias_change(self, *_):
        e = self._sel_entry
        if not e: return
        self._aliases.set(e.path, self._alias_var.get().strip())
        for iid in self._iids_by_path.get(e.path, []):
            try: self._tree.item(iid, text=f"  {self._alias_var.get() or Path(e.path).name}  [{self._node_map[iid]['slot']}]")
            except: pass

    def _play(self):
        e = self._sel_entry
        if not e or not self._archive_path: return
        def w():
            pcm, rate = _get_pcm(e, self._archive_path)
            if pcm: _play_wav(_pcm_to_wav(pcm, rate)); self.after(0, lambda: self._draw_mini(e))
        threading.Thread(target=w, daemon=True).start()

    def _open_loop_editor(self):
        e = self._sel_entry
        if not e or not self._archive_path: return
        _WaveformEditor(self, e, self._archive_path,
                        on_save=lambda ee: self._update_detail(ee, self._node_map.get(self._tree.selection()[0] if self._tree.selection() else "", {}).get("slot", "")))

    def _export_wav(self):
        e = self._sel_entry
        if not e: return
        outdir = self._out_var.get().strip() or os.path.expanduser("~")
        out = filedialog.asksaveasfilename(
            defaultextension=".wav", initialdir=outdir,
            filetypes=[("WAV", "*.wav"), ("All", "*")],
            initialfile=Path(e.path).stem + ".wav", parent=self)
        if not out: return
        try: export_wav(e, out, self._archive_path); messagebox.showinfo("Exported", f"Saved to:\n{out}", parent=self)
        except Exception as ex: messagebox.showerror("Export failed", str(ex), parent=self)

    def _assign_audio(self):
        e = self._sel_entry
        if not e: return
        src = filedialog.askopenfilename(
            title="Choose replacement audio",
            filetypes=[("Audio", "*.mp3 *.ogg *.wav *.flac *.aiff"), ("All", "*")],
            parent=self)
        if not src: return
        self._assignments[e.path] = src
        self._update_write_all_btn()
        self._btn_clr.state(["!disabled"])
        for iid in self._iids_by_path.get(e.path, []):
            try: self._tree.set(iid, "file", Path(src).name)
            except: pass

    def _clear_assigned(self):
        e = self._sel_entry
        if not e or e.path not in self._assignments: return
        del self._assignments[e.path]
        self._update_write_all_btn()
        self._btn_clr.state(["disabled"])
        for iid in self._iids_by_path.get(e.path, []):
            try: self._tree.set(iid, "file", "")
            except: pass

    def _update_write_all_btn(self):
        n = len(self._assignments) + len(self._seq_assignments)
        self._write_all_var.set(f"Write All  ({n})")
        self._btn_write.state(["!disabled"] if n else ["disabled"])

    def _write_all(self):
        if not (self._assignments or self._seq_assignments) or not self._archive_path: return
        outdir = self._out_var.get().strip() or os.path.dirname(self._archive_path)
        mod = os.path.join(outdir, "mods", "custom_audio.o2r")
        os.makedirs(os.path.dirname(mod), exist_ok=True)
        done, errs = 0, []
        for path, src in self._assignments.items():
            e = self._samples.get(path)
            if not e: continue
            try: do_replace(e, src, self._archive_path, mod); done += 1
            except Exception as ex: errs.append(f"{Path(path).name}: {ex}")
        for path, src in self._seq_assignments.items():
            seq = self._sequences.get(path)
            if not seq: continue
            try: do_replace_seq(seq, src, self._archive_path, mod); done += 1
            except Exception as ex: errs.append(f"{Path(path).name}: {ex}")
        msg = f"Written {done} replacement(s) to:\n{mod}"
        if errs: msg += f"\n\nErrors ({len(errs)}):\n" + "\n".join(errs[:8])
        messagebox.showinfo("Write All complete", msg, parent=self)

    def _scan_folder(self):
        folder = filedialog.askdirectory(title="Scan folder", parent=self)
        if not folder: return
        found = []
        for root, _, files in os.walk(folder):
            for fn in files:
                if fn.lower().endswith((".mp3", ".ogg", ".wav", ".flac", ".aiff")):
                    found.append(os.path.join(root, fn))
        auto = 0
        for fp in found:
            stem = Path(fp).stem.lower()
            for sp, e in self._samples.items():
                if Path(sp).stem.lower() == stem and sp not in self._assignments:
                    self._assignments[sp] = fp; auto += 1; break
        self._update_write_all_btn()
        messagebox.showinfo("Scan result",
                            f"Found {len(found)} audio file(s).\nAuto-assigned {auto} sample(s).",
                            parent=self)

    def _export_aliases(self):
        if not self._aliases.all():
            messagebox.showinfo("No aliases", "No aliases defined yet.", parent=self)
            return
        outdir = self._out_var.get().strip() or os.path.expanduser("~")
        out = filedialog.asksaveasfilename(
            defaultextension=".json", initialdir=outdir,
            filetypes=[("JSON", "*.json"), ("All", "*")],
            initialfile="aliases.json", parent=self)
        if not out: return
        with open(out, "w", encoding="utf-8") as f:
            json.dump(self._aliases.all(), f, indent=2)
        messagebox.showinfo("Exported", f"Aliases saved to:\n{out}", parent=self)

    # ── Sequences ─────────────────────────────────────────────────────────────

    def _refresh_seq(self):
        filt = self._sfilt.get().lower()
        self._seq_tree.delete(*self._seq_tree.get_children())
        shown = 0
        for path, seq in sorted(self._sequences.items(), key=lambda x: x[1].seq_id):
            seq_name = SEQ_NAMES.get(seq.seq_id, Path(path).stem)
            if filt and filt not in seq_name.lower() and filt not in str(seq.seq_id) and filt not in path.lower(): continue
            banks_str = ", ".join(Path(b).stem for b in seq.banks) or "—"
            size_str  = _fmt_size(len(seq.m64_data)) if seq.m64_data else "—"
            asgn = self._seq_assignments.get(path, "")
            tags = ("assigned",) if asgn else ()
            self._seq_tree.insert("", tk.END, iid=path, tags=tags,
                                  values=(seq.seq_id, seq_name, Path(asgn).name if asgn else "", banks_str, size_str))
            shown += 1
        self._scount_var.set(f"{shown} sequences")

    def _on_seq_sel(self, _event):
        sel = self._seq_tree.selection()
        if not sel: return
        seq = self._sequences.get(sel[0])
        if not seq: return
        self._sel_seq = seq
        name = f"{seq.seq_id} — {SEQ_NAMES.get(seq.seq_id, Path(seq.path).stem)}"
        banks_str = "\n".join(Path(b).stem for b in seq.banks) or "—"
        self._seq_det_vars["Path"].set(seq.path)
        self._seq_det_vars["Seq ID"].set(name)
        self._seq_det_vars["Banks"].set(banks_str)
        self._seq_det_vars["Size"].set(f"{len(seq.m64_data):,} bytes" if seq.m64_data else "—")
        for b in (self._seq_btn_play, self._seq_btn_exp, self._seq_btn_asgn):
            b.state(["!disabled"])
        self._seq_btn_clr.state(["!disabled"] if seq.path in self._seq_assignments else ["disabled"])

    def _seq_play(self):
        seq = self._sel_seq
        if not seq or not seq.m64_data: return
        messagebox.showinfo("Playback", "M64 sequence playback requires the SM64 engine.\nExport .m64 and use it in-game.", parent=self)

    def _seq_export(self):
        seq = self._sel_seq
        if not seq or not seq.m64_data: return
        outdir = self._out_var.get().strip() or os.path.expanduser("~")
        stem = Path(seq.path).stem
        out = filedialog.asksaveasfilename(
            defaultextension=".m64", initialdir=outdir,
            filetypes=[("M64 sequence", "*.m64"), ("All", "*")],
            initialfile=stem + ".m64", parent=self)
        if not out: return
        with open(out, "wb") as f: f.write(seq.m64_data)
        messagebox.showinfo("Exported", f"Saved to:\n{out}", parent=self)

    def _seq_assign(self):
        seq = self._sel_seq
        if not seq: return
        src = filedialog.askopenfilename(
            title="Choose replacement audio",
            filetypes=[("Audio", "*.mp3 *.ogg *.wav *.flac *.aiff"), ("All", "*")],
            parent=self)
        if not src: return
        self._seq_assignments[seq.path] = src
        self._update_write_all_btn()
        self._seq_btn_clr.state(["!disabled"])
        try:
            self._seq_tree.set(seq.path, "file", Path(src).name)
            self._seq_tree.item(seq.path, tags=("assigned",))
        except Exception: pass
        self._seq_txt.configure(state="normal")
        self._seq_txt.delete("1.0", "end")
        self._seq_txt.insert("1.0",
            f"[Assigned: {Path(src).name}]\n"
            f"Will be written as streamed audio via CustomFormat on Write All.\n"
            f"Archive path: sound/sequences/{seq.seq_id:02d}.{Path(src).suffix.lstrip('.')}")
        self._seq_txt.configure(state="disabled")

    def _seq_clear(self):
        seq = self._sel_seq
        if not seq or seq.path not in self._seq_assignments: return
        del self._seq_assignments[seq.path]
        self._update_write_all_btn()
        self._seq_btn_clr.state(["disabled"])
        try:
            self._seq_tree.set(seq.path, "file", "")
            self._seq_tree.item(seq.path, tags=())
        except Exception: pass
        self._seq_dump()

    def _seq_dump(self):
        seq = self._sel_seq
        if not seq: return
        txt = _hexdump(seq.m64_data) if seq.m64_data else "(no M64 data)"
        self._seq_txt.configure(state="normal")
        self._seq_txt.delete("1.0", "end")
        self._seq_txt.insert("1.0", txt)
        self._seq_txt.configure(state="disabled")

    def _seq_clear_txt(self):
        self._seq_txt.configure(state="normal")
        self._seq_txt.delete("1.0", "end")
        self._seq_txt.configure(state="disabled")

    # ── Alias editor ──────────────────────────────────────────────────────────

    def _alias_editor(self):
        win = tk.Toplevel(self); win.title("Alias Editor")
        win.configure(bg=BG); win.geometry("740x480")
        fr = tk.Frame(win, bg=BG); fr.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)
        tree = ttk.Treeview(fr, columns=("path","alias"), show="headings")
        tree.heading("path",  text="Sample path"); tree.heading("alias", text="Alias")
        tree.column("path",  width=460, stretch=True); tree.column("alias", width=240, stretch=True)
        vsb = ttk.Scrollbar(fr, orient=tk.VERTICAL, command=tree.yview)
        tree.configure(yscrollcommand=vsb.set); vsb.pack(side=tk.RIGHT, fill=tk.Y); tree.pack(fill=tk.BOTH, expand=True)
        for p, a in sorted(self._aliases.all().items()): tree.insert("","end",iid=p,values=(p,a))
        bf = tk.Frame(win, bg=BG); bf.pack(fill=tk.X, padx=6, pady=4)
        def delete_sel():
            for iid in tree.selection(): self._aliases.set(iid, ""); tree.delete(iid)
        _btn(bf, "Delete selected", delete_sel).pack(side=tk.LEFT, padx=4)
        _btn(bf, "Close", win.destroy).pack(side=tk.RIGHT, padx=4)

    # ── Project save/load ─────────────────────────────────────────────────────

    def _save_project(self):
        out = filedialog.asksaveasfilename(
            defaultextension=".ssproj",
            filetypes=[("Ghostship Project", "*.ssproj"), ("All", "*")], parent=self)
        if not out: return
        proj = {"version": 1, "archive": self._archive_path,
                "outdir": self._out_var.get(), "aliases": self._aliases.all(),
                "assignments": self._assignments,
                "seq_assignments": self._seq_assignments}
        import zipfile as _zf
        with _zf.ZipFile(out, "w", _zf.ZIP_DEFLATED) as zf:
            zf.writestr("project.json", json.dumps(proj, indent=2))
        messagebox.showinfo("Saved", f"Project saved to:\n{out}", parent=self)

    def _open_project(self):
        path = filedialog.askopenfilename(
            title="Open Project",
            filetypes=[("Ghostship Project", "*.ssproj"), ("All", "*")], parent=self)
        if not path: return
        try:
            with zipfile.ZipFile(path) as zf:
                proj = json.loads(zf.read("project.json").decode())
            for p, a in proj.get("aliases", {}).items(): self._aliases.set(p, a)
            self._assignments     = proj.get("assignments", {})
            self._seq_assignments = proj.get("seq_assignments", {})
            self._update_write_all_btn()
            if d := proj.get("outdir", ""): self._out_var.set(d); self._outdir = d
            if a := proj.get("archive", ""):
                if os.path.exists(a): self._load(a)
                else: messagebox.showwarning("Archive not found", f"Archive not found:\n{a}", parent=self)
        except Exception as ex: messagebox.showerror("Load failed", str(ex), parent=self)


# ─────────────────────────────────────────────────────────────────────────────
# CLI
# ─────────────────────────────────────────────────────────────────────────────

def _cmd_list(args):
    s, _, _ = scan_archive(args.archive)
    for p in sorted(s):
        e = s[p]
        print(f"{p}\t{e.codec_str()}\t{e.sample_size}\t{e.loop.start}-{e.loop.end}\t{e.tuning:.6f}")

def _cmd_export(args):
    s, _, _ = scan_archive(args.archive)
    e = s.get(args.sample)
    if not e: print(f"Not found: {args.sample}", file=sys.stderr); sys.exit(1)
    export_wav(e, args.output, args.archive); print(f"Exported to {args.output}")

def _cmd_replace(args):
    s, _, _ = scan_archive(args.archive)
    e = s.get(args.sample)
    if not e: print(f"Not found: {args.sample}", file=sys.stderr); sys.exit(1)
    mod = do_replace(e, args.audio, args.archive, args.mod); print(f"Written to {mod}")

def _cmd_gui(args):
    App(getattr(args, "archive", "") or "").mainloop()


def main():
    p = argparse.ArgumentParser(prog="sample_editor", description="Ghostship Sample Editor")
    sub = p.add_subparsers(dest="cmd")

    g = sub.add_parser("gui");    g.add_argument("archive", nargs="?", default=""); g.set_defaults(func=_cmd_gui)
    l = sub.add_parser("list");   l.add_argument("archive"); l.set_defaults(func=_cmd_list)
    e = sub.add_parser("export"); e.add_argument("archive"); e.add_argument("sample"); e.add_argument("output"); e.set_defaults(func=_cmd_export)
    r = sub.add_parser("replace"); r.add_argument("archive"); r.add_argument("sample"); r.add_argument("audio"); r.add_argument("--mod", default=""); r.set_defaults(func=_cmd_replace)

    args = p.parse_args()
    (args.func if hasattr(args, "func") else _cmd_gui)(args if hasattr(args, "func") else argparse.Namespace(archive=""))


if __name__ == "__main__":
    main()
