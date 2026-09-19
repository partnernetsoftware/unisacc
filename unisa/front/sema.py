"""Types and symbols -- classic data structures, never neuralized. [T-1] [W-4] [W-5]

The two decisions that ARE table-shaped go through the oracle:
  scope: (ctx, kind)     -> bind_global | bind_param | bind_local | lookup | ...
  type : (t1, op, t2)    -> result type or illegal
"""

TY_SIZE = {"void": 1, "i8": 1, "i16": 2, "i32": 4,
           "u8": 1, "u16": 2, "u32": 4}               # [G-2] else 8
UNSIGNED = ("u8", "u16", "u32", "u64")
RANK = {"i8": 1, "u8": 1, "i16": 2, "u16": 2,
        "i32": 3, "u32": 3, "i64": 4, "u64": 4}


def unsigned_result(t1, t2):
    """Is `t1 op t2` an unsigned operation?  The same rule the type table
    uses (C99 6.3.1.8), asked here because the WALKER picks the machine op
    and the table only reports the result type."""
    def promote(t):
        k = t.kind if t is not None else "i64"
        return "i32" if RANK.get(k, 9) < 3 else k
    a, b = promote(t1), promote(t2)
    ua, ub = a[0] == "u", b[0] == "u"
    if not (ua or ub):
        return False
    if ua and ub:
        return True
    u, sg = (a, b) if ua else (b, a)
    # the other side may be a pointer or an aggregate, which has no rank
    return RANK.get(u, 9) >= RANK.get(sg, 9)
NARROW = ("i8", "i16", "i32", "u8", "u16", "u32")

# The type table's TOPS axis is canonical: one relational op stands for all
# four, one equality op for both.  Projecting onto it is classic key encoding,
# and [P-2] means it must be total before the oracle is asked.
TOP_CANON = {"<": "<", ">": "<", "<=": "<", ">=": "<",
             "==": "==", "!=": "==",
             "+": "+", "-": "-", "*": "*", "/": "/", "%": "%",
             "=": "=", "&": "&", "[]": "[]", ".": ".",
             "call": "call", "sizeof": "sizeof", ",": ",", "un*": "un*",
             "|": "|", "^": "^", "<<": "<<", ">>": ">>"}


class Type:
    __slots__ = ("kind", "to", "n", "tag", "ret")

    def __init__(self, kind, to=None, n=0, tag=None, ret=None):
        self.kind, self.to, self.n, self.tag, self.ret = kind, to, n, tag, ret

    def size(self, structs=None):
        if self.kind == "arr":
            return self.n * self.to.size(structs)
        if self.kind == "struct":
            return structs[self.tag].size if structs else 8
        return TY_SIZE.get(self.kind, 8)

    def __repr__(self):
        if self.kind == "ptr":
            return "%r*" % self.to
        if self.kind == "arr":
            return "%r[%d]" % (self.to, self.n)
        if self.kind == "struct":
            return "struct %s" % self.tag
        return self.kind


VOID = Type("void")
I8 = Type("i8")
I16 = Type("i16")
I32 = Type("i32")
I64 = Type("i64")
U8 = Type("u8")
U16 = Type("u16")
U32 = Type("u32")
U64 = Type("u64")


def ptr(t):
    return Type("ptr", to=t)


class Struct:
    """Also covers unions: every field sits at offset 0 and the size is the
    widest member."""
    __slots__ = ("tag", "fields", "size", "is_union")

    def __init__(self, tag, is_union=False):
        self.tag, self.fields, self.size = tag, {}, 0
        self.is_union = is_union

    def embed(self, ty, structs):
        """Splice an anonymous member's fields into this aggregate.  C11
        6.7.2.1p13: the members of an unnamed struct or union are members of
        the containing one, so a name lookup has to find them here."""
        if ty.kind != "struct" or ty.tag not in structs:
            return
        inner = structs[ty.tag]
        sz = inner.size
        if self.is_union:
            base = 0
            self.size = max(self.size, sz)
        else:
            align = min(8, sz if sz in (1, 2, 4, 8) else 8)
            self.size = (self.size + align - 1) // align * align
            base = self.size
            self.size += sz
        for nm, (fty, foff) in inner.fields.items():
            self.fields[nm] = (fty, base + foff)

    def add(self, name, ty, structs):
        sz = ty.size(structs)
        if self.is_union:
            self.fields[name] = (ty, 0)
            self.size = max(self.size, sz)
            return
        align = min(8, sz if sz in (1, 2, 4, 8) else 8)
        self.size = (self.size + align - 1) // align * align
        self.fields[name] = (ty, self.size)
        self.size += sz


class Sym:
    __slots__ = ("name", "ty", "kind", "off", "sym")

    def __init__(self, name, ty, kind, off=0, sym=None):
        self.name, self.ty, self.kind = name, ty, kind
        self.off, self.sym = off, sym      # off: frame offset  sym: data label


class Scope:
    """Symbol table + the scope-table oracle calls."""

    def __init__(self, oracle):
        self.o = oracle
        self.stack = [{}]
        self.structs = {}
        self.typedefs = {}
        self.enums = {}
        self.enum_tags = set()

    def push(self):
        self.stack.append({})

    def pop(self):
        self.stack.pop()

    def depth(self):
        return len(self.stack)

    def kind_of(self, tok):
        """Map a token to the scope table's KIND axis."""
        if tok.kind == "type":
            return "typedef_id" if tok.text in self.typedefs else "type_kw"
        if tok.kind == "id":
            return "typedef_id" if tok.text in self.typedefs else "id"
        if tok.kind == "*":
            return "star"
        if tok.kind == "(":
            return "lparen"
        return "id"

    def act(self, ctx, tok):
        return self.o.ask("scope", (ctx, self.kind_of(tok)))    # [W-5]

    def declare(self, name, ty, kind, off=0, sym=None):
        s = Sym(name, ty, kind, off, sym)
        self.stack[-1][name] = s
        return s

    def lookup(self, name):
        for d in reversed(self.stack):
            if name in d:
                return d[name]
        return None

    # -- the type table -----------------------------------------------------
    def combine(self, t1, op, t2):
        """[W-4] result type of `t1 op t2`, via the type table."""
        k = self.o.ask("type", (self._ax(t1), TOP_CANON[op], self._ax(t2)))
        return k

    @staticmethod
    def _ax(t):
        """Project a Type onto the type table's TYS axis."""
        if t is None:
            return "void"
        return t.kind if t.kind in ("void", "i8", "i16", "i32", "i64",
                                    "u8", "u16", "u32", "u64",
                                    "ptr", "arr", "struct", "fn") else "i64"
