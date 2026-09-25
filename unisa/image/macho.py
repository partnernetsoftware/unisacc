"""Mach-O 64. [I-2]

Modern macOS (and arm64 in particular) will not load a bare
header+LC_SEGMENT+LC_UNIXTHREAD file: it wants __PAGEZERO, LC_MAIN and
LC_BUILD_VERSION, and it enforces page alignment.  It also enforces segment
protections, so the data -- which holds our scratch cells and therefore gets
written -- has to live in a separate rw __DATA segment, not inside r-x __TEXT.
"""
from ..bits import round_up
import struct

VMADDR = 0x100000000
PAGE = 0x4000
PAGE4 = 0x1000       # the code-signing page, which is 4 KB whatever PAGE is
CPU = {"x86_64": (0x01000007, 3), "arm64": (0x0100000C, 0)}
LC_SEGMENT_64, LC_BUILD_VERSION = 0x19, 0x32
LC_LOAD_DYLINKER, LC_LOAD_DYLIB, LC_MAIN = 0xE, 0xC, 0x80000028
LC_SYMTAB, LC_DYSYMTAB, LC_DYLD_INFO_ONLY = 0x2, 0xB, 0x80000022
STRTAB = 8            # a string table is never empty: it starts with NULs
DYLD = b"/usr/lib/dyld"
LIBSYS = b"/usr/lib/libSystem.B.dylib"
SEG, SECT = 72, 80
# Apple Silicon does not support static executables at all: every arm64 macOS
# binary must be brought up by dyld.  So the image carries LC_LOAD_DYLINKER +
# LC_LOAD_DYLIB and enters through LC_MAIN, even though the code itself imports
# nothing and talks to the kernel with raw `svc`.  __LINKEDIT exists because
# codesign needs somewhere to put the signature.
# dyld decides how to rebase a PIE image by looking at the load commands.  With
# no LC_DYLD_INFO and no chained fixups it takes the LEGACY relocation path,
# dereferences the LC_DYSYMTAB we never emitted, and segfaults INSIDE DYLD
# before our first instruction -- `dyld3::MachOAnalyzer::forEachRebase_
# Relocations`, EXC_BAD_ACCESS at 0x48, which is `locreloff` read off a null
# dysymtab.  Darwin 25 tolerates the omission; Darwin 23 and 24 do not.  So we
# emit all three, empty: dyld then takes the opcode path and finds nothing to
# do.  [I-15]
NCMDS = 12  # PAGEZERO TEXT DATA LINKEDIT DYLINKER DYLIB MAIN BUILD ... SIG
LC_CODE_SIGNATURE = 0x1D
# An arm64 image the kernel will run must be SIGNED -- an unsigned one is
# killed on sight.  Ad-hoc means no certificate and no CMS: a CodeDirectory
# whose SHA-256 hashes cover every page of the file, which is exactly what
# `codesign -s -` writes.  Emitting it here is what lets an image we produced
# run on Apple silicon with no other tool in the loop. [I-19]
CS_MAGIC_EMBEDDED = 0xFADE0CC0
CS_MAGIC_CODEDIRECTORY = 0xFADE0C02
CS_ADHOC = 0x00000002
CS_EXECSEG_MAIN_BINARY = 0x1
IDENT = b"unisa\x00"        # fixed, so two runs give the same bytes
            # + DYLD_INFO_ONLY SYMTAB DYSYMTAB


def _round(v, a=PAGE):
    return round_up(v, a)


def _pad4(b):
    return b + b"\x00" * ((-len(b)) % 8)


def _dylinker():
    # cmdsize must be a multiple of 8 on a 64-bit image: 12 + 20, not 12 + 16
    # (the kernel let 28 through; objdump and otool's parser did not)
    body = DYLD + b"\x00" * (20 - len(DYLD))
    return struct.pack("<III", LC_LOAD_DYLINKER, 12 + len(body), 12) + body


def _dylib():
    body = _pad4(LIBSYS + b"\x00")
    return struct.pack("<IIIIII", LC_LOAD_DYLIB, 24 + len(body), 24,
                       0, 0x10000, 0x10000) + body


# codesign APPENDS LC_CODE_SIGNATURE to the load commands.  With the header
# region packed exactly full it overwrites the first bytes of __text, which
# disassembles as `udf` and dies with SIGILL.  Real linkers leave slack here.
SLACK = 156          # 256, less __bss's 80, the dylinker's 4 and the code
                     # signature command's 16: HDRS holds


def _cmdsz(arch):
    return (SEG + (SEG + SECT) + (SEG + 2 * SECT) + SEG + len(_dylinker()) + len(_dylib())
            + 24 + 24 + 48 + 24 + 80 + 16)


def _cd_len():
    """CodeDirectory without its hashes: the fixed part plus the identifier."""
    return 88 + len(IDENT)


