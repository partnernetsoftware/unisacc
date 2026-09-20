"""Tape emitter. [W-6]

Register convention on the tape:
    r0  accumulator / return value / arg0      r4 r5  args
    r1  binop scratch (lhs)                    r6     frame pointer
    r2  spare                                  r7     stack pointer
    r3  arg3

Every mnemonic this module emits is chosen by the irsel table -- the emitter
never hardcodes one. [G-8]
"""
from .tape import Tape

ACC, LHS, TMP, FP, SP = "r0", "r1", "r2", "r6", "r7"
# r0..r4 carry arguments; r5 is reserved as the indirect-call scratch, because
# popping a callee address into any argument register would clobber an argument
# that is already in place.  Five arguments is the documented limit.
ARGREGS = ("r0", "r1", "r2", "r3", "r4")
CALLEE = "r5"

# C operator -> (irsel family, flavor, swap operands?)
ALU = {
    "+": ("alu", "add", False), "-": ("alu", "sub", False),
    "*": ("alu", "mul", False),
    "<": ("alu", "lt", False), "<=": ("alu", "le", False),
    ">": ("alu", "gt", True), ">=": ("alu", "ge", True),
    "==": ("alu", "eq", False), "!=": ("alu", "ne", False),
    "&": ("alu", "and", False), "|": ("alu", "or", False),
    "^": ("alu", "xor", False), "<<": ("alu", "shl", False),
    ">>": ("alu", "shr", False),
}


# irsel recipe name -> tape mnemonic (builtins carry a leading dot)
RECIPE_OP = {
    "add64": "add64", "sub64": "sub64", "mul64": "mul64",
    "slt64": "slt64", "sle64": "sle64", "eq": "eq", "ne": "ne",
    "load64": "load64", "store64": "store64",
    "lea": ".lea", "ld": ".ld", "st": ".st", "zero": ".zero",
    "jump": "jump", "jumpz": "jumpz", "ret": "ret",
    "call": "call", "callpush": "call", "arg": ".arg", "frame": ".frame",
    "callr": "callr",
    "imm": "imm", "print": ".print", "write": ".write", "exit": ".exit",
    "and64": "and64", "or64": "or64", "xor64": "xor64",
    "shl64": "shl64", "shr64": "shr64",
    "ult64": "ult64", "ule64": "ule64", "lshr64": "lshr64",
}


