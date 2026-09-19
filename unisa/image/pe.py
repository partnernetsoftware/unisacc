"""PE32+: MZ stub + PE\\0\\0 + optional header 0x20b + one .text. [I-3]

TimeDateStamp is pinned to 0 -- a build stamp would break [D-5].
"""
import struct

IMAGEBASE = 0x140000000
MACHINE = {"x86_64": 0x8664, "arm64": 0xAA64}
SECT_ALIGN, FILE_ALIGN = 0x1000, 0x200


def _round(v, a):
    return (v + a - 1) // a * a


def HDRS(arch):
    return _round(64 + 4 + 20 + 240 + 40, FILE_ALIGN)


def write(arch, text, data, entry):
    text = text + data
    dos = bytearray(64)
    dos[0:2] = b"MZ"
    struct.pack_into("<I", dos, 0x3C, 64)
    opt = 240
    hdrs = 64 + 4 + 20 + opt + 40
    raw = _round(hdrs, FILE_ALIGN)
    tsize = _round(len(text), FILE_ALIGN)
    p = bytearray(dos)
    p += b"PE\x00\x00"
    p += struct.pack("<HHIIIHH", MACHINE[arch], 1, 0, 0, 0, opt, 0x22)
    o = bytearray()
    # PE32+ standard fields (24B): no BaseOfData -- that field is PE32 only
    o += struct.pack("<HBBIIIII", 0x20B, 14, 0, tsize, 0, 0, raw + entry, raw)
    o += struct.pack("<QIIHHHHHHIIIIHH", IMAGEBASE, SECT_ALIGN, FILE_ALIGN,
                     6, 0, 0, 0, 6, 0, 0,
                     _round(raw + tsize, SECT_ALIGN), raw, 0, 3, 0)
    o += struct.pack("<QQQQII", 0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    o += b"\x00" * (16 * 8)
    o = o[:opt].ljust(opt, b"\x00")
    p += o
    p += struct.pack("<8sIIIIIIHHI", b".text", len(text), raw, tsize, raw,
                     0, 0, 0, 0, 0x60000020)
    p = p.ljust(raw, b"\x00")
    return bytes(p) + text.ljust(tsize, b"\x00")
