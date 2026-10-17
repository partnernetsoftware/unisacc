"""Generic tape interpreter -- the reference for --fold. [TP-4] [A-2]

Target-independent on purpose: it knows nothing about syscall numbers, ABIs or
instruction encodings.  Everything target-specific lives in lower.py and
exec_target.py, and the fold compares those six results against this one.
"""
from .bits import MASK64
from . import fp
from .fp import OPS3 as FOPS3, OPS2 as FOPS2
from .padded import copy_data
import os as _os
import struct as _struct
_Q = _struct.Struct('<Q')

from .tape import REGS, SP, MEM_SIZE, STACK_TOP, DATA_BASE

MASK = MASK64
SIGN = 1 << 63
MAX_STEPS = 4_000_000_000   # the VM is the reference, not a speed target


class Halt(Exception):
    def __init__(self, code):
        self.code = code


_RI = {name: i for i, name in enumerate(REGS)}
_BIN = {'add64': 3, 'sub64': 4, 'mul64': 5, 'and64': 6, 'or64': 7, 'xor64': 8, 'shl64': 9,
        'shr64': 10, 'lshr64': 11, 'eq': 12, 'ne': 13, 'ult64': 14, 'ule64': 15, 'slt64': 16, 'sle64': 17}


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
        self._brk = MEM_SIZE // 2        # where mmap hands out pages
        copy_data(self.mem, DATA_BASE, tape.data)
        # argv lives just above the program's data, NUL terminated
        self.argv = []
        p = DATA_BASE + len(tape.data) + 64
        for a in (argv or []):
            b = a.encode("latin-1") + b"\x00"
            self.mem[p:p + len(b)] = b
            self.argv.append(p)
            p += len(b)
        # ...then a NULL and the environment, where a Unix kernel puts envp:
        # getenv walks `.argv` past argc [S-15 D2]
        self.nargs = len(self.argv)
        self.argv.append(0)
        room = 65536
        for k, v in _os.environ.items():
            b = ("%s=%s" % (k, v)).encode("latin-1", "replace") + b"\x00"
            if len(b) > room:
                break
            room -= len(b)
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

    def syscall(self, name, a0, a1, a2, a3=0, a4=0, a5=0):
        """[TP-5] the handful of syscalls a self-hosting compiler needs.

        `mmap` here is a bump allocation inside the machine's own memory and
        `mprotect` is a no-op: this model has one flat address space and no
        protection, which is enough to RUN what a program maps -- executing
        it needs a real kernel, and that is what the target images do."""
        if name == "mmap":
            n = (a1 + 0xFFF) & ~0xFFF
            p = self._brk
            if p + n > STACK_TOP - 0x10000:
                return u64(-1)
            self._brk = p + n
            self.mem[p:p + n] = b"\x00" * n
            return p
        if name == "mprotect":
            return 0
        if name == "munmap":
            return 0
        if name == "write":
            if a0 == 1 or a0 == 2:
                self.out.extend(self.mem[a1:a1 + a2])
                return a2
            try:
                return _os.write(a0, bytes(self.mem[a1:a1 + a2]))
            except OSError as e:
                return u64(-(e.errno or 1))   # -errno, as the native gates answer (fopen sets errno from it)
        if name == "read":
            try:
                d = _os.read(a0, a2)
            except OSError as e:
                return u64(-(e.errno or 1))   # -errno, as the native gates answer (fopen sets errno from it)
            self.mem[a1:a1 + len(d)] = d
            return len(d)
        if name == "open":
            try:
                fl, md = open_args(self.os, a1, a2)
                return _os.open(self.cstr(a0).decode(), fl, md)
            except OSError as e:
                return u64(-(e.errno or 1))   # -errno, as the native gates answer (fopen sets errno from it)
        if name == "close":
            try:
                _os.close(a0)
                return 0
            except OSError as e:
                return u64(-(e.errno or 1))   # -errno, as the native gates answer (fopen sets errno from it)
        if name == "lseek":
            try:
                off = a1 - (1 << 64) if a1 >= 1 << 63 else a1
                return _os.lseek(a0, off, a2)
            except OSError as e:
                return u64(-(e.errno or 1))   # -errno, as the native gates answer (fopen sets errno from it)
        if name == "unlink":
            try:
                _os.unlink(self.cstr(a0).decode())
                return 0
            except OSError as e:
                return u64(-(e.errno or 1))   # -errno, as the native gates answer (fopen sets errno from it)
        if name == "rename":
            try:
                _os.rename(self.cstr(a0).decode(), self.cstr(a1).decode())
                return 0
            except OSError as e:
                return u64(-(e.errno or 1))   # -errno, as the native gates answer (fopen sets errno from it)
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

    def run_reference(self):
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
                elif op in FOPS3:
                    r[ri[a[0]]] = fp.op3(op, r[ri[a[1]]], r[ri[a[2]]])
                elif op in FOPS2:
                    r[ri[a[0]]] = fp.op2(op, r[ri[a[1]]])
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
                elif op == "callm":
                    target = self.ld(self._addr(ri[a[0]], a[1]), 8)
                    r[SP] = u64(r[SP] - 8)
                    self.st(r[SP], pc, 8)
                    pc = target
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
                    r[ri[a[0]]] = self.nargs
                elif op == ".argv":
                    k = r[ri[a[1]]]
                    r[ri[a[0]]] = self.argv[k] if k < len(self.argv) else 0
                elif op == ".sys":
                    v = u64(self.syscall(a[0], r[ri[a[1]]],
                                         r[ri[a[2]]], r[ri[a[3]]]))
                    self._gate_clobber(r, v)
                elif op == ".sys6":
                    v = u64(self.syscall(a[0], *[r[ri[x]] for x in a[1:7]]))
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

    # -- fast path -------------------------------------------------------
    # The hot opcodes run inline on decoded operands (register indices, jump
    # targets, widths resolved once); every other opcode goes through _slow, the
    # reference chain.  Same results, step count, halts, fault text and memory
    # effects as run_reference: unchecked stack traffic stays unchecked, checked
    # loads and stores still go through _addr.
    _HOT = frozenset(("imm", "mov", "add64", "sub64", "mul64", "and64", "or64", "xor64",
                      "shl64", "shr64", "lshr64", "eq", "ne", "ult64", "ule64", "slt64",
                      "sle64", "load64", "store64", ".ld", ".st", "jump", "jumpz", "call", "ret"))

    def _decode(self):
        ri = {name: i for i, name in enumerate(REGS)}
        labels = self.t.labels
        out = []
        for ins in self.t.code:
            op, a = ins.op, ins.args
            if op == ".frame":
                out.append((40, SP, a[0], None))
                continue
            if op == ".lea":
                try:
                    sym = a[1]
                    if sym in self.t.syms:
                        v = self.t.syms[sym]
                    elif sym in labels:
                        v = labels[sym]
                    else:
                        v = int(sym, 0)
                except (ValueError, TypeError):
                    out.append((0, None, None, None))   # keep the reference behaviour: fails when executed
                    continue
                out.append((41, ri[a[0]], u64(v), None))
                continue
            if op not in self._HOT:
                out.append((0, None, None, None))
            elif op == "imm":
                out.append((1, ri[a[0]], u64(a[1]), None))
            elif op == "mov":
                out.append((2, ri[a[0]], ri[a[1]], None))
            elif op in _BIN:
                out.append((_BIN[op], ri[a[0]], ri[a[1]], ri[a[2]]))
            elif op == "load64":
                out.append((30, ri[a[0]], ri[a[1]], a[2]))
            elif op == "store64":
                out.append((31, ri[a[0]], a[1], ri[a[2]]))
            elif op == ".ld":
                out.append((32, ri[a[0]], ri[a[1]], (a[2], a[3])))
            elif op == ".st":
                out.append((33, ri[a[0]], a[1], (ri[a[2]], a[3])))
            elif op == "jump":
                out.append((34, labels.get(a[0]), a[0], None))
            elif op == "jumpz":
                out.append((35, ri[a[0]], labels.get(a[1]), a[1]))
            elif op == "call":
                out.append((36, labels.get(a[0]), a[0], None))
            else:                                  # ret
                out.append((37, None, None, None))
        return out

    def _slow(self, op, a, pc):
        """The reference chain for one opcode outside the inline set; returns the next pc."""
        t, r = self.t, self.r
        labels = t.labels
        ri = _RI
        if op in FOPS3:
            r[ri[a[0]]] = fp.op3(op, r[ri[a[1]]], r[ri[a[2]]])
        elif op in FOPS2:
            r[ri[a[0]]] = fp.op2(op, r[ri[a[1]]])
        elif op == ".lea":
            s = a[1]
            if s in t.syms:
                v = t.syms[s]
            elif s in labels:
                v = labels[s]
            else:
                v = int(s, 0)
            r[ri[a[0]]] = u64(v)
        elif op == ".zero":
            ad = self._addr(ri[a[0]], a[1])
            self.mem[ad:ad + a[2]] = b"\x00" * a[2]
        elif op == ".udiv" or op == ".umod":
            x, y = r[ri[a[1]]], r[ri[a[2]]]
            if y == 0:
                raise Halt(136)
            q = x // y
            r[ri[a[0]]] = u64(q if op == ".udiv" else x - q * y)
        elif op == ".div" or op == ".mod":
            x, y = s64(r[ri[a[1]]]), s64(r[ri[a[2]]])
            if y == 0:
                raise Halt(136)
            q = abs(x) // abs(y)
            if (x < 0) != (y < 0):
                q = -q
            r[ri[a[0]]] = u64(q if op == ".div" else x - q * y)
        elif op == "callr":
            r[SP] = u64(r[SP] - 8)
            self.st(r[SP], pc, 8)
            return r[ri[a[0]]]
        elif op == "callm":
            target = self.ld(self._addr(ri[a[0]], a[1]), 8)
            r[SP] = u64(r[SP] - 8)
            self.st(r[SP], pc, 8)
            return target
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
            r[ri[a[0]]] = self.nargs
        elif op == ".argv":
            k = r[ri[a[1]]]
            r[ri[a[0]]] = self.argv[k] if k < len(self.argv) else 0
        elif op == ".sys":
            v = u64(self.syscall(a[0], r[ri[a[1]]], r[ri[a[2]]], r[ri[a[3]]]))
            self._gate_clobber(r, v)
        elif op == ".sys6":
            v = u64(self.syscall(a[0], *[r[ri[x]] for x in a[1:7]]))
            self._gate_clobber(r, v)
        elif op == ".exit":
            raise Halt(u64(r[ri[a[0]]]) & 0xFF)
        elif op == "nop":
            pass
        else:
            raise AssertionError("vm: unhandled %r" % op)
        return pc

    def run(self):
        dec = self._decode()
        code = self.t.code
        labels = self.t.labels
        r, mem = self.r, self.mem
        n = len(dec)
        max_steps = self.max_steps
        steps = self.steps
        pc = labels.get("_start", labels.get("main", 0))
        M = MASK
        try:
            while True:
                steps += 1
                if steps > max_steps:
                    raise Halt(124)               # timeout
                if pc >= n:
                    raise Halt(u64(r[0]) & 0xFF)
                k, x, y, z = dec[pc]
                pc += 1
                if k == 40:                        # .frame  x=SP y=amount
                    r[x] = (r[x] - y) & M
                elif k == 41:                      # .lea (constant address)  x=dst y=value
                    r[x] = y
                elif k == 1:                       # imm
                    r[x] = y
                elif k == 30:                      # load64  x=dst y=base z=off
                    a = (r[y] + z) & M
                    if a >= MEM_SIZE:
                        self.pc_now = pc - 1
                        self.steps = steps
                        self._addr(y, z)
                    if a + 8 <= MEM_SIZE:
                        r[x] = _Q.unpack_from(mem, a)[0]
                    else:
                        r[x] = int.from_bytes(mem[a:a + 8], "little")
                elif k == 31:                      # store64  x=base y=off z=src
                    a = (r[x] + y) & M
                    if a >= MEM_SIZE:
                        self.pc_now = pc - 1
                        self.steps = steps
                        self._addr(x, y)
                    if a + 8 <= MEM_SIZE:
                        _Q.pack_into(mem, a, r[z] & M)
                    else:
                        mem[a:a + 8] = (r[z] & 0xFFFFFFFFFFFFFFFF).to_bytes(8, "little")
                elif k == 4:                       # sub64
                    r[x] = (r[y] - r[z]) & M
                elif k == 32:                      # .ld  x=dst y=base z=(off,w)
                    off, w = z
                    a = (r[y] + off) & M
                    if a >= MEM_SIZE:
                        self.pc_now = pc - 1
                        self.steps = steps
                        self._addr(y, off)
                    v = int.from_bytes(mem[a:a + w], "little")
                    if w < 8 and (v >> (w * 8 - 1)) & 1:
                        v -= 1 << (w * 8)
                    r[x] = v & M
                elif k == 3:                       # add64
                    r[x] = (r[y] + r[z]) & M
                elif k == 35:                      # jumpz  x=reg y=target z=name
                    if r[x] == 0:
                        if y is None:
                            self.steps = steps
                            pc = labels[z]
                        else:
                            pc = y
                elif k == 33:                      # .st  x=base y=off z=(src,w)
                    src, w = z
                    a = (r[x] + y) & M
                    if a >= MEM_SIZE:
                        self.pc_now = pc - 1
                        self.steps = steps
                        self._addr(x, y)
                    mem[a:a + w] = (r[src] & ((1 << (w * 8)) - 1)).to_bytes(w, "little")
                elif k == 5:                       # mul64
                    r[x] = (r[y] * r[z]) & M
                elif k == 2:                       # mov
                    r[x] = r[y]
                elif k == 9:                       # shl64
                    r[x] = (r[y] << (r[z] & 63)) & M
                elif k == 10:                      # shr64
                    r[x] = (s64(r[y]) >> (r[z] & 63)) & M
                elif k == 12:                      # eq
                    r[x] = 1 if r[y] == r[z] else 0
                elif k == 13:                      # ne
                    r[x] = 1 if r[y] != r[z] else 0
                elif k == 14:                      # ult64
                    r[x] = 1 if r[y] < r[z] else 0
                elif k == 15:                      # ule64
                    r[x] = 1 if r[y] <= r[z] else 0
                elif k == 16:                      # slt64
                    r[x] = 1 if s64(r[y]) < s64(r[z]) else 0
                elif k == 17:                      # sle64
                    r[x] = 1 if s64(r[y]) <= s64(r[z]) else 0
                elif k == 6:                       # and64
                    r[x] = r[y] & r[z]
                elif k == 7:                       # or64
                    r[x] = r[y] | r[z]
                elif k == 8:                       # xor64
                    r[x] = r[y] ^ r[z]
                elif k == 11:                      # lshr64
                    r[x] = r[y] >> (r[z] & 63)
                elif k == 34:                      # jump  x=target y=name
                    if x is None:
                        self.steps = steps
                        pc = labels[y]            # KeyError, as the reference raises it
                    else:
                        pc = x
                elif k == 36:                      # call  x=target y=name
                    sp = (r[SP] - 8) & M
                    r[SP] = sp
                    mem[sp:sp + 8] = (pc & 0xFFFFFFFFFFFFFFFF).to_bytes(8, "little")
                    if x is None:
                        self.steps = steps
                        pc = labels[y]
                    else:
                        pc = x
                elif k == 37:                      # ret
                    a = r[SP]
                    pc = int.from_bytes(mem[a:a + 8], "little")
                    r[SP] = (r[SP] + 8) & M
                else:                              # any other opcode: the reference chain, one instruction
                    self.pc_now = pc - 1
                    self.steps = steps
                    ins = code[pc - 1]
                    pc = self._slow(ins.op, ins.args, pc)
                    steps = self.steps
        except Halt as h:
            self.steps = steps
            if self.fault and h.code == 139:
                import sys as _s
                print("vm fault: address %#x at pc %d (%s) after %d steps"
                      % (self.fault[0], self.fault[1],
                         self.t.code[self.fault[1]], self.fault[2]),
                      file=_s.stderr)
            return bytes(self.out), h.code, self.steps

def run(tape, max_steps=MAX_STEPS, argv=None, os_=None):
    return VM(tape, max_steps, argv, os_).run()
