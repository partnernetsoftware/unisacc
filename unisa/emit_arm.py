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
# the command-line splitter: x0 = read, x1 = write, x2 = argc, x3 = argv[]
WINARGS_BODY = (0x39400004, 0x7100809F, 0x54000060, 0x7100249F, 0x54000061, 0x91000400, 0x17FFFFFA, 0x34000364, 0xF100FC5F, 0x5400032A, 0xF8227861, 0x91000442, 0xD2800005, 0x39400004, 0x34000264, 0x7100889F, 0x54000081, 0xD24000A5, 0x91000400, 0x17FFFFFA, 0xB50000A5, 0x7100809F, 0x540000E0, 0x7100249F, 0x540000A0, 0x39000024, 0x91000421, 0x91000400, 0x17FFFFF1, 0x3900003F, 0x91000421, 0x91000400, 0x17FFFFE0, 0x3900003F)


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
        if k == "addr":
            return adrp_add(N(a[0]), text_va + off, v + shift)
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
    if o in FP_OPS:
        return _fp(o, a)
    if o == ".zero":
        # n bytes at [base+off] <- 0, stored from xzr (rt = 31) in the widest
        # pieces that fit.  It fell through to `brk` and nobody noticed: the
        # Python walker never emitted it for a local, and the self-hosted
        # compiler's tapes are only ever interpreted.
        out, k, n = b"", 0, a[2]
        while k < n:
            wd = 8
            while k + wd > n:
                wd //= 2
            out += mem(True, 31, N(a[0]), a[1] + k, wd)
            k += wd
        return out
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
        pre = b""
        if len(a) > 2 and a[2]:                      # Linux: argc at [sp]
            pre = w(0xF94003E0) + w(0x910023E1)      # ldr x0,[sp]; add x1,sp,#8
            off += 8
        return pre + adrp_add(IP1, text_va + off, a[0] + shift) + \
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
    if o == "winargs":
        # Windows hands over no argv: split GetCommandLineA() in place (the
        # write pointer never passes the read pointer), quotes honoured, at
        # most 63 arguments, into the argv array a[2]
        out = _callimp(text_va + off, imps, "GetCommandLineA")
        out += w(0xAA0003E1) + w(0xD2800002)         # mov x1, x0; mov x2, #0
        out += adrp_add(3, text_va + off + len(out), a[2] + shift)
        out += b"".join(w(v) for v in WINARGS_BODY)
        out += adrp_add(IP0, text_va + off + len(out), a[0] + shift) + _str(2, IP0)
        out += adrp_add(IP0, text_va + off + len(out), a[1] + shift) + _str(3, IP0)
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
    if o == "itoa":
        return _itoa(text_va + off, a[0] + shift, a[1] + shift, a[2] + shift)
    if o == "gate":
        g = ins.meta.get("gate")
        if g == "winapi":
            return _winapi(ins, off, shift, text_va, imps)
        out = w(0xD4001001) if g == "svc80" else w(0xD4000001)   # svc
        if ins.meta.get("carry"):
            # Darwin: carry set means the call failed and x0 holds errno.
            # `b.cc +8` over `neg x0, x0`, so the caller sees -errno. [I-20]
            out += w(0x54000043) + w(0xCB0003E0)
        return out
    if o == "jump":
        return w(0x14000000 | _relfield(ins, labels[a[0]] - off))
    if o == "call":
        # tape semantics: push the return address on the tape stack (x7).  `bl`
        # would put it in lr, which recursion clobbers.
        return adr(IP1, text_va + off, text_va + off + 16) + \
            w(0xD1002000 | (7 << 5) | 7) + \
            w(0xF9000000 | (7 << 5) | IP1) + \
            w(0x14000000 | _relfield(ins, labels[a[0]] - (off + 12)))
    if o == "jumpz":                                 # cbz Xt, label
        return w(0xB4000000 | _relfield(ins, labels[a[1]] - off) | N(a[0]))
    return None


# The reloc stage's answer IS the displacement field: its width and where it
# sits in the word.  A wrong answer is a wrong branch, not an ignored hint.
RELFIELD = {"arm26": (26, 0), "arm19": (19, 5)}


