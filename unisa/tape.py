"""Tape: the target-independent instruction stream. [TP]

Line oriented text.  Labels `L:`.  r0-r7, r7 = SP starting at 0x10000.
64 KB little-endian memory.  Arithmetic wraps mod 2^64, signed two's complement.

Calling convention (tape level): args in r0..r5, return value in r0,
return address pushed on the stack by `call`.

`.str NAME "..."` is an assembler directive, not an instruction: it reserves
bytes in the data area and binds NAME for `.lea`.
"""

from .padded import ZData
import re

REGS = tuple("r%d" % i for i in range(8))
SP = 7
# 64 KB was fine for the examples; a self-hosting compiler needs room for its
# tables, so the interpreter's memory is 16 MB.  Native code is not affected --
# there the stack is the real one and globals live in __DATA.
MEM_SIZE = 0x1000000
STACK_TOP = 0x1000000
DATA_BASE = 0x100

# op -> operand shape.  r=register  i=immediate  L=label  s=symbol-or-imm
LINK_RECORDS = {"global": 4, "extern": 5, "gdef": 7}
UNIT_RECORD = 6

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
    "lshr64":  ("r", "r", "r"),
    ".div":    ("r", "r", "r"),
    ".mod":    ("r", "r", "r"),
    ".udiv":   ("r", "r", "r"),
    ".umod":   ("r", "r", "r"),
    "slt64":   ("r", "r", "r"),
    "sle64":   ("r", "r", "r"),
    "ult64":   ("r", "r", "r"),
    "ule64":   ("r", "r", "r"),
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
    "callm":   ("r", "i"),  # target at [base+offset]; lower uses an ISA scratch
    ".hostcall": ("r", "r"), # native fixed-six integer/pointer ABI bridge
    ".librarycall": ("r", "r"), # model-declared injected library function, gated by resources
    ".libraryaddr": ("r", "s", "i", "i", "i", "i"), # declared borrowed data address and canonical ABI facts
    ".hostaddr": ("r", "i"), # one of four dynamic-loader bootstrap slots
    "ret":     (),
    ".frame":  ("i",),
    ".arg":    ("i", "r"),
    ".print":  ("r",),
    ".write":  ("r", "r"),
    ".exit":   ("r",),
    ".sys":    ("s", "r", "r", "r"),   # catalog op name + 3 args -> r0
    ".sys6":   ("s", "r", "r", "r", "r", "r", "r"),   # ...and six, for mmap
    ".argc":   ("r",),                 # argument count
    ".argv":   ("r", "r"),             # rd = argv[rs], a char*
    "nop":     (),
}
# floating point [TP]: see fp.py for what each one means
from .fp import OPS3 as _FOPS3, OPS2 as _FOPS2
SHAPE.update({op: ("r", "r", "r") for op in _FOPS3})
SHAPE.update({op: ("r", "r") for op in _FOPS2})

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
        self.records = []       # source-order semantic records, including data definitions
        self.data = ZData()     # packed literals, based at DATA_BASE; zero runs are lengths (R3')
        self.syms = {}          # name -> address
        # Offsets in `data` that hold an absolute data address (a global
        # pointer initialised with a string literal).  The interpreter uses
        # them as-is; an image must relocate them onto its own data segment.
        self.relocs = []

    # -- building ---------------------------------------------------------
    def label(self, name):
        self.labels[name] = len(self.code)
        self.records.append(("label", name))

    def emit(self, op, *args):
        assert op in SHAPE, "unknown tape op %r" % op
        exp = len(SHAPE[op])
        assert len(args) == exp, "%s takes %d operands, got %d" % (op, exp, len(args))
        self.code.append(Insn(op, list(args)))
        self.records.append(("insn", op, tuple(args)))
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
        if len(raw) > 16 and not any(raw):
            self.records.append(("bss", name, len(raw)))
        else:
            self.records.append(("str", name, bytes(raw)))
        return addr

    # -- text -------------------------------------------------------------
    def to_text(self):
        out = []
        for name, addr in sorted(self.syms.items(), key=lambda kv: kv[1]):
            end = addr - DATA_BASE
            nxt = min([a - DATA_BASE for a in self.syms.values()
                       if a - DATA_BASE > end] + [len(self.data)])
            # a large zero-filled global is `.bss`, not a quoted literal: a
            # 64 KB arena would otherwise serialise as 64 KB of "\0"
            if nxt - end > 16 and self.data.count(0, end, nxt) == nxt - end:
                out.append(".bss %s %d" % (name, nxt - end))
            else:
                out.append(".str %s %s" % (name, _quote(self.data[end:nxt])))
        rev = {}
        for name, pc in self.labels.items():
            rev.setdefault(pc, []).append(name)
        for pc, ins in enumerate(self.code):
            for nm in sorted(rev.get(pc, [])):
                out.append("%s:" % nm)
            out.append("  " + _fmt(ins))
        for nm in sorted(rev.get(len(self.code), [])):
            out.append("%s:" % nm)
        if any(rec[0] in LINK_RECORDS or rec[0] == "unit" for rec in self.records):
            return self.to_canonical_text()
        return "\n".join(out) + "\n"

    def to_canonical_text(self):
        """Print semantic records in their original order, without text style data."""
        out = []
        for record in self.records:
            kind = record[0]
            if kind == "unit":
                out.append(".unit %d" % record[1])
            elif kind in LINK_RECORDS:
                out.append(".%s %s" % (kind, record[1]))
            elif kind == "label":
                out.append(record[1] + ":")
            elif kind == "str":
                out.append(".str %s %s" % (record[1], _quote(record[2])))
            elif kind == "bss":
                out.append(".bss %s %d" % (record[1], record[2]))
            else:
                out.append("  " + _fmt(Insn(record[1], record[2])))
        return "\n".join(out) + ("\n" if out else "")


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
    if op == "callm":
        return "%-7s [%s%+d]" % (op, a[0], a[1])
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
        if len(out) >= 1048576:
            raise ValueError("tape: .str exceeds tape reader capacity")
        c = s[i]
        if c == "\\" and i + 1 < len(s):
            n = s[i + 1]
            if n == "x":
                if i + 3 >= len(s) or any(c not in "0123456789abcdefABCDEF"
                                           for c in s[i + 2:i + 4]):
                    raise ValueError("tape: malformed hex escape")
                out.append(int(s[i + 2:i + 4], 16))
                i += 4
                continue
            out.extend(ESCAPES.get(n, n).encode("latin-1"))
            i += 2
            continue
        if c == "\\":
            raise ValueError("tape: trailing string escape")
        if c == '"':
            raise ValueError("tape: unescaped quote in .str")
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
            before = len(t.records)
            t.string(name, bytes(int(n)), align=8)   # calloc: pages untouched
            record = ("bss", name, int(n))
            if len(t.records) == before:
                t.records.append(record)
            else:
                t.records[-1] = record
            continue
        if line.split()[0] == ".unit":
            if line.split() != [".unit", "2"] or t.records:
                raise ValueError("line %d: malformed unit version record" % lineno)
            t.records.append(("unit", 2))
            continue
        if line.split()[0] in (".global", ".extern", ".gdef"):
            parts = line.split()
            if len(parts) != 2 or not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9.$]*", parts[1]):
                raise ValueError("line %d: malformed linkage record" % lineno)
            if parts[0] == ".gdef" and (not t.records or t.records[0] != ("unit", 2)):
                raise ValueError("line %d: .gdef needs .unit 2" % lineno)
            t.records.append((parts[0][1:], parts[1]))
            continue
        if line.startswith(".str "):
            rest = line[5:].strip()
            name, lit = rest.split(None, 1)
            lit = lit.strip()
            if len(lit) < 2 or lit[0] != '"' or lit[-1] != '"':
                raise ValueError("line %d: malformed .str literal" % lineno)
            blob = _unescape(lit[1:-1])
            before = len(t.records)
            t.string(name, blob)
            record = ("str", name, blob)
            if len(t.records) == before:
                t.records.append(record)
            else:
                t.records[-1] = record
            continue
        if line.endswith(":") and " " not in line[:-1]:
            pending.append((line[:-1], lineno))
            t.labels[line[:-1]] = len(t.code)
            t.records.append(("label", line[:-1]))
            continue
        body = line.replace("[", " ").replace("]", " ")
        parts = [p for p in body.replace(",", " ").split() if p]
        op, toks = parts[0], parts[1:]
        assert op in SHAPE, "line %d: unknown op %r" % (lineno, op)
        grouped = _regroup(op, toks)
        if len(grouped) != len(SHAPE[op]):
            raise ValueError("line %d: wrong operand count for %s" % (lineno, op))
        args = []
        for kind, tok in zip(SHAPE[op], grouped):
            if kind == "r":
                assert tok in REGS, "line %d: %r is not a register" % (lineno, tok)
                args.append(tok)
            elif kind in ("i",):
                args.append(int(tok, 0))
            else:
                args.append(tok)
        t.code.append(Insn(op, args, lineno))
        t.records.append(("insn", op, tuple(args)))
    return t


def _regroup(op, toks):
    """[rb+K] was flattened to one token like 'r1+8' -- split it back."""
    out = []
    for tok in toks:
        if op in ("load64", "store64", ".ld", ".st", ".zero", "callm") and \
                tok[:1] == "r" and ("+" in tok[1:] or "-" in tok[1:]):
            i = max(tok.rfind("+"), tok.rfind("-"))
            out.append(tok[:i])
            out.append(tok[i:])
        else:
            out.append(tok)
    return out
