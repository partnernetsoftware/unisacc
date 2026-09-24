"""UJS catalog — jtape ops, shapes, stubs, wasm forms. Single source for
isel/enc/ic-related vocabs. [IC-2] [LW-4]
"""

# ---- value / shape --------------------------------------------------------
TYS = ("null", "bool", "i64", "f64", "str", "list", "dict", "fn", "tup")
# Finite key signatures for dict shapes.  "open" = runtime-grown / unknown.
KEYSIG = ("empty", "k1", "k2", "k3", "k4", "open")
SHAPES = (
    "s_null", "s_bool", "s_i64", "s_f64", "s_str",
    "s_list", "s_tup", "s_fn",
    "s_dict_empty", "s_dict_k1", "s_dict_k2", "s_dict_k3", "s_dict_k4",
    "s_dict_open", "s_unknown",
)

# ---- jtape ops ------------------------------------------------------------
JOPS = (
    "nop", "const", "load_l", "store_l", "load_g", "store_g",
    "drop",
    "add", "sub", "mul", "div", "mod",
    "lt", "le", "gt", "ge", "eq", "ne",
    "and", "or", "not",
    "idx", "setidx", "dot", "len", "keys", "in",
    "mklist", "mkdict", "mktup",
    "jump", "jumpz", "jumpnz",
    "call", "ret", "spread", "restpack",
    "typeof", "shapeof",
    "switch",
    "ic_enter",
    "print",
)

# ---- IC -------------------------------------------------------------------
IC_OPS = (
    "add", "sub", "mul", "div", "mod",
    "lt", "eq", "idx", "setidx", "dot", "call", "len", "in",
)
GUARDS = ("none", "type_ok", "len_ok", "key_ok", "mono")
STUBS = (
    "interp",
    "add_i64", "add_f64", "sub_i64", "mul_i64", "div_f64",
    "cmp_i64", "cmp_f64",
    "idx_list", "idx_dict", "idx_str",
    "set_list", "set_dict",
    "dot_dict", "call_fn", "len_list", "len_str", "in_dict",
    "bad",
)

# ---- wasm lowering --------------------------------------------------------
# poly_bin / heap / typeof: constructed forms so isel never needs "trap" for
# in-language ops (emit asks the table; missing coverage is a gold bug).
WASM_FORMS = (
    "nop", "i64_bin", "f64_bin", "i64_cmp", "f64_cmp",
    "bool_logic", "local", "global", "const",
    "br", "br_if", "call", "ret",
    "poly_bin", "heap", "typeof",
    "host", "trap",
)
IMMCLASS = ("none", "i64", "f64", "bool", "str_ix", "slot", "label", "count")
ENCTMPL = (
    "empty", "op", "op_imm", "op_slot", "op_label", "op_host", "trap",
)
RELKIND = ("none", "br_target", "fn_ref", "str_ref")
JMPKIND = ("br", "br_if", "call", "switch")

# shape_id from (ty, keysig)
_SHAPE_MAP = {
    ("null", "empty"): "s_null",
    ("bool", "empty"): "s_bool",
    ("i64", "empty"): "s_i64",
    ("f64", "empty"): "s_f64",
    ("str", "empty"): "s_str",
    ("list", "empty"): "s_list",
    ("tup", "empty"): "s_tup",
    ("fn", "empty"): "s_fn",
    ("dict", "empty"): "s_dict_empty",
    ("dict", "k1"): "s_dict_k1",
    ("dict", "k2"): "s_dict_k2",
    ("dict", "k3"): "s_dict_k3",
    ("dict", "k4"): "s_dict_k4",
    ("dict", "open"): "s_dict_open",
}


def shape_of(ty, keysig):
    return _SHAPE_MAP.get((ty, keysig), "s_unknown")


