import sys
import os
import re
import argparse
import math
from enum import Enum
from io import BytesIO
from typing import Optional, Dict

from PIL import Image

class TextureType(Enum):
    Error = 0
    RGBA32bpp = 1
    RGBA16bpp = 2
    Palette4bpp = 3
    Palette8bpp = 4
    Grayscale4bpp = 5
    Grayscale8bpp = 6
    GrayscaleAlpha4bpp = 7
    GrayscaleAlpha8bpp = 8
    GrayscaleAlpha16bpp = 9


TexturePixelMultipliers: Dict[TextureType, float] = {
    TextureType.Error: 0.0,
    TextureType.RGBA32bpp: 4.0,
    TextureType.RGBA16bpp: 2.0,
    TextureType.Palette4bpp: 0.5,
    TextureType.Palette8bpp: 1.0,
    TextureType.Grayscale4bpp: 0.5,
    TextureType.Grayscale8bpp: 1.0,
    TextureType.GrayscaleAlpha4bpp: 0.5,
    TextureType.GrayscaleAlpha8bpp: 1.0,
    TextureType.GrayscaleAlpha16bpp: 2.0,
}

class TextureTypeUtils:
    @staticmethod
    def num_to_texture_type(num: int) -> TextureType:
        mapping = {
            1: TextureType.RGBA32bpp,
            2: TextureType.RGBA16bpp,
            3: TextureType.Palette4bpp,
            4: TextureType.Palette8bpp,
            5: TextureType.Grayscale4bpp,
            6: TextureType.Grayscale8bpp,
            7: TextureType.GrayscaleAlpha4bpp,
            8: TextureType.GrayscaleAlpha8bpp,
            9: TextureType.GrayscaleAlpha16bpp,
        }
        if num not in mapping:
            raise ValueError(f"Unknown texture type number: {num}")
        return mapping[num]

    @staticmethod
    def str_to_texture_type(string: str) -> TextureType:
        mapping = {
            'RGBA32': TextureType.RGBA32bpp,
            'RGBA16': TextureType.RGBA16bpp,
            'TLUT': TextureType.RGBA16bpp,
            'CI4': TextureType.Palette4bpp,
            'CI8': TextureType.Palette8bpp,
            'I4': TextureType.Grayscale4bpp,
            'I8': TextureType.Grayscale8bpp,
            'IA4': TextureType.GrayscaleAlpha4bpp,
            'IA8': TextureType.GrayscaleAlpha8bpp,
            'IA16': TextureType.GrayscaleAlpha16bpp,
        }
        if string not in mapping:
            raise ValueError(f"Unknown texture type string: {string}")
        return mapping[string]

    @staticmethod
    def get_buffer_size(tex_type: TextureType, width: int, height: int) -> int:
        pixels = width * height
        if tex_type == TextureType.RGBA32bpp:
            return pixels * 4
        elif tex_type in (TextureType.RGBA16bpp, TextureType.GrayscaleAlpha16bpp):
            return pixels * 2
        elif tex_type in (TextureType.Palette8bpp, TextureType.Grayscale8bpp, TextureType.GrayscaleAlpha8bpp):
            return pixels
        elif tex_type in (TextureType.Palette4bpp, TextureType.Grayscale4bpp, TextureType.GrayscaleAlpha4bpp):
            return (pixels + 1) // 2  # Equivalent to Math.ceil
        return 0


class Texture:
    def __init__(self, width: int, height: int, texture_type: TextureType, tex_data: bytearray = None, tlut: Optional['Texture'] = None):
        self.width = width
        self.height = height
        self.texture_type = texture_type
        self.tex_data_size = TextureTypeUtils.get_buffer_size(texture_type, width, height)
        self.tex_data = tex_data if tex_data is not None else bytearray(self.tex_data_size)
        self.tlut = tlut

    def to_png_bytes(self) -> bytes:
        return N64Graphics.convert_n64_to_png(self)


# --- Main Implementation ---