def _relfield(ins, d):
    bits, at = RELFIELD[ins.meta["reloc"]]
    return (_disp(d, bits) & ((1 << bits) - 1)) << at


def _itoa(pc, src, buf, lenp):
    """`.print`: the signed value at `src` in decimal at `buf`, its length at
    `lenp`.  It had no encoding at all -- a `.print` reached a native image
    as `brk` -- because only the self-hosted compiler emits it.  x9-x17 are
    not tape registers; the digits are counted first, then written from the
    end, so no reversal is needed.  |v| is taken unsigned: -LONG_MIN is fine."""
    out = adrp_add(9, pc, src) + _ldr(10, 9)                 # x10 = v
    out += _movz(11, 0)                                      # x11 = negative?
    out += w(0xF100001F | (10 << 5))                         # cmp x10, #0
    out += w(0x54000000 | (3 << 5) | 0xA)                    # b.ge +3
    out += w(0xCB000000 | (10 << 16) | (31 << 5) | 10)       # neg x10
    out += _movz(11, 1)
    out += _movz(12, 10)                                     # x12 = 10
    out += w(0xAA0003E0 | (10 << 16) | 13)                   # x13 = x10
    out += _movz(14, 0)                                      # x14 = count
    out += w(0x9AC00800 | (12 << 16) | (13 << 5) | 13)       # udiv x13, x13, x12
    out += w(0x91000400 | (14 << 5) | 14)                    # add x14, x14, #1
    out += w(0xB5000000 | (((-2) & 0x7FFFF) << 5) | 13)      # cbnz x13, -2
    out += w(0x8B000000 | (11 << 16) | (14 << 5) | 14)       # x14 += neg
    out += adrp_add(9, pc + len(out), lenp) + _str(14, 9)    # *lenp = x14
    out += adrp_add(9, pc + len(out), buf)                   # x9 = buf
    out += w(0x8B000000 | (14 << 16) | (9 << 5) | 13)        # x13 = end
    out += w(0x9AC00800 | (12 << 16) | (10 << 5) | 15)       # udiv x15, x10, x12
    out += w(0x9B008000 | (12 << 16) | (10 << 10) | (15 << 5) | 17)   # msub
    out += w(0x91000000 | (48 << 10) | (17 << 5) | 17)       # + '0'
    out += w(0xD1000400 | (13 << 5) | 13)                    # x13 -= 1
    out += w(0x39000000 | (13 << 5) | 17)                    # strb w17, [x13]
    out += w(0xAA0003E0 | (15 << 16) | 10)                   # x10 = x15
    out += w(0xB5000000 | (((-6) & 0x7FFFF) << 5) | 10)      # cbnz x10, -6
    out += w(0xB4000000 | (3 << 5) | 11)                     # cbz x11, +3
    out += _movz(17, 45)                                     # '-'
    out += w(0x39000000 | (9 << 5) | 17)                     # strb w17, [x9]
    return out


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
    if op == "mmap":
        return _callimp(pc, imps, "VirtualAlloc")       # four args, in x0..x3
    if op == "mprotect":
        scr0 = m.get("scr0", 0) + shift
        scr1 = m.get("scr1", 0) + shift
        out = adrp_add(3, pc, written)                  # x3 = &old
        out += _callimp(pc + len(out), imps, "VirtualProtect")
        # see emit_x86: arm64 Windows needs the instruction cache flushed,
        # and GetCurrentProcess() is always the pseudo-handle -1
        out += _movn(0, -1)                             # x0 = -1
        out += adrp_add(IP0, pc + len(out), scr0)
        out += _ldr(1, IP0)                             # x1 = the address
        out += adrp_add(IP0, pc + len(out), scr1)
        out += _ldr(2, IP0)                             # x2 = the length
        out += _callimp(pc + len(out), imps, "FlushInstructionCache")
        # POSIX returns 0 on success, VirtualProtect nonzero
        out += w(0xF100001F)                            # cmp x0, #0
        out += w(0x9A9F17E0)                            # cset x0, eq
        return out
    if op == "munmap":
        out = w(0xD2900002)                             # x2 = 0x8000 MEM_RELEASE
        out += w(0xAA1F03E1)                            # x1 = 0 (dwSize)
        out += _callimp(pc + len(out), imps, "VirtualFree")
        out += w(0xF100001F)                            # cmp x0, #0
        out += w(0x9A9F17E0)                            # cset x0, eq
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


