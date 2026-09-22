"""Catalog -- the single source of truth for isel/abi/enc/combo. [C-8]

The 9-head gold is DERIVED by nine() from (op, os, arch). Never hand-labelled. [G-9]
"""

OS = ("lnx", "osx", "win")
ARCH = ("x86_64", "arm64")
TARGETS = tuple("%s/%s" % (o, a) for o in OS for a in ARCH)

# op -> (lnx_x64, lnx_arm, osx_bsd, winapi)                              [C-1]
SYSCALLS = {
    "exit":          (60, 93, 1, "ExitProcess"),
    "read":          (0, 63, 3, "ReadFile"),
    "write":         (1, 64, 4, "WriteFile"),
    "open":          (2, 56, 5, "CreateFileW"),       # lnx arm = openat
    "close":         (3, 57, 6, "CloseHandle"),
    "mmap":          (9, 222, 197, "VirtualAlloc"),
    "munmap":        (11, 215, 73, "VirtualFree"),
    "mprotect":      (10, 226, 74, "VirtualProtect"),
    "getpid":        (39, 172, 20, "GetCurrentProcessId"),
    "clock_gettime": (228, 113, 116, "QueryPerformanceCounter"),
    "nanosleep":     (35, 101, 240, "Sleep"),
    "futex":         (202, 98, 515, "WaitOnAddress"),
    "socket":        (41, 198, 97, "WSASocketW"),
    "connect":       (42, 203, 98, "connect"),
    "bind":          (49, 200, 104, "bind"),
    "listen":        (50, 201, 106, "listen"),
    "accept":        (43, 202, 30, "accept"),
    "clone":         (56, 220, 360, "CreateThread"),
    "execve":        (59, 221, 59, "CreateProcessW"),
}

SYSOPS = tuple(SYSCALLS.keys())                                        # 19
MOPS = ("add64", "sub64", "xor64", "mul64", "slt64", "sle64", "load64",
        "store64", "jump", "jumpz", "call", "ret", "nop", "cas64", "fence",
        "syscall_gate", "tls_base", "cycle_counter", "stack_enter",
        "and64", "or64", "shl64", "shr64", "callr",
        # unsigned needs its own compare, shift and divide: the signed ones
        # give the wrong answer above 2^63 (and above 2^31 after a cast)
        "ult64", "ule64", "lshr64",                                # 27
        # floating point, on IEEE bit patterns in the general registers:
        # d = binary64, s = binary32 [TP] (fp.py says what each one means)
        "fadd64", "fsub64", "fmul64", "fdiv64", "flt64", "fle64", "feq64",
        "fadd32", "fsub32", "fmul32", "fdiv32", "flt32", "fle32", "feq32",
        "cvtid", "cvtud", "cvtis", "cvtus", "cvtdi", "cvtdu", "cvtsd",
        "cvtds", "fsqrt64", "fsqrt32")                             # 51
OPS = SYSOPS + MOPS                                                    # [V-1]

OSX_CLASS_BIT = 0x02000000                                             # [C-2]

# MOP -> (x86_64 mnemonic, arm64 mnemonic)                             [C-4 family]
MNEMONIC = {
    "add64": ("add", "add"),        "sub64": ("sub", "sub"),
    "xor64": ("xor", "eor"),        "mul64": ("imul", "mul"),
    "and64": ("and", "and"),        "or64": ("or", "orr"),
    "shl64": ("shl", "lsl"),        "shr64": ("sar", "asr"),
    "callr": ("call", "blr"),
    "slt64": ("setl", "cset"),      "sle64": ("setle", "cset"),
    "ult64": ("setb", "cset"),      "ule64": ("setbe", "cset"),
    "lshr64": ("shr", "lsr"),
    "load64": ("mov", "ldr"),       "store64": ("mov", "str"),
    "jump": ("jmp", "b"),           "jumpz": ("jz", "cbz"),
    "call": ("call", "bl"),         "ret": ("ret", "ret"),
    "nop": ("nop", "nop"),          "cas64": ("cmpxchg", "casal"),
    "fence": ("mfence", "dmb"),     "syscall_gate": ("syscall", "svc"),
    "tls_base": ("rdfsbase", "mrs"), "cycle_counter": ("rdtsc", "mrs"),
    "stack_enter": ("push", "stp"),
    "fadd64": ("addsd", "fadd"),    "fsub64": ("subsd", "fsub"),
    "fmul64": ("mulsd", "fmul"),    "fdiv64": ("divsd", "fdiv"),
    "flt64": ("ucomisd", "fcmp"),   "fle64": ("ucomisd", "fcmp"),
    "feq64": ("ucomisd", "fcmp"),
    "fadd32": ("addss", "fadd"),    "fsub32": ("subss", "fsub"),
    "fmul32": ("mulss", "fmul"),    "fdiv32": ("divss", "fdiv"),
    "flt32": ("ucomiss", "fcmp"),   "fle32": ("ucomiss", "fcmp"),
    "feq32": ("ucomiss", "fcmp"),
    "cvtid": ("cvtsi2sd", "scvtf"), "cvtud": ("cvtsi2sd", "ucvtf"),
    "cvtis": ("cvtsi2ss", "scvtf"), "cvtus": ("cvtsi2ss", "ucvtf"),
    "cvtdi": ("cvttsd2si", "fcvtzs"), "cvtdu": ("cvttsd2si", "fcvtzu"),
    "cvtsd": ("cvtss2sd", "fcvt"),  "cvtds": ("cvtsd2ss", "fcvt"),
    "fsqrt64": ("sqrtsd", "fsqrt"), "fsqrt32": ("sqrtss", "fsqrt"),
}

