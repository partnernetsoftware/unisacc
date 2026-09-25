"""Target-machine interpreter. [X]

It holds the arch's NAMED registers and the OS's OWN ABI + syscall table, and
resolves only what lowering produced: (os, sysno) or (os, winapi name).  It
never looks back at the generic tape op. [X-2]

That is the whole reason --fold is a test: a wrong syscall number, a wrong
argument register or a wrong gate lands here as a trap or as wrong bytes.
"""
from . import fp
from .fp import OPS3 as _FOPS3, OPS2 as _FOPS2
from . import catalog as C
from .tape import MEM_SIZE, STACK_TOP, DATA_BASE
from .vm import Halt, s64, u64, MASK, open_args

# the machine's own ABI -- never read from the nets [X-1]
# This machine keeps its OWN copy of the ABI on purpose: checking the
# lowering against the table the lowering came from would prove nothing.
ABI_ARGS = {
    ("lnx", "x86_64"): ("rdi", "rsi", "rdx", "r10", "r8", "r9"),
    ("osx", "x86_64"): ("rdi", "rsi", "rdx", "r10", "r8", "r9"),
    ("win", "x86_64"): ("rcx", "rdx", "r8", "r9"),
    ("lnx", "arm64"): ("x0", "x1", "x2", "x3", "x4", "x5"),
    ("osx", "arm64"): ("x0", "x1", "x2", "x3", "x4", "x5"),
    ("win", "arm64"): ("x0", "x1", "x2", "x3"),
}
WINAPI_EFFECT = {"WriteFile": "write", "ExitProcess": "exit",
                 "ReadFile": "read", "CloseHandle": "close",
                 "CreateFileW": "open"}


class Trap(Exception):
    pass


