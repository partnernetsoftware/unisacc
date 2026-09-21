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
            # n < 0 marks a variable-length array: its storage is not in the
            # frame at all, so it takes no room there and its `sizeof` is a
            # runtime load, not this.
            return 0 if self.n < 0 else self.n * self.to.size(structs)
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
    __slots__ = ("tag", "fields", "size", "is_union", "order", "oindex",
                 "bits", "obits", "bitpos", "align")

    def __init__(self, tag, is_union=False):
        self.tag, self.fields, self.size = tag, {}, 0
        self.is_union = is_union
        # A bit-field has no address, so `fields` gives it the offset of its
        # STORAGE UNIT and `bits` says where inside that unit it lives.
        self.bits = {}              # name -> (bit offset in unit, width, signed)
        self.obits = []             # the same, per `order` entry, or None
        self.bitpos = 0             # the packing cursor, in bits
        self.align = 1              # the widest member's alignment
        # `fields` is for lookup and is FLAT: an anonymous member's fields are
        # spliced in.  `order` is for initialisers, where that same anonymous
        # member counts as one element.  `{1, 2, 3, {4, 5}}` needs both views.
        self.order = []
        self.oindex = {}

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
            if nm in inner.bits:
                bo, w, sg = inner.bits[nm]
                self.bits[nm] = (bo, w, sg)
        self.order.append((base, ty))      # one element for initialisers
        self.obits.append(None)
        self.bitpos = self.size * 8

    def _align_of(self, ty, structs):
        if ty.kind == "arr":
            return self._align_of(ty.to, structs)
        if ty.kind == "struct" and structs and ty.tag in structs:
            return structs[ty.tag].align
        sz = ty.size(structs)
        return sz if sz in (1, 2, 4, 8) else 8

    def finish(self):
        """C99 6.7.2.1p15: an object of struct type has to be able to sit in
        an array, so the size is rounded up to the alignment.  Without this a
        `struct { int a; char b; }` was 5 bytes and every array of one was
        laid out differently from the platform's."""
        a = self.align
        self.size = (max(self.size, (self.bitpos + 7) // 8) + a - 1) // a * a

    def add(self, name, ty, structs):
        sz = ty.size(structs)
        self.align = max(self.align, self._align_of(ty, structs))
        if self.is_union:
            self.fields[name] = (ty, 0)
            self.oindex[name] = len(self.order)
            self.order.append((0, ty))
            self.obits.append(None)
            self.size = max(self.size, sz)
            return
        align = min(8, sz if sz in (1, 2, 4, 8) else 8)
        # a plain member starts at the next byte, whatever bits precede it
        self.size = max(self.size, (self.bitpos + 7) // 8)
        self.size = (self.size + align - 1) // align * align
        self.fields[name] = (ty, self.size)
        self.oindex[name] = len(self.order)
        self.order.append((self.size, ty))
        self.obits.append(None)
        self.size += sz
        self.bitpos = self.size * 8

    def add_bits(self, name, ty, width, signed, structs):
        """A bit-field.  C99 6.7.2.1 leaves the layout implementation-defined;
        this is what both our targets' ABIs do -- pack in declaration order
        and start a new storage unit when the field would straddle one.  A
        width of zero names nothing and forces that break."""
        unit = ty.size(structs) * 8
        self.align = max(self.align, self._align_of(ty, structs))
        if self.is_union:
            if name:
                self.fields[name] = (ty, 0)
                self.bits[name] = (0, width, signed)
                self.oindex[name] = len(self.order)
                self.order.append((0, ty))
                self.obits.append((0, width, signed))
            self.size = max(self.size, ty.size(structs))
            return
        pos = max(self.bitpos, self.size * 8)
        if width == 0:
            self.bitpos = (pos + unit - 1) // unit * unit
            self.size = max(self.size, self.bitpos // 8)
            return
        if pos % unit + width > unit:
            pos = (pos + unit - 1) // unit * unit
        off = pos // unit * (unit // 8)
        if name:
            self.fields[name] = (ty, off)
            self.bits[name] = (pos - off * 8, width, signed)
            self.oindex[name] = len(self.order)
            self.order.append((off, ty))
            self.obits.append((pos - off * 8, width, signed))
        self.bitpos = pos + width
        self.size = max(self.size, (self.bitpos + 7) // 8)


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
        self.enum_neg = {}
        # struct/union tags are scoped like ordinary names: an inner
        # `struct T { ... };` shadows an outer T instead of overwriting it.
        # `structs` stays flat and is keyed by the RESOLVED name.
        self.tagstack = [{}]
        # typedef names are block-scoped too: `typedef enum { e } h;` inside a
        # function must not still be a type name after the closing brace.
        # `typedefs` stays flat; this records what each block shadowed.
        self.tdstack = [{}]

    def push(self):
        self.stack.append({})
        self.tagstack.append({})
        self.tdstack.append({})

    def pop(self):
        self.stack.pop()
        self.tagstack.pop()
        for n, prev in self.tdstack.pop().items():
            if prev is None:
                self.typedefs.pop(n, None)
            else:
                self.typedefs[n] = prev

    def typedef(self, name, ty):
        top = self.tdstack[-1]
        if name not in top:
            top[name] = self.typedefs.get(name)
        self.typedefs[name] = ty

    def tag_lookup(self, tag):
        for d in reversed(self.tagstack):
            if tag in d:
                return d[tag]
        return None

    def tag_bind(self, tag, defining, uniq):
        """Resolve a struct/union tag to its key in `structs`.

        A definition binds in the CURRENT scope -- with a fresh key when the
        tag is already visible from an outer one, so the outer type survives.
        A mere reference resolves outward, and declares the tag here if it is
        new."""
        cur = self.tagstack[-1].get(tag)
        if cur is not None:
            return cur
        if defining and self.tag_lookup(tag) is not None:
            key = "%s#%d" % (tag, uniq)
        else:
            key = self.tag_lookup(tag) or tag
        self.tagstack[-1][tag] = key
        return key

    def depth(self):
        return len(self.stack)

    def kind_of(self, tok):
        """Map a token to the scope table's KIND axis."""
        if tok.kind == "type":
            return "typedef_id" if tok.text in self.typedefs else "type_kw"
        if tok.kind == "id":
            return "typedef_id" if tok.text in self.typedefs else "id"
        # `struct`/`union`/`enum` open a type name exactly as `int` does --
        # without this the table never sees `sizeof(struct S)` as a type
        if tok.kind in ("struct", "union", "enum"):
            return "type_kw"
        if tok.kind == "*":
            return "star"
        if tok.kind == "(":
            return "lparen"
        return "id"

    def act(self, ctx, tok, declared=False):  # declared: the position fixes it as a name
        # A name in declarator position is being DECLARED: a typedef name
        # there shadows the typedef (C99 6.2.1p4), so its kind is `id`.
        k = "id" if declared and tok.kind in ("id", "type") else self.kind_of(tok)
        return self.o.ask("scope", (ctx, k))    # [W-5]

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