class N64Graphics:

    @staticmethod
    def has_alpha(texture: Texture) -> bool:
        t = texture.texture_type
        return t in (
            TextureType.Palette4bpp,
            TextureType.Palette8bpp,
            TextureType.RGBA32bpp,
            TextureType.RGBA16bpp,
            TextureType.GrayscaleAlpha16bpp,
            TextureType.GrayscaleAlpha8bpp,
            TextureType.GrayscaleAlpha4bpp,
        )

    @staticmethod
    def pixels_to_png(texture: Texture, data: bytes) -> bytes:
        """Converts raw generic RGBA bytes directly to PNG"""
        img = Image.frombytes("RGBA", (texture.width, texture.height), data)
        out_buffer = BytesIO()
        img.save(out_buffer, format="PNG")
        return out_buffer.getvalue()

    @staticmethod
    def convert_raw_to_n64(texture: Texture, img: Image.Image) -> None:
        """Converts a standard PIL Image into raw N64 byte sequences"""
        texture.width = img.width
        texture.height = img.height
        texture.tex_data_size = TextureTypeUtils.get_buffer_size(texture.texture_type, texture.width, texture.height)
        texture.tex_data = bytearray(texture.tex_data_size)

        w, h = texture.width, texture.height
        tex_data = texture.tex_data
        tex_type = texture.texture_type

        if tex_type == TextureType.RGBA16bpp:
            img_rgba = img.convert("RGBA")
            pixels = img_rgba.load()
            for y in range(h):
                for x in range(w):
                    r, g, b, a = pixels[x, y]
                    pos = ((y * w) + x) * 2
                    r5 = r // 8
                    g5 = g // 8
                    b5 = b // 8
                    a_bit = 1 if a != 0 else 0

                    data_val = (r5 << 11) | (g5 << 6) | (b5 << 1) | a_bit
                    tex_data[pos] = (data_val & 0xFF00) >> 8
                    tex_data[pos + 1] = data_val & 0x00FF

        elif tex_type == TextureType.RGBA32bpp:
            img_rgba = img.convert("RGBA")
            pixels = img_rgba.load()
            for y in range(h):
                for x in range(w):
                    r, g, b, a = pixels[x, y]
                    pos = ((y * w) + x) * 4
                    tex_data[pos:pos+4] = [r, g, b, a]

        elif tex_type == TextureType.Palette4bpp:
            img_p = img.convert("P") if img.mode != "P" else img
            pixels = img_p.load()
            for y in range(h):
                for x in range(0, w, 2):
                    pos = ((y * w) + x) // 2
                    idx1 = pixels[x, y]
                    idx2 = pixels[x + 1, y] if x + 1 < w else 0
                    tex_data[pos] = ((idx1 & 0x0F) << 4) | (idx2 & 0x0F)

        elif tex_type == TextureType.Palette8bpp:
            img_p = img.convert("P") if img.mode != "P" else img
            pixels = img_p.load()
            for y in range(h):
                for x in range(w):
                    pos = (y * w) + x
                    tex_data[pos] = pixels[x, y] & 0xFF

        elif tex_type == TextureType.Grayscale4bpp:
            img_l = img.convert("L") if img.mode != "L" else img
            pixels = img_l.load()
            for y in range(h):
                for x in range(0, w, 2):
                    pos = ((y * w) + x) // 2
                    r1 = pixels[x, y]
                    r2 = pixels[x + 1, y] if x + 1 < w else 0
                    tex_data[pos] = (((r1 // 16) << 4) | (r2 // 16)) & 0xFF

        elif tex_type == TextureType.Grayscale8bpp:
            img_l = img.convert("L") if img.mode != "L" else img
            pixels = img_l.load()
            for y in range(h):
                for x in range(w):
                    pos = (y * w) + x
                    tex_data[pos] = pixels[x, y] & 0xFF

        elif tex_type == TextureType.GrayscaleAlpha4bpp:
            img_la = img.convert("LA") if img.mode != "LA" else img
            pixels = img_la.load()
            for y in range(h):
                for x in range(0, w, 2):
                    pos = ((y * w) + x) // 2
                    l1, a1 = pixels[x, y]
                    l2, a2 = pixels[x + 1, y] if x + 1 < w else (0, 0)
                    
                    alpha_bit1 = 1 if a1 != 0 else 0
                    alpha_bit2 = 1 if a2 != 0 else 0

                    data_val = ((((l1 // 32) << 1) | alpha_bit1) << 4) | (((l2 // 32) << 1) | alpha_bit2)
                    tex_data[pos] = data_val & 0xFF

        elif tex_type == TextureType.GrayscaleAlpha8bpp:
            img_la = img.convert("LA") if img.mode != "LA" else img
            pixels = img_la.load()
            for y in range(h):
                for x in range(w):
                    pos = (y * w) + x
                    l, a = pixels[x, y]
                    tex_data[pos] = (((l // 16) << 4) | (a // 16)) & 0xFF

        elif tex_type == TextureType.GrayscaleAlpha16bpp:
            img_la = img.convert("LA") if img.mode != "LA" else img
            pixels = img_la.load()
            for y in range(h):
                for x in range(w):
                    pos = ((y * w) + x) * 2
                    l, a = pixels[x, y]
                    tex_data[pos] = l & 0xFF
                    tex_data[pos + 1] = a & 0xFF

        elif tex_type == TextureType.Error:
            raise ValueError("Unknown texture type")

    @staticmethod
    def convert_n64_to_png(texture: Texture) -> Optional[bytes]:
        """Converts raw N64 byte sequences to PNG bytes"""
        w, h = texture.width, texture.height
        tex_data = texture.tex_data
        tex_type = texture.texture_type
        is_palette = tex_type in (TextureType.Palette4bpp, TextureType.Palette8bpp)

        img: Optional[Image.Image] = None

        if tex_type == TextureType.RGBA32bpp:
            img = Image.frombytes("RGBA", (w, h), bytes(tex_data))

        elif tex_type == TextureType.RGBA16bpp:
            img = Image.new("RGBA", (w, h))
            pixels = img.load()
            for y in range(h):
                for x in range(w):
                    pos = ((y * w) + x) * 2
                    data_val = (tex_data[pos] << 8) | tex_data[pos + 1]
                    r = ((data_val & 0xF800) >> 11) * 8
                    g = ((data_val & 0x07C0) >> 6) * 8
                    b = ((data_val & 0x003E) >> 1) * 8
                    a = (data_val & 0x01) * 255
                    pixels[x, y] = (r, g, b, a)

        elif tex_type == TextureType.Palette4bpp:
            img = Image.new("P", (w, h))
            pixels = img.load()
            palette = [0] * (256 * 4)  # RGBA palette buffer
            for y in range(h):
                for x in range(0, w, 2):
                    pos = ((y * w) + x) // 2
                    idx1 = (tex_data[pos] & 0xF0) >> 4
                    idx2 = tex_data[pos] & 0x0F
                    
                    pixels[x, y] = idx1
                    if x + 1 < w:
                        pixels[x + 1, y] = idx2

                    # default fallback grayscale palette colors if no TLUT
                    palette[idx1*4 : idx1*4+4] = [idx1*16, idx1*16, idx1*16, 255]
                    palette[idx2*4 : idx2*4+4] = [idx2*16, idx2*16, idx2*16, 255]

        elif tex_type == TextureType.Palette8bpp:
            img = Image.new("P", (w, h))
            pixels = img.load()
            palette = [0] * (256 * 4)
            for y in range(h):
                for x in range(w):
                    pos = (y * w) + x
                    idx = tex_data[pos]
                    pixels[x, y] = idx
                    palette[idx*4 : idx*4+4] = [idx, idx, idx, 255]

        elif tex_type == TextureType.Grayscale4bpp:
            img = Image.new("L", (w, h))
            pixels = img.load()
            for y in range(h):
                for x in range(0, w, 2):
                    pos = ((y * w) + x) // 2
                    g1 = tex_data[pos] & 0xF0
                    g2 = (tex_data[pos] & 0x0F) << 4
                    
                    pixels[x, y] = g1
                    if x + 1 < w:
                        pixels[x + 1, y] = g2

        elif tex_type == TextureType.Grayscale8bpp:
            img = Image.new("L", (w, h))
            pixels = img.load()
            for y in range(h):
                for x in range(w):
                    pos = (y * w) + x
                    pixels[x, y] = tex_data[pos]

        elif tex_type == TextureType.GrayscaleAlpha4bpp:
            img = Image.new("LA", (w, h))
            pixels = img.load()
            for y in range(h):
                for x in range(0, w, 2):
                    pos = ((y * w) + x) // 2
                    val = tex_data[pos]

                    d1 = (val & 0xF0) >> 4
                    g1 = ((d1 & 0x0E) >> 1) * 32
                    a1 = (d1 & 0x01) * 255
                    pixels[x, y] = (g1, a1)

                    if x + 1 < w:
                        d2 = val & 0x0F
                        g2 = ((d2 & 0x0E) >> 1) * 32
                        a2 = (d2 & 0x01) * 255
                        pixels[x + 1, y] = (g2, a2)

        elif tex_type == TextureType.GrayscaleAlpha8bpp:
            img = Image.new("LA", (w, h))
            pixels = img.load()
            for y in range(h):
                for x in range(w):
                    pos = (y * w) + x
                    val = tex_data[pos]
                    g = val & 0xF0
                    a = (val & 0x0F) << 4
                    pixels[x, y] = (g, a)

        elif tex_type == TextureType.GrayscaleAlpha16bpp:
            img = Image.new("LA", (w, h))
            pixels = img.load()
            for y in range(h):
                for x in range(w):
                    pos = ((y * w) + x) * 2
                    g = tex_data[pos]
                    a = tex_data[pos + 1]
                    pixels[x, y] = (g, a)
        else:
            return None

        # --- Palette Handling (TLUT) ---
        if is_palette:
            if texture.tlut:
                pal_bytes = texture.tlut.to_png_bytes()
                pal_img = Image.open(BytesIO(pal_bytes)).convert("RGBA")
                pal_pixels = pal_img.load()
                
                # Update the RGBA buffer using the TLUT image data
                for y in range(pal_img.height):
                    for x in range(pal_img.width):
                        index = y * pal_img.width + x
                        if index >= 256:
                            continue
                        r, g, b, a = pal_pixels[x, y]
                        palette[index*4 : index*4+4] = [r, g, b, a]
            
            # Apply the raw RGBA buffer to the PIL 'P' mode image
            img.putpalette(bytearray(palette), rawmode='RGBA')

        # Serialize resulting PIL Image to PNG bytes
        out_buffer = BytesIO()
        img.save(out_buffer, format="PNG")
        return out_buffer.getvalue()

def preprocess_embed(bytes_data: bytes, filename: str) -> bytes:
    if not filename.lower().endswith('.png'):
        return bytes_data

    filename_lower = filename.lower()

    if 'rgba16' in filename_lower:
        tex_type = TextureType.RGBA16bpp
    elif 'rgba32' in filename_lower:
        tex_type = TextureType.RGBA32bpp
    elif 'ci4' in filename_lower:
        tex_type = TextureType.Palette4bpp
    elif 'ci8' in filename_lower:
        tex_type = TextureType.Palette8bpp
    else:
        raise ValueError(f"Unknown texture format in filename: {filename}")

    try:
        img = Image.open(BytesIO(bytes_data))
    except Exception as e:
        raise ValueError(f"Failed to read PNG data for {filename}: {e}")

    texture = Texture(
        width=img.width,
        height=img.height,
        texture_type=tex_type
    )

    N64Graphics.convert_raw_to_n64(texture, img)
    return bytes(texture.tex_data)

def amalgamate(file_path, include_dirs, processed, out_file):
    abs_path = os.path.abspath(file_path)

    # Inclusion guard: prevents circular dependencies AND acts as a global #pragma once
    if abs_path in processed:
        return
    processed.add(abs_path)

    try:
        with open(abs_path, 'r', encoding='utf-8') as f:
            for line in f:
                embed = re.match(r'^\s*#\s*embed\s+([<"])([^>"]+)[>"]', line)
                if embed:
                    delimiter = embed.group(1)
                    target_name = embed.group(2)
                    target_path = None

                    search_paths = list(include_dirs)
                    if delimiter == '"':
                        local_dir = os.path.dirname(abs_path)
                        search_paths.insert(0, local_dir)

                    for path in search_paths:
                        potential_path = os.path.join(path, target_name)
                        if os.path.exists(potential_path):
                            target_path = potential_path
                            break

                    if target_path:
                        out_file.write(f"/* --- Start Embedded Bytes: {target_name} --- */\n")
                        try:
                            with open(target_path, 'rb') as bin_file:
                                raw_bytes = bin_file.read()

                            # Preprocess the embedded bytes based on the file type
                            raw_bytes = preprocess_embed(raw_bytes, target_name)

                            # Convert bytes to C hex format (e.g., "0xff")
                            hex_array = [f"0x{b:02x}" for b in raw_bytes]
                            # Write in chunks of 16 bytes per line for readability
                            for i in range(0, len(hex_array), 16):
                                chunk = hex_array[i:i+16]
                                # Add a trailing comma to all lines, C allows trailing commas in arrays
                                out_file.write("    " + ", ".join(chunk) + ",\n")
                        except Exception as e:
                            print(f"Warning: Failed to read embedded file {target_path}: {e}")
                            out_file.write(f"/* ERROR: Failed to read {target_name} */\n")
                        out_file.write(f"/* --- End Embedded Bytes: {target_name} --- */\n")
                        continue
                    else:
                        print(f"Warning: Could not find embedded file {target_name} requested from {abs_path}")
                        out_file.write(f"/* ERROR: Could not find {target_name} */\n")
                include = re.match(r'^\s*#\s*include\s+([<"])([^>"]+)[>"]', line)
                if include:
                    delimiter = include.group(1)
                    header_name = include.group(2)
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