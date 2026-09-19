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
    base = (STU if store else LDU)[wd]
    return w(base | ((off & 0x1FF) << 12) | (rn << 5) | rt)


ALU3 = {"add64": 0x8B000000, "sub64": 0xCB000000, "xor64": 0xCA000000,
        "and64": 0x8A000000, "or64": 0xAA000000,
        "shl64": 0x9AC02000, "shr64": 0x9AC02800}
INVCOND = {"slt64": 0xA, "sle64": 0xC, "eq": 0x1, "ne": 0x0}   # ge, gt, ne, eq
IP1 = 17          # x16 is the Darwin syscall-number register -- use IP1


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


def encode(ins, off, labels, arch="arm64", syms=None, shift=0, text_va=0):
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
        n = a[0]
        base = 0xD1000000 if n >= 0 else 0x91000000      # sub/add imm
        sp = 7
        return w(base | ((abs(n) & 0xFFF) << 10) | (sp << 5) | sp)
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
    if o == ".div":                                  # sdiv
        return w(0x9AC00C00 | (N(a[2]) << 16) | (N(a[1]) << 5) | N(a[0]))
    if o == ".mod":                                  # sdiv then msub
        return w(0x9AC00C00 | (N(a[2]) << 16) | (N(a[1]) << 5) | IP1) + \
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
    if o == "spinit":                                # mov Xd, sp
        return w(0x91000000 | (31 << 5) | N(a[0]))
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
            return w(0x94000000)                     # bl <thunk>
        return w(0xD4000001)                         # svc #0
    if o == "jump":
        return w(0x14000000 | (((labels[a[0]] - off) >> 2) & 0x3FFFFFF))
    if o == "call":
        # tape semantics: push the return address on the tape stack (x7).  `bl`
        # would put it in lr, which recursion clobbers.
        return adr(IP1, text_va + off, text_va + off + 16) + \
            w(0xD1002000 | (7 << 5) | 7) + \
            w(0xF9000000 | (7 << 5) | IP1) + \
            w(0x14000000 | (((labels[a[0]] - (off + 12)) >> 2) & 0x3FFFFFF))
    if o == "jumpz":                                 # cbz Xt, label
        return w(0xB4000000 | ((((labels[a[1]] - off) >> 2) & 0x7FFFF) << 5) |
                 N(a[0]))
    return None


def size(ins, labels):
    b = encode(ins, 0, {k: 0 for k in labels}, "arm64", {}, 0, 0)
    return len(b) if b is not None else 4
