"""Generic tape interpreter -- the reference for --fold. [TP-4] [A-2]

Target-independent on purpose: it knows nothing about syscall numbers, ABIs or
instruction encodings.  Everything target-specific lives in lower.py and
exec_target.py, and the fold compares those six results against this one.
"""
import os as _os

from .tape import REGS, SP, MEM_SIZE, STACK_TOP, DATA_BASE

MASK = (1 << 64) - 1
SIGN = 1 << 63
MAX_STEPS = 4_000_000_000   # the VM is the reference, not a speed target


class Halt(Exception):
    def __init__(self, code):
        self.code = code


def s64(x):
    return x - (1 << 64) if x & SIGN else x


def u64(x):
    return x & MASK


# O_CREAT, O_TRUNC, O_APPEND.  These are not portable numbers -- Linux and
# the BSDs chose different bits -- and include/stdio.h picks the target's, so
# an interpreter running on some other host has to translate them back.
_OFLAGS = {"lnx": (64, 512, 1024), "osx": (512, 1024, 8)}


def open_args(os_, a1, a2):
    """(flags, mode) for THIS host, from what the target's C library passed.

    On Windows there is no open(2) at all: the gate is CreateFileA, so what
    arrives is a desired access and a creation disposition, not POSIX flags."""
    if os_ == "win":
        if not (a1 & 0x40000000):                    # not GENERIC_WRITE
            return _os.O_RDONLY, 0o644
        f = _os.O_WRONLY | _os.O_CREAT
        f |= _os.O_TRUNC if a2 == 2 else _os.O_APPEND if a2 == 4 else 0
        return f, 0o644
    cr, tr, ap = _OFLAGS[os_]
    f = a1 & 3
    if a1 & cr:
        f |= _os.O_CREAT
    if a1 & tr:
        f |= _os.O_TRUNC
    if a1 & ap:
        f |= _os.O_APPEND
    return f, (a2 or 0o644)


