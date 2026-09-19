"""ELF64, one PT_LOAD at 0x400000. [I-1]"""
import struct

VADDR = 0x400000
EHDR, PHDR = 64, 56
MACHINE = {"x86_64": 0x3E, "arm64": 0xB7}


def HDRS(arch):
    return EHDR + PHDR


def write(arch, text, data, entry):
    hdrs = EHDR + PHDR
    total = hdrs + len(text) + len(data)
    e = bytearray()
    e += b"\x7fELF" + bytes([2, 1, 1, 0]) + b"\x00" * 8      # e_ident
    e += struct.pack("<HHI", 2, MACHINE[arch], 1)            # type exec, mach, ver
    e += struct.pack("<QQQ", VADDR + hdrs + entry, EHDR, 0)  # entry, phoff, shoff
    e += struct.pack("<IHHHHHH", 0, EHDR, PHDR, 1, 0, 0, 0)
    e += struct.pack("<IIQQQQQQ", 1, 5, 0, VADDR, VADDR,
                     total, total, 0x1000)                   # PT_LOAD r-x
    assert len(e) == hdrs, len(e)
    return bytes(e) + text + data
