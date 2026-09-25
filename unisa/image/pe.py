"""PE32+: MZ stub, PE header, .text / .rdata / .data, and a real import table.
[I-3] [I-16]

The first version put text and data in one r-x section whose RVA was not a
multiple of SectionAlignment, and imported nothing -- Windows refused to load
it at all (`Access is denied`, exit 5).  A PE that is only structurally
plausible is not a program: the loader checks the alignment, and a program
that cannot call kernel32 cannot write to a handle it does not have.

TimeDateStamp is pinned to 0 -- a build stamp would break [D-5].
"""
from ..bits import round_up
import struct

IMAGEBASE = 0x140000000
MACHINE = {"x86_64": 0x8664, "arm64": 0xAA64}
SECT_ALIGN, FILE_ALIGN = 0x1000, 0x200
TEXT_RVA = 0x1000
HDR_FILE = 0x400            # file offset of the first section

DLL = b"KERNEL32.dll"
# Fixed order: the IAT slot for a name is its index, and lowering resolves
# `__imp_<name>` to that slot's address.
IMPORTS = ("GetStdHandle", "WriteFile", "ReadFile", "CloseHandle",
           "CreateFileA", "ExitProcess", "GetCommandLineA", "VirtualAlloc",
           "VirtualProtect", "VirtualFree", "FlushInstructionCache",
           "SetFilePointer", "DeleteFileA", "MoveFileExA")


_round = round_up


def HDRS(arch):
    """Where the text starts, relative to the image base."""
    return TEXT_RVA


# arm64 Windows also insists on a load-config directory.  Strip directory 10
# from a real arm64 binary and it stops loading, exactly as stripping the base
# relocations does; an all-zero structure with its Size filled in is enough
# when CFG is off.  [I-17]
LOADCFG = 0x140
COOKIE_FIELD = 0x58     # IMAGE_LOAD_CONFIG_DIRECTORY64.SecurityCookie


def _idata(rva, cookie=0):
    """The import section, laid out at `rva`.  Returns (bytes, iat_offset)."""
    n = len(IMPORTS)
    desc = 20 * 2                       # one descriptor + the null terminator
    int_off = desc                      # import name table
    iat_off = int_off + (n + 1) * 8     # import address table
    nm_off = iat_off + (n + 1) * 8      # hint/name entries
    names, off = bytearray(), nm_off
    hint_rva = []
    for f in IMPORTS:
        hint_rva.append(rva + off)
        e = struct.pack("<H", 0) + f.encode() + b"\x00"
        if len(e) % 2:
            e += b"\x00"
        names += e
        off += len(e)
    dll_rva = rva + off
    names += DLL + b"\x00"
    off += len(DLL) + 1
    off = (off + 7) // 8 * 8
    cfg_off = off
    off += LOADCFG

    b = bytearray()
    b += struct.pack("<IIIII", rva + int_off, 0, 0, dll_rva, rva + iat_off)
    b += b"\x00" * 20                                   # null descriptor
    for t in (int_off, iat_off):                        # INT and IAT: same
        assert len(b) == t
        for h in hint_rva:
            b += struct.pack("<Q", h)
        b += b"\x00" * 8
    assert len(b) == nm_off
    b += names
    b += b"\x00" * (cfg_off - len(b))
    cfg = bytearray(LOADCFG)
    struct.pack_into("<I", cfg, 0, LOADCFG)
    if cookie:
        # The loader writes the stack cookie through this pointer, and refuses
        # the image when it is null -- zero just this field in a working arm64
        # binary and it stops loading.  [I-17]
        struct.pack_into("<Q", cfg, COOKIE_FIELD, cookie)
    b += bytes(cfg)
    return bytes(b), iat_off, cfg_off


IDATA_LEN = len(_idata(0)[0])
IAT_OFF = _idata(0)[1]
CFG_OFF = _idata(0)[2]


def _reloc(rvas):
    """A .reloc section, from absolute RVAs.

    ARM64 Windows will not load an image that is not genuinely relocatable.
    Established by taking a real arm64 binary apart one field at a time:
    removing the base-relocation directory breaks it, and so does replacing
    its entries with ABSOLUTE padding while leaving the directory in place.
    Same physical fact as I-7 (arm64 macOS requires MH_PIE), enforced harder.
    [I-17]"""
    b = bytearray()
    ents = sorted(set(rvas))
    i = 0
    while i < len(ents):
        page = ents[i] // 0x1000 * 0x1000
        grp = []
        while i < len(ents) and ents[i] - page < 0x1000:
            grp.append(ents[i] - page)
            i += 1
        words = [(10 << 12) | off for off in grp]       # DIR64
        if len(words) % 2:
            words.append(0)                             # ABSOLUTE, padding
        b += struct.pack("<II", page, 8 + 2 * len(words))
        for w in words:
            b += struct.pack("<H", w)
    return bytes(b)


def _rvas(textlen):
    """-> (rdata_rva, data_rva)"""
    r = TEXT_RVA + _round(textlen, SECT_ALIGN)
    return r, r + _round(IDATA_LEN, SECT_ALIGN)


def data_rva(textlen):
    return _rvas(textlen)[1]


def imports(arch, textlen):
    """`__imp_<name>` -> the absolute address of its IAT slot."""
    r, _ = _rvas(textlen)
    base = IMAGEBASE + r + IAT_OFF
    return {"__imp_" + f: base + 8 * i for i, f in enumerate(IMPORTS)}


