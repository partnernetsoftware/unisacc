"""One file, two instruction sets. [I-19]

A Mach-O universal ("fat") binary is the standard way macOS carries more than
one architecture in one file: a big-endian header of slice descriptors, then
the ordinary images, each page aligned.  The kernel picks the slice.

This is the honest first half of the multi-ISA claim.  It is one OS; a
cosmopolitan-style file that is simultaneously an ELF, a Mach-O and a PE is a
different problem and is not what this emits.
"""
from ..bits import round_up
import struct

FAT_MAGIC = 0xCAFEBABE
CPU = {"x86_64": (0x01000007, 3), "arm64": (0x0100000C, 0)}
ALIGN = {"x86_64": 12, "arm64": 14}       # log2; arm64 pages are 16 KB


_round = round_up


def write(slices):
    """slices: [(arch, image_bytes)], listed in the order given."""
    hdr = 8 + 20 * len(slices)
    descs, body, off = bytearray(), bytearray(), hdr
    for arch, img in slices:
        cpu, sub, al = CPU[arch] + (ALIGN[arch],)
        at = _round(off, 1 << al)
        body += b"\x00" * (at - off) + img
        descs += struct.pack(">IIIII", cpu, sub, at, len(img), al)
        off = at + len(img)
    return struct.pack(">II", FAT_MAGIC, len(slices)) + bytes(descs) + bytes(body)
