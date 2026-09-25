"""tape -> TargetProgram. [L]

Four stages drive every target fact here, and each answer is used (a wrong
one changes the image -- tests/ablate.sh):
    enc    (op,os,arch) -> form
    abi    (op,os,arch) -> sysno, arg0..arg5, ret, gate, nrreg
    reloc  (kind,arch)  -> rel32|arm26|arm19, the displacement field
    regmap (treg,arch)  -> the machine register holding tape register rN
[T-1]
"""
from . import catalog as C
from .tape import REGS as TAPE_REGS

# Scratch lives in the DATA area, not at a magic absolute address, so the same
# offsets resolve for the interpreter (base 0x100) and for a real image (base
# = where the loader maps the data that follows the text).
SCRATCH = 144         # SCR0, SCR1, PRINTLEN, PRINTBUF, ARGC, ARGV, SYSA[6],
                      # SYSFP, SYSSP
SYSA = 80             # six cells: a 6-argument syscall spills its sources
                      # here, because setting an argument register can clobber
                      # a tape register another argument still lives in
SYSFP = 128           # ...and on x86-64 two of the six argument registers ARE
SYSSP = 136           # the tape's frame and stack pointers (r9, r10), so a
                      # six-argument syscall saves and restores them [S-9]
PRINTMAX = 24
# Windows only.  A WinAPI call is a real call: it clobbers every volatile
# register, and ALL EIGHT tape registers are volatile on both Win64 ABIs --
# including r7, the tape stack pointer.  So the gate brackets the call with a
# save area, and the tape gets a stack of its own instead of borrowing the
# process stack the way `spinit` does elsewhere.  [I-18]
WIN_HSTD = SCRATCH + PRINTMAX            # GetStdHandle(-10/-11/-12)
WIN_WRITTEN = WIN_HSTD + 24              # the DWORD WriteFile insists on
WIN_SAVE = WIN_WRITTEN + 8               # r0..r7
WIN_ARGVA = WIN_SAVE + 64               # argv[64], split from GetCommandLineA
WIN_EXTRA = WIN_ARGVA + 512
WIN_STACK = 0x10000
FAULTS = ("osx_class_bit", "win_argregs", "arm_gate")
SYSV = ("rdi", "rsi", "rdx")

JMPKIND = {"jump": "jmp", "jumpz": "jz", "call": "call"}


class TIns:
    __slots__ = ("op", "args", "meta")

    def __init__(self, op, args=(), meta=None):
        self.op, self.args, self.meta = op, list(args), meta or {}

    def __repr__(self):
        return "%s %s" % (self.op, ", ".join(str(a) for a in self.args))


class TargetProgram:
    def __init__(self, target, data, syms):
        self.target = target
        self.os, self.arch = target.split("/")
        self.code = []
        self.labels = {}
        self.data = data
        self.syms = syms

    def emit(self, op, *args, **meta):
        self.code.append(TIns(op, args, meta))


def facts(oracle, op, os_, arch, drive="spec"):
    """The target facts for one catalog op. [L-2]

    Only what the lowering USES is asked (tests/ablate.sh checks each one
    changes the image).  isel is not asked: its `form` is the arch-level view
    enc supersedes, and its `symbol` is a mnemonic no encoder reads -- it
    stays a constructed stage for the spec/combo experiments, not a question
    the compiler pretends to obey."""
    if drive == "combo":
        f = oracle.ask("combo", (op, os_, arch))
        return dict(f)
    f = dict(oracle.ask("abi", (op, os_, arch)))
    f["form"] = oracle.ask("enc", (op, os_, arch))       # os-aware
    return f


def zero_last(data, syms, base):
    """Lay the data out initialised-first: every blob (a symbol's bytes, up
    to the next symbol) that holds a nonzero byte, in order, then every blob
    that is all zeros.  Each keeps its address mod 8.  The zeros then form one
    tail, which an image maps without storing -- unisacc's own tables sat
    after its buffers and made its image 91 MB of mostly nothing.  Code names
    data only through `syms` and no data byte holds an address (both front
    ends set pointer globals at run time), so moving a blob is invisible."""
    starts = sorted(set(a - base for a in syms.values()))
    if not starts or starts[0] != 0:
        starts = [0] + starts
    ends = starts[1:] + [len(data)]
    blobs = [(s, e) for s, e in zip(starts, ends)]
    nz = [b for b in blobs if any(data[b[0]:b[1]])]
    zz = [b for b in blobs if not any(data[b[0]:b[1]])]
    out = bytearray()
    new = {}
    for s, e in nz + zz:
        out.extend(b"\x00" * ((s - len(out)) % 8))
        new[s] = len(out)
        out.extend(data[s:e])
    return out, {n: base + new[a - base] for n, a in syms.items()}