# real bytes for the three the spec pins                               [C-4]
BYTES = {
    ("add64", "x86_64"): b"\x48\x01\xf0", ("add64", "arm64"): b"\x00\x00\x01\x8b",
    ("ret", "x86_64"): b"\xc3",           ("ret", "arm64"): b"\xc0\x03\x5f\xd6",
    ("syscall_gate", "x86_64"): b"\x0f\x05",
    ("syscall_gate", "arm64"): b"\x01\x00\x00\xd4",
}

FORMS = ("syscall", "svc", "winapi", "x86", "arm")                     # [V-2]
GATES = ("syscall", "svc0", "svc80", "winapi", "none")
REGS = ("rdi", "rsi", "rdx", "r10", "rcx", "r8", "r9", "rax",
        "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "none")
TLS = ("fsbase", "tpidr_el0", "teb", "none")

# r0-r7 -> machine registers                                           [C-5]
REGMAP = {
    "x86_64": ("rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10"),
    "arm64":  ("x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"),
}


def enc_form(op, os_, arch):
    """[G-6] -- also isel's os-independent form when os is folded out."""
    mop = op in MOPS
    if os_ == "win" and not mop:
        return "winapi"
    if arch == "arm64" and not mop:
        return "svc"
    if (not mop) and arch == "x86_64" and os_ in ("lnx", "osx"):
        return "syscall"
    return "x86" if arch == "x86_64" else "arm"


def sysno(op, os_, arch):
    """[C-1] [C-2] [C-7]"""
    if op in MOPS or os_ == "win":
        return "none"
    lx, la, ox, _ = SYSCALLS[op]
    if os_ == "lnx":
        return str(lx if arch == "x86_64" else la)
    return hex(OSX_CLASS_BIT | ox)          # osx, both arches


def symbol(op, arch):
    """os-independent by construction; the WinAPI name is a catalog lookup
    that classic lowering performs when enc_form()=='winapi'. [C-7]"""
    if op in MOPS:
        return MNEMONIC[op][0 if arch == "x86_64" else 1]
    return op


def gate(op, os_, arch):
    """[C-3]"""
    if op in MOPS and op != "syscall_gate":
        return "none"
    if os_ == "win":
        return "winapi"
    if arch == "arm64":
        return "svc80" if os_ == "osx" else "svc0"
    return "syscall"


def argregs(op, os_, arch):
    """[C-3] [C-6] -> (arg0, arg1, arg2, ret)"""
    if op in MOPS:
        return ("none", "none", "none", "none")
    if os_ == "win":
        if arch == "x86_64":
            return ("rcx", "rdx", "r8", "rax")
        return ("x0", "x1", "x2", "x0")
    if arch == "x86_64":
        return ("rdi", "rsi", "rdx", "rax")
    return ("x0", "x1", "x2", "x0")


def tls(op, os_, arch):
    """[C-6]"""
    if op != "tls_base":
        return "none"
    if os_ == "win":
        return "teb"
    return "tpidr_el0" if arch == "arm64" else "fsbase"


def nine(op, os_, arch):
    """The 9 heads, derived. [G-9] [S-6]"""
    a0, a1, a2, rt = argregs(op, os_, arch)
    return {
        "form": enc_form(op, os_, arch),
        "symbol": symbol(op, arch),
        "gate": gate(op, os_, arch),
        "sysno": sysno(op, os_, arch),
        "arg0": a0, "arg1": a1, "arg2": a2, "ret": rt,
        "tls": tls(op, os_, arch),
    }


def isel_two(op, arch):
    """isel is keyed (op, arch), so it may only carry os-independent facts.

    [S-1 corrected] `gate` used to live here, but a gate is an OS fact:
    x86_64 is `syscall` on lnx/osx and `winapi` on win; arm64 is `svc0` on lnx
    and `svc80` on osx.  A key of (op, arch) cannot express that, so gate moved
    to abi.  Head count is unchanged: isel 2 + abi 7 = 9.  `form` here is the
    ARCH-level view; enc (op x os x arch) is authoritative for lowering.
    """
    return {"form": enc_form(op, "lnx", arch), "symbol": symbol(op, arch)}


# The syscall-number register is classic knowledge, not a head -- and it is an
# OS fact, not just an arch one: Linux/arm64 passes it in x8, Darwin/arm64 in
# x16.  Getting this wrong hangs the process instead of failing loudly.
NR_REG = {("lnx", "x86_64"): "rax", ("osx", "x86_64"): "rax",
          ("win", "x86_64"): "rax",
          ("lnx", "arm64"): "x8", ("osx", "arm64"): "x16",
          ("win", "arm64"): "x16"}


def gate_expected(os_, arch):
    if os_ == "win":
        return "winapi"
    if arch == "arm64":
        return "svc80" if os_ == "osx" else "svc0"
    return "syscall"


def sysno_table(os_, arch):
    """(os, arch) -> {int sysno: op}.  Built from the catalog; a wrong number
    simply is not in here, which is what gives --fold its teeth. [X-2]"""
    d = {}
    for op in SYSOPS:
        v = sysno(op, os_, arch)
        if v != "none":
            d[int(v, 0)] = op
    return d


WINAPI = {op: SYSCALLS[op][3] for op in SYSOPS}


def vocab(head):
    """Sorted union of what the derivation actually emits. [V-2]"""
    vals = set()
    for op in OPS:
        for o in OS:
            for a in ARCH:
                vals.add(nine(op, o, a)[head])
        for a in ARCH:
            t = isel_two(op, a)
            if head in t:
                vals.add(t[head])
    vals.add("none")
    return tuple(sorted(vals))