class VM:
    def __init__(self, tape, max_steps=MAX_STEPS, argv=None, os_=None):
        self.os = os_ or getattr(tape, "src_os", "lnx")
        self.t = tape
        self.max_steps = max_steps
        self.mem = bytearray(MEM_SIZE)
        self.mem[DATA_BASE:DATA_BASE + len(tape.data)] = tape.data
        # argv lives just above the program's data, NUL terminated
        self.argv = []
        p = DATA_BASE + len(tape.data) + 64
        for a in (argv or []):
            b = a.encode("latin-1") + b"\x00"
            self.mem[p:p + len(b)] = b
            self.argv.append(p)
            p += len(b)
        self.r = [0] * 8
        self.r[SP] = STACK_TOP
        self.out = bytearray()
        self.steps = 0
        self.fault = None
        self.pc_now = 0

    # -- memory ----------------------------------------------------------
    def _addr(self, base, off):
        a = (self.r[base] + off) & MASK
        if a >= MEM_SIZE:
            self.fault = (a, self.pc_now, self.steps)
            raise Halt(139)          # segv-ish
        return a

    def ld(self, a, w, signed=True):
        v = int.from_bytes(self.mem[a:a + w], "little")
        if signed and w < 8 and (v >> (w * 8 - 1)) & 1:
            v -= 1 << (w * 8)
        return u64(v)

    def st(self, a, v, w):
        self.mem[a:a + w] = (v & ((1 << (w * 8)) - 1)).to_bytes(w, "little")

    # -- the OS ----------------------------------------------------------
    def cstr(self, p):
        e = self.mem.find(b"\x00", p)
        return bytes(self.mem[p:e if e >= 0 else len(self.mem)])

    def syscall(self, name, a0, a1, a2):
        """[TP-5] the handful of syscalls a self-hosting compiler needs."""
        if name == "write":
            if a0 == 1 or a0 == 2:
                self.out.extend(self.mem[a1:a1 + a2])
                return a2
            try:
                return _os.write(a0, bytes(self.mem[a1:a1 + a2]))
            except OSError:
                return u64(-1)
        if name == "read":
            try:
                d = _os.read(a0, a2)
            except OSError:
                return u64(-1)
            self.mem[a1:a1 + len(d)] = d
            return len(d)
        if name == "open":
            try:
                fl, md = open_args(self.os, a1, a2)
                return _os.open(self.cstr(a0).decode(), fl, md)
            except OSError:
                return u64(-1)
        if name == "close":
            try:
                _os.close(a0)
                return 0
            except OSError:
                return u64(-1)
        if name == "exit":
            raise Halt(a0 & 0xFF)
        raise AssertionError("vm: unsupported syscall %r" % name)

    # -- run -------------------------------------------------------------
    POISON = 0xDEAD5EA1DEAD5EA1

    def _gate_clobber(self, r, ret):
        """A gate is a syscall on a real machine, and lowering puts its
        arguments in r0-r2 -- so those registers do NOT survive it.  The
        interpreter used to leave them alone, which let generated code keep a
        value in r1 across a `.write` and work here while failing natively.
        Poison them so the interpreter is no more forgiving than the CPU.
        [TP-6]"""
        r[0] = u64(ret)
        r[1] = self.POISON
        r[2] = self.POISON

    def run(self):
        t, r = self.t, self.r
        code, labels = t.code, t.labels
        pc = labels.get("_start", labels.get("main", 0))
        ri = {name: i for i, name in enumerate(REGS)}
        try:
            while True:
                self.steps += 1
                if self.steps > self.max_steps:
                    raise Halt(124)               # timeout
                if pc >= len(code):
                    raise Halt(u64(r[0]) & 0xFF)
                ins = code[pc]
                self.pc_now = pc
                op, a = ins.op, ins.args
                pc += 1

                if op == "imm":
                    r[ri[a[0]]] = u64(a[1])
                elif op == "mov":
                    r[ri[a[0]]] = r[ri[a[1]]]
                elif op == "add64":
                    r[ri[a[0]]] = u64(r[ri[a[1]]] + r[ri[a[2]]])
                elif op == "sub64":
                    r[ri[a[0]]] = u64(r[ri[a[1]]] - r[ri[a[2]]])
                elif op == "mul64":
                    r[ri[a[0]]] = u64(r[ri[a[1]]] * r[ri[a[2]]])
                elif op == "and64":
                    r[ri[a[0]]] = r[ri[a[1]]] & r[ri[a[2]]]
                elif op == "or64":
                    r[ri[a[0]]] = r[ri[a[1]]] | r[ri[a[2]]]
                elif op == "shl64":
                    r[ri[a[0]]] = u64(r[ri[a[1]]] << (r[ri[a[2]]] & 63))
                elif op == "shr64":
                    r[ri[a[0]]] = u64(s64(r[ri[a[1]]]) >> (r[ri[a[2]]] & 63))
                elif op == "lshr64":              # unsigned: no sign to keep
                    r[ri[a[0]]] = r[ri[a[1]]] >> (r[ri[a[2]]] & 63)
                elif op == "xor64":
                    r[ri[a[0]]] = r[ri[a[1]]] ^ r[ri[a[2]]]
                elif op == ".udiv" or op == ".umod":
                    x, y = r[ri[a[1]]], r[ri[a[2]]]
                    if y == 0:
                        raise Halt(136)
                    q = x // y
                    r[ri[a[0]]] = u64(q if op == ".udiv" else x - q * y)
                elif op == ".div" or op == ".mod":
                    x, y = s64(r[ri[a[1]]]), s64(r[ri[a[2]]])
                    if y == 0:
                        raise Halt(136)           # [TP] div by zero
                    q = abs(x) // abs(y)
                    if (x < 0) != (y < 0):
                        q = -q
                    r[ri[a[0]]] = u64(q if op == ".div" else x - q * y)
                elif op == "slt64":
                    r[ri[a[0]]] = 1 if s64(r[ri[a[1]]]) < s64(r[ri[a[2]]]) else 0
                elif op == "sle64":
                    r[ri[a[0]]] = 1 if s64(r[ri[a[1]]]) <= s64(r[ri[a[2]]]) else 0
                elif op == "ult64":
                    r[ri[a[0]]] = 1 if r[ri[a[1]]] < r[ri[a[2]]] else 0
                elif op == "ule64":
                    r[ri[a[0]]] = 1 if r[ri[a[1]]] <= r[ri[a[2]]] else 0
                elif op == "eq":
                    r[ri[a[0]]] = 1 if r[ri[a[1]]] == r[ri[a[2]]] else 0
                elif op == "ne":
                    r[ri[a[0]]] = 1 if r[ri[a[1]]] != r[ri[a[2]]] else 0
                elif op == "load64":
                    r[ri[a[0]]] = self.ld(self._addr(ri[a[1]], a[2]), 8)
                elif op == "store64":
                    self.st(self._addr(ri[a[0]], a[1]), r[ri[a[2]]], 8)
                elif op == ".ld":
                    r[ri[a[0]]] = self.ld(self._addr(ri[a[1]], a[2]), a[3])
                elif op == ".st":
                    self.st(self._addr(ri[a[0]], a[1]), r[ri[a[2]]], a[3])
                elif op == ".lea":
                    s = a[1]
                    if s in t.syms:
                        v = t.syms[s]
                    elif s in labels:         # a code label: its address
                        v = labels[s]
                    else:
                        v = int(s, 0)
                    r[ri[a[0]]] = u64(v)
                elif op == ".zero":
                    ad = self._addr(ri[a[0]], a[1])
                    self.mem[ad:ad + a[2]] = b"\x00" * a[2]
                elif op == "jump":
                    pc = labels[a[0]]
                elif op == "jumpz":
                    if r[ri[a[0]]] == 0:
                        pc = labels[a[1]]
                elif op == "call":
                    r[SP] = u64(r[SP] - 8)
                    self.st(r[SP], pc, 8)
                    pc = labels[a[0]]
                elif op == "callr":               # indirect call
                    r[SP] = u64(r[SP] - 8)
                    self.st(r[SP], pc, 8)
                    pc = r[ri[a[0]]]
                elif op == "ret":
                    pc = self.ld(r[SP], 8)
                    r[SP] = u64(r[SP] + 8)
                elif op == ".frame":
                    r[SP] = u64(r[SP] - a[0])
                elif op == ".arg":
                    r[a[0]] = r[ri[a[1]]]
                elif op == ".print":
                    self.out.extend(str(s64(r[ri[a[0]]])).encode())
                    self._gate_clobber(r, 0)
                elif op == ".write":
                    p, n = r[ri[a[0]]], r[ri[a[1]]]
                    self.out.extend(self.mem[p:p + n])
                    self._gate_clobber(r, n)
                elif op == ".argc":
                    r[ri[a[0]]] = len(self.argv)
                elif op == ".argv":
                    k = r[ri[a[1]]]
                    r[ri[a[0]]] = self.argv[k] if k < len(self.argv) else 0
                elif op == ".sys":
                    v = u64(self.syscall(a[0], r[ri[a[1]]],
                                         r[ri[a[2]]], r[ri[a[3]]]))
                    self._gate_clobber(r, v)
                elif op == ".exit":
                    raise Halt(u64(r[ri[a[0]]]) & 0xFF)
                elif op == "nop":
                    pass
                else:
                    raise AssertionError("vm: unhandled %r" % op)
        except Halt as h:
            if self.fault and h.code == 139:
                import sys as _s
                print("vm fault: address %#x at pc %d (%s) after %d steps"
                      % (self.fault[0], self.fault[1],
                         self.t.code[self.fault[1]], self.fault[2]),
                      file=_s.stderr)
            return bytes(self.out), h.code, self.steps


def run(tape, max_steps=MAX_STEPS, argv=None, os_=None):
    return VM(tape, max_steps, argv, os_).run()