class Machine:
    def __init__(self, tp, max_steps=20_000_000, argv=None):
        self.tp = tp
        self.os, self.arch = tp.os, tp.arch
        self.src_os = getattr(tp, "src_os", tp.os)
        self.max_steps = max_steps
        self.mem = bytearray(MEM_SIZE)
        self._brk = len(self.mem) // 2   # where mmap hands out pages
        self.mem[DATA_BASE:DATA_BASE + len(tp.data)] = tp.data
        self.R = {r: 0 for r in C.REGS if r != "none"}
        self.R["x16"] = self.R["x17"] = 0      # arm64 IP0/IP1: NR reg + scratch
        self.sp = C.REGMAP[self.arch][7]
        self.R[self.sp] = STACK_TOP
        self.out = bytearray()
        self.steps = 0
        self.systab = C.sysno_table(self.os, self.arch)
        self.argv = []
        p = DATA_BASE + len(tp.data) + 64
        for a in (argv or []):
            b = a.encode("latin-1") + b"\x00"
            self.mem[p:p + len(b)] = b
            self.argv.append(p)
            p += len(b)

    # -- memory -----------------------------------------------------------
    def addr(self, base, off):
        a = (self.R[base] + off) & MASK
        if a >= MEM_SIZE:
            raise Halt(139)
        return a

    def ld(self, a, w=8, signed=True):
        v = int.from_bytes(self.mem[a:a + w], "little")
        if signed and w < 8 and (v >> (w * 8 - 1)) & 1:
            v -= 1 << (w * 8)
        return u64(v)

    def st(self, a, v, w=8):
        self.mem[a:a + w] = (v & ((1 << (w * 8)) - 1)).to_bytes(w, "little")

    def src(self, s):
        k, v = s
        if k == "imm":
            return u64(v)
        if k == "reg":
            return self.R[v]
        if k == "addr":                  # a data address, as a value
            return u64(v)
        return self.ld(v, 8)

    # -- the gate ---------------------------------------------------------
    def gate(self, m):
        want = C.gate_expected(self.os, self.arch)
        if m["gate"] != want:
            raise Trap("gate %s, this machine issues %s" % (m["gate"], want))
        if m["form"] == "winapi":
            if self.os != "win":
                raise Trap("winapi form on %s" % self.os)
            name = m.get("winapi")
            op = WINAPI_EFFECT.get(name)
            if op is None:
                raise Trap("unknown WinAPI %r" % name)
        else:
            if self.os == "win":
                raise Trap("syscall form on win")
            n = self.R[C.NR_REG[(self.os, self.arch)]]
            op = self.systab.get(n)
            if op is None:
                raise Trap("no syscall %d (0x%x) on %s/%s"
                           % (n, n, self.os, self.arch))
        regs = ABI_ARGS[(self.os, self.arch)]
        a0, a1, a2 = (self.R[r] for r in regs[:3])
        import os as _os
        if op == "mmap":
            # a bump allocation in this machine's memory; the pages are as
            # executable as anything else here, which is why a program that
            # maps code and jumps into it runs under this model too
            n = (a1 + 0xFFF) & ~0xFFF
            p = self._brk
            if p + n > len(self.mem) - 0x20000:
                return u64(-1)
            self._brk = p + n
            self.mem[p:p + n] = b"\x00" * n
            return p
        if op in ("mprotect", "munmap"):
            return 0
        if op == "write":
            if a0 == 1 or a0 == 2:
                self.out.extend(self.mem[a1:a1 + a2])
                return a2
            try:
                return _os.write(a0, bytes(self.mem[a1:a1 + a2]))
            except OSError:
                # a bad descriptor here means lowering handed the syscall the
                # wrong register -- that is exactly what --fault probes [A-8]
                raise Trap("write to handle %d" % a0)
        if op == "read":
            try:
                d = _os.read(a0, a2)
            except OSError:
                raise Trap("read from handle %d" % a0)
            self.mem[a1:a1 + len(d)] = d
            return len(d)
        if op == "open":
            p0, f0, m0 = a0, a1, a2
            if (self.os, self.arch) == ("lnx", "arm64"):
                # this target has no `open`: it is `openat`, so the directory
                # fd is in front and everything else has shifted up one, with
                # the mode in a fourth register [see lower.py]
                p0, f0, m0 = a1, a2, self.R[regs[3]]
            e = self.mem.find(b"\x00", p0)
            path = bytes(self.mem[p0:e if e >= 0 else p0]).decode()
            try:
                fl, md = open_args(self.src_os, f0, m0)
                return _os.open(path, fl, md)
            except OSError:
                return u64(-1)
        if op == "close":
            try:
                _os.close(a0)
                return 0
            except OSError:
                return u64(-1)
        if op == "exit":
            raise Halt(a0 & 0xFF)
        raise Trap("unsupported syscall %r" % op)

    # -- run --------------------------------------------------------------
    def run(self):
        R, code, labels = self.R, self.tp.code, self.tp.labels
        pc = labels.get("_start", labels.get("main", 0))
        try:
            while True:
                self.steps += 1
                if self.steps > self.max_steps:
                    raise Halt(124)
                if pc >= len(code):
                    raise Halt(R[C.REGMAP[self.arch][0]] & 0xFF)
                ins = code[pc]
                o, a = ins.op, ins.args
                pc += 1

                if o == "setreg":
                    R[a[0]] = self.src(a[1])
                elif o == "setmem":
                    self.st(a[0], R[a[1]])
                elif o == "itoa":
                    txt = str(s64(self.ld(a[0]))).encode()
                    self.mem[a[1]:a[1] + len(txt)] = txt
                    self.st(a[2], len(txt))
                elif o == "gate":
                    # the result goes in the ABI's return register.  Dropping
                    # it left the syscall NUMBER sitting there, which nothing
                    # noticed until a program used open() or read()'s answer.
                    v = self.gate(ins.meta)
                    rr = ins.meta.get("ret")
                    if v is not None and rr and rr != "none":
                        R[rr] = u64(v)
                elif o == "imm":
                    R[a[0]] = u64(a[1])
                elif o == "mov":
                    R[a[0]] = R[a[1]]
                elif o == "add64":
                    R[a[0]] = u64(R[a[1]] + R[a[2]])
                elif o == "sub64":
                    R[a[0]] = u64(R[a[1]] - R[a[2]])
                elif o == "sext":                   # [J9] the return truncation
                    v = R[a[1]] & ((1 << (8 * a[2])) - 1)
                    if v >> (8 * a[2] - 1):
                        v -= 1 << (8 * a[2])
                    R[a[0]] = u64(v)
                elif o == "addi":                   # [J9] fused imm forms
                    R[a[0]] = u64(R[a[1]] + a[2])
                elif o == "subi":
                    R[a[0]] = u64(R[a[1]] - a[2])
                elif o == "lsli":
                    R[a[0]] = u64(R[a[1]] << a[2])
                elif o == "mul64":
                    R[a[0]] = u64(R[a[1]] * R[a[2]])
                elif o == "and64":
                    R[a[0]] = R[a[1]] & R[a[2]]
                elif o == "or64":
                    R[a[0]] = R[a[1]] | R[a[2]]
                elif o == "shl64":
                    R[a[0]] = u64(R[a[1]] << (R[a[2]] & 63))
                elif o == "shr64":
                    R[a[0]] = u64(s64(R[a[1]]) >> (R[a[2]] & 63))
                elif o == "lshr64":
                    R[a[0]] = R[a[1]] >> (R[a[2]] & 63)
                elif o == "xor64":
                    R[a[0]] = R[a[1]] ^ R[a[2]]
                elif o in (".udiv", ".umod"):
                    x, y = R[a[1]], R[a[2]]
                    if y == 0:
                        raise Halt(136)
                    q = x // y
                    R[a[0]] = u64(q if o == ".udiv" else x - q * y)
                elif o in (".div", ".mod"):
                    x, y = s64(R[a[1]]), s64(R[a[2]])
                    if y == 0:
                        raise Halt(136)
                    q = abs(x) // abs(y)
                    if (x < 0) != (y < 0):
                        q = -q
                    R[a[0]] = u64(q if o == ".div" else x - q * y)
                elif o == "slt64":
                    R[a[0]] = 1 if s64(R[a[1]]) < s64(R[a[2]]) else 0
                elif o == "sle64":
                    R[a[0]] = 1 if s64(R[a[1]]) <= s64(R[a[2]]) else 0
                elif o == "ult64":
                    R[a[0]] = 1 if R[a[1]] < R[a[2]] else 0
                elif o == "ule64":
                    R[a[0]] = 1 if R[a[1]] <= R[a[2]] else 0
                elif o == "eq":
                    R[a[0]] = 1 if R[a[1]] == R[a[2]] else 0
                elif o == "ne":
                    R[a[0]] = 1 if R[a[1]] != R[a[2]] else 0
                elif o in _FOPS3:                 # [TP] fp.py, shared
                    R[a[0]] = fp.op3(o, R[a[1]], R[a[2]])
                elif o in _FOPS2:
                    R[a[0]] = fp.op2(o, R[a[1]])
                elif o == "load64":
                    R[a[0]] = self.ld(self.addr(a[1], a[2]))
                elif o == "store64":
                    self.st(self.addr(a[0], a[1]), R[a[2]])
                elif o == ".ld":
                    R[a[0]] = self.ld(self.addr(a[1], a[2]), a[3])
                elif o == ".st":
                    self.st(self.addr(a[0], a[1]), R[a[2]], a[3])
                elif o == ".lea":
                    s = a[1]
                    if s in self.tp.syms:
                        v = self.tp.syms[s]
                    elif s in labels:
                        v = labels[s]
                    else:
                        v = int(s, 0)
                    R[a[0]] = u64(v)
                elif o == ".zero":
                    ad = self.addr(a[0], a[1])
                    self.mem[ad:ad + a[2]] = b"\x00" * a[2]
                elif o == "jump":
                    pc = labels[a[0]]
                elif o == "jumpz":
                    if R[a[0]] == 0:
                        pc = labels[a[1]]
                elif o == "call":
                    R[self.sp] = u64(R[self.sp] - 8)
                    self.st(R[self.sp], pc)
                    pc = labels[a[0]]
                elif o == "callr":
                    R[self.sp] = u64(R[self.sp] - 8)
                    self.st(R[self.sp], pc)
                    pc = R[a[0]]
                elif o == "ret":
                    pc = self.ld(R[self.sp])
                    R[self.sp] = u64(R[self.sp] + 8)
                elif o == ".frame":
                    R[self.sp] = u64(R[self.sp] - a[0])
                elif o == "push":
                    R[self.sp] = u64(R[self.sp] - 8)
                    self.st(R[self.sp], R[a[0]])
                elif o == "pop":
                    R[a[0]] = self.ld(R[self.sp])
                    R[self.sp] = u64(R[self.sp] + 8)
                elif o in ("argsave", "winargs"):
                    self.st(a[0], len(self.argv))
                elif o == "argvget":
                    k = R[a[1]]
                    R[a[0]] = self.argv[k] if k < len(self.argv) else 0
                elif o == "spinit":
                    if len(a) > 1 and a[1] is not None:
                        R[self.sp] = a[1]      # Windows: the tape's own stack
                elif o in ("winsave", "winrest", "winstdh"):
                    # The register save area and the standard handles are a
                    # real-machine concern: this interpreter's gate does not
                    # clobber anything and its handles are fds.  [I-18]
                    pass
                elif o == "nop":
                    pass
                else:
                    raise Trap("unhandled target op %r" % o)
        except Halt as h:
            return bytes(self.out), h.code, self.steps, None
        except Trap as t:
            return bytes(self.out), 3, self.steps, str(t)


def execute(tp, max_steps=20_000_000, argv=None):
    return Machine(tp, max_steps, argv).run()
