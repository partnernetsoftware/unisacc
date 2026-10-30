"""m4 的 emit：与 ujs emit_wasm 同形的 WAT 生成，但算术/逻辑族由 isel 形态分派。

常量、辅助函数与其余 op 处理逐字取自 ujs（运行时由 ujs 模块提供）。
闸：emit_wat 的文本必须与 ujs 的 emit_wat 逐字相等（由 m4 check/selftest 覆盖）。
"""
from ujs.seed.construct import emit_wasm as _E
from ujs.seed.construct.emit_wasm import DirectEmitError, can_emit_direct

from .contract import ISEL_CHOICE

# 运行时常量与辅助函数直接取自 ujs 模块
globals().update({k: v for k, v in vars(_E).items() if not k.startswith("__")})

# 进入算术/逻辑族的 op（形态由 isel 决定）
_FAMILY_OPS = ("add", "sub", "mul", "div", "mod", "lt", "le", "gt", "ge", "eq", "ne", "and", "or")
_BIN_CODE = {"sub": 0, "mul": 1, "div": 2, "mod": 3}
_CMP_CODE = {"lt": 0, "le": 1, "gt": 2, "ge": 3, "eq": 4, "ne": 5}
# 每个 op 允许的 isel 形态；网给出别的形态即明确拒收。
# eq/ne 有两个合法形态，由网选文本（见 contract.ISEL_CHOICE）。
_LEGAL_FORM = {
    "add": ("poly_bin",),
    "sub": ("i64_bin",), "mul": ("i64_bin",), "mod": ("i64_bin",),
    "div": ("f64_bin",),
    "lt": ("i64_cmp",), "le": ("i64_cmp",), "gt": ("i64_cmp",),
    "ge": ("i64_cmp",),
    "eq": ISEL_CHOICE["eq"], "ne": ISEL_CHOICE["ne"],
    "and": ("bool_logic",), "or": ("bool_logic",),
}


# 每个 op 允许的 enc 模板（实测于语料：gold 对 (isel 形态, 立即数类) 的映射）。
# enc 给出别的模板即明确拒收；未列入的 op 也拒收（m4 不声称覆盖它）。
_LEGAL_ENC = {
    "nop": ("empty",), "drop": ("empty",), "ic_enter": ("empty",),
    "const": ("op", "op_imm"),
    "load_l": ("op_slot",), "load_g": ("op_slot",),
    "store_l": ("op_slot",), "store_g": ("op_slot",),
    "jump": ("op_label",), "jumpz": ("op_label",), "jumpnz": ("op_label",),
    "mklist": ("op_imm",), "mkdict": ("op_imm",), "dot": ("op_imm",),
    "call": ("op",), "ret": ("op",), "typeof": ("op",),
    "idx": ("op",), "len": ("op",), "setidx": ("op",),
    "add": ("op",), "sub": ("op",), "mul": ("op",), "div": ("op",), "mod": ("op",),
    "lt": ("op",), "le": ("op",), "gt": ("op",), "ge": ("op",), "eq": ("op",), "ne": ("op",),
    "and": ("op",), "or": ("op",), "not": ("op",),
}


def _imm_of(ins):
    op = ins.op
    if op == "const":
        return {"i64": "i64", "f64": "f64", "bool": "bool", "str": "str_ix"}.get(ins.a, "none")
    if op in ("load_l", "store_l", "load_g", "store_g"):
        return "slot"
    if op in ("jump", "jumpz", "jumpnz"):
        return "label"
    if op in ("mklist", "mkdict", "mktup", "call"):
        return "count"
    if op == "dot":
        return "str_ix"
    return "none"


def _enc_gate(ins, oracle):
    if ins.op not in _LEGAL_ENC:
        raise DirectEmitError("m4: no pinned enc template for op %s" % ins.op)
    form = oracle.ask("isel", (ins.op,))
    tmpl = oracle.ask("enc", (form, _imm_of(ins)))
    if tmpl not in _LEGAL_ENC[ins.op]:
        raise DirectEmitError("m4: enc %r illegal for %s (form %s)" % (tmpl, ins.op, form))


