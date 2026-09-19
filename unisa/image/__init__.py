"""ELF64 / Mach-O 64 / PE32+ wrappers. [I]

Structurally real headers, byte-reproducible (no timestamps, no paths, no build
IDs). [D-5]  We wrap but never execve -- the fold interprets. [X-3]
"""
from . import elf, macho, pe

WRITER = {"lnx": elf.write, "osx": macho.write, "win": pe.write}
MAGIC = {"lnx": "7f454c46", "osx": "cffaedfe", "win": "4d5a"}
# headers are fixed-size per format, so the load address of the text and of the
# data that follows it is known BEFORE the code is encoded -- which is what lets
# `.lea` emit a real address instead of a placeholder.
HDRS = {"lnx": elf.HDRS, "osx": macho.HDRS, "win": pe.HDRS}
BASE = {"lnx": elf.VADDR, "osx": macho.VMADDR, "win": pe.IMAGEBASE}


def layout(os_, arch, textlen):
    """-> (text_vaddr, data_vaddr).  Mach-O puts the data in its own rw
    segment, so it starts on the next page, not straight after the text."""
    h = HDRS[os_](arch)
    t = BASE[os_] + h
    if os_ == "osx":
        return t, BASE[os_] + macho._round(h + textlen)
    if os_ == "lnx":
        # the data is written to, so it needs its own rw PT_LOAD -- which means
        # its own page, not the bytes straight after the text [I-12]
        return t, BASE[os_] + elf._round(h + textlen)
    return t, t + textlen


def relocate(tp, data, shift):
    """Apply the data-segment shift to absolute addresses stored IN the data
    (global pointers initialised with string literals)."""
    out = bytearray(data)
    for at in getattr(tp, "relocs", []):
        v = int.from_bytes(out[at:at + 8], "little")
        out[at:at + 8] = ((v + shift) & ((1 << 64) - 1)).to_bytes(8, "little")
    return bytes(out)


def build(tp, text, data, entry):
    return WRITER[tp.os](tp.arch, text, data, entry)