class Emitter:
    def __init__(self, oracle):
        self.o = oracle
        self.t = Tape()
        self._n = 0
        self._strs = {}
        self.need_strlen = False
        self.need_itoa = False
        self.need_itoab = False
        # (global, literal) pairs: a global pointer initialised with a string.
        # The address is NOT written into the data -- a PIE image slides, so a
        # baked-in address is wrong at run time.  `_start` computes them
        # PC-relatively instead.  [W-13]
        self.init_ptrs = []
        self.chbuf = None

    # -- plumbing ---------------------------------------------------------
    def recipe(self, family, flavor):
        """[W-6] the one neural decision in this module."""
        r = self.o.ask("irsel", (family, flavor))
        assert r != "bad", "irsel: no recipe for %s/%s" % (family, flavor)
        return RECIPE_OP[r]

    def new_label(self, p="L"):
        self._n += 1
        return "%s%d" % (p, self._n)

    def label(self, name):
        self.t.label(name)

    def emit(self, op, *a):
        self.t.emit(op, *a)

    def intern(self, s):
        if s not in self._strs:
            name = "s%d" % len(self._strs)
            self.t.string(name, s.encode("latin-1") + b"\x00")   # C strings are NUL-terminated
            self._strs[s] = name
        return self._strs[s]

    # -- primitives -------------------------------------------------------
    def imm(self, reg, k):
        self.emit(self.recipe("lit", "imm"), reg, k)

    def push(self, reg=ACC):
        self.emit(self.recipe("call", "frame"), 8)
        self.emit(self.recipe("mem", "store"), SP, 0, reg)

    def pop(self, reg):
        self.emit(self.recipe("mem", "load"), reg, SP, 0)
        self.emit(self.recipe("call", "frame"), -8)

    def load(self, reg, base, off, width=8):
        if width == 8:
            self.emit(self.recipe("mem", "load"), reg, base, off)
        else:
            self.emit(self.recipe("mem", "ld"), reg, base, off, width)

    def store(self, base, off, reg, width=8):
        if width == 8:
            self.emit(self.recipe("mem", "store"), base, off, reg)
        else:
            self.emit(self.recipe("mem", "st"), base, off, reg, width)

    def lea(self, reg, sym):
        self.emit(self.recipe("mem", "lea"), reg, sym)

    UNS = {"<": "ult", "<=": "ule", ">": "ugt", ">=": "uge", ">>": "lshr"}

    def binop(self, op, uns=False, width=8):
        """lhs on the stack, rhs in ACC -> result in ACC."""
        fam, flav, swap = ALU[op]
        if uns and op in self.UNS:
            flav = self.UNS[op]
        self.pop(LHS)
        self.narrow_pair(uns, width)
        mn = self.recipe(fam, flav)
        if swap:
            self.emit(mn, ACC, ACC, LHS)
        else:
            self.emit(mn, ACC, LHS, ACC)

    def bits_get(self, bitoff, width, signed, unit):
        """ACC holds the address of a bit-field's storage unit; leave the
        field's value in ACC.

        Shift the field up to the top of the word and back down again: that
        puts its own top bit where an arithmetic shift will copy it, which is
        the whole of the sign question.  A bit-field has no address, so this
        is the only way to read one."""
        self.load(ACC, ACC, 0, unit)
        self._bits_extract(bitoff, width, signed)

    def _bits_extract(self, bitoff, width, signed):
        up = 64 - bitoff - width
        if up:
            self.imm(LHS, up)
            self.emit(self.recipe("alu", "shl"), ACC, ACC, LHS)
        down = 64 - width
        if down:
            self.imm(LHS, down)
            self.emit(self.recipe("alu", "shr" if signed else "lshr"),
                      ACC, ACC, LHS)

    def bits_set(self, bitoff, width, signed, unit):
        """Address on the stack, new value in ACC.  Read, modify, write --
        and leave the field's value, which is the value of the assignment
        and is NOT the value assigned when it does not fit."""
        mask = ((1 << width) - 1) << bitoff
        if bitoff:
            self.imm(LHS, bitoff)
            self.emit(self.recipe("alu", "shl"), ACC, ACC, LHS)
        self.imm(LHS, mask)
        self.emit(self.recipe("alu", "and"), ACC, ACC, LHS)
        self.pop(LHS)                             # the address
        self.push(LHS)
        self.load(TMP, LHS, 0, unit)
        self.imm(LHS, ~mask & ((1 << (unit * 8)) - 1))
        self.emit(self.recipe("alu", "and"), TMP, TMP, LHS)
        self.emit(self.recipe("alu", "or"), ACC, ACC, TMP)
        self.pop(LHS)
        self.store(LHS, 0, ACC, unit)
        self._bits_extract(bitoff, width, signed)

    def blockcopy(self, dst, src, n):
        """Copy `n` bytes from [src] to [dst].  The size is a compile-time
        constant, so this unrolls instead of calling a helper."""
        off = 0
        while off < n:
            w = 8 if n - off >= 8 else (4 if n - off >= 4 else
                                        (2 if n - off >= 2 else 1))
            self.load(TMP, src, off, w)
            self.store(dst, off, TMP, w)
            off += w

    def zext(self, width):
        """Clear the bits above `width` bytes.  A narrow load sign-extends --
        which is right for `char` and wrong for `unsigned char` -- and a cast
        to an unsigned type has to zero the top, not copy the sign."""
        if width >= 8:
            return
        self.imm(LHS, (1 << (width * 8)) - 1)
        self.emit(self.recipe("alu", "and"), ACC, ACC, LHS)

    def narrow_pair(self, uns, width):
        """Convert both operands to the common type before operating.  For
        `unsigned int` that means masking to 32 bits: C says the operands are
        converted, and without it `(int)-1 != 0xffffffffu` comes out true."""
        if not uns or width >= 8:
            return
        m = (1 << (width * 8)) - 1
        self.imm(TMP, m)
        self.emit(self.recipe("alu", "and"), ACC, ACC, TMP)
        self.emit(self.recipe("alu", "and"), LHS, LHS, TMP)

    def divmod_(self, op, uns=False, width=8):
        self.pop(LHS)
        self.narrow_pair(uns, width)
        mn = (".div" if op == "/" else ".mod")
        self.emit(("." + "u" + mn[1:]) if uns else mn, ACC, LHS, ACC)

    def neg(self):
        self.emit(self.recipe("lit", "imm"), LHS, 0)
        self.emit(self.recipe("alu", "neg"), ACC, LHS, ACC)

    def bitnot(self):
        """~x is x ^ -1; no new tape op needed."""
        self.emit(self.recipe("lit", "imm"), LHS, -1)
        self.emit(self.recipe("alu", "xor"), ACC, ACC, LHS)

    def logical_not(self):
        self.imm(LHS, 0)
        self.emit(self.recipe("alu", "eq"), ACC, ACC, LHS)

    def jump(self, l):
        self.emit(self.recipe("ctrl", "jump"), l)

    def jumpz(self, l, reg=ACC):
        self.emit(self.recipe("ctrl", "jumpz"), reg, l)

    def call(self, l):
        self.emit(self.recipe("call", "call"), l)

    def call_reg(self, reg=ACC):
        self.emit(self.recipe("call", "callr"), reg)

    def ret(self):
        self.emit(self.recipe("ctrl", "ret"))

    def frame(self, n):
        self.emit(self.recipe("call", "frame"), n)

    def arg(self, i, reg):
        self.emit(self.recipe("call", "arg"), i, reg)

    # -- function shell ---------------------------------------------------
    def prologue(self, name, nlocals=0):
        """Returns the index of the locals `.frame` so one-pass code can patch
        the frame size once the body has been walked."""
        self.label(name)
        self.frame(8)
        self.store(SP, 0, FP)
        self.emit("mov", FP, SP)
        self.frame(nlocals)
        return len(self.t.code) - 1

    def epilogue(self):
        self.emit("mov", SP, FP)
        self.load(FP, SP, 0)
        self.frame(-8)
        self.ret()

    # -- printf -----------------------------------------------------------
    def write_literal(self, s):
        if not s:
            return
        self.lea(ACC, self.intern(s))
        self.imm(LHS, len(s.encode("latin-1")))
        self.emit(self.recipe("lit", "write"), ACC, LHS)

    def print_int(self, reg=ACC):
        """Desugared into the emitted __itoa helper plus a write, so it lowers
        to ordinary tape ops and therefore to real machine code.  `.print` stays
        in the tape ISA for hand-written tapes; generated code no longer uses
        it."""
        self.need_itoa = True
        if reg != ACC:
            self.emit("mov", ACC, reg)
        self.call("__itoa")                 # -> ACC = ptr, LHS = len
        self.emit(self.recipe("lit", "write"), ACC, LHS)

    def print_str(self, reg=ACC):
        """ACC holds char*; length via the emitted __strlen helper."""
        self.need_strlen = True
        self.push(reg)
        self.call("__strlen")
        self.emit("mov", LHS, ACC)
        self.pop(ACC)
        self.emit(self.recipe("lit", "write"), ACC, LHS)

    def print_char(self, reg=ACC):
        if self.chbuf is None:
            self.chbuf = "__chbuf"
            self.t.string(self.chbuf, b"\x00")
        self.emit("mov", TMP, reg)
        self.lea(ACC, self.chbuf)
        self.store(ACC, 0, TMP, 1)
        self.imm(LHS, 1)
        self.emit(self.recipe("lit", "write"), ACC, LHS)

    PADMAX = 64

    def _padstr(self, ch):
        name = "__pad%d" % ch
        self.t.string(name, bytes([ch]) * self.PADMAX)
        return name

    def _clamp_hi(self, lim):
        """ACC = min(ACC, lim), branch free: min(a,b) = b + ((a-b) & (a-b)>>63)"""
        self.imm(TMP, lim)
        self.emit("sub64", ACC, ACC, TMP)            # a - b
        self.emit("mov", LHS, ACC)
        self.imm(TMP, 63)
        self.emit("shr64", LHS, LHS, TMP)            # arithmetic: 0 or -1
        self.emit(self.recipe("alu", "and"), ACC, ACC, LHS)
        self.imm(TMP, lim)
        self.emit("add64", ACC, ACC, TMP)

    def _clamp_lo0(self):
        """ACC = max(ACC, 0), branch free: a & ~(a >> 63)"""
        self.emit("mov", LHS, ACC)
        self.imm(TMP, 63)
        self.emit("shr64", LHS, LHS, TMP)
        self.imm(TMP, -1)
        self.emit("xor64", LHS, LHS, TMP)
        self.emit(self.recipe("alu", "and"), ACC, ACC, LHS)

    def print_field(self, kind, width=0, left=False, zero=False, prec=None):
        """printf with a field width.  The value is already in ACC.

        Padding is a write from a static run of 64 identical bytes, with the
        length computed at run time and clamped to zero -- a write of length 0
        is a no-op, so the whole thing is branch free."""
        if kind == "int":
            self.need_itoa = True
            self.call("__itoa")                      # ACC = ptr, LHS = len
        elif kind in ("hex", "HEX", "oct"):
            self.need_itoab = True
            self.imm(LHS, 8 if kind == "oct" else 16)
            self.imm(TMP, 65 if kind == "HEX" else 97)
            self.call("__itoab")
        elif kind == "str":
            self.need_strlen = True
            self.push(ACC)
            self.call("__strlen")
            if prec is not None:
                self._clamp_hi(prec)
            self.emit("mov", LHS, ACC)
            self.pop(ACC)
        else:                                        # a single character
            if self.chbuf is None:
                self.chbuf = "__chbuf"
                self.t.string(self.chbuf, b"\x00")
            self.emit("mov", TMP, ACC)
            self.lea(ACC, self.chbuf)
            self.store(ACC, 0, TMP, 1)
            self.imm(LHS, 1)
        if width <= 0:
            self.emit(self.recipe("lit", "write"), ACC, LHS)
            return
        width = min(width, self.PADMAX)
        pad = self._padstr(48 if zero and kind != "str" else 32)
        self.push(ACC)                               # [ptr]
        self.push(LHS)                               # [ptr][len]
        if left:
            self.pop(LHS)
            self.pop(ACC)
            self.push(LHS)                           # a write CLOBBERS r0-r2
            self.emit(self.recipe("lit", "write"), ACC, LHS)
            self.pop(LHS)                            # so `len` comes back off
            self.imm(ACC, width)                     # the stack, not a register
            self.emit("sub64", ACC, ACC, LHS)
        else:
            self.imm(ACC, width)
            self.pop(LHS)                            # len; stack [ptr]
            self.emit("sub64", ACC, ACC, LHS)        # pad = width - len
            self.push(LHS)                           # [ptr][len]
        self._clamp_lo0()
        self.emit("mov", LHS, ACC)
        self.lea(ACC, pad)
        self.emit(self.recipe("lit", "write"), ACC, LHS)
        if not left:
            self.pop(LHS)
            self.pop(ACC)
            self.emit(self.recipe("lit", "write"), ACC, LHS)

    def truncate(self, width):
        """Narrow ACC to `width` bytes with sign extension, through a stack
        slot -- the tape has sized load/store, so no new op is needed."""
        if width >= 8:
            return
        self.frame(8)
        self.store(SP, 0, ACC, width)
        self.load(ACC, SP, 0, width)
        self.frame(-8)

    def mask32(self):
        """%u: keep the low 32 bits, unsigned."""
        self.imm(TMP, 0xFFFFFFFF)
        self.emit(self.recipe("alu", "and"), ACC, ACC, TMP)

    def exit_(self, reg=ACC):
        self.emit(self.recipe("lit", "exit"), reg)

    def finish(self):
        if self.need_strlen:
            self._emit_strlen()
        if self.need_itoa:
            self._emit_itoa()
        if self.need_itoab:
            self._emit_itoab()
        return self.t

    def _emit_itoab(self):
        """uint64 -> text in base LHS, letters starting at TMP ('a' or 'A').
        ACC in; ACC = ptr and LHS = len out.  Scratch is r3-r5 and two stack
        slots -- r6 and r7 are FP and SP and must not be touched."""
        self.t.string("__xbuf", b"\x00" * 24, align=8)
        loop, alpha, add, done = ("itoab_loop", "itoab_alpha",
                                  "itoab_add", "itoab_done")
        self.label("__itoab")
        self.frame(16)
        self.store(SP, 0, LHS)                       # base
        self.store(SP, 8, TMP)                       # letter base
        self.emit("mov", TMP, ACC)                   # n
        self.lea(LHS, "__xbuf")
        self.imm("r3", 24)
        self.emit("add64", LHS, LHS, "r3")           # p = buf + 24
        self.imm("r4", 0)                            # len
        self.label(loop)
        self.load("r3", SP, 0)
        self.emit(".umod", "r5", TMP, "r3")
        self.emit(".udiv", TMP, TMP, "r3")
        self.imm("r3", 10)
        self.emit("slt64", ACC, "r5", "r3")          # digit < 10 ?
        self.jumpz(alpha, ACC)
        self.imm("r3", 48)                           # '0'
        self.jump(add)
        self.label(alpha)
        self.load("r3", SP, 8)
        self.imm(ACC, 10)
        self.emit("sub64", "r5", "r5", ACC)
        self.label(add)
        self.emit("add64", "r5", "r5", "r3")
        self.imm("r3", 1)
        self.emit("sub64", LHS, LHS, "r3")
        self.store(LHS, 0, "r5", 1)
        self.emit("add64", "r4", "r4", "r3")
        self.jumpz(done, TMP)
        self.jump(loop)
        self.label(done)
        self.frame(-16)
        self.emit("mov", ACC, LHS)
        self.emit("mov", LHS, "r4")
        self.ret()

    def _emit_itoa(self):
        """int64 -> decimal text.  ACC in; ACC = ptr, LHS = len out."""
        self.t.string("__ibuf", b"\x00" * 24, align=8)
        pos, loop, neg, done = ("itoa_pos", "itoa_loop", "itoa_neg", "itoa_done")
        self.label("__itoa")
        self.frame(8)
        self.imm("r5", 0)
        self.emit("slt64", "r5", ACC, "r5")          # value < 0 ?
        self.store(SP, 0, "r5")
        self.jumpz(pos, "r5")
        self.imm("r3", 0)
        self.emit("sub64", ACC, "r3", ACC)           # value = -value
        self.label(pos)
        self.emit("mov", TMP, ACC)                   # n
        self.lea(LHS, "__ibuf")
        self.imm("r3", 24)
        self.emit("add64", LHS, LHS, "r3")           # p = buf + 24
        self.imm("r4", 0)                            # len
        self.label(loop)
        self.imm("r3", 10)
        self.emit(".mod", "r5", TMP, "r3")
        self.emit(".div", TMP, TMP, "r3")
        self.imm("r3", 48)
        self.emit("add64", "r5", "r5", "r3")         # '0' + digit
        self.imm("r3", 1)
        self.emit("sub64", LHS, LHS, "r3")
        self.store(LHS, 0, "r5", 1)
        self.emit("add64", "r4", "r4", "r3")
        self.jumpz(neg, TMP)
        self.jump(loop)
        self.label(neg)
        self.load("r5", SP, 0)
        self.jumpz(done, "r5")
        self.imm("r3", 45)                           # '-'
        self.imm("r5", 1)
        self.emit("sub64", LHS, LHS, "r5")
        self.store(LHS, 0, "r3", 1)
        self.emit("add64", "r4", "r4", "r5")
        self.label(done)
        self.frame(-8)
        self.emit("mov", ACC, LHS)
        self.emit("mov", LHS, "r4")
        self.ret()

    def _emit_strlen(self):
        top, end = "__strlen_top", "__strlen_end"
        self.label("__strlen")
        self.emit("mov", TMP, ACC)
        self.imm(LHS, 0)
        self.label(top)
        self.emit("add64", "r4", TMP, LHS)
        self.emit(".ld", "r5", "r4", 0, 1)
        self.jumpz(end, "r5")
        self.imm("r5", 1)
        self.emit("add64", LHS, LHS, "r5")
        self.jump(top)
        self.label(end)
        self.emit("mov", ACC, LHS)
        self.ret()