def emit_wasm(fn, out_path, oracle=None):
    """与 ujs emit_wasm 同：WAT → wat2wasm；WAT 由 m4 的 emit_wat 生成。"""
    import os
    import subprocess
    import tempfile
    wat = emit_wat(fn, oracle)
    with tempfile.TemporaryDirectory() as td:
        wp = os.path.join(td, "p.wat")
        with open(wp, "w") as f:
            f.write(wat)
        subprocess.check_call(["wat2wasm", wp, "-o", out_path])
    return {"wasm": out_path, "bytes": os.path.getsize(out_path), "direct": True}


def _body(fn: Fn, str_off: list[tuple[int, int]], fn_index: dict[str, int],
          name: str, oracle) -> str:
    slots = list(fn.localslot)
    for ins in fn.code:
        if ins.op in ("load_l", "store_l", "load_g", "store_g") and ins.a not in slots:
            slots.append(ins.a)
    six = {n: i for i, n in enumerate(slots)}
    n = len(fn.code)

    def la(nm):
        return f"(i32.add (i32.const {LOCALS0}) (i32.const {six[nm]*4}))"

    def npc(i):
        return f"(i32.const {i+1}) (local.set $pc) (br $dispatch)"

    def spc(t):
        return f"(i32.const {t}) (local.set $pc) (br $dispatch)"

    nparams = int((fn.meta or {}).get("nparams") or 0)
    rest_name = (fn.meta or {}).get("rest")
    rest_ix = six[rest_name] if rest_name in six else None

    parts = [f"(func ${name} (result i32)",
             "(local $pc i32) (local $a i32) (local $b i32) (local $c i32)",
             "(local $i i32) (local $p i32) (local $t i32) (local $nn i32)"]
    if name != "main_body":
        parts.append("(local.set $nn (global.get $call_argc))")
        for pi in range(nparams):
            parts.append(
                f"(i32.store (i32.add (i32.const {LOCALS0}) (i32.const {pi * 4})) "
                f"(i32.load (i32.add (i32.const {SAVE0 + 256}) (i32.const {pi * 4}))))")
        if rest_ix is not None:
            parts.append(f"""
          (local.set $i (i32.sub (local.get $nn) (i32.const {nparams})))
          (if (i32.lt_s (local.get $i) (i32.const 0)) (then (local.set $i (i32.const 0))))
          (local.set $p (call $alloc (i32.add (i32.const 8) (i32.mul (local.get $i) (i32.const 4)))))
          (i32.store8 (local.get $p) (i32.const {TAG_LIST}))
          (i32.store (i32.add (local.get $p) (i32.const 4)) (local.get $i))
          (local.set $t (i32.const 0))
          (block $rd (loop $RL
            (br_if $rd (i32.ge_u (local.get $t) (local.get $i)))
            (i32.store (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $t) (i32.const 4)))
              (i32.load (i32.add (i32.const {SAVE0 + 256})
                (i32.mul (i32.add (local.get $t) (i32.const {nparams})) (i32.const 4)))))
            (local.set $t (i32.add (local.get $t) (i32.const 1)))
            (br $RL)))
          (i32.store (i32.add (i32.const {LOCALS0}) (i32.const {rest_ix * 4})) (local.get $p))
""")
    parts += ["(local.set $pc (i32.const 0))", "(loop $dispatch"]
    for i in range(n - 1, -1, -1):
        parts.append(f"(block $bb{i}")
    parts.append("(block $trap")
    parts.append("(local.get $pc)")
    parts.append("(br_table " + " ".join(str(j + 1) for j in range(n)) + " 0)")
    parts.append(") (unreachable)")

    for i, ins in enumerate(fn.code):
        parts.append(")")
        _enc_gate(ins, oracle)
        op, a, b = ins.op, ins.a, ins.b
        if op in ("nop", "ic_enter"):
            parts.append(npc(i))
        elif op == "const":
            if a == "null":
                parts.append("(call $push (i32.const 0))")
            elif a == "bool":
                parts.append(f"(call $push (call $mk_bool (i32.const {1 if b else 0})))")
            elif a == "i64":
                parts.append(f"(call $push (call $mk_i64 (i64.const {int(b)})))")
            elif a == "f64":
                parts.append(f"(call $push (call $mk_f64 (f64.const {float(b)})))")
            elif a == "str":
                off, ln = str_off[int(b)]
                parts.append(f"(call $push (call $mk_str (i32.const {off}) (i32.const {ln})))")
            elif a == "fn":
                parts.append(f"(call $push (call $mk_fn (i32.const {fn_index[b]})))")
            else:
                raise DirectEmitError("const " + str(a))
            parts.append(npc(i))
        elif op in ("load_l", "load_g"):
            parts.append(f"(call $push (i32.load {la(a)}))")
            parts.append(npc(i))
        elif op in ("store_l", "store_g"):
            parts.append(f"(i32.store {la(a)} (call $pop))")
            parts.append(npc(i))
        elif op == "drop":
            parts.append("(drop (call $pop))")
            parts.append(npc(i))
        elif op in _FAMILY_OPS:
            # 形态由网（isel）决定；这里只把形态映射到运行时辅助函数
            form = oracle.ask("isel", (op,))
            if form not in _LEGAL_FORM[op]:
                raise DirectEmitError("m4: isel form %r illegal for %s" % (form, op))
            parts.append("(local.set $b (call $pop)) (local.set $a (call $pop))")
            if form == "poly_bin":
                parts.append("(call $push (call $op_add (local.get $a) (local.get $b)))")
            elif form in ("i64_bin", "f64_bin"):
                parts.append(f"(call $push (call $op_bin (i32.const {_BIN_CODE[op]}) (local.get $a) (local.get $b)))")
            elif form == "i64_cmp":
                parts.append(f"(call $push (call $op_cmp (i32.const {_CMP_CODE[op]}) (local.get $a) (local.get $b)))")
            elif form == "f64_cmp":
                # 与 i64_cmp 不同的文本：按 f64 比较后再装箱。gold 不选这条，语料字节不变。
                pred = {"eq": "f64.eq", "ne": "f64.ne"}[op]
                parts.append(
                    "(call $push (call $mk_bool (%s (call $as_f64 (local.get $a)) "
                    "(call $as_f64 (local.get $b)))))" % pred)
            elif form == "bool_logic":
                fnm = {"and": "i32.and", "or": "i32.or"}[op]
                parts.append(f"(call $push (call $mk_bool ({fnm} (call $truthy (local.get $a)) (call $truthy (local.get $b)))))")
            else:
                raise DirectEmitError("m4: form %s not handled for %s" % (form, op))
            parts.append(npc(i))
        elif op == "not":
            parts.append("(local.set $a (call $pop))")
            parts.append("(call $push (call $mk_bool (i32.eqz (call $truthy (local.get $a)))))")
            parts.append(npc(i))
        elif op == "jump":
            parts.append(spc(int(a)))
        elif op == "jumpz":
            parts.append("(local.set $a (call $pop))")
            parts.append(
                "(if (i32.eqz (call $truthy (local.get $a))) "
                f"(then (i32.const {int(a)}) (local.set $pc) (br $dispatch)))")
            parts.append(npc(i))
        elif op == "jumpnz":
            parts.append("(local.set $a (call $pop))")
            parts.append(
                "(if (call $truthy (local.get $a)) "
                f"(then (i32.const {int(a)}) (local.set $pc) (br $dispatch)))")
            parts.append(npc(i))
        elif op == "ret":
            parts.append("(return (call $pop))")
        elif op == "mklist":
            cnt = int(a)
            parts.append(f"""
          (local.set $nn (i32.const {cnt}))
          (local.set $p (call $alloc (i32.add (i32.const 8) (i32.mul (local.get $nn) (i32.const 4)))))
          (i32.store8 (local.get $p) (i32.const {TAG_LIST}))
          (i32.store (i32.add (local.get $p) (i32.const 4)) (local.get $nn))
          (local.set $i (local.get $nn))
          (block $done (loop $L
            (br_if $done (i32.eqz (local.get $i)))
            (local.set $i (i32.sub (local.get $i) (i32.const 1)))
            (i32.store (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $i) (i32.const 4))) (call $pop))
            (br $L)))
          (call $push (local.get $p))
""")
            parts.append(npc(i))
        elif op == "mkdict":
            cnt = int(a)
            parts.append(f"""
          (local.set $nn (i32.const {cnt}))
          (local.set $p (call $alloc (i32.add (i32.const 8) (i32.mul (local.get $nn) (i32.const 8)))))
          (i32.store8 (local.get $p) (i32.const {TAG_DICT}))
          (i32.store (i32.add (local.get $p) (i32.const 4)) (local.get $nn))
          (local.set $i (local.get $nn))
          (block $done (loop $L
            (br_if $done (i32.eqz (local.get $i)))
            (local.set $i (i32.sub (local.get $i) (i32.const 1)))
            (local.set $b (call $pop)) (local.set $a (call $pop))
            (i32.store (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $i) (i32.const 8))) (local.get $a))
            (i32.store (i32.add (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $i) (i32.const 8))) (i32.const 4)) (local.get $b))
            (br $L)))
          (call $push (local.get $p))
""")
            parts.append(npc(i))
        elif op == "mktup":
            cnt = int(a)
            parts.append(f"""
          (local.set $nn (i32.const {cnt}))
          (local.set $p (call $alloc (i32.add (i32.const 8) (i32.mul (local.get $nn) (i32.const 4)))))
          (i32.store8 (local.get $p) (i32.const {TAG_TUP}))
          (i32.store (i32.add (local.get $p) (i32.const 4)) (local.get $nn))
          (local.set $i (local.get $nn))
          (block $done (loop $L
            (br_if $done (i32.eqz (local.get $i)))
            (local.set $i (i32.sub (local.get $i) (i32.const 1)))
            (i32.store (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $i) (i32.const 4))) (call $pop))
            (br $L)))
          (call $push (local.get $p))
""")
            parts.append(npc(i))
        elif op == "len":
            parts.append("(local.set $a (call $pop))")
            parts.append("(call $push (call $mk_i64 (i64.extend_i32_u (call $len_of (local.get $a)))))")
            parts.append(npc(i))
        elif op == "idx":
            parts.append(f"""
          (local.set $b (call $pop)) (local.set $a (call $pop))
          (local.set $t (call $tag_of (local.get $a)))
          (if (i32.or (i32.eq (local.get $t) (i32.const {TAG_LIST}))
                      (i32.eq (local.get $t) (i32.const {TAG_TUP})))
            (then (call $push (i32.load (i32.add (i32.add (local.get $a) (i32.const 8))
              (i32.mul (i32.wrap_i64 (call $i64_of (local.get $b))) (i32.const 4))))))
            (else (if (i32.eq (local.get $t) (i32.const {TAG_DICT}))
              (then (call $push (call $dict_get (local.get $a) (local.get $b))))
              (else (unreachable)))))
""")
            parts.append(npc(i))
        elif op == "setidx":
            # stack: … xs ix val  → pop val, ix, xs; store; push val
            parts.append(f"""
          (local.set $c (call $pop)) (local.set $b (call $pop)) (local.set $a (call $pop))
          (local.set $t (call $tag_of (local.get $a)))
          (if (i32.or (i32.eq (local.get $t) (i32.const {TAG_LIST}))
                      (i32.eq (local.get $t) (i32.const {TAG_TUP})))
            (then (i32.store (i32.add (i32.add (local.get $a) (i32.const 8))
              (i32.mul (i32.wrap_i64 (call $i64_of (local.get $b))) (i32.const 4))) (local.get $c)))
            (else (if (i32.eq (local.get $t) (i32.const {TAG_DICT}))
              (then (call $dict_set (local.get $a) (local.get $b) (local.get $c)))
              (else (unreachable)))))
          (call $push (local.get $c))
""")
            parts.append(npc(i))
        elif op == "dot":
            off, ln = str_off[int(a)]
            parts.append(f"""
          (local.set $a (call $pop))
          (local.set $b (call $mk_str (i32.const {off}) (i32.const {ln})))
          (call $push (call $dict_get (local.get $a) (local.get $b)))
""")
            parts.append(npc(i))
        elif op == "typeof":
            parts.append("(local.set $a (call $pop))")
            parts.append("(call $push (call $typeof_name (local.get $a)))")
            parts.append(npc(i))
        elif op == "call":
            argc = int(a)
            for ai in range(argc):
                parts.append("(local.set $a (call $pop))")
                parts.append(
                    f"(i32.store (i32.add (i32.const {SAVE0+256}) (i32.const {(argc-1-ai)*4})) (local.get $a))")
            parts.append(f"""
          (local.set $a (call $pop))
          (if (i32.ne (call $tag_of (local.get $a)) (i32.const {TAG_FN})) (then (unreachable)))
          (local.set $t (i32.load (i32.add (local.get $a) (i32.const 4))))
          (local.set $i (i32.const 0))
          (block $ds (loop $LS
            (br_if $ds (i32.ge_u (local.get $i) (i32.const 64)))
            (i32.store (i32.add (i32.const {SAVE0}) (i32.mul (local.get $i) (i32.const 4)))
              (i32.load (i32.add (i32.const {LOCALS0}) (i32.mul (local.get $i) (i32.const 4)))))
            (local.set $i (i32.add (local.get $i) (i32.const 1)))
            (br $LS)))
          (local.set $i (i32.const 0))
          (block $dc (loop $Lc
            (br_if $dc (i32.ge_u (local.get $i) (i32.const 64)))
            (i32.store (i32.add (i32.const {LOCALS0}) (i32.mul (local.get $i) (i32.const 4))) (i32.const 0))
            (local.set $i (i32.add (local.get $i) (i32.const 1)))
            (br $Lc)))
""")
            parts.append(f"(global.set $call_argc (i32.const {argc}))")
            parts.append("(call $push (call $dispatch_fn (local.get $t)))")
            parts.append(f"""
          (local.set $i (i32.const 0))
          (block $dr (loop $Lr
            (br_if $dr (i32.ge_u (local.get $i) (i32.const 64)))
            (i32.store (i32.add (i32.const {LOCALS0}) (i32.mul (local.get $i) (i32.const 4)))
              (i32.load (i32.add (i32.const {SAVE0}) (i32.mul (local.get $i) (i32.const 4)))))
            (local.set $i (i32.add (local.get $i) (i32.const 1)))
            (br $Lr)))
""")
            parts.append(npc(i))
        else:
            raise DirectEmitError("emit gap " + op)

    parts.append("(unreachable)) (unreachable))")
    return "\n".join(parts)