# ---- floating point [TP] ---------------------------------------------------
# The value is its bit pattern in a general register.  Each op moves it into
# v16/v17 -- caller-saved, and used nowhere else -- computes, and moves the
# result back: `fmov` is a bit copy, so nothing is converted on the way.
D16, D17 = 16, 17


def _fmov_to(v, x, dbl):      # FMOV Dv, Xx  /  FMOV Sv, Wx
    return w((0x9E670000 if dbl else 0x1E270000) | (x << 5) | v)


def _fmov_from(x, v, dbl):    # FMOV Xx, Dv  /  FMOV Wx, Sv (zero-extends)
    return w((0x9E660000 if dbl else 0x1E260000) | (v << 5) | x)


FARITH = {"fadd": 0x1E202800, "fsub": 0x1E203800, "fmul": 0x1E200800,
          "fdiv": 0x1E201800}
# cset Xd, cond == csinc Xd, xzr, xzr, invert(cond).  After fcmp: MI is
# ordered less-than, LS less-or-equal, EQ equal -- and all three are FALSE
# when either operand is NaN, which is C's answer.
FCMP_INV = {"flt": 0x5, "fle": 0x8, "feq": 0x1}      # PL, HI, NE
FP_OPS = tuple(k + b for k in list(FARITH) + list(FCMP_INV)
               for b in ("64", "32")) + (
    "cvtid", "cvtud", "cvtis", "cvtus", "cvtdi", "cvtdu", "cvtsd", "cvtds",
    "fsqrt64", "fsqrt32")


def _fp(o, a):
    if o[:4] in FARITH or o[:3] in FCMP_INV:
        dbl = o.endswith("64")
        ty = 0x00400000 if dbl else 0          # the ftype field: 01 = double
        d, n, m = N(a[0]), N(a[1]), N(a[2])
        out = _fmov_to(D16, n, dbl) + _fmov_to(D17, m, dbl)
        if o[:4] in FARITH:
            out += w(FARITH[o[:4]] | ty | (D17 << 16) | (D16 << 5) | D16)
            return out + _fmov_from(d, D16, dbl)
        out += w(0x1E202000 | ty | (D17 << 16) | (D16 << 5))       # fcmp
        return out + w(0x9A9F07E0 | (FCMP_INV[o[:3]] << 12) | d)   # cset
    d, n = N(a[0]), N(a[1])
    if o in ("cvtid", "cvtud", "cvtis", "cvtus"):  # scvtf / ucvtf from Xn
        base = {"cvtid": 0x9E620000, "cvtud": 0x9E630000,
                "cvtis": 0x9E220000, "cvtus": 0x9E230000}[o]
        return w(base | (n << 5) | D16) + _fmov_from(d, D16, o[-1] == "d")
    if o in ("cvtdi", "cvtdu"):                    # fcvtzs / fcvtzu to Xd
        return _fmov_to(D16, n, True) + \
            w((0x9E780000 if o == "cvtdi" else 0x9E790000) | (D16 << 5) | d)
    if o == "cvtsd":                               # fcvt Dd, Sn
        return _fmov_to(D16, n, False) + w(0x1E22C000 | (D16 << 5) | D16) + \
            _fmov_from(d, D16, True)
    if o == "cvtds":                               # fcvt Sd, Dn
        return _fmov_to(D16, n, True) + w(0x1E624000 | (D16 << 5) | D16) + \
            _fmov_from(d, D16, False)
    dbl = o == "fsqrt64"                           # fsqrt
    return _fmov_to(D16, n, dbl) + \
        w((0x1E61C000 if dbl else 0x1E21C000) | (D16 << 5) | D16) + \
        _fmov_from(d, D16, dbl)


def size(ins, labels):
    b = encode(ins, 0, {k: 0 for k in labels}, "arm64", {}, 0, 0, {})
    return len(b) if b is not None else 4
