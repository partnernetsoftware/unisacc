"""Mach-O 64. [I-2]

Modern macOS (and arm64 in particular) will not load a bare
header+LC_SEGMENT+LC_UNIXTHREAD file: it wants __PAGEZERO, LC_MAIN and
LC_BUILD_VERSION, and it enforces page alignment.  It also enforces segment
protections, so the data -- which holds our scratch cells and therefore gets
written -- has to live in a separate rw __DATA segment, not inside r-x __TEXT.
"""
import struct

VMADDR = 0x100000000
PAGE = 0x4000
CPU = {"x86_64": (0x01000007, 3), "arm64": (0x0100000C, 0)}
LC_SEGMENT_64, LC_BUILD_VERSION = 0x19, 0x32
LC_LOAD_DYLINKER, LC_LOAD_DYLIB, LC_MAIN = 0xE, 0xC, 0x80000028
DYLD = b"/usr/lib/dyld"
LIBSYS = b"/usr/lib/libSystem.B.dylib"
SEG, SECT = 72, 80
# Apple Silicon does not support static executables at all: every arm64 macOS
# binary must be brought up by dyld.  So the image carries LC_LOAD_DYLINKER +
# LC_LOAD_DYLIB and enters through LC_MAIN, even though the code itself imports
# nothing and talks to the kernel with raw `svc`.  __LINKEDIT exists because
# codesign needs somewhere to put the signature.
NCMDS = 8   # PAGEZERO TEXT DATA LINKEDIT DYLINKER DYLIB MAIN + BUILD = 8


def _round(v, a=PAGE):
    return (v + a - 1) // a * a


def _pad4(b):
    return b + b"\x00" * ((-len(b)) % 8)


def _dylinker():
    body = _pad4(DYLD + b"\x00")
    return struct.pack("<III", LC_LOAD_DYLINKER, 12 + len(body), 12) + body


def _dylib():
    body = _pad4(LIBSYS + b"\x00")
    return struct.pack("<IIIIII", LC_LOAD_DYLIB, 24 + len(body), 24,
                       0, 0x10000, 0x10000) + body


# codesign APPENDS LC_CODE_SIGNATURE to the load commands.  With the header
# region packed exactly full it overwrites the first bytes of __text, which
# disassembles as `udf` and dies with SIGILL.  Real linkers leave slack here.
SLACK = 256


def _cmdsz(arch):
    return (SEG + (SEG + SECT) * 2 + SEG + len(_dylinker()) + len(_dylib())
            + 24 + 24)


def HDRS(arch):
    return 32 + _cmdsz(arch) + SLACK


def _seg(name, vmaddr, vmsize, fileoff, filesize, maxp, initp, nsects):
    return struct.pack("<II16sQQQQiiII", LC_SEGMENT_64,
                       SEG + SECT * nsects, name, vmaddr, vmsize,
                       fileoff, filesize, maxp, initp, nsects, 0)


def _sect(sect, seg, addr, size, off, flags):
    return struct.pack("<16s16sQQIIIIIIII", sect, seg, addr, size, off,
                       2, 0, 0, flags, 0, 0, 0)


def write(arch, text, data, entry):
    cpu, sub = CPU[arch]
    hdrs = HDRS(arch)
    textsz = _round(hdrs + len(text))
    datasz = _round(max(1, len(data)))
    link = textsz + datasz
    m = bytearray()
    m += struct.pack("<IiiIIIII", 0xFEEDFACF, cpu, sub, 2, NCMDS,
                     # arm64 macOS REQUIRES MH_PIE; the code is position independent
                     # (adrp+add / rip-relative), so sliding is fine
                     _cmdsz(arch), 0x200085, 0)
    m += _seg(b"__PAGEZERO", 0, VMADDR, 0, 0, 0, 0, 0)
    m += _seg(b"__TEXT", VMADDR, textsz, 0, textsz, 5, 5, 1)
    m += _sect(b"__text", b"__TEXT", VMADDR + hdrs, len(text), hdrs,
               0x80000400)
    m += _seg(b"__DATA", VMADDR + textsz, datasz, textsz, datasz, 3, 3, 1)
    m += _sect(b"__data", b"__DATA", VMADDR + textsz, len(data), textsz, 0)
    m += _seg(b"__LINKEDIT", VMADDR + link, PAGE, link, 0, 1, 1, 0)
    m += _dylinker()
    m += _dylib()
    m += struct.pack("<IIQQ", LC_MAIN, 24, hdrs + entry, 0)
    m += struct.pack("<IIIIII", LC_BUILD_VERSION, 24, 1, 13 << 16,
                     13 << 16, 0)
    assert len(m) == hdrs - SLACK, (len(m), hdrs)
    m += b"\x00" * SLACK
    out = bytearray(m)
    out += text
    out += b"\x00" * (textsz - len(out))
    out += data
    out += b"\x00" * (link - len(out))
    return bytes(out)
