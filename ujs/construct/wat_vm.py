"""Full UJS bytecode WASM emitter. [LW-3]"""
from __future__ import annotations
import os, struct, subprocess, tempfile
from .bc import OP, TAG

HEAP, FREEP, LOCALS, STACK, PROG = 65536, 1024, 4096, 6144, 16384

def esc(bs: bytes) -> str:
    return "".join("\\%02x" % b for b in bs)

def pack_program(blob: dict) -> bytes:
    out = bytearray()
    code = blob["code"]
    if isinstance(code, list):
        code = bytes(code)
    out += struct.pack("<I", len(code))
    out += code
    strings = list(blob.get("strings") or [])
    out += struct.pack("<I", len(strings))
    for s in strings:
        b = s.encode("utf-8")
        out += struct.pack("<I", len(b))
        out += b
        out += b"\x00" * ((4 - (len(b) % 4)) % 4)
    fns = list(blob.get("fns") or [])
    out += struct.pack("<I", len(fns))
    for f in fns:
        locs = list(f.get("locals") or [])
        rest = f.get("rest")
        rest_ix = locs.index(rest) if rest in locs else 0xFFFFFFFF
        nested = pack_program(f)
        out += struct.pack("<III", f.get("nparams", 0), rest_ix & 0xFFFFFFFF, len(locs))
        out += nested
    return bytes(out)
