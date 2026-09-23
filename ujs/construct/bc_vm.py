"""Python reference VM for full UJS bytecode (optional twin of native/ujs_vm.c).

未接线验收；语义对照用。主路径：``vm.py`` (jtape) / ``native/ujs_vm.c``。
"""
from __future__ import annotations

from .bc import OP, TAG, OP_NAME
from . import value as V
from .catalog import shape_of


class Trap(Exception):
    def __init__(self, kind, message):
        self.kind, self.message = kind, message
        super().__init__("%s: %s" % (kind, message))


def run_blob(blob, globals_map=None, locals_map=None, *,
             mutate_globals=False, host_print=None):
    """blob: encode_fn result (dict). Returns (Value, locals_dict, globals_dict)."""
    G = {}
    for k, v in (globals_map or {}).items():
        G[k] = v if isinstance(v, tuple) else V.from_py(v)
    # map global names used by blob
    gnames = list(blob.get("globals") or [])
    gslots = [G.get(n, V.wrap_null()) for n in gnames]

    Lnames = list(blob.get("locals") or [])
    L = [V.wrap_null() for _ in Lnames]
    init = dict(locals_map or {})
    for i, n in enumerate(Lnames):
        if n in init:
            v = init[n]
            L[i] = v if isinstance(v, tuple) else V.from_py(v)

    strings = list(blob.get("strings") or [])
    fns = list(blob.get("fns") or [])
    code = bytes(blob["code"]) if not isinstance(blob["code"], bytes) else blob["code"]
    if isinstance(blob["code"], list):
        code = bytes(blob["code"])

    stack = []
    pc = 0

    def push(v):
        stack.append(v)

    def pop():
        if not stack:
            raise Trap("stack", "underflow")
        return stack.pop()

    while pc < len(code):
        op = code[pc]
        pc += 1
        name = OP_NAME.get(op)
        if name is None:
            raise Trap("op", "bad %d" % op)

        if name == "nop" or name == "ic_enter":
            if name == "ic_enter":
                pc += 1
            continue
        if name == "const_null":
            push(V.wrap_null()); continue
        if name == "const_bool":
            push(V.wrap_bool(code[pc])); pc += 1; continue
        if name == "const_i64":
            import struct
            push(V.wrap_i64(struct.unpack_from("<q", code, pc)[0])); pc += 8
            continue
        if name == "const_f64":
            import struct
            push(V.wrap_f64(struct.unpack_from("<d", code, pc)[0])); pc += 8
            continue
        if name == "const_str":
            import struct
            sid = struct.unpack_from("<I", code, pc)[0]; pc += 4
            push(V.wrap_str(strings[sid])); continue
        if name == "const_fn":
            import struct
            fid = struct.unpack_from("<I", code, pc)[0]; pc += 4
            push(V.wrap_fn(("bc", fns[fid]))); continue
        if name == "load_l":
            push(L[code[pc]]); pc += 1; continue
        if name == "store_l":
            L[code[pc]] = pop(); pc += 1; continue
        if name == "load_g":
            push(gslots[code[pc]]); pc += 1; continue
        if name == "store_g":
            if not mutate_globals:
                raise Trap("readonly", gnames[code[pc]])
            gslots[code[pc]] = pop(); pc += 1; continue
        if name == "drop":
            pop(); continue
        if name in ("add", "sub", "mul", "div", "mod"):
            b, a = pop(), pop()
            push(_binop(name, a, b)); continue
        if name in ("lt", "le", "gt", "ge", "eq", "ne"):
            b, a = pop(), pop()
            push(_cmp(name, a, b)); continue
        if name == "not":
            push(V.wrap_bool(not V.truthy(pop()))); continue
        if name == "and":
            b, a = pop(), pop()
            push(V.wrap_bool(V.truthy(a) and V.truthy(b))); continue
        if name == "or":
            b, a = pop(), pop()
            push(V.wrap_bool(V.truthy(a) or V.truthy(b))); continue
        if name == "jump":
            import struct
            pc = struct.unpack_from("<I", code, pc)[0]; continue
        if name == "jumpz":
            import struct
            tgt = struct.unpack_from("<I", code, pc)[0]; pc += 4
            if not V.truthy(pop()):
                pc = tgt
            continue
        if name == "jumpnz":
            import struct
            tgt = struct.unpack_from("<I", code, pc)[0]; pc += 4
            if V.truthy(pop()):
                pc = tgt
            continue
        if name == "ret":
            v = pop() if stack else V.wrap_null()
            Lout = {Lnames[i]: L[i] for i in range(len(Lnames))}
            for i, n in enumerate(gnames):
                G[n] = gslots[i]
            return v, Lout, G
        if name == "idx":
            ix, xs = pop(), pop(); push(_idx(xs, ix)); continue
        if name == "setidx":
            val, ix, xs = pop(), pop(), pop(); _setidx(xs, ix, val); push(val)
            continue
        if name == "dot":
            import struct
            sid = struct.unpack_from("<I", code, pc)[0]; pc += 4
            push(_idx(pop(), V.wrap_str(strings[sid]))); continue
        if name == "len":
            v = pop(); t, x = v
            if t not in ("list", "str", "tup", "dict"):
                raise Trap("type", "len")
            push(V.wrap_i64(len(x))); continue
        if name == "keys":
            v = pop()
            if v[0] != "dict":
                raise Trap("type", "keys")
            push(V.wrap_list(V.wrap_str(k) for k in v[1].keys())); continue
        if name == "in":
            container, key = pop(), pop()
            if container[0] != "dict" or key[0] != "str":
                raise Trap("type", "in")
            push(V.wrap_bool(key[1] in container[1])); continue
        if name == "mklist":
            n = code[pc]; pc += 1
            xs = [pop() for _ in range(n)]; xs.reverse()
            push(V.wrap_list(xs)); continue
        if name == "mkdict":
            n = code[pc]; pc += 1
            pairs = []
            for _ in range(n):
                val, key = pop(), pop()
                if key[0] != "str":
                    raise Trap("type", "dict key")
                pairs.append((key[1], val))
            pairs.reverse()
            push(V.wrap_dict(pairs)); continue
        if name == "mktup":
            n = code[pc]; pc += 1
            xs = [pop() for _ in range(n)]; xs.reverse()
            push(V.wrap_tup(xs)); continue
        if name == "typeof":
            push(V.wrap_str(pop()[0])); continue
        if name == "shapeof":
            v = pop(); ty = v[0]
            ks = V.keysig_of(v[1]) if ty == "dict" else "empty"
            push(V.wrap_str(shape_of(ty, ks))); continue
        if name == "print":
            v = pop()
            if host_print:
                host_print(V.to_py(v))
            else:
                print(V.to_py(v))
            push(V.wrap_null()); continue
        if name == "spread":
            v = pop()
            if v[0] not in ("list", "tup"):
                raise Trap("type", "spread")
            for item in v[1]:
                push(item)
            continue
        if name == "restpack":
            n = code[pc]; pc += 1
            xs = [pop() for _ in range(n)]; xs.reverse()
            push(V.wrap_list(xs)); continue
        if name == "call":
            argc = code[pc]; pc += 1
            if argc < 0:
                raise Trap("call", "spread argc")
            args = [pop() for _ in range(argc)]; args.reverse()
            callee = pop()
            if callee[0] != "fn":
                raise Trap("type", "call")
            target = callee[1]
            if isinstance(target, tuple) and target[0] == "bc":
                child = target[1]
                argmap = {}
                locs = child.get("locals") or []
                nparams = child.get("nparams", 0)
                rest = child.get("rest")
                for i, nm in enumerate(locs):
                    if rest and nm == rest:
                        continue
                    argmap[nm] = args[i] if i < nparams and i < len(args) else V.wrap_null()
                if rest:
                    argmap[rest] = V.wrap_list(args[nparams:])
                # merge child strings — child blob is self-contained
                rv, _, _ = run_blob(child, G, argmap,
                                    mutate_globals=mutate_globals,
                                    host_print=host_print)
                push(rv)
            elif callable(target):
                push(V.from_py(target(*[V.to_py(a) for a in args])))
            else:
                raise Trap("type", "bad fn")
            continue
        raise Trap("op", name)

    v = stack[-1] if stack else V.wrap_null()
    Lout = {Lnames[i]: L[i] for i in range(len(Lnames))}
    for i, n in enumerate(gnames):
        G[n] = gslots[i]
    return v, Lout, G