def write(arch, text, data, entry, relocs=(), bss=0, full=None, stub=b""):
    """`stub` goes between the DOS header and the PE header, where the DOS
    stub used to sit.  An APE file puts its shell script there: the same
    bytes are a PE for Windows and a script for a Unix shell. [S-10]"""
    full = len(data) if full is None else full
    global HDR_FILE, TEXT_RVA
    hdr_file, text_rva = HDR_FILE, TEXT_RVA
    if stub:
        hdr_file = max(HDR_FILE, _round(488 + len(stub), FILE_ALIGN))
        text_rva = max(TEXT_RVA, _round(hdr_file, SECT_ALIGN))
    HDR_FILE, TEXT_RVA = hdr_file, text_rva
    try:
        return _write(arch, text, data, entry, relocs, bss, full, stub)
    finally:
        HDR_FILE, TEXT_RVA = 0x400, 0x1000


def _write(arch, text, data, entry, relocs, bss, full, stub):
    rd_rva, dt_rva = _rvas(len(text))
    # the cookie follows the FULL data; it is a zero word, so it may as well
    # be zero-filled with the rest of the tail instead of stored
    cookie_rva = dt_rva + _round(max(1, full), 8)
    idata, _, _ = _idata(rd_rva, IMAGEBASE + cookie_rva)
    rd_file = HDR_FILE + _round(len(text), FILE_ALIGN)
    dt_file = rd_file + _round(len(idata), FILE_ALIGN)
    dvs = _round(max(1, full), 8) + 8 + bss    # bss costs image, not file
    data = bytes(data).ljust(_round(max(1, len(data)), 8), b"\x00")
    rl_rva = dt_rva + _round(dvs, SECT_ALIGN)
    rl_file = dt_file + _round(len(data), FILE_ALIGN)
    # the cookie pointer in the load config is itself an absolute address, so
    # it is also the one relocation every image of ours carries
    reloc = _reloc([rd_rva + CFG_OFF + COOKIE_FIELD] +
                   [dt_rva + r for r in relocs])
    img_size = rl_rva + _round(len(reloc), SECT_ALIGN)

    dos = bytearray(64)
    dos[0:2] = b"MZ"
    struct.pack_into("<I", dos, 0x3C, 64 + len(stub))
    if stub:
        # the script lives inside the DOS stub area; the fields the loader
        # reads (the magic, and e_lfanew at 0x3C) are quoted text to a shell.
        # The first 62 bytes fill the DOS header itself, so the PE header
        # lands at 64 + what is left -- that is len(stub) + 2.
        dos[2:64] = stub[:62].ljust(62, b"\x00")
        dos[0x3C:0x40] = struct.pack("<I", len(stub) + 2)
        stub = stub[62:]
    opt = 240
    p = bytearray(dos)
    p += stub
    p += b"PE\x00\x00"
    p += struct.pack("<HHIIIHH", MACHINE[arch], 4, 0, 0, 0, opt, 0x22)
    o = bytearray()
    # PE32+ standard fields: no BaseOfData, that field is PE32 only
    o += struct.pack("<HBBIIIII", 0x20B, 14, 0, _round(len(text), FILE_ALIGN),
                     0, 0, TEXT_RVA + entry, TEXT_RVA)
    # Subsystem 4.0.  A higher version puts the loader on its strict path,
    # where DYNAMIC_BASE demands relocations it will actually check; at 4.0 it
    # uses the relaxed rules and a position-independent image with no .reloc
    # loads fine.  Declaring 10.0 was what made every image we emitted fail
    # with STATUS_INVALID_IMAGE_FORMAT.  [I-16]
    o += struct.pack("<QIIHHHHHHIIIIHH", IMAGEBASE, SECT_ALIGN, FILE_ALIGN,
                     4, 0, 0, 0, 4, 0, 0,
                     img_size, HDR_FILE, 0,
                     3,        # IMAGE_SUBSYSTEM_WINDOWS_CUI
                     0x8160)   # HIGH_ENTROPY_VA | DYNAMIC_BASE | NX_COMPAT
                               # | TERMINAL_SERVER_AWARE  [I-17]
    o += struct.pack("<QQQQII", 0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    dirs = [(0, 0)] * 16
    dirs[1] = (rd_rva, 40)                              # import directory
    dirs[5] = (rl_rva, len(reloc))                      # base relocations
    dirs[10] = (rd_rva + CFG_OFF, LOADCFG)              # load config [I-17]
    dirs[12] = (rd_rva + IAT_OFF, (len(IMPORTS) + 1) * 8)   # IAT
    for (a, sz) in dirs:
        o += struct.pack("<II", a, sz)
    o = o[:opt].ljust(opt, b"\x00")
    p += o

    def sect(name, rva, vsize, foff, fsize, flags):
        return struct.pack("<8sIIIIIIHHI", name, vsize, rva,
                           _round(fsize, FILE_ALIGN), foff, 0, 0, 0, 0, flags)

    p += sect(b".text", TEXT_RVA, len(text), HDR_FILE, len(text), 0x60000020)
    p += sect(b".rdata", rd_rva, len(idata), rd_file, len(idata), 0x40000040)
    p += sect(b".data", dt_rva, dvs, dt_file, len(data), 0xC0000040)
    p += sect(b".reloc", rl_rva, len(reloc), rl_file, len(reloc), 0x42000040)
    p = p.ljust(HDR_FILE, b"\x00")
    out = bytearray(p)
    out += text.ljust(_round(len(text), FILE_ALIGN), b"\x00")
    out += idata.ljust(_round(len(idata), FILE_ALIGN), b"\x00")
    out += data.ljust(_round(len(data), FILE_ALIGN), b"\x00")
    out += reloc.ljust(_round(len(reloc), FILE_ALIGN), b"\x00")
    return bytes(out)
