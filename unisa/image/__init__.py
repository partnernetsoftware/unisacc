"""ELF64 / Mach-O 64 / PE32+ wrappers. [I]

Structurally real headers, byte-reproducible (no timestamps, no paths, no build
IDs). [D-5]  We wrap but never execve -- the fold interprets. [X-3]
"""
from . import elf, macho, pe
from ..bits import MASK64

WRITER = {"lnx": elf.write, "osx": macho.write, "win": pe.write}
MAGIC = {"lnx": "7f454c46", "osx": "cffaedfe", "win": "4d5a"}
# headers are fixed-size per format, so the load address of the text and of the
# data that follows it is known BEFORE the code is encoded -- which is what lets
# `.lea` emit a real address instead of a placeholder.
HDRS = {"lnx": elf.HDRS, "osx": macho.HDRS, "win": pe.HDRS}
BASE = {"lnx": elf.VADDR, "osx": macho.VMADDR, "win": pe.IMAGEBASE}


def layout(os_, arch, textlen, dynamic=False):
    """-> (text_vaddr, data_vaddr).  Mach-O puts the data in its own rw
    segment, so it starts on the next page, not straight after the text."""
    h = 784 if os_ == "lnx" and dynamic else HDRS[os_](arch)
    t = BASE[os_] + h
    if os_ == "osx":
        return t, BASE[os_] + macho._round(h + textlen)
    if os_ == "lnx":
        # the data is written to, so it needs its own rw PT_LOAD -- which means
        # its own page, not the bytes straight after the text [I-12]
        return t, BASE[os_] + elf._round(h + textlen) + (32 if dynamic else 0)
    if os_ == "win":
        # .text / .rdata (the import table) / .data, each on its own page:
        # a section RVA must be a multiple of SectionAlignment [I-16]
        return t, BASE[os_] + pe.data_rva(textlen)
    return t, t + textlen


def imports(os_, arch, textlen):
    """`__imp_<name>` -> the absolute address of its IAT slot.  Only Windows
    has any: on Linux and macOS we talk to the kernel directly."""
    return pe.imports(arch, textlen) if os_ == "win" else {}


def relocate(tp, data, shift):
    """Apply the data-segment shift to absolute addresses stored IN the data
    (global pointers initialised with string literals)."""
    relocs = getattr(tp, "relocs", [])
    if not relocs:
        return data
    # R3: copy only the stored prefix; the zero tail stays unresident
    out = bytearray(len(data))
    k = len(data.rstrip(b"\x00"))
    out[:k] = memoryview(data)[:k]
    for at in relocs:
        v = int.from_bytes(out[at:at + 8], "little")
        out[at:at + 8] = ((v + shift) & MASK64).to_bytes(8, "little")
    return out


def build(tp, text, data, entry, stub=b""):
    # Only the data up to its last nonzero byte is stored; the loader zero-
    # fills the rest of the `full` length.  lower.zero_last put the zeros last.
    full = len(data)
    data = bytes(data.rstrip(b"\x00"))   # R3: rstrip first, so only the stored prefix is copied
    if tp.os == "win":
        return pe.write(tp.arch, text, data, entry,
                        relocs=getattr(tp, "relocs", ()),
                        bss=getattr(tp, "bss", 0), full=full, stub=stub)
    dynamic = tp.os == "lnx" and any(i.op in ("hostcall", "hostaddr") for i in tp.code)
    if tp.os == "lnx":
        return elf.write(tp.arch, text, data, entry, full=full, dynamic=dynamic)
    return WRITER[tp.os](tp.arch, text, data, entry, full=full)
