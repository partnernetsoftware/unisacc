"""ELF64: r-x text + rw data, two PT_LOADs. [I-1] [I-12]

One PT_LOAD with PF_R|PF_X was enough for the interpreter and enough for the
`readelf` check, and it segfaulted on the first instruction of real Linux --
the scratch cells and the print buffer live in `data`, so the program writes to
its own image.  Mach-O had already forced this lesson (I-8); ELF hid it until
CI ran the file on a real kernel.
"""
from ..bits import round_up
import struct

VADDR = 0x400000
PAGE = 0x1000
EHDR, PHDR = 64, 56
NPH = 2
MACHINE = {"x86_64": 0x3E, "arm64": 0xB7}


def _round(v, a=PAGE):
    return round_up(v, a)


def HDRS(arch):
    return EHDR + PHDR * NPH


def write(arch, text, data, entry, full=None, dynamic=False):
    full = len(data) if full is None else full   # p_memsz; the rest is bss
    if dynamic:
        return _write_dynamic(arch, text, data, entry, full)
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


def _write_dynamic(arch, text, data, entry, full):
    """Linux host bridge: four ld.so GLOB_DAT slots below the logical data base."""
    h = 784
    tend = h + len(text)
    doff = _round(tend)
    slots = VADDR + doff
    interp = b'/lib/ld-linux-aarch64.so.1' if arch == 'arm64' else b'/lib64/ld-linux-x86-64.so.2'
    e = bytearray(b'\x7fELF' + bytes((2, 1, 1, 0)) + bytes(8))
    e += struct.pack('<HHIQQQIHHHHHH', 2, MACHINE[arch], 1,
                     VADDR + h + entry, EHDR, 0, 0, EHDR, PHDR, 4, 0, 0, 0)
    def ph(typ, flags, off, va, size, mem, align):
        e.extend(struct.pack('<IIQQQQQQ', typ, flags, off, va, va, size, mem, align))
    ph(3, 4, 288, VADDR + 288, len(interp) + 1, len(interp) + 1, 1)
    ph(1, 5, 0, VADDR, tend, tend, PAGE)
    ph(1, 6, doff, slots, 32 + len(data), 32 + full, PAGE)
    ph(2, 4, 608, VADDR + 608, 176, 176, 8)
    e += (interp + b'\0').ljust(32, b'\0')
    e += b'\0libc.so.6\0dlopen\0dlsym\0dlclose\0dlerror\0'
    e += bytes(24)
    for offset in (11, 18, 24, 32):
        e += struct.pack('<IBBHQQ', offset, 0x12, 0, 0, 0, 0)
    e += struct.pack('<II', 1, 5) + bytes(24)
    for i in range(4):
        e += struct.pack('<QQQ', slots + 8 * i,
                         ((i + 1) << 32) | (1025 if arch == 'arm64' else 6), 0)
    for tag, value in ((1, 1), (4, VADDR + 480), (5, VADDR + 320),
                       (6, VADDR + 360), (10, 40), (11, 24),
                       (7, VADDR + 512), (8, 96), (9, 24), (30, 8), (0, 0)):
        e += struct.pack('<QQ', tag, value)
    assert len(e) == h
    return bytes(e) + text + bytes(doff - tend) + bytes(32) + data
