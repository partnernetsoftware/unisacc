"""AArch64 encoder. [C-4]  Fixed 4-byte instructions; unencodable -> brk #0."""
BRK = (0xD4200000).to_bytes(4, "little")


def N(r):
    return int(r[1:]) if r[0] == "x" else 31


def w(v):
    return (v & 0xFFFFFFFF).to_bytes(4, "little")


# scaled positive offset (imm12) vs unscaled signed offset (imm9).  Locals live
# at [fp - off], so the unscaled form is not optional.
LDS = {8: 0xF9400000, 4: 0xB9800000, 2: 0x79800000, 1: 0x39800000}
STS = {8: 0xF9000000, 4: 0xB9000000, 2: 0x79000000, 1: 0x39000000}
LDU = {8: 0xF8400000, 4: 0xB8800000, 2: 0x78800000, 1: 0x38800000}
STU = {8: 0xF8000000, 4: 0xB8000000, 2: 0x78000000, 1: 0x38000000}


def mem(store, rt, rn, off, wd):
    if off >= 0 and off % wd == 0 and (off // wd) < 4096:
        base = (STS if store else LDS)[wd]
        return w(base | ((off // wd) << 10) | (rn << 5) | rt)
    if -256 <= off < 256:
        base = (STU if store else LDU)[wd]
        return w(base | ((off & 0x1FF) << 12) | (rn << 5) | rt)
    # Out of BOTH ranges.  imm9 is SIGNED, so masking it was not a truncation
    # but a sign flip: `[fp, #-260]` encoded as `[fp, #+252]`, and the program
    # read a different local -- quietly, with no fault.  Anything past 256
    # bytes of frame needs the address in a register.  [I-22]
    out = movimm(IP0, abs(off))
    out += w((0xCB000000 if off < 0 else 0x8B000000)      # sub/add IP0,rn,IP0
             | (IP0 << 16) | (rn << 5) | IP0)
    base = (STS if store else LDS)[wd]
    return out + w(base | (IP0 << 5) | rt)


ALU3 = {"add64": 0x8B000000, "sub64": 0xCB000000, "xor64": 0xCA000000,
        "and64": 0x8A000000, "or64": 0xAA000000,
        "shl64": 0x9AC02000, "shr64": 0x9AC02800,
        "lshr64": 0x9AC02400}
# cset encodes the INVERTED condition: ge, gt, ne, eq, hs, hi
INVCOND = {"slt64": 0xA, "sle64": 0xC, "eq": 0x1, "ne": 0x0,
           "ult64": 0x2, "ule64": 0x8}
IP0 = 16
IP1 = 17          # x16 is the Darwin syscall-number register -- use IP1
# Windows/arm64 [I-18].  A WinAPI call is an ordinary AAPCS64 call, so it
# clobbers x0-x17 -- which is every tape register, the tape SP included.  The
# gate therefore brackets the call, and translates the POSIX shape the tape
# speaks into the one kernel32 expects.
STD_FIRST = -10   # GetStdHandle: -10 stdin, -11 stdout, -12 stderr


def _ldrx(rt, rn, rm):
    """LDR Xt, [Xn, Xm, LSL #3]"""
    return w(0xF8607800 | (rm << 16) | (rn << 5) | rt)


def _ldr(rt, rn, off=0):
    return w(0xF9400000 | ((off // 8) << 10) | (rn << 5) | rt)


def _str(rt, rn, off=0):
    return w(0xF9000000 | ((off // 8) << 10) | (rn << 5) | rt)


def _movz(d, v):
    return w(0xD2800000 | ((v & 0xFFFF) << 5) | d)


def _movn(d, v):
    """MOVN Xd, #imm -- for the negative GetStdHandle selectors"""
    return w(0x92800000 | (((-v - 1) & 0xFFFF) << 5) | d)


def _callimp(pc, imps, name):
    a = (imps or {}).get("__imp_" + name, 0)
    return adrp_add(IP1, pc, a) + _ldr(IP1, IP1) + w(0xD63F0000 | (IP1 << 5))


def movimm(d, v, fixed=0):
    """fixed=k forces exactly k words so the instruction size is address
    independent -- required for `.lea`, whose operand is only known in pass 3."""
    out = w(0xD2800000 | ((v & 0xFFFF) << 5) | d)
    n = 1
    for sh in (1, 2, 3):
        part = (v >> (16 * sh)) & 0xFFFF
        if part or (fixed and n < fixed):
            out += w(0xF2800000 | (sh << 21) | (part << 5) | d)
            n += 1
    return out


def adr(d, pc, target):
    """ADR Xd, label -- PC-relative byte offset, 21-bit signed."""
    imm = target - pc
    return w(0x10000000 | ((imm & 3) << 29) |
             (((imm >> 2) & 0x7FFFF) << 5) | d)


def adrp_add(d, pc, target):
    """PC-relative address in 8 bytes, fixed width.  arm64 macOS requires PIE,
    so the image slides and absolute addresses are not an option."""
    page = (target >> 12) - (pc >> 12)
    lo12 = target & 0xFFF
    immlo, immhi = page & 3, (page >> 2) & 0x7FFFF
    return w(0x90000000 | (immlo << 29) | (immhi << 5) | d) + \
        w(0x91000000 | (lo12 << 10) | (d << 5) | d)


def _disp(bytes_, bits):
    """A branch displacement, in instructions, checked against its field.

    Masking an immediate that does not fit is how `[fp, #-260]` became
    `[fp, #+252]` -- a sign flip, silently, in code that ran.  [I-22]  The
    branch fields are wide enough that nothing we compile has reached them
    yet, so the only useful thing to do with an overflow is refuse."""
    v = bytes_ >> 2
    lim = 1 << (bits - 1)
    if not -lim <= v < lim:
        raise AssertionError(
            "arm64: branch of %d bytes does not fit in imm%d" % (bytes_, bits))
    return v


def encode(ins, off, labels, arch="arm64", syms=None, shift=0,
           text_va=0, imps=None):
    o, a = ins.op, ins.args
    if o == "mov":                                  # orr Xd, xzr, Xm
        return w(0xAA0003E0 | (N(a[1]) << 16) | N(a[0]))
    if o == "imm":
        v, out, d = a[1] & ((1 << 64) - 1), b"", N(a[0])
        out += w(0xD2800000 | ((v & 0xFFFF) << 5) | d)          # movz
        for sh in (1, 2, 3):
            part = (v >> (16 * sh)) & 0xFFFF
            if part:
                out += w(0xF2800000 | (sh << 21) | (part << 5) | d)  # movk
        return out
    if o in ALU3:
        return w(ALU3[o] | (N(a[2]) << 16) | (N(a[1]) << 5) | N(a[0]))
    if o == "mul64":
        return w(0x9B007C00 | (N(a[2]) << 16) | (N(a[1]) << 5) | N(a[0]))
    if o == "load64":
        return mem(False, N(a[0]), N(a[1]), a[2], 8)
    if o == "store64":
        return mem(True, N(a[2]), N(a[0]), a[1], 8)
    if o in INVCOND:                                 # cmp + cset
        return w(0xEB00001F | (N(a[2]) << 16) | (N(a[1]) << 5)) + \
            w(0x9A9F07E0 | (INVCOND[o] << 12) | N(a[0]))
    if o == "setreg":
        k, v = a[1]
        if k == "imm":
            return movimm(N(a[0]), v & ((1 << 64) - 1))
        if k == "reg":
            return w(0xAA0003E0 | (N(v) << 16) | N(a[0]))
        return adrp_add(IP1, text_va + off, v + shift) + \
            w(0xF9400000 | (IP1 << 5) | N(a[0]))
    if o == "setmem":
        return adrp_add(IP1, text_va + off, a[0] + shift) + \
            w(0xF9000000 | (IP1 << 5) | N(a[1]))
    if o == ".frame":
        n, sp = a[0], 7                     # the tape SP is x7, not real sp
        if abs(n) < 4096:
            base = 0xD1000000 if n >= 0 else 0x91000000  # sub/add imm12
            return w(base | (abs(n) << 10) | (sp << 5) | sp)
        # imm12 stops at 4095 and masking it moved SP by the wrong amount --
        # a Blowfish key is a 4,168-byte local, so its frame landed on top of
        # the caller's.  Past the immediate's range the amount goes in a
        # register.  Same lesson as [I-22]: a field that is too small must be
        # detected, never masked.
        return movimm(IP0, abs(n)) + \
            w((0xCB000000 if n >= 0 else 0x8B000000)     # sub/add x7,x7,x16
              | (IP0 << 16) | (sp << 5) | sp)
    if o == ".lea":
        sym = a[1]
        if syms and sym in syms:
            addr = syms[sym] + shift          # data symbol
        elif sym in labels:
            addr = text_va + labels[sym]      # code label: a function address
        else:
            try:                              # a bare integer operand
                addr = int(str(sym), 0) + shift
            except ValueError:                # sizing pass: width is fixed
                addr = 0
        return adrp_add(N(a[0]), text_va + off, addr)
    if o == ".ld":
        return mem(False, N(a[0]), N(a[1]), a[2], a[3])
    if o == ".st":
        return mem(True, N(a[2]), N(a[0]), a[1], a[3])
    if o == "callr":                                 # same stack discipline
        return adr(IP1, text_va + off, text_va + off + 16) + \
            w(0xD1002000 | (7 << 5) | 7) + \
            w(0xF9000000 | (7 << 5) | IP1) + \
            w(0xD61F0000 | (N(a[0]) << 5))           # br Xn
    if o in (".div", ".udiv"):                       # sdiv / udiv
        d = 0x9AC00C00 if o == ".div" else 0x9AC00800
        return w(d | (N(a[2]) << 16) | (N(a[1]) << 5) | N(a[0]))
    if o in (".mod", ".umod"):                       # divide then msub
        d = 0x9AC00C00 if o == ".mod" else 0x9AC00800
        return w(d | (N(a[2]) << 16) | (N(a[1]) << 5) | IP1) + \
            w(0x9B008000 | (N(a[2]) << 16) | (N(a[1]) << 10) |
              (IP1 << 5) | N(a[0]))
    if o == "argsave":                               # Darwin: x0=argc x1=argv
        return adrp_add(IP1, text_va + off, a[0] + shift) + \
            w(0xF9000000 | (IP1 << 5) | 0) + \
            adrp_add(IP1, text_va + off + 12, a[1] + shift) + \
            w(0xF9000000 | (IP1 << 5) | 1)
    if o == "argvget":                               # rd = ((char**)argv)[rs]
        return adrp_add(IP1, text_va + off, a[2] + shift) + \
            w(0xF9400000 | (IP1 << 5) | IP1) + \
            w(0x8B000000 | (N(a[1]) << 16) | (3 << 10) |
              (IP1 << 5) | IP1) + \
            w(0xF9400000 | (IP1 << 5) | N(a[0]))
    if o == "spinit":
        if len(a) > 1 and a[1] is not None:
            # Windows: the tape's own stack.  PC-relative, not an absolute
            # immediate -- DYNAMIC_BASE is mandatory on arm64 (I-17) so the
            # image slides.
            return adrp_add(N(a[0]), text_va + off, a[1] + shift)
        return w(0x91000000 | (31 << 5) | N(a[0]))   # mov Xd, sp
    if o == "winsave":
        out = adrp_add(IP0, text_va + off, a[0] + shift)
        for k in range(8):
            out += _str(k, IP0, 8 * k)
        return out
    if o == "winrest":
        out = w(0xAA000000 | (0 << 16) | (31 << 5) | IP1)   # mov IP1, x0
        out += adrp_add(IP0, text_va + off + len(out), a[0] + shift)
        for k in range(1, 8):
            out += _ldr(k, IP0, 8 * k)
        out += w(0xAA000000 | (IP1 << 16) | (31 << 5) | N(a[1]))
        return out
    if o == "winstdh":
        out = b""
        for k in range(3):
            out += _movn(0, STD_FIRST - k)
            out += _callimp(text_va + off + len(out), imps, "GetStdHandle")
            out += adrp_add(IP0, text_va + off + len(out), a[0] + shift)
            out += _str(0, IP0, 8 * k)
        return out
    if o == "ret":
        return w(0xF9400000 | (7 << 5) | IP1) + \
            w(0x91002000 | (7 << 5) | 7) + \
            w(0xD61F0000 | (IP1 << 5))
    if o == "nop":
        return w(0xD503201F)
    if o == "gate":
        g = ins.meta.get("gate")
        if g == "svc80":
            return w(0xD4001001)                     # svc #0x80
        if g == "winapi":
            return _winapi(ins, off, shift, text_va, imps)
        return w(0xD4000001)                         # svc #0
    if o == "jump":
        return w(0x14000000 | (_disp(labels[a[0]] - off, 26) & 0x3FFFFFF))
    if o == "call":
        # tape semantics: push the return address on the tape stack (x7).  `bl`
        # would put it in lr, which recursion clobbers.
        return adr(IP1, text_va + off, text_va + off + 16) + \
            w(0xD1002000 | (7 << 5) | 7) + \
            w(0xF9000000 | (7 << 5) | IP1) + \
            w(0x14000000 | (_disp(labels[a[0]] - (off + 12), 26) & 0x3FFFFFF))
    if o == "jumpz":                                 # cbz Xt, label
        return w(0xB4000000 | ((_disp(labels[a[1]] - off, 19) & 0x7FFFF) << 5)
                 | N(a[0]))
    return None


def _fd2handle(pc, hstd):
    """fd 0/1/2 name a standard handle; anything above is already one."""
    out = w(0xF1000C1F)                              # cmp x0, #3
    body = adrp_add(IP0, pc + 8, hstd) + _ldrx(0, IP0, 0)
    out += w(0x54000002 | ((len(body) // 4 + 1) << 5))   # b.hs over it
    return out + body


def _winapi(ins, off, shift, text_va, imps):
    m = ins.meta
    op = m.get("catop")
    hstd = m.get("hstd", 0) + shift
    written = m.get("written", 0) + shift
    pc = text_va + off
    if op == "exit":
        return _callimp(pc, imps, "ExitProcess")
    if op in ("write", "read"):
        out = _fd2handle(pc, hstd)
        out += adrp_add(3, pc + len(out), written)   # x3 = &written
        out += w(0xAA1F03E4)                         # mov x4, xzr
        out += _callimp(pc + len(out), imps,
                        "WriteFile" if op == "write" else "ReadFile")
        out += adrp_add(IP0, pc + len(out), written)
        out += _ldr(0, IP0)                          # the POSIX return value
        return out
    if op == "close":
        out = _fd2handle(pc, hstd)
        return out + _callimp(pc + len(out), imps, "CloseHandle")
    if op == "open":
        # The gate carries Windows' own shapes, because only the C library
        # knows which platform it is compiling for: arg1 is dwDesiredAccess
        # and arg2 is dwCreationDisposition (see include/stdio.h).  Hardcoding
        # GENERIC_READ|OPEN_EXISTING here made every fopen("w") a silent
        # failure.  The remaining parameters never vary.
        out = w(0xAA0203E4)                          # x4 = x2 (disposition)
        out += _movz(2, 3)                           # FILE_SHARE_READ|WRITE
        out += w(0xAA1F03E3)                         # x3 = 0  (no security)
        out += _movz(5, 0x80)                        # FILE_ATTRIBUTE_NORMAL
        out += w(0xAA1F03E6)                         # x6 = 0  (no template)
        return out + _callimp(pc + len(out), imps, "CreateFileA")
    return None


def size(ins, labels):
    b = encode(ins, 0, {k: 0 for k in labels}, "arm64", {}, 0, 0, {})
    return len(b) if b is not None else 4
