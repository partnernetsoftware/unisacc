"""tape -> TargetProgram. [L]

Five tables drive every target fact here:
    enc   (op,os,arch) -> form          <- authoritative form
    isel  (op,arch)    -> symbol
    abi   (op,os,arch) -> sysno, arg0-2, ret, tls, gate
    reloc (kind,arch)  -> rel32|arm26|arm19
Nothing target-specific is hardcoded except the machine's own register map,
which is classic algebra. [T-1]
"""
from . import catalog as C
from .tape import REGS as TAPE_REGS

# Scratch lives in the DATA area, not at a magic absolute address, so the same
# offsets resolve for the interpreter (base 0x100) and for a real image (base
# = where the loader maps the data that follows the text).
SCRATCH = 80          # SCR0, SCR1, PRINTLEN, PRINTBUF, ARGC, ARGV
PRINTMAX = 24
# Windows only.  A WinAPI call is a real call: it clobbers every volatile
# register, and ALL EIGHT tape registers are volatile on both Win64 ABIs --
# including r7, the tape stack pointer.  So the gate brackets the call with a
# save area, and the tape gets a stack of its own instead of borrowing the
# process stack the way `spinit` does elsewhere.  [I-18]
WIN_HSTD = SCRATCH + PRINTMAX            # GetStdHandle(-10/-11/-12)
WIN_WRITTEN = WIN_HSTD + 24              # the DWORD WriteFile insists on
WIN_SAVE = WIN_WRITTEN + 8               # r0..r7
WIN_EXTRA = WIN_SAVE + 64
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
    """The 9 target facts for one catalog op. [L-2]"""
    if drive == "combo":
        f = oracle.ask("combo", (op, os_, arch))
        return dict(f)
    f = dict(oracle.ask("isel", (op, arch)))
    f.update(oracle.ask("abi", (op, os_, arch)))
    f["form"] = oracle.ask("enc", (op, os_, arch))       # os-aware, wins
    return f


