"""ELF64: r-x text + rw data, two PT_LOADs. [I-1] [I-12]

One PT_LOAD with PF_R|PF_X was enough for the interpreter and enough for the
`readelf` check, and it segfaulted on the first instruction of real Linux --
the scratch cells and the print buffer live in `data`, so the program writes to
its own image.  Mach-O had already forced this lesson (I-8); ELF hid it until
CI ran the file on a real kernel.
"""
import struct

VADDR = 0x400000
PAGE = 0x1000
EHDR, PHDR = 64, 56
NPH = 2
MACHINE = {"x86_64": 0x3E, "arm64": 0xB7}


def _round(v, a=PAGE):
    return (v + a - 1) // a * a


def HDRS(arch):
    return EHDR + PHDR * NPH


def write(arch, text, data, entry, full=None):
    full = len(data) if full is None else full   # p_memsz; the rest is bss
    hdrs = HDRS(arch)
    tend = hdrs + len(text)
    doff = _round(tend)                 # p_offset == p_vaddr (mod PAGE)
    e = bytearray()
    e += b"\x7fELF" + bytes([2, 1, 1, 0]) + b"\x00" * 8      # e_ident
    e += struct.pack("<HHI", 2, MACHINE[arch], 1)            # type exec, mach, ver
    e += struct.pack("<QQQ", VADDR + hdrs + entry, EHDR, 0)  # entry, phoff, shoff
    e += struct.pack("<IHHHHHH", 0, EHDR, PHDR, NPH, 0, 0, 0)
    e += struct.pack("<IIQQQQQQ", 1, 5, 0, VADDR, VADDR,
                     tend, tend, PAGE)                       # PT_LOAD r-x
    e += struct.pack("<IIQQQQQQ", 1, 6, doff, VADDR + doff, VADDR + doff,
                     len(data), full, PAGE)                  # PT_LOAD rw-
    assert len(e) == hdrs, len(e)
    return bytes(e) + text + b"\x00" * (doff - tend) + data
