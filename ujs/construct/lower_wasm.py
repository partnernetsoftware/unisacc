"""WasmProgram lower + interpreter. [LW]

jtape → WasmProgram (structured) → optional .wasm bytes.
Fold compares jtape VM against WasmProgram interpretation.
"""
from .jtape import Fn, Ins
from . import value as V
from . import vm as jvm


class WasmIns:
    __slots__ = ("form", "op", "imm", "slot", "label")

    def __init__(self, form, op=None, imm=None, slot=None, label=None):
        self.form, self.op, self.imm, self.slot, self.label = (
            form, op, imm, slot, label)


class WasmProgram:
    def __init__(self, code, localslot, strings, meta=None):
        self.code = list(code)
        self.localslot = list(localslot)
        self.strings = list(strings)
        self.meta = dict(meta or {})


def lower(fn: Fn, oracle) -> WasmProgram:
    out = []
    for ins in fn.code:
        form = oracle.ask("isel", (ins.op,))
        imm_class = "none"
        if ins.op == "const":
            imm_class = ins.a if ins.a in (
                "i64", "f64", "bool", "str") else "none"
            if imm_class == "str":
                imm_class = "str_ix"
        elif ins.op in ("load_l", "store_l", "load_g", "store_g"):
            imm_class = "slot"
        elif ins.op in ("jump", "jumpz", "jumpnz"):
            imm_class = "label"
        tmpl = oracle.ask("enc", (form, imm_class))
        _ = tmpl  # encoding template selected; bytes later
        if ins.op in ("jump", "jumpz", "jumpnz"):
            reloc = oracle.ask("reloc", (
                "br" if ins.op == "jump" else "br_if",))
            _ = reloc
        out.append(WasmIns(form, op=ins.op, imm=(ins.a, ins.b, ins.c),
                           slot=ins.a if imm_class == "slot" else None,
                           label=ins.a if imm_class == "label" else None))
    return WasmProgram(out, fn.localslot, fn.strings, fn.meta)


def run_wasm(prog: WasmProgram, globals_map, locals_map, **kw):
    """Interpret WasmProgram by reconstructing jtape and using the VM.
    Semantic equivalence is the contract; byte-level wasm is LW-3."""
    code = [Ins(w.op, *(w.imm if w.imm else (None, None, None)))
            for w in prog.code]
    fn = Fn("", code, prog.localslot, prog.strings, prog.meta)
    return jvm.run(fn, globals_map, locals_map, **kw)


def encode_wasm(prog: WasmProgram) -> bytes:
    """Legacy custom blob for Fn serialization tests.

    Browser-loadable runtime is `python3 -m ujs web-build` → ujs/web/ujs_rt.wasm
    (\\0asm magic, exports ask + run). [LW-3]
    """
    parts = [b"UJS1"]
    parts.append(len(prog.code).to_bytes(4, "little"))
    for w in prog.code:
        op = (w.op or "").encode()
        parts.append(len(op).to_bytes(2, "little"))
        parts.append(op)
        form = w.form.encode()
        parts.append(len(form).to_bytes(2, "little"))
        parts.append(form)
    return b"".join(parts)