def lower(tape, target, oracle, fault=None, drive="spec"):
    from .tape import DATA_BASE
    os_, arch = target.split("/")
    data = bytearray(tape.data)
    pad = (-len(data)) % 8
    data.extend(b"\x00" * pad)
    base = DATA_BASE + len(data)
    SCR0, SCR1, PRINTLEN, PRINTBUF = base, base + 8, base + 16, base + 24
    ARGC, ARGV = base + 48, base + 56
    win = os_ == "win"
    HSTD, WRITTEN, SAVE = (base + WIN_HSTD, base + WIN_WRITTEN,
                           base + WIN_SAVE)
    STACKTOP = base + WIN_EXTRA + WIN_STACK
    data.extend(b"\x00" * (SCRATCH + PRINTMAX +
                           (WIN_EXTRA - SCRATCH - PRINTMAX if win else 0)))
    tp = TargetProgram(target, bytes(data), tape.syms)
    tp.data_len = len(data)
    # the tape stack is zero-filled, so it is bss: it costs image size but not
    # file size, which is the difference between a 4 KB .exe and a 68 KB one
    tp.bss = WIN_STACK if win else 0
    tp.relocs = list(tape.relocs)
    rmap = dict(zip(TAPE_REGS, C.REGMAP[arch]))
    sp = rmap["r7"]
    nr = C.NR_REG[(os_, arch)]

    def R(x):
        return rmap[x] if x in rmap else x

    def syscall_seq(op, arg_srcs):
        f = facts(oracle, op, os_, arch, drive)
        args = (f["arg0"], f["arg1"], f["arg2"])
        if fault == "win_argregs" and os_ == "win":
            args = SYSV                                   # [L-3] wrong ABI regs
        gate = f["gate"]
        if fault == "arm_gate" and gate in ("svc0", "svc80"):
            gate = "svc0" if gate == "svc80" else "svc80"
        sysno = f["sysno"]
        if sysno != "none":
            n = int(sysno, 0)
            if fault == "osx_class_bit" and os_ == "osx":
                n &= ~C.OSX_CLASS_BIT                     # [L-3] drop the bit
            tp.emit("setreg", nr, ("imm", n), role="sysno")
        if win:
            tp.emit("winsave", SAVE)
        for i, src in enumerate(arg_srcs):
            tp.emit("setreg", args[i], src, role="arg%d" % i)
        tp.emit("gate", form=f["form"], gate=gate, symbol=f["symbol"],
                winapi=C.WINAPI.get(op), catop=op, sysno=sysno,
                hstd=HSTD, written=WRITTEN)
        if win:
            tp.emit("winrest", SAVE, f["ret"])

    entry_pc = tape.labels.get("_start", 0)
    for pc, ins in enumerate(tape.code):
        for name, at in tape.labels.items():
            if at == pc:
                tp.labels[name] = len(tp.code)
        if pc == entry_pc:
            # The tape's SP is a register; a real process has a real stack, so
            # bind it AT THE ENTRY LABEL -- not at tape index 0, which is some
            # other function once `_start` moves.  The interpreter already
            # starts SP at the top of its own memory, so this is a no-op there.
            tp.emit("spinit", sp, STACKTOP if win else None)
            if win:
                # the three standard handles, once, before anything prints
                tp.emit("winstdh", HSTD)
            else:
                # the loader hands over argc/argv; stash them before anything
                tp.emit("argsave", ARGC, ARGV)
        o, a = ins.op, ins.args

        if o == ".write":
            tp.emit("setmem", SCR0, R(a[0]))
            tp.emit("setmem", SCR1, R(a[1]))
            syscall_seq("write", [("imm", 1), ("mem", SCR0), ("mem", SCR1)])
        elif o == ".print":
            tp.emit("setmem", SCR0, R(a[0]))
            tp.emit("itoa", SCR0, PRINTBUF, PRINTLEN)
            syscall_seq("write", [("imm", 1), ("imm", PRINTBUF),
                                  ("mem", PRINTLEN)])
        elif o == ".sys":
            for k in range(3):
                tp.emit("setmem", [SCR0, SCR1, PRINTLEN][k], R(a[1 + k]))
            syscall_seq(a[0], [("mem", SCR0), ("mem", SCR1),
                               ("mem", PRINTLEN)])
            tp.emit("mov", C.REGMAP[arch][0],
                    facts(oracle, a[0], os_, arch, drive)["ret"])
        elif o == ".exit":
            tp.emit("setmem", SCR0, R(a[0]))
            syscall_seq("exit", [("mem", SCR0), ("imm", 0), ("imm", 0)])
        elif o == ".argc":
            tp.emit("setreg", R(a[0]), ("mem", ARGC), role="argc")
        elif o == ".argv":
            tp.emit("argvget", R(a[0]), R(a[1]), ARGV)
        elif o == ".arg":
            f = facts(oracle, "add64", os_, arch, drive)      # a plain move
            tp.emit("mov", C.REGMAP[arch][a[0]], R(a[1]), symbol=f["symbol"])
        elif o in ("jump", "jumpz", "call"):
            kind = JMPKIND[o]
            rk = oracle.ask("reloc", (kind, arch))             # [L-1]
            cop = "call" if o == "call" else o
            f = facts(oracle, cop if cop in C.OPS else "jump", os_, arch, drive)
            tp.emit(o, *[R(x) for x in a], reloc=rk, form=f["form"],
                    symbol=f["symbol"])
        else:
            meta = {}
            if o in C.OPS:
                f = facts(oracle, o, os_, arch, drive)
                meta = {"form": f["form"], "symbol": f["symbol"]}
            tp.emit(o, *[R(x) if isinstance(x, str) else x for x in a], **meta)

    for name, at in tape.labels.items():
        if at == len(tape.code):
            tp.labels[name] = len(tp.code)
    return tp