def _sig_len(code_limit):
    slots = (code_limit + PAGE4 - 1) // PAGE4
    return 12 + 8 + _cd_len() + 32 * slots      # SuperBlob + one index + CD


def _signature(image, code_limit, exec_limit):
    """The ad-hoc SuperBlob for `image[:code_limit]`."""
    import hashlib
    slots = (code_limit + PAGE4 - 1) // PAGE4
    cd = struct.pack(">IIIIIIIIIBBBBI", CS_MAGIC_CODEDIRECTORY,
                     _cd_len() + 32 * slots,
                     0x20400,                    # version
                     CS_ADHOC,                   # flags
                     _cd_len(),                  # hashOffset
                     88,                         # identOffset: after the
                                                 # fixed part of version 0x20400
                     0,                          # nSpecialSlots
                     slots, code_limit,
                     32,                         # hashSize
                     2,                          # hashType: SHA-256
                     0, 12,                      # platform, log2(pageSize)
                     0)                          # spare2
    cd += struct.pack(">IIIQQQQ", 0, 0, 0, 0,    # scatter, team, spare3, cl64
                      0, exec_limit, CS_EXECSEG_MAIN_BINARY)
    assert len(cd) == 88, len(cd)                # the fixed part
    cd += IDENT
    assert len(cd) == _cd_len(), (len(cd), _cd_len())
    for i in range(slots):
        page = bytes(image[i * PAGE4:(i + 1) * PAGE4])
        cd += hashlib.sha256(page).digest()
    blob = struct.pack(">III", CS_MAGIC_EMBEDDED, 12 + 8 + len(cd), 1)
    blob += struct.pack(">II", 0, 20)            # slot 0 (CodeDirectory), off
    return blob + cd


def HDRS(arch):
    return 32 + _cmdsz(arch) + SLACK


def _seg(name, vmaddr, vmsize, fileoff, filesize, maxp, initp, nsects):
    return struct.pack("<II16sQQQQiiII", LC_SEGMENT_64,
                       SEG + SECT * nsects, name, vmaddr, vmsize,
                       fileoff, filesize, maxp, initp, nsects, 0)


def _sect(sect, seg, addr, size, off, flags):
    return struct.pack("<16s16sQQIIIIIIII", sect, seg, addr, size, off,
                       2, 0, 0, flags, 0, 0, 0)


def write(arch, text, data, entry, full=None):
    full = len(data) if full is None else full
    cpu, sub = CPU[arch]
    hdrs = HDRS(arch)
    textsz = _round(hdrs + len(text))
    datavm = _round(max(1, full))           # mapped
    datasz = _round(len(data))              # stored; __bss zero-fills the rest
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
    m += _seg(b"__DATA", VMADDR + textsz, datavm, textsz, datasz, 3, 3, 2)
    m += _sect(b"__data", b"__DATA", VMADDR + textsz, len(data), textsz, 0)
    m += _sect(b"__bss", b"__DATA", VMADDR + textsz + len(data),
               full - len(data), 0, 1)                   # S_ZEROFILL
    # __LINKEDIT holds the string table and then the signature
    sigoff = (link + STRTAB + 15) // 16 * 16
    siglen = _sig_len(sigoff)
    linksz = sigoff - link + siglen
    m += _seg(b"__LINKEDIT", VMADDR + textsz + datavm, _round(linksz), link,
              linksz, 1, 1, 0)
    m += _dylinker()
    m += _dylib()
    m += struct.pack("<IIQQ", LC_MAIN, 24, hdrs + entry, 0)
    m += struct.pack("<IIIIII", LC_BUILD_VERSION, 24, 1, 13 << 16,
                     13 << 16, 0)
    # empty, but present -- see the note on NCMDS  [I-15]
    m += struct.pack("<II" + "I" * 10, LC_DYLD_INFO_ONLY, 48, *([0] * 10))
    m += struct.pack("<IIIIII", LC_SYMTAB, 24, link, 0, link, STRTAB)
    m += struct.pack("<II" + "I" * 18, LC_DYSYMTAB, 80, *([0] * 18))
    m += struct.pack("<IIII", LC_CODE_SIGNATURE, 16, sigoff, siglen)
    assert len(m) == hdrs - SLACK, (len(m), hdrs)
    m += b"\x00" * SLACK
    out = bytearray(m)
    out += text
    out += b"\x00" * (textsz - len(out))
    out += data
    out += b"\x00" * (link - len(out))
    out += b"\x00" * STRTAB                 # the string table itself
    out += b"\x00" * (sigoff - len(out))
    # the hashes cover everything written so far, this image's own header
    # included -- which is why the signature is last and its own bytes are not
    out += _signature(out, sigoff, textsz)
    return bytes(out)