def _binop(op, a, b):
    ta, tb = a[0], b[0]
    if ta == "i64" and tb == "i64" and op != "div":
        x, y = a[1], b[1]
        if op == "add":
            return V.wrap_i64(x + y)
        if op == "sub":
            return V.wrap_i64(x - y)
        if op == "mul":
            return V.wrap_i64(x * y)
        if op == "mod":
            return V.wrap_i64(x % y)
    if ta in ("i64", "f64") and tb in ("i64", "f64"):
        x, y = float(a[1]), float(b[1])
        if op == "add":
            return V.wrap_f64(x + y)
        if op == "sub":
            return V.wrap_f64(x - y)
        if op == "mul":
            return V.wrap_f64(x * y)
        if op == "div":
            return V.wrap_f64(x / y)
        if op == "mod":
            return V.wrap_f64(x % y)
    if op == "add" and ta == "str" and tb == "str":
        return V.wrap_str(a[1] + b[1])
    raise Trap("type", "%s %s %s" % (ta, op, tb))


def _cmp(op, a, b):
    ta, tb = a[0], b[0]
    if ta == tb and ta in ("i64", "f64", "str", "bool"):
        x, y = a[1], b[1]
    elif ta in ("i64", "f64") and tb in ("i64", "f64"):
        x, y = float(a[1]), float(b[1])
    elif ta == "null" and tb == "null":
        x = y = None
    else:
        raise Trap("type", "cmp %s %s" % (ta, tb))
    if op == "eq":
        return V.wrap_bool(x == y)
    if op == "ne":
        return V.wrap_bool(x != y)
    if op == "lt":
        return V.wrap_bool(x < y)
    if op == "le":
        return V.wrap_bool(x <= y)
    if op == "gt":
        return V.wrap_bool(x > y)
    if op == "ge":
        return V.wrap_bool(x >= y)
    raise Trap("op", op)


def _idx(xs, ix):
    t, x = xs
    if t == "list" and ix[0] == "i64":
        return x[ix[1]]
    if t == "tup" and ix[0] == "i64":
        return x[ix[1]]
    if t == "str" and ix[0] == "i64":
        return V.wrap_str(x[ix[1]])
    if t == "dict" and ix[0] == "str":
        if ix[1] not in x:
            raise Trap("key", ix[1])
        return x[ix[1]]
    raise Trap("type", "idx")


def _setidx(xs, ix, val):
    t, x = xs
    if t == "list" and ix[0] == "i64":
        x[ix[1]] = val; return
    if t == "dict" and ix[0] == "str":
        x[ix[1]] = val; return
    raise Trap("type", "setidx")
