"""Tape: the target-independent instruction stream. [TP]

Line oriented text.  Labels `L:`.  r0-r7, r7 = SP starting at 0x10000.
64 KB little-endian memory.  Arithmetic wraps mod 2^64, signed two's complement.

Calling convention (tape level): args in r0..r5, return value in r0,
return address pushed on the stack by `call`.

`.str NAME "..."` is an assembler directive, not an instruction: it reserves
bytes in the data area and binds NAME for `.lea`.
"""

REGS = tuple("r%d" % i for i in range(8))
SP = 7
# 64 KB was fine for the examples; a self-hosting compiler needs room for its
# tables, so the interpreter's memory is 16 MB.  Native code is not affected --
# there the stack is the real one and globals live in __DATA.
MEM_SIZE = 0x1000000
STACK_TOP = 0x1000000
DATA_BASE = 0x100

# op -> operand shape.  r=register  i=immediate  L=label  s=symbol-or-imm
SHAPE = {
    "imm":     ("r", "i"),
    "mov":     ("r", "r"),
    "add64":   ("r", "r", "r"),
    "sub64":   ("r", "r", "r"),
    "mul64":   ("r", "r", "r"),
    "xor64":   ("r", "r", "r"),
    "and64":   ("r", "r", "r"),
    "or64":    ("r", "r", "r"),
    "shl64":   ("r", "r", "r"),
    "shr64":   ("r", "r", "r"),
    ".div":    ("r", "r", "r"),
    ".mod":    ("r", "r", "r"),
    "slt64":   ("r", "r", "r"),
    "sle64":   ("r", "r", "r"),
    "eq":      ("r", "r", "r"),
    "ne":      ("r", "r", "r"),
    "load64":  ("r", "r", "i"),
    "store64": ("r", "i", "r"),
    ".ld":     ("r", "r", "i", "i"),
    ".st":     ("r", "i", "r", "i"),
    ".lea":    ("r", "s"),
    ".zero":   ("r", "i", "i"),
    "jump":    ("L",),
    "jumpz":   ("r", "L"),
    "call":    ("L",),
    "callr":   ("r",),
    "ret":     (),
    ".frame":  ("i",),
    ".arg":    ("i", "r"),
    ".print":  ("r",),
    ".write":  ("r", "r"),
    ".exit":   ("r",),
    ".sys":    ("s", "r", "r", "r"),   # catalog op name + 3 args -> r0
    ".argc":   ("r",),                 # argument count
    ".argv":   ("r", "r"),             # rd = argv[rs], a char*
    "nop":     (),
}

ESCAPES = {"n": "\n", "t": "\t", "r": "\r", "0": "\0",
           "\\": "\\", '"': '"', "'": "'"}


class Insn:
    __slots__ = ("op", "args", "line")

    def __init__(self, op, args, line=0):
        self.op, self.args, self.line = op, args, line

    def __repr__(self):
        return "%s %s" % (self.op, ", ".join(str(a) for a in self.args))


class Tape:
    def __init__(self):
        self.code = []          # [Insn]
        self.labels = {}        # name -> pc
        self.data = bytearray() # packed literals, based at DATA_BASE
        self.syms = {}          # name -> address
        # Offsets in `data` that hold an absolute data address (a global
        # pointer initialised with a string literal).  The interpreter uses
        # them as-is; an image must relocate them onto its own data segment.
        self.relocs = []

    # -- building ---------------------------------------------------------
    def label(self, name):
        self.labels[name] = len(self.code)

    def emit(self, op, *args):
        assert op in SHAPE, "unknown tape op %r" % op
        exp = len(SHAPE[op])
        assert len(args) == exp, "%s takes %d operands, got %d" % (op, exp, len(args))
        self.code.append(Insn(op, list(args)))
        return len(self.code) - 1

    def string(self, name, raw, align=1):
        """Intern a byte literal, return its address.

        `align` matters: arm64 faults with SIGBUS on an unaligned 64-bit
        load/store, so anything a program will access as a word -- every global
        -- has to start on an 8-byte boundary."""
        if name in self.syms:
            return self.syms[name]
        if align > 1:
            pad = (-len(self.data)) % align
            self.data.extend(b"\x00" * pad)
        addr = DATA_BASE + len(self.data)
        self.data.extend(raw)
        self.syms[name] = addr
        return addr

    # -- text -------------------------------------------------------------
    def to_text(self):
        out = []
        for name, addr in sorted(self.syms.items(), key=lambda kv: kv[1]):
            end = addr - DATA_BASE
            nxt = min([a - DATA_BASE for a in self.syms.values()
                       if a - DATA_BASE > end] + [len(self.data)])
            blob = self.data[end:nxt]
            # a large zero-filled global is `.bss`, not a quoted literal: a
            # 64 KB arena would otherwise serialise as 64 KB of "\0"
            if len(blob) > 16 and not any(blob):
                out.append(".bss %s %d" % (name, len(blob)))
            else:
                out.append(".str %s %s" % (name, _quote(blob)))
        rev = {}
        for name, pc in self.labels.items():
            rev.setdefault(pc, []).append(name)
        for pc, ins in enumerate(self.code):
            for nm in sorted(rev.get(pc, [])):
                out.append("%s:" % nm)
            out.append("  " + _fmt(ins))
        for nm in sorted(rev.get(len(self.code), [])):
            out.append("%s:" % nm)
        return "\n".join(out) + "\n"


