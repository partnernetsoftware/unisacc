"""jtape VM — semantic source of truth. [TP-1][TP-3]"""
from . import value as V
from .jtape import Fn
from .catalog import shape_of
from .gold import STAGES


class Trap(Exception):
    def __init__(self, kind, message):
        self.kind, self.message = kind, message
        super().__init__("%s: %s" % (kind, message))


def run(fn: Fn, globals_map, locals_map, *, mutate_globals=False,
        oracle=None, ic=False, hot_threshold=8):
    """Execute Fn.  Returns (value, locals_out, globals_out)."""
    G = {k: (v if isinstance(v, tuple) else V.from_py(v))
         for k, v in dict(globals_map or {}).items()}
    L = {k: (v if isinstance(v, tuple) else V.from_py(v))
         for k, v in dict(locals_map or {}).items()}
    # ensure declared slots exist
    for name in fn.localslot:
        L.setdefault(name, V.wrap_null())

    stack = []
    pc = 0
    code = fn.code
    strings = fn.strings
    hits = {}  # site -> count
    o = oracle

    def push(v):
        stack.append(v)

    def pop():
        if not stack:
            raise Trap("stack", "underflow")
        return stack.pop()

    def ask_ic(shape, op, guard):
        if o is None:
            from .catalog import ic_stub
            return ic_stub(shape, op, guard)
        return o.ask("ic", (shape, op, guard))

    while pc < len(code):
        ins = code[pc]
        op = ins.op
        if op == "nop":
            pc += 1
            continue
        if op == "const":
            kind, val = ins.a, ins.b
            if kind == "null":
                push(V.wrap_null())
            elif kind == "bool":
                push(V.wrap_bool(val))
            elif kind == "i64":
                push(V.wrap_i64(val))
            elif kind == "f64":
                push(V.wrap_f64(val))
            elif kind == "str":
                push(V.wrap_str(strings[val] if isinstance(val, int) else val))
            elif kind == "fn":
                fns = fn.meta.get("fns", {})
                if val not in fns:
                    raise Trap("fn", "unknown %s" % val)
                push(V.wrap_fn(fns[val]))
            else:
                raise Trap("const", kind)
            pc += 1
            continue
        if op == "drop":
            pop()
            pc += 1
            continue
        if op == "load_l":
            n = ins.a
            if n not in L:
                raise Trap("unbound", n)
            push(L[n])
            pc += 1
            continue
        if op == "store_l":
            L[ins.a] = pop()
            pc += 1
            continue
        if op == "load_g":
            n = ins.a
            if n not in G:
                raise Trap("unbound", n)
            push(G[n])
            pc += 1
            continue
        if op == "store_g":
            if not mutate_globals:
                raise Trap("readonly", ins.a)
            G[ins.a] = pop()
            pc += 1
            continue
        if op in ("add", "sub", "mul", "div", "mod"):
            b, a = pop(), pop()
            push(_binop(op, a, b, ic, hits, pc, ask_ic))
            pc += 1
            continue
        if op in ("lt", "le", "gt", "ge", "eq", "ne"):
            b, a = pop(), pop()
            push(_cmp(op, a, b))
            pc += 1
            continue
        if op == "not":
            push(V.wrap_bool(not V.truthy(pop())))
            pc += 1
            continue
        if op == "and":
            b, a = pop(), pop()
            push(V.wrap_bool(V.truthy(a) and V.truthy(b)))
            pc += 1
            continue
        if op == "or":
            b, a = pop(), pop()
            push(V.wrap_bool(V.truthy(a) or V.truthy(b)))
            pc += 1
            continue
        if op == "jump":
            pc = ins.a
            continue
        if op == "jumpz":
            pc = ins.a if not V.truthy(pop()) else pc + 1
            continue
        if op == "jumpnz":
            pc = ins.a if V.truthy(pop()) else pc + 1
            continue
        if op == "switch":
            # a = dict value->label, b = default label
            disc = pop()
            table = ins.a
            key = _switch_key(disc)
            pc = table.get(key, ins.b)
            continue
        if op == "ret":
            v = pop() if stack else V.wrap_null()
            return v, L, G
        if op == "idx":
            ix, xs = pop(), pop()
            push(_idx(xs, ix))
            pc += 1
            continue
        if op == "setidx":
            val, ix, xs = pop(), pop(), pop()
            _setidx(xs, ix, val)
            push(val)
            pc += 1
            continue
        if op == "dot":
            obj = pop()
            key = V.wrap_str(ins.a)
            push(_idx(obj, key))
            pc += 1
            continue
        if op == "len":
            v = pop()
            t, x = v
            if t in ("list", "str", "tup", "dict"):
                push(V.wrap_i64(len(x)))
            else:
                raise Trap("type", "len on %s" % t)
            pc += 1
            continue
        if op == "keys":
            v = pop()
            if v[0] != "dict":
                raise Trap("type", "keys")
            push(V.wrap_list(V.wrap_str(k) for k in v[1].keys()))
            pc += 1
            continue
        if op == "in":
            container, key = pop(), pop()
            if container[0] != "dict" or key[0] != "str":
                raise Trap("type", "in")
            push(V.wrap_bool(key[1] in container[1]))
            pc += 1
            continue
        if op == "mklist":
            n = ins.a
            xs = [pop() for _ in range(n)]
            xs.reverse()
            push(V.wrap_list(xs))
            pc += 1
            continue
        if op == "mkdict":
            n = ins.a
            pairs = []
            for _ in range(n):
                val, key = pop(), pop()
                if key[0] != "str":
                    raise Trap("type", "dict key")
                pairs.append((key[1], val))
            pairs.reverse()
            push(V.wrap_dict(pairs))
            pc += 1
            continue
        if op == "mktup":
            n = ins.a
            xs = [pop() for _ in range(n)]
            xs.reverse()
            push(V.wrap_tup(xs))
            pc += 1
            continue
        if op == "typeof":
            push(V.wrap_str(pop()[0]))
            pc += 1
            continue
        if op == "shapeof":
            v = pop()
            ty = v[0]
            ks = V.keysig_of(v[1]) if ty == "dict" else "empty"
            push(V.wrap_str(shape_of(ty, ks)))
            pc += 1
            continue
        if op == "print":
            v = pop()
            pr = G.get("print")
            if pr is not None and callable(pr[1] if isinstance(pr, tuple) else pr):
                fn_ = pr[1] if isinstance(pr, tuple) else pr
                fn_(V.to_py(v))
            else:
                print(V.to_py(v))
            push(V.wrap_null())
            pc += 1
            continue
        if op == "ic_enter":
            # annotation only; execution is classic [IC-4]
            pc += 1
            continue
        if op == "call":
            # stack: ... args callee ; a = argc
            argc = ins.a
            args = [pop() for _ in range(argc)]
            args.reverse()
            callee = pop()
            if callee[0] != "fn":
                raise Trap("type", "call on %s" % callee[0])
            # host callable or nested Fn
            target = callee[1]
            if isinstance(target, Fn):
                argmap = {}
                for i, name in enumerate(target.localslot):
                    argmap[name] = args[i] if i < len(args) else V.wrap_null()
                # rest
                rest_name = target.meta.get("rest")
                nparams = target.meta.get("nparams", len(target.localslot))
                if rest_name:
                    argmap[rest_name] = V.wrap_list(args[nparams:])
                rv, _, _ = run(target, G, argmap, mutate_globals=mutate_globals,
                               oracle=o, ic=ic, hot_threshold=hot_threshold)
                push(rv)
            elif callable(target):
                push(V.from_py(target(*[V.to_py(a) for a in args])))
            else:
                raise Trap("type", "bad fn")
            pc += 1
            continue
        if op == "spread":
            v = pop()
            if v[0] not in ("list", "tup"):
                raise Trap("type", "spread")
            for item in v[1]:
                push(item)
            pc += 1
            continue
        if op == "restpack":
            # pack top `a` stack values into a list (used by call sites)
            n = ins.a
            xs = [pop() for _ in range(n)]
            xs.reverse()
            push(V.wrap_list(xs))
            pc += 1
            continue
        raise Trap("op", "unknown %s" % op)

    return (stack[-1] if stack else V.wrap_null()), L, G


def _switch_key(disc):
    t, x = disc
    if t in ("i64", "str", "bool"):
        return x
    if t == "null":
        return None
    return ("__ty__", t)


def _binop(op, a, b, ic, hits, site, ask_ic):
    ta, tb = a[0], b[0]
    if ic:
        hits[site] = hits.get(site, 0) + 1
        shape = shape_of(ta, "empty")
        stub = ask_ic(shape, op if op in STAGES["ic"]._idx[1] else "add",
                      "type_ok" if ta == tb else "none")
        # stub selection verified by icfold; execution still classic [IC-4]
        _ = stub
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
        x = float(a[1])
        y = float(b[1])
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
    raise Trap("type", "idx %s[%s]" % (t, ix[0]))


def _setidx(xs, ix, val):
    t, x = xs
    if t == "list" and ix[0] == "i64":
        x[ix[1]] = val
        return
    if t == "dict" and ix[0] == "str":
        x[ix[1]] = val
        return
    raise Trap("type", "setidx")