def isel_form(op):
    if op in ("nop", "drop", "ic_enter"):
        return "nop"
    # add is poly (i64 | str | f64); sub/mul/mod stay numeric-bin default
    if op == "add":
        return "poly_bin"
    if op in ("sub", "mul", "mod"):
        return "i64_bin"
    if op == "div":
        return "f64_bin"
    if op in ("lt", "le", "gt", "ge", "eq", "ne"):
        return "i64_cmp"
    if op in ("and", "or", "not"):
        return "bool_logic"
    if op in ("load_l", "store_l"):
        return "local"
    if op in ("load_g", "store_g"):
        return "global"
    if op == "const":
        return "const"
    if op == "jump":
        return "br"
    if op in ("jumpz", "jumpnz", "switch"):
        return "br_if"
    if op == "call":
        return "call"
    if op == "ret":
        return "ret"
    if op in ("idx", "setidx", "dot", "len", "keys", "in",
              "mklist", "mkdict", "mktup", "spread", "restpack"):
        return "heap"
    if op in ("typeof", "shapeof"):
        return "typeof"
    if op == "print":
        return "host"
    return "trap"


def enc_template(form, imm):
    if form == "nop":
        return "empty"
    if form == "trap":
        return "trap"
    if form == "host":
        return "op_host"
    if form in ("br", "br_if"):
        return "op_label"
    if form in ("local", "global"):
        return "op_slot"
    if form == "const":
        return "op_imm" if imm != "none" else "op"
    if form in ("i64_bin", "f64_bin", "i64_cmp", "f64_cmp", "bool_logic",
                "poly_bin", "typeof", "call", "ret"):
        return "op"
    if form == "heap":
        if imm in ("count", "str_ix"):
            return "op_imm"
        if imm == "slot":
            return "op_slot"
        return "op"
    return "trap"


def reloc_kind(kind):
    return {"br": "br_target", "br_if": "br_target",
            "call": "fn_ref", "switch": "br_target"}.get(kind, "none")


def ic_stub(shape, op, guard):
    """Total function on SHAPES × IC_OPS × GUARDS. [IC-1]"""
    if guard == "none" or shape == "s_unknown":
        return "interp"
    if op == "add":
        if shape == "s_i64" and guard in ("type_ok", "mono"):
            return "add_i64"
        if shape == "s_f64" and guard in ("type_ok", "mono"):
            return "add_f64"
        return "interp"
    if op == "sub" and shape == "s_i64" and guard in ("type_ok", "mono"):
        return "sub_i64"
    if op == "mul" and shape == "s_i64" and guard in ("type_ok", "mono"):
        return "mul_i64"
    if op == "div" and shape == "s_f64" and guard in ("type_ok", "mono"):
        return "div_f64"
    if op in ("lt", "eq"):
        if shape == "s_i64" and guard in ("type_ok", "mono"):
            return "cmp_i64"
        if shape == "s_f64" and guard in ("type_ok", "mono"):
            return "cmp_f64"
        return "interp"
    if op == "idx":
        if shape == "s_list" and guard in ("type_ok", "len_ok", "mono"):
            return "idx_list"
        if shape.startswith("s_dict") and guard in ("type_ok", "key_ok", "mono"):
            return "idx_dict"
        if shape == "s_str" and guard in ("type_ok", "len_ok", "mono"):
            return "idx_str"
        return "interp"
    if op == "setidx":
        if shape == "s_list" and guard in ("type_ok", "len_ok", "mono"):
            return "set_list"
        if shape.startswith("s_dict") and guard in ("type_ok", "key_ok", "mono"):
            return "set_dict"
        return "interp"
    if op == "dot" and shape.startswith("s_dict") and guard in (
            "type_ok", "key_ok", "mono"):
        return "dot_dict"
    if op == "call" and shape == "s_fn" and guard in ("type_ok", "mono"):
        return "call_fn"
    if op == "len":
        if shape == "s_list" and guard in ("type_ok", "mono"):
            return "len_list"
        if shape == "s_str" and guard in ("type_ok", "mono"):
            return "len_str"
        return "interp"
    if op == "in" and shape.startswith("s_dict") and guard in (
            "type_ok", "key_ok", "mono"):
        return "in_dict"
    return "interp"