def emit_wat(fn: Fn, oracle: Oracle | None = None) -> str:
    o = oracle or Oracle(drive="gold")
    ok, why = can_emit_direct(fn, o)
    if not ok:
        raise DirectEmitError(why)

    nested = dict(fn.meta.get("fns") or {})
    strings = list(fn.strings)
    for nf in nested.values():
        for s in nf.strings:
            if s not in strings:
                strings.append(s)
    for fobj in [fn, *nested.values()]:
        for ins in fobj.code:
            if ins.op == "dot" and isinstance(ins.a, str) and ins.a not in strings:
                strings.append(ins.a)

    blob = bytearray()
    str_off = []
    for s in strings:
        b = s.encode("utf-8")
        str_off.append((STRDATA0 + len(blob), len(b)))
        blob += b

    fn_names = sorted(nested.keys())
    fn_index = {name: i for i, name in enumerate(fn_names)}

    out = ["(module", _helpers()]
    if blob:
        out.append(f'(data (i32.const {STRDATA0}) "{_esc(bytes(blob))}")')

    # nested functions first (so dispatch can call them)
    for i, name in enumerate(fn_names):
        out.append(_body(_normalize(nested[name], strings), str_off, fn_index, f"fn_{i}", o))

    nfn = len(fn_names)
    if nfn == 0:
        out.append("(func $dispatch_fn (param $ix i32) (result i32) (unreachable))")
    else:
        d = ["(func $dispatch_fn (param $ix i32) (result i32)", "(block $bad"]
        for i in range(nfn - 1, -1, -1):
            d.append(f"(block $f{i}")
        d.append("(local.get $ix)")
        d.append("(br_table " + " ".join(str(i) for i in range(nfn)) + f" {nfn})")
        for i in range(nfn):
            d.append(")")  # close $f_i
            d.append(f"(return (call $fn_{i}))")
        d.append(")")  # close $bad
        d.append("(unreachable))")  # default + close func
        out.append("\n".join(d))

    out.append(_body(_normalize(fn, strings), str_off, fn_index, "main_body", o))
    flat = _slot_names(fn)
    local_names, globals_used = _gl_names(fn)
    out.append(_host_api_wat(flat, local_names, globals_used))
    out.append(f"""
  (func $main_export (result i32)
    (drop (call $host_reset))
    (call $main_body))
  (func $tag_of_export (param $h i32) (result i32) (call $tag_of (local.get $h)))
  (func $i64_of_export (param $h i32) (result i64) (call $i64_of (local.get $h)))
  (func $f64_of_export (param $h i32) (result f64)
    (if (result f64) (i32.eq (call $tag_of (local.get $h)) (i32.const {TAG_F64}))
      (then (call $f64_of (local.get $h)))
      (else (f64.convert_i64_s (call $i64_of (local.get $h))))))
  (func $str_len_export (param $h i32) (result i32) (call $len_of (local.get $h)))
  (func $str_ptr_export (param $h i32) (result i32)
    (if (result i32) (i32.eqz (local.get $h)) (then (i32.const 0))
      (else (i32.add (local.get $h) (i32.const 8)))))
  (func $mem_base (result i32) (i32.const 0))
  (func $mem_size (result i32) (i32.const {MEM_BYTES}))
  (export "main_export" (func $main_export))
  (export "clear_slots" (func $clear_slots))
  (export "run_step" (func $run_step))
  (export "host_reset" (func $host_reset))
  (export "host_run" (func $host_run))
  (export "host_set_local" (func $host_set_local))
  (export "host_set_global" (func $host_set_global))
  (export "host_get_local" (func $host_get_local))
  (export "host_get_global" (func $host_get_global))
  (export "host_mk_null" (func $host_mk_null))
  (export "host_mk_bool" (func $host_mk_bool))
  (export "host_mk_i64" (func $host_mk_i64))
  (export "host_mk_f64" (func $host_mk_f64))
  (export "host_mk_str" (func $host_mk_str))
  (export "host_mk_list" (func $host_mk_list))
  (export "host_list_set" (func $host_list_set))
  (export "host_list_get" (func $host_list_get))
  (export "host_mk_dict" (func $host_mk_dict))
  (export "host_dict_set" (func $host_dict_set))
  (export "host_dict_key" (func $host_dict_key))
  (export "host_dict_val" (func $host_dict_val))
  (export "host_len" (func $host_len))
  (export "host_scratch" (func $host_scratch))
  (export "host_prog_addr" (func $host_prog_addr))
  (export "host_load_image" (func $host_load_image))
  (export "last_ic_stub_export" (func $last_ic_stub_export))
  (export "tag_of_export" (func $tag_of_export))
  (export "i64_of_export" (func $i64_of_export))
  (export "f64_of_export" (func $f64_of_export))
  (export "str_len_export" (func $str_len_export))
  (export "str_ptr_export" (func $str_ptr_export))
  (export "mem_base" (func $mem_base))
  (export "mem_size" (func $mem_size))
)
""")
    return "\n".join(out)

