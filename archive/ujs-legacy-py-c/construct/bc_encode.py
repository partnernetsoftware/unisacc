"""Encode Fn (jtape) → full UJS bytecode. [LW-3]"""
from __future__ import annotations

import struct

from .bc import OP
from .jtape import Fn, Ins


class EncodeError(Exception):
    pass


def encode_fn(fn: Fn) -> dict:
    """Return {code: bytes, strings: [str], fns: [dict], locals: [str],
    nlocals, nglobals hint}. Nested fns from meta['fns'] are encoded too.
    """
    strings = list(fn.strings)
    str_ix = {s: i for i, s in enumerate(strings)}

    def intern(s: str) -> int:
        if s not in str_ix:
            str_ix[s] = len(strings)
            strings.append(s)
        return str_ix[s]

    # nested functions: name -> Fn
    nested = dict(fn.meta.get("fns") or {})
    fn_names = sorted(nested.keys())
    fn_ix = {n: i for i, n in enumerate(fn_names)}
    encoded_fns = []

    slots = {n: i for i, n in enumerate(fn.localslot)}
    # globals addressed by name intern + runtime map; use gslot table
    globals_used = []
    g_ix = {}

    def gslot(name: str) -> int:
        if name not in g_ix:
            g_ix[name] = len(globals_used)
            globals_used.append(name)
        return g_ix[name]

    def ensure_local(name: str) -> int:
        if name not in slots:
            slots[name] = len(slots)
        return slots[name]

    # Pre-scan to ensure all store/load locals registered
    for ins in fn.code:
        if ins.op in ("load_l", "store_l") and isinstance(ins.a, str):
            ensure_local(ins.a)

    # Rebuild slot list in index order
    inv = [None] * len(slots)
    for n, i in slots.items():
        inv[i] = n
    local_names = inv

    abs_ops = []  # (opname, *args) with jumps as insn indices into fn.code

    for ins in fn.code:
        op = ins.op
        if op == "nop":
            abs_ops.append(("nop",))
        elif op == "ic_enter":
            # ins.a is raw binop token ('+') or IC_OPS name
            from . import catalog as C
            raw = ins.a or "add"
            name = {
                "+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod",
                "<": "lt", "==": "eq",
            }.get(raw, raw)
            if name not in C.IC_OPS:
                name = "add"
            abs_ops.append(("ic_enter", C.IC_OPS.index(name)))
        elif op == "drop":
            abs_ops.append(("drop",))
        elif op == "const":
            kind, val = ins.a, ins.b
            if kind == "null":
                abs_ops.append(("const_null",))
            elif kind == "bool":
                abs_ops.append(("const_bool", 1 if val else 0))
            elif kind == "i64":
                abs_ops.append(("const_i64", int(val)))
            elif kind == "f64":
                abs_ops.append(("const_f64", float(val)))
            elif kind == "str":
                sid = val if isinstance(val, int) else intern(val)
                # if val is index into fn.strings
                if isinstance(val, int):
                    sid = val
                    while len(strings) <= sid:
                        # shouldn't happen
                        strings.append("")
                    # prefer fn.strings
                    if val < len(fn.strings):
                        sid = intern(fn.strings[val])
                else:
                    sid = intern(val)
                abs_ops.append(("const_str", sid))
            elif kind == "fn":
                if val not in fn_ix:
                    raise EncodeError("unknown fn const %r" % val)
                abs_ops.append(("const_fn", fn_ix[val]))
            else:
                raise EncodeError("const %s" % kind)
        elif op == "load_l":
            abs_ops.append(("load_l", ensure_local(ins.a)))
        elif op == "store_l":
            abs_ops.append(("store_l", ensure_local(ins.a)))
        elif op == "load_g":
            abs_ops.append(("load_g", gslot(ins.a)))
        elif op == "store_g":
            abs_ops.append(("store_g", gslot(ins.a)))
        elif op in ("add", "sub", "mul", "div", "mod", "lt", "le", "gt", "ge",
                    "eq", "ne", "and", "or", "not", "idx", "setidx", "len",
                    "keys", "in", "typeof", "shapeof", "spread", "print",
                    "ret"):
            abs_ops.append((op,))
        elif op == "dot":
            abs_ops.append(("dot", intern(ins.a)))
        elif op in ("mklist", "mkdict", "mktup", "call", "restpack"):
            abs_ops.append((op, int(ins.a)))
        elif op in ("jump", "jumpz", "jumpnz"):
            abs_ops.append((op, int(ins.a)))  # code insn index
        elif op == "switch":
            # table dict + default — encode as jump chain not needed;
            # compiler currently uses compare chain, so switch op rare
            raise EncodeError("raw switch op: use compare-chain front")
        else:
            raise EncodeError("op %r" % op)

    # Map code index -> abs index
    code_to_abs = {}
    ai = 0
    for ci, ins in enumerate(fn.code):
        code_to_abs[ci] = ai
        ai += 1
    code_to_abs[len(fn.code)] = ai

    def op_size(item):
        name = item[0]
        if name in ("nop", "const_null", "drop", "add", "sub", "mul", "div",
                    "mod", "lt", "le", "gt", "ge", "eq", "ne", "and", "or",
                    "not", "idx", "setidx", "len", "keys", "in", "typeof",
                    "shapeof", "spread", "print", "ret"):
            return 1
        if name in ("const_bool", "load_l", "store_l", "load_g", "store_g",
                    "mklist", "mkdict", "mktup", "call", "restpack",
                    "ic_enter"):
            return 2
        if name == "const_i64":
            return 1 + 8
        if name == "const_f64":
            return 1 + 8
        if name in ("const_str", "const_fn", "dot"):
            return 1 + 4
        if name in ("jump", "jumpz", "jumpnz"):
            return 1 + 4
        raise EncodeError("size %s" % name)

    byte_off = []
    off = 0
    for item in abs_ops:
        byte_off.append(off)
        off += op_size(item)
    end_off = off

    def enc_one(item) -> bytes:
        name = item[0]
        out = bytearray()
        out.append(OP[name])
        if name == "const_bool":
            out.append(item[1] & 0xFF)
        elif name == "const_i64":
            out += struct.pack("<q", int(item[1]))
        elif name == "const_f64":
            out += struct.pack("<d", float(item[1]))
        elif name in ("const_str", "const_fn", "dot"):
            out += struct.pack("<I", int(item[1]))
        elif name in ("load_l", "store_l", "load_g", "store_g",
                    "mklist", "mkdict", "mktup", "call", "restpack",
                    "ic_enter"):
            out.append(int(item[1]) & 0xFF)
        elif name in ("jump", "jumpz", "jumpnz"):
            out += struct.pack("<I", int(item[1]))
        return bytes(out)

    final = bytearray()
    for item in abs_ops:
        name = item[0]
        if name in ("jump", "jumpz", "jumpnz"):
            ti = item[1]
            if ti not in code_to_abs:
                raise EncodeError("bad jump %d" % ti)
            abs_i = code_to_abs[ti]
            tgt = end_off if abs_i >= len(byte_off) else byte_off[abs_i]
            final += enc_one((name, tgt))
        else:
            final += enc_one(item)

    # encode nested fns recursively (no further nesting of names for now)
    for n in fn_names:
        child = encode_fn(nested[n])
        encoded_fns.append({
            "name": n,
            "code": list(child["code"]),
            "strings": child["strings"],
            "locals": child["locals"],
            "nparams": nested[n].meta.get("nparams", 0),
            "rest": nested[n].meta.get("rest"),
            "fns": child["fns"],
            "globals": child["globals"],
        })
        # merge child strings into parent? keep separate per-fn for simplicity

    return {
        "code": bytes(final),
        "strings": strings,
        "locals": local_names,
        "globals": globals_used,
        "fns": encoded_fns,
        "nparams": fn.meta.get("nparams", 0),
        "rest": fn.meta.get("rest"),
        "name": fn.meta.get("name"),
    }
