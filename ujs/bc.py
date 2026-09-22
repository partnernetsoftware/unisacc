"""Full UJS bytecode ISA — one encoding for Python ref VM and WASM. [LW-3]

Stack holds i32 *handles* into a heap (0 = null sentinel handle).
Immediate-ish values still live on the heap for uniformity in WASM.
"""
from __future__ import annotations

# Keep opcode numbers stable — WASM and demos hard-code them.
OP = {
    "nop": 0,
    "const_null": 1,
    "const_bool": 2,      # u8
    "const_i64": 3,       # i64
    "const_f64": 4,       # f64 bits
    "const_str": 5,       # u32 strtab index
    "const_fn": 6,        # u32 fntab index
    "load_l": 7,          # u8 slot
    "store_l": 8,
    "load_g": 9,          # u8 gslot
    "store_g": 10,
    "drop": 11,
    "add": 12,
    "sub": 13,
    "mul": 14,
    "div": 15,
    "mod": 16,
    "lt": 17,
    "le": 18,
    "gt": 19,
    "ge": 20,
    "eq": 21,
    "ne": 22,
    "and": 23,
    "or": 24,
    "not": 25,
    "idx": 26,
    "setidx": 27,
    "dot": 28,            # u32 strtab key
    "len": 29,
    "keys": 30,
    "in": 31,
    "mklist": 32,         # u8 count
    "mkdict": 33,         # u8 count
    "mktup": 34,          # u8 count
    "jump": 35,           # u32 abs byte offset
    "jumpz": 36,
    "jumpnz": 37,
    "call": 38,           # u8 argc
    "ret": 39,
    "spread": 40,
    "restpack": 41,       # u8 n
    "typeof": 42,
    "shapeof": 43,
    "switch": 44,         # complex: u16 ncases, then (keytag,key,off)* , default off
    "ic_enter": 45,       # u8 opid (ignored by exec)
    "print": 46,
}

TAG = {
    "null": 0,
    "bool": 1,
    "i64": 2,
    "f64": 3,
    "str": 4,
    "list": 5,
    "dict": 6,
    "fn": 7,
    "tup": 8,
}

OP_NAME = {v: k for k, v in OP.items()}
TAG_NAME = {v: k for k, v in TAG.items()}