def lower(tape, target, oracle, fault=None, drive="spec"):
    from .tape import DATA_BASE
    os_, arch = target.split("/")
    data, syms = zero_last(tape.data, tape.syms, DATA_BASE)
    pad = (-len(data)) % 8
    data.extend(b"\x00" * pad)
    base = DATA_BASE + len(data)
    SCR0, SCR1, PRINTLEN, PRINTBUF = base, base + 8, base + 16, base + 24
    ARGC, ARGV = base + 48, base + 56
    SYSCELL = [base + SYSA + 8 * i for i in range(6)]
    FPCELL, SPCELL = base + SYSFP, base + SYSSP
    win = os_ == "win"
    HSTD, WRITTEN, SAVE = (base + WIN_HSTD, base + WIN_WRITTEN,
                           base + WIN_SAVE)
    STACKTOP = base + WIN_EXTRA + WIN_STACK
    data.extend(b"\x00" * (SCRATCH + PRINTMAX +
                           (WIN_EXTRA - SCRATCH - PRINTMAX if win else 0)))
    tp = TargetProgram(target, bytes(data), syms)
    tp.src_os = getattr(tape, "src_os", tp.os)
    tp.data_len = len(data)
    # the tape stack is zero-filled, so it is bss: it costs image size but not
    # file size, which is the difference between a 4 KB .exe and a 68 KB one
    tp.bss = WIN_STACK if win else 0
    tp.relocs = list(tape.relocs)
    rmap = {r: oracle.ask("regmap", (r, arch)) for r in TAPE_REGS}
    sp = rmap["r7"]

    def R(x):
        return rmap[x] if x in rmap else x

    def syscall_seq(op, arg_srcs):
        f = facts(oracle, op, os_, arch, drive)
        args = (f["arg0"], f["arg1"], f["arg2"],
                f["arg3"], f["arg4"], f["arg5"])
        if fault == "win_argregs" and os_ == "win":
            args = SYSV                                   # [L-3] wrong ABI regs
        gate = f["gate"]
        if fault == "arm_gate" and gate in ("svc0", "svc80"):
            gate = "svc0" if gate == "svc80" else "svc80"
        sysno = f["sysno"]
        if sysno == "none" and not win and op not in C.MOPS:
            # no number and not a WinAPI call: emitting the gate anyway would
            # enter the kernel with whatever the number register held
            raise ValueError("%s: no system call for this on %s" % (op, os_))
        if sysno != "none":
            n = int(sysno, 0)
            if fault == "osx_class_bit" and os_ == "osx":
                n &= ~C.OSX_CLASS_BIT                     # [L-3] drop the bit
            tp.emit("setreg", f["nrreg"], ("imm", n), role="sysno")
        if win:
            tp.emit("winsave", SAVE)
        for i, src in enumerate(arg_srcs):
            if args[i] == "none":
                # Win64 passes the fifth argument and beyond on the stack,
                # which this gate does not do.  The WinAPI shapes it calls
                # take four or fewer, so the rest are simply not passed --
                # `mmap` becomes VirtualAlloc(addr, size, type, protect).
                if os_ == "win":
                    break
                raise NotImplementedError(
                    "%s/%s passes syscall argument %d on the stack, which "
                    "this gate does not do (%s)" % (os_, arch, i, op))
            tp.emit("setreg", args[i], src, role="arg%d" % i)
        # Darwin reports a failed syscall in the CARRY flag and returns the
        # errno POSITIVE; Linux returns -errno.  Without this the caller sees
        # ENOENT (2) as a valid file descriptor -- which is exactly what a
        # self-built compiler did when it looked for a header that was not
        # there. [I-20]
        tp.emit("gate", form=f["form"], gate=gate, carry=(os_ == "osx"),
                winapi=C.WINAPI.get(op), catop=op, sysno=sysno,
                retconv=f["retconv"], winimp=f["winimp"],
                ret=f["ret"], hstd=HSTD, written=WRITTEN,
                scr0=SCR0, scr1=SCR1)
        if win:
            tp.emit("winrest", SAVE, f["ret"])

    entry_pc = tape.labels.get("_start", 0)
    # [S-15 B1] push/pop pairing needs to know which tape pcs a label can
    # reach: the second half of a pair must not be a jump target
    targets = set(tape.labels.values())
    # labels by pc, built once: scanning every label per instruction was
    # 160k x 5k comparisons on unisacc.c, 9 s of a 28 s target
    at_pc = {}
    for name, at in tape.labels.items():
        at_pc.setdefault(at, []).append(name)
    skip = False
    for pc, ins in enumerate(tape.code):
        for name in at_pc.get(pc, ()):
            tp.labels[name] = len(tp.code)
        if skip:                     # the second half of a fused pair
            skip = False
            continue
        if pc == entry_pc:
            # The tape's SP is a register; a real process has a real stack, so
            # bind it AT THE ENTRY LABEL -- not at tape index 0, which is some
            # other function once `_start` moves.  The interpreter already
            # starts SP at the top of its own memory, so this is a no-op there.
            if win:
                # The standard handles come FIRST: these are real calls, and
                # the tape stack pointer is a volatile register on both Win64
                # ABIs, so it must not be live across them. [I-18]
                tp.emit("winstdh", HSTD)
                tp.emit("winargs", ARGC, ARGV, base + WIN_ARGVA)
            # x86-64: the tape stack IS the process stack now that r7 is
            # rsp, on Windows as well -- the WinAPI gate aligns and restores
            # rsp itself.  arm64 keeps a stack of its own on Windows: x7 is
            # not sp, and a WinAPI call would run over it.
            tp.emit("spinit", sp,
                    STACKTOP if (win and arch == "arm64") else None)
            if not win:
                # the loader hands over argc/argv; stash them before anything
                tp.emit("argsave", ARGC, ARGV, os_ == "lnx")
        o, a = ins.op, ins.args

        if o == ".write":
            tp.emit("setmem", SCR0, R(a[0]))
            tp.emit("setmem", SCR1, R(a[1]))
            syscall_seq("write", [("imm", 1), ("mem", SCR0), ("mem", SCR1)])
        elif o == ".print":
            tp.emit("setmem", SCR0, R(a[0]))
            tp.emit("itoa", SCR0, PRINTBUF, PRINTLEN)
            # the buffer's ADDRESS, relocated like any data address: as an
            # `imm` it was the interpreter's number, and a native write read
            # from nowhere
            syscall_seq("write", [("imm", 1), ("addr", PRINTBUF),
                                  ("mem", PRINTLEN)])
        elif o == ".sys":
            for k in range(3):
                tp.emit("setmem", [SCR0, SCR1, PRINTLEN][k], R(a[1 + k]))
            # how the three tape arguments become the call's: the abi
            # table's `argshape` [I4].  Linux/arm64 has no open, unlink or
            # rename -- the numbers are the *at forms, which take AT_FDCWD
            # first; renameat2 takes it twice, and flags 0 in x4, the tape's
            # r4 (scratch at a `.sys`, as r0-r2 already are).
            shape = facts(oracle, a[0], os_, arch, drive)["argshape"]
            A0, A1, A2 = ("mem", SCR0), ("mem", SCR1), ("mem", PRINTLEN)
            FD, Z = ("imm", -100), ("imm", 0)
            srcs = {"plain": [A0, A1, A2], "atfd_1": [FD, A0, A1, A2],
                    "atfd_1_zero": [FD, A0, Z],
                    "atfd_2_zero5": [FD, A0, FD, A1, Z]}[shape]
            syscall_seq(a[0], srcs)
            tp.emit("mov", rmap["r0"],
                    facts(oracle, a[0], os_, arch, drive)["ret"])
        elif o == ".exit":
            tp.emit("setmem", SCR0, R(a[0]))
            syscall_seq("exit", [("mem", SCR0), ("imm", 0), ("imm", 0)])
        elif o == ".sys6":
            # six arguments: spill them all, then fill the argument registers
            for i in range(6):
                tp.emit("setmem", SYSCELL[i], R(a[i + 1]))
            tp.emit("setmem", FPCELL, rmap["r6"])
            tp.emit("setmem", SPCELL, rmap["r7"])
            syscall_seq(a[0], [("mem", c) for c in SYSCELL])
            tp.emit("mov", rmap["r0"],
                    facts(oracle, a[0], os_, arch, drive)["ret"])
            tp.emit("setreg", rmap["r6"], ("mem", FPCELL), role="fp")
            tp.emit("setreg", rmap["r7"], ("mem", SPCELL), role="sp")
        elif o == ".argc":
            tp.emit("setreg", R(a[0]), ("mem", ARGC), role="argc")
        elif o == ".argv":
            tp.emit("argvget", R(a[0]), R(a[1]), ARGV)
        elif o == ".arg":
            f = facts(oracle, "add64", os_, arch, drive)      # a plain move
            tp.emit("mov", rmap["r%d" % a[0]], R(a[1]))
        elif (arch == "x86_64" and o == ".frame" and a[0] == 8
              and pc + 1 < len(tape.code) and pc + 1 not in targets
              and tape.code[pc + 1].op == "store64"
              and tape.code[pc + 1].args[0] == "r7"
              and tape.code[pc + 1].args[1] == 0):
            # `.frame 8; store64 [r7+0], r` is a push -- one byte, or two
            tp.emit("push", R(tape.code[pc + 1].args[2]))
            skip = True
        elif (arch == "x86_64" and o == "load64" and a[1] == "r7" and a[2] == 0
              and pc + 1 < len(tape.code) and pc + 1 not in targets
              and tape.code[pc + 1].op == ".frame"
              and tape.code[pc + 1].args[0] == -8):
            tp.emit("pop", R(a[0]))
            skip = True
        elif o in ("jump", "jumpz", "call"):
            kind = JMPKIND[o]
            rk = oracle.ask("reloc", (kind, arch))             # [L-1]
            cop = "call" if o == "call" else o
            f = facts(oracle, cop if cop in C.OPS else "jump", os_, arch, drive)
            tp.emit(o, *[R(x) for x in a], reloc=rk, form=f["form"])
        else:
            meta = {}
            if o in C.OPS:
                f = facts(oracle, o, os_, arch, drive)
                meta = {"form": f["form"]}
            tp.emit(o, *[R(x) if isinstance(x, str) else x for x in a], **meta)

    for name, at in tape.labels.items():
        if at == len(tape.code):
            tp.labels[name] = len(tp.code)
    return tp