def _quote(bs):
    inv = {v: k for k, v in ESCAPES.items()}
    s = '"'
    for b in bs:
        c = chr(b)
        if c in inv and c != "'":
            s += "\\" + inv[c]
        elif 32 <= b < 127:
            s += c
        else:
            s += "\\x%02x" % b
    return s + '"'


def _fmt(ins):
    op, a = ins.op, ins.args
    if op in ("load64", ".ld"):
        tail = ", %d" % a[3] if op == ".ld" else ""
        return "%-7s %s, [%s%+d]%s" % (op, a[0], a[1], a[2], tail)
    if op in ("store64", ".st"):
        tail = ", %d" % a[3] if op == ".st" else ""
        return "%-7s [%s%+d], %s%s" % (op, a[0], a[1], a[2], tail)
    if op == ".zero":
        return "%-7s [%s%+d], %d" % (op, a[0], a[1], a[2])
    return ("%-7s " % op + ", ".join(str(x) for x in a)).rstrip()


# -- parsing ---------------------------------------------------------------
def _unescape(s):
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c == "\\" and i + 1 < len(s):
            n = s[i + 1]
            if n == "x":
                out.append(int(s[i + 2:i + 4], 16))
                i += 4
                continue
            out.extend(ESCAPES.get(n, n).encode("latin-1"))
            i += 2
            continue
        out.extend(c.encode("latin-1"))
        i += 1
    return bytes(out)


def _strip_comment(raw):
    """`;` starts a comment -- but not inside a string literal.  Binary data in
    a `.str` routinely contains 0x3b, and cutting there truncates the data."""
    q = False
    i = 0
    while i < len(raw):
        c = raw[i]
        if q:
            if c == "\\":
                i += 2
                continue
            if c == '"':
                q = False
        else:
            if c == '"':
                q = True
            elif c == ";":
                return raw[:i]
        i += 1
    return raw


def parse(text):
    t = Tape()
    pending = []
    for lineno, raw in enumerate(text.splitlines(), 1):
        line = _strip_comment(raw).strip()
        if not line:
            continue
        if line.startswith(".bss "):
            name, n = line[5:].split()
            t.string(name, b"\x00" * int(n), align=8)
            continue
        if line.startswith(".str "):
            rest = line[5:].strip()
            name, lit = rest.split(None, 1)
            t.string(name, _unescape(lit.strip()[1:-1]))
            continue
        if line.endswith(":") and " " not in line[:-1]:
            pending.append((line[:-1], lineno))
            t.labels[line[:-1]] = len(t.code)
            continue
        body = line.replace("[", " ").replace("]", " ")
        parts = [p for p in body.replace(",", " ").split() if p]
        op, toks = parts[0], parts[1:]
        assert op in SHAPE, "line %d: unknown op %r" % (lineno, op)
        args = []
        for kind, tok in zip(SHAPE[op], _regroup(op, toks)):
            if kind == "r":
                assert tok in REGS, "line %d: %r is not a register" % (lineno, tok)
                args.append(tok)
            elif kind in ("i",):
                args.append(int(tok, 0))
            else:
                args.append(tok)
        t.code.append(Insn(op, args, lineno))
    return t


def _regroup(op, toks):
    """[rb+K] was flattened to one token like 'r1+8' -- split it back."""
    out = []
    for tok in toks:
        if op in ("load64", "store64", ".ld", ".st", ".zero") and \
                tok[:1] == "r" and ("+" in tok[1:] or "-" in tok[1:]):
            i = max(tok.rfind("+"), tok.rfind("-"))
            out.append(tok[:i])
            out.append(tok[i:])
        else:
            out.append(tok)
    return out
