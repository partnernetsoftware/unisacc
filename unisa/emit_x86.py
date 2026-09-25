"""x86-64 encoder. [C-4]

Covers the ops lowering actually emits; anything else becomes `ud2` and is
counted, so `unisa compile` can report honest coverage rather than pretend.
"""
NUM = {"rax": 0, "rcx": 1, "rdx": 2, "rbx": 3, "rsp": 4, "rbp": 5,
       "rsi": 6, "rdi": 7, "r8": 8, "r9": 9, "r10": 10, "r11": 11,
       "r12": 12, "r13": 13, "r14": 14, "r15": 15}
UD2 = b"\x0f\x0b"


def rex(w=1, r=0, x=0, b=0):
    return bytes([0x40 | (w << 3) | (r << 2) | (x << 1) | b])


def modrm(mod, reg, rm):
    return bytes([(mod << 6) | ((reg & 7) << 3) | (rm & 7)])


def _alu(opc, dst, src):
    d, s = NUM[dst], NUM[src]
    return rex(1, s >> 3, 0, d >> 3) + bytes([opc]) + modrm(3, s, d)


def mov_rr(dst, src):
    return _alu(0x89, dst, src)                      # mov r/m64, r64


def mov_ri(dst, imm):
    d = NUM[dst]
    v = imm & MASK64
    # SHORT form: `mov r32, imm32` zero-extends to 64 bits, so any value that
    # fits in an unsigned 32-bit field needs five bytes (six for r8-r15)
    # rather than movabs' ten.  The value is a literal, final in the sizing
    # pass -- an address goes through rip()/`.lea`, never through here.
    if v >> 32 == 0:
        pre = b"\x41" if d >= 8 else b""
        return pre + bytes([0xB8 + (d & 7)]) + v.to_bytes(4, "little")
    return rex(1, 0, 0, d >> 3) + bytes([0xB8 + (d & 7)]) + \
        v.to_bytes(8, "little")


def mem(opc, reg, base, disp, w=1):
    r, b = NUM[reg], NUM[base]
    pre = rex(w, r >> 3, 0, b >> 3)
    op = bytes([opc]) if isinstance(opc, int) else opc
    # SHORT forms.  `disp` is always a frame offset or a small literal -- never
    # a label address -- so its width is final in the sizing pass.  rm==5 has
    # no mod=00 form ([rip+disp32] takes that slot), but no base we use is
    # rbp/r13, so the guard is belt and braces.
    # rm == 4 means "SIB follows"; 0x24 is the SIB that names rsp itself.
    # The tape stack pointer is rsp now, so every [r7+disp] pays this byte.
    sib = b"\x24" if (b & 7) == 4 else b""
    if disp == 0 and (b & 7) != 5:
        return pre + op + modrm(0, r, b) + sib
    if -128 <= disp <= 127:
        return pre + op + modrm(1, r, b) + sib + (disp & 0xFF).to_bytes(1, "little")
    return pre + op + modrm(2, r, b) + sib + \
        (disp & 0xFFFFFFFF).to_bytes(4, "little")


def load_w(reg, base, disp, width):
    """sign-extending load of `width` bytes into a 64-bit register"""
    if width == 8:
        return mem(0x8B, reg, base, disp)
    if width == 4:
        return mem(0x63, reg, base, disp)             # movsxd
    return mem(b"\x0f\xbe" if width == 1 else b"\x0f\xbf",
               reg, base, disp)


# ---- floating point [TP] ---------------------------------------------------
# A floating value is its bit pattern in a general register.  Each op moves it
# into xmm0/xmm1 -- volatile under every ABI we target, and used nowhere
# else -- computes there, and moves the result back.  movq/movd copy bits.
def _sse_g(pfx, opc, xmm, gpr, w, gpr_in_reg=False):
    """an SSE op with a general-register operand: the mandatory prefix, then
    REX, then 0F opc.  Normally the xmm is ModRM.reg and the gpr ModRM.rm;
    cvttsd2si writes the gpr, so there it is reg and the xmm rm."""
    g = NUM[gpr]
    if gpr_in_reg:
        r = rex(w, g >> 3, 0, 0)
        mr = modrm(3, g, xmm)
    else:
        r = rex(w, 0, 0, g >> 3)
        mr = modrm(3, xmm, g)
    if r == b"\x40":
        r = b""                                   # no REX needed
    return bytes([pfx]) + r + bytes([0x0F, opc]) + mr


def _sse_x(pfx, opc, xd, xs, imm=None):
    """an SSE op between xmm registers (xmm0-7: no REX)"""
    out = (bytes([pfx]) if pfx else b"") + bytes([0x0F, opc]) + modrm(3, xd, xs)
    return out + (bytes([imm]) if imm is not None else b"")


def _movq_x(x, g):  return _sse_g(0x66, 0x6E, x, g, 1)    # movq xmm, r64
def _movq_g(g, x):  return _sse_g(0x66, 0x7E, x, g, 1)    # movq r64, xmm
def _movd_x(x, g):  return _sse_g(0x66, 0x6E, x, g, 0)    # movd xmm, r32
def _movd_g(g, x):  return _sse_g(0x66, 0x7E, x, g, 0)    # movd r32, xmm (zero-extends)


def _and1(g):
    d = NUM[g]
    return rex(1, 0, 0, d >> 3) + b"\x83" + modrm(3, 4, d) + b"\x01"


FARITH = {"fadd": 0x58, "fsub": 0x5C, "fmul": 0x59, "fdiv": 0x5E}
# CMPSD/CMPSS predicates: EQ_OQ, LT_OS, LE_OS -- each FALSE when either side
# is NaN, which is C's answer, and no flag or parity juggling is needed
FCMP = {"feq": 0, "flt": 1, "fle": 2}
FP_OPS = tuple(k + b for k in list(FARITH) + list(FCMP)
               for b in ("64", "32")) + (
    "cvtid", "cvtud", "cvtis", "cvtus", "cvtdi", "cvtdu", "cvtsd", "cvtds",
    "fsqrt64", "fsqrt32")


def _u2f(ra, dbl):
    """u64 -> double/float: there is no such instruction below AVX-512.  With
    the top bit clear the signed convert is exact; with it set, halve the
    value keeping the lost bit sticky, convert, and double -- the standard
    sequence, and correctly rounded."""
    pfx = 0xF2 if dbl else 0xF3
    a = NUM[ra]
    test = rex(1, a >> 3, 0, a >> 3) + b"\x85" + modrm(3, a, a)
    small = _sse_g(pfx, 0x2A, 0, ra, 1)                          # cvtsi2s? xmm0, ra
    big = mov_rr("r11", ra) + rex(1, 0, 0, 1) + b"\xd1" + modrm(3, 5, 11) + \
        mov_rr("rbx", ra) + _and1("rbx") + _alu(0x09, "r11", "rbx") + \
        _sse_g(pfx, 0x2A, 0, "r11", 1) + _sse_x(pfx, 0x58, 0, 0)  # + xmm0, xmm0
    small += b"\xeb" + bytes([len(big)])                         # jmp over big
    return test + b"\x78" + bytes([len(small)]) + small + big      # js big


def _fp(o, a):
    if o[:4] in FARITH or o[:3] in FCMP:
        dbl = o.endswith("64")
        ld, st = (_movq_x, _movq_g) if dbl else (_movd_x, _movd_g)
        pfx = 0xF2 if dbl else 0xF3
        out = ld(0, a[1]) + ld(1, a[2])
        if o[:4] in FARITH:
            return out + _sse_x(pfx, FARITH[o[:4]], 0, 1) + st(a[0], 0)
        out += _sse_x(pfx, 0xC2, 0, 1, FCMP[o[:3]])             # cmpsd/cmpss
        return out + st(a[0], 0) + _and1(a[0])
    if o == "cvtid":
        return _sse_g(0xF2, 0x2A, 0, a[1], 1) + _movq_g(a[0], 0)
    if o == "cvtis":
        return _sse_g(0xF3, 0x2A, 0, a[1], 1) + _movd_g(a[0], 0)
    if o == "cvtud":
        return _u2f(a[1], True) + _movq_g(a[0], 0)
    if o == "cvtus":
        return _u2f(a[1], False) + _movd_g(a[0], 0)
    if o == "cvtdi":                                  # cvttsd2si r64, xmm0
        return _movq_x(0, a[1]) + _sse_g(0xF2, 0x2C, 0, a[0], 1, True)
    if o == "cvtdu":
        # below 2^63 the signed truncation is right; above it, subtract 2^63
        # first and put the top bit back afterwards
        two63 = 0x43E0000000000000
        pre = _movq_x(0, a[1]) + mov_ri("r11", two63) + _movq_x(1, "r11") + \
            _sse_x(0x66, 0x2E, 0, 1)                              # ucomisd
        small = _sse_g(0xF2, 0x2C, 0, a[0], 1, True)
        big = _sse_x(0xF2, 0x5C, 0, 1) + _sse_g(0xF2, 0x2C, 0, a[0], 1, True) + \
            mov_ri("r11", 1 << 63) + _alu(0x31, a[0], "r11")
        small += b"\xeb" + bytes([len(big)])
        return pre + b"\x73" + bytes([len(small)]) + small + big  # jae big
    if o == "cvtsd":
        return _movd_x(0, a[1]) + _sse_x(0xF3, 0x5A, 0, 0) + _movq_g(a[0], 0)
    if o == "cvtds":
        return _movq_x(0, a[1]) + _sse_x(0xF2, 0x5A, 0, 0) + _movd_g(a[0], 0)
    dbl = o == "fsqrt64"
    ld, st = (_movq_x, _movq_g) if dbl else (_movd_x, _movd_g)
    return ld(0, a[1]) + _sse_x(0xF2 if dbl else 0xF3, 0x51, 0, 0) + st(a[0], 0)


def store_w(reg, base, disp, width):
    if width == 8:
        return mem(0x89, reg, base, disp)
    if width == 4:
        return mem(0x89, reg, base, disp, w=0)
    if width == 2:
        return b"\x66" + mem(0x89, reg, base, disp, w=0)
    return mem(0x88, reg, base, disp, w=0)


from .bits import MASK64
from .catalog import ENCSPEC as _ENC
ALU2 = _ENC["x86_64"]["alu2"]         # [I5] one table, both back ends
SETCC = _ENC["x86_64"]["setcc"]
SHIFTEXT = _ENC["x86_64"]["shiftext"]
SCRATCH = 11                                                      # r11
SCR = "r11"       # neither r11 nor rbx is in REGMAP, so neither is a tape
SCR2 = "rbx"      # register; we exit by syscall and never return to a caller


def _alias(dst, s1, s2):
    """x86 ALU is two-operand: `dst = s1 op s2` becomes `mov dst,s1; op dst,s2`.
    When `dst` IS `s2` the mov destroys the right-hand operand first, so
    `17 - 5` computed `17 - 17`.  The interpreter and arm64 (three-operand) are
    both immune, which is why this survived until an x86_64 kernel ran the
    code.  [I-14]"""
    if dst == s1:
        return b"", s2
    if s2 == dst:
        return mov_rr(SCR, s2), SCR
    return b"", s2


def cmp_set(cc, dst, ra, rb):
    out = _alu(0x39, ra, rb)                       # cmp ra, rb
    out += b"\x41\x0f" + bytes([cc]) + modrm(3, 0, SCRATCH)   # setcc r11b
    d = NUM[dst]
    out += rex(1, d >> 3, 0, 1) + b"\x0f\xb6" + modrm(3, d, SCRATCH)
    return out


def REGS8():
    from .catalog import REGMAP
    return REGMAP["x86_64"]


def _spsub(n):
    return rex(1, 0, 0, 0) + b"\x83" + modrm(3, 5, 4) + bytes([n])


# Win64 requires rsp to be 16-byte aligned at the call, and kernel32 uses
# aligned SSE moves, so getting it wrong is an access violation inside the
# callee rather than a polite error.  We cannot assume what rsp is -- the tape
# never touches it -- so align it explicitly and put it back from rbx, which
# the callee is required to preserve.  [I-18]
# the command-line splitter: rsi = read, rdi = write, rcx = argc, r8 = argv[]
WINARGS_BODY = bytes.fromhex(
    "0fb6063c2074043c09750548ffc6ebf084c074494883f93f7d4349893cc848ffc14531c90fb60684c0742f3c2275094183f10148ffc6ebec4585c975083c20740e3c09740a880748ffc748ffc6ebd5c6070048ffc748ffc6eba6c60700")


def _align_pre(extra):
    n = 32 + ((extra * 8 + 15) // 16) * 16
    return (mov_rr("rbx", "rsp")                      # rbx = rsp
            + rex(1, 0, 0, 0) + b"\x83" + modrm(3, 4, 4) + b"\xf0"  # and rsp,-16
            + _spsub(n)), n


def _align_post():
    return mov_rr("rsp", "rbx")


def _stackarg(slot, val):
    return (rex(1, 0, 0, 0) + b"\xc7" + modrm(1, 0, 4) + b"\x24"
            + bytes([slot]) + (val & 0xFFFFFFFF).to_bytes(4, "little"))


def _callimp(pc, imps, name):
    """Load the IAT slot, then call the register."""
    a = (imps or {}).get("__imp_" + name, 0)
    out = rip(0x8B, "rax", pc + 7, a)            # mov rax, [rip+disp32]
    return out + rex(0, 0, 0, 0)[:0] + b"\xff\xd0"   # call rax


def _fd2handle_x86(pc, hstd):
    """rcx holds an fd; 0/1/2 name a standard handle, higher IS one."""
    out = rex(1, 0, 0, 0) + b"\x83" + modrm(3, 7, 1) + b"\x03"   # cmp rcx, 3
    body = rip(0x8D, SCR, pc + 4 + 2 + 7, hstd)                   # lea r11
    body += rex(1, 0, 0, 1) + b"\x8b" + modrm(0, 1, 4) + b"\xcb"  # mov rcx,
    out += b"\x73" + bytes([len(body)])                           # jae over
    return out + body


def _winapi(ins, off, shift, text_va, imps):
    """a WinAPI gate: the op's own argument moves and call (the body), then
    the conversion of its answer to POSIX's (the tail) -- which import it
    calls and which tail it takes are the abi table's `winimp` and
    `retconv` [I4]"""
    m = ins.meta
    out = _winbody(ins, off, shift, text_va, imps)
    if out is None:
        return None
    return out + _wintail(m.get("retconv", "none"), text_va + off + len(out),
                          m.get("written", 0) + shift)


def _wintail(rc, pc, written):
    if rc == "wcount":                                   # the bytes moved
        return rip(0x8B, "rax", pc + 7, written)
    if rc == "bool_inv":                                 # BOOL -> 0 ok, 1 not
        return (rex(1, 0, 0, 0) + b"\x83" + modrm(3, 7, 0) + b"\x00"   # cmp rax,0
                + b"\x0f\x94\xc0" + rex(1, 0, 0, 0) + b"\x0f\xb6\xc0")   # sete; movzx
    if rc == "bool_neg":                                 # BOOL -> 0 ok, -1 not
        return (b"\x85\xc0" + b"\x0f\x94\xc0"             # test eax; sete al
                + b"\x48\x0f\xb6\xc0" + b"\x48\xf7\xd8")  # movzx; neg rax
    if rc == "dword_sx":                                 # DWORD, -1 on failure
        return b"\x48\x63\xc0"                          # movsxd rax, eax
    return b""


def _winbody(ins, off, shift, text_va, imps):
    m = ins.meta
    imp = m.get("winimp")
    op = m.get("catop")
    hstd = m.get("hstd", 0) + shift
    written = m.get("written", 0) + shift
    pc = text_va + off
    if op == "exit":
        out, _ = _align_pre(0)
        out += _callimp(pc + len(out), imps, imp)
        return out + _align_post()
    if op in ("write", "read"):
        out = _fd2handle_x86(pc, hstd)
        out += rip(0x8D, "r9", pc + len(out) + 7, written)   # r9 = &written
        pre, _ = _align_pre(1)
        out += pre
        out += _stackarg(32, 0)                              # lpOverlapped
        out += _callimp(pc + len(out), imps, imp)
        return out + _align_post()
    if op == "mmap":
        # VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect) --
        # four arguments, which is exactly what Win64 passes in registers
        pre, _ = _align_pre(0)
        out = pre + _callimp(pc + len(pre), imps, imp)
        return out + _align_post()
    if op == "mprotect":
        # VirtualProtect(addr, size, newProtect, &old): the old protection
        # has to go somewhere, and the WriteFile scratch cell is free here
        scr0 = m.get("scr0", 0) + shift
        scr1 = m.get("scr1", 0) + shift
        out = rip(0x8D, "r9", pc + 7, written)                # r9 = &old
        pre, _ = _align_pre(0)
        out += pre
        out += _callimp(pc + len(out), imps, imp)
        out += _align_post()
        # Windows on arm64 will not execute code that is still only in the
        # data cache, and changing the protection does not flush it: the
        # first instruction raises STATUS_ILLEGAL_INSTRUCTION.  The current
        # process is the pseudo-handle -1, so nothing else is imported.
        out += mov_ri("rcx", (-1) & MASK64)
        out += rip(0x8B, "rdx", pc + len(out) + 7, scr0)      # the address
        out += rip(0x8B, "r8", pc + len(out) + 7, scr1)       # the length
        pre2, _ = _align_pre(0)
        out += pre2
        out += _callimp(pc + len(out), imps, "FlushInstructionCache")
        return out + _align_post()                # POSIX's 0/1: the tail
    if op == "munmap":
        out = mov_ri("r8", 0x8000)                      # MEM_RELEASE
        out += mov_ri("rdx", 0)                         # dwSize must be 0
        pre, _ = _align_pre(0)
        out += pre
        out += _callimp(pc + len(out), imps, imp)
        return out + _align_post()
    if op == "close":
        out = _fd2handle_x86(pc, hstd)
        pre, _ = _align_pre(0)
        out += pre
        out += _callimp(pc + len(out), imps, imp)
        return out + _align_post()
    if op == "open":
        # arg1 (rdx) is already dwDesiredAccess and arg2 (r8) is
        # dwCreationDisposition -- the C library builds both, because only it
        # knows the platform.  The disposition is CreateFileA's FIFTH
        # parameter, so it has to move to the shadow-space slot before r8
        # becomes the share mode.
        out = bytearray(mov_rr("rax", "r8"))      # stash the disposition
        pre, _ = _align_pre(3)
        out += pre
        # [rsp+32] = dwCreationDisposition -- CreateFileA's fifth parameter,
        # so it cannot stay in r8, which is the third
        out += rex(1, 0, 0, 0) + b"\x89" + modrm(1, 0, 4) + b"\x24\x20"
        out += mov_ri("r8", 3)                               # FILE_SHARE_R|W
        out += mov_ri("r9", 0)                               # no security
        out += _stackarg(40, 0x80)                           # FILE_ATTR_NORMAL
        out += _stackarg(48, 0)                              # hTemplateFile
        out += _callimp(pc + len(out), imps, imp)
        return bytes(out) + _align_post()
    if op == "lseek":
        # SetFilePointer(handle, low, NULL, method): SEEK_SET/CUR/END are
        # FILE_BEGIN/CURRENT/END.  It answers a DWORD, -1 on failure:
        # sign-extended, that is POSIX's -1 (files under 2 GB).
        out = mov_rr("r9", "r8") + mov_ri("r8", 0)
        out += _fd2handle_x86(pc + len(out), hstd)
        pre, _ = _align_pre(0)
        out += pre
        out += _callimp(pc + len(out), imps, imp)
        return out + _align_post()
    if op in ("unlink", "rename"):
        # DeleteFileA(path) / MoveFileExA(old, new, REPLACE_EXISTING): a BOOL,
        # which POSIX spells 0 / -1
        out = mov_ri("r8", 1) if op == "rename" else b""
        pre, _ = _align_pre(0)
        out += pre
        out += _callimp(pc + len(out), imps, imp)
        return out + _align_post()
    return None


def _itoa(pc, src, buf, lenp):
    """`.print`: the signed value at `src` in decimal at `buf`, its length at
    `lenp`.  It had no encoding (ud2) -- only the self-hosted compiler emits
    `.print`.  r11-r15 and rbx are not tape registers; div needs rax/rdx,
    which the syscall that follows loads afresh anyway.  Digits are counted,
    then written from the end.  |v| is taken unsigned: -LONG_MIN is fine."""
    def r(opc, reg, target, out):      # rip-relative at the current position
        return rip(opc, reg, pc + len(out) + 7, target)
    out = bytearray()
    out += r(0x8B, "rax", src, out)                              # rax = v
    out += b"\x4d\x31\xe4"                                        # xor r12, r12
    out += b"\x48\x85\xc0"                                        # test rax, rax
    neg = b"\x48\xf7\xd8" + mov_ri("r12", 1)                      # neg rax; r12 = 1
    out += b"\x79" + bytes([len(neg)]) + neg                       # jns over
    out += mov_rr("r15", "rax") + mov_ri("r11", 10)
    out += mov_rr("r13", "rax") + b"\x4d\x31\xf6"                  # r13 = |v|; r14 = 0
    loop = mov_rr("rax", "r13") + b"\x48\x31\xd2" + b"\x49\xf7\xf3"   # div r11
    loop += mov_rr("r13", "rax") + b"\x49\xff\xc6"                 # inc r14
    loop += b"\x4d\x85\xed"                                        # test r13, r13
    out += loop + b"\x75" + bytes([(-(len(loop) + 2)) & 0xFF])      # jnz loop
    out += b"\x4d\x01\xe6"                                        # r14 += r12
    out += r(0x89, "r14", lenp, out)                             # *lenp = r14
    out += r(0x8D, "rbx", buf, out)                              # rbx = buf
    out += b"\x4e\x8d\x2c\x33"                                    # lea r13, [rbx+r14]
    loop = mov_rr("rax", "r15") + b"\x48\x31\xd2" + b"\x49\xf7\xf3"
    loop += mov_rr("r15", "rax") + b"\x48\x83\xc2\x30"             # rdx += '0'
    loop += b"\x49\xff\xcd" + b"\x41\x88\x55\x00"                # dec r13; mov [r13], dl
    loop += b"\x4d\x85\xff"                                        # test r15, r15
    out += loop + b"\x75" + bytes([(-(len(loop) + 2)) & 0xFF])      # jnz loop
    tail = b"\xc6\x03\x2d"                                         # mov byte [rbx], '-'
    out += b"\x4d\x85\xe4" + b"\x74" + bytes([len(tail)]) + tail    # test r12; jz over
    return bytes(out)


def _sp():
    from .catalog import REGMAP
    return REGMAP["x86_64"][7]          # the tape SP -- rsp, since [S-15 B1]


def push(reg):
    d = NUM[reg] if isinstance(reg, str) else reg
    return (rex(0, 0, 0, 1) if d >= 8 else b"") + bytes([0x50 | (d & 7)])


def pop(reg):
    d = NUM[reg] if isinstance(reg, str) else reg
    return (rex(0, 0, 0, 1) if d >= 8 else b"") + bytes([0x58 | (d & 7)])


def alu_imm(reg, opc, n):
    """`op r64, imm` with /opc.  SHORT form: 0x83 takes a sign-extended imm8,
    four bytes rather than seven.  `n` is always a frame size or a literal --
    never a label address -- so the width is final in the sizing pass."""
    d = NUM[reg] if isinstance(reg, str) else reg
    pre = rex(1, 0, 0, d >> 3)
    if -128 <= n <= 127:
        return pre + b"\x83" + modrm(3, opc, d) + bytes([n & 0xFF])
    return pre + b"\x81" + modrm(3, opc, d) + \
        (n & 0xFFFFFFFF).to_bytes(4, "little")


def _spadj(n, opc):
    """opc 5 = sub, 0 = add, on the tape SP"""
    return alu_imm(_sp(), opc, n)


def rip(opc, reg, pc_next, target):
    """[rip+disp32], 7 bytes fixed."""
    r = NUM[reg]
    d = target - pc_next
    return rex(1, r >> 3, 0, 0) + bytes([opc]) + modrm(0, r, 5) + \
        (d & 0xFFFFFFFF).to_bytes(4, "little")


def encode(ins, off, labels, arch="x86_64", syms=None, shift=0,
           text_va=0, imps=None, short=False):
    """-> bytes, or None when the op has no encoding here.  `short`: the
    assembler has found this branch's target within a signed byte."""
    o, a = ins.op, ins.args
    if o == "mov":
        return mov_rr(a[0], a[1])
    if o == "imm":
        return mov_ri(a[0], a[1])
    if o in ALU2:
        pre, src2 = _alias(a[0], a[1], a[2])
        if a[0] != a[1]:
            pre += mov_rr(a[0], a[1])
        return pre + _alu(ALU2[o], a[0], src2)
    if o in SHIFTEXT:
        # The count has to be in cl -- and rcx is a TAPE register (r4, also
        # arg3), so it must be saved and put back.  Do the whole thing in the
        # scratches so neither operand can be the register we are about to
        # clobber.  [I-14]
        out = mov_rr(SCR, a[1]) + mov_rr(SCR2, a[2])
        out += push("rcx")                                   # save tape rcx
        out += mov_rr("rcx", SCR2)
        out += rex(1, 0, 0, 1) + b"\xd3" + \
            modrm(3, SHIFTEXT[o], NUM[SCR])
        out += pop("rcx")                                    # restore
        return out + mov_rr(a[0], SCR)
    if o == "mul64":
        pre, src2 = _alias(a[0], a[1], a[2])
        if a[0] != a[1]:
            pre += mov_rr(a[0], a[1])
        d, s2 = NUM[a[0]], NUM[src2]
        return pre + rex(1, d >> 3, 0, s2 >> 3) + b"\x0f\xaf" + modrm(3, d, s2)
    if o == "load64":
        return load_w(a[0], a[1], a[2], 8)
    if o == "store64":
        return store_w(a[2], a[0], a[1], 8)
    if o in SETCC:
        return cmp_set(SETCC[o], a[0], a[1], a[2])
    if o == "setreg":
        k, v = a[1]
        if k == "imm":
            return mov_ri(a[0], v)
        if k == "reg":
            return mov_rr(a[0], v)
        if k == "addr":                              # lea: the address itself
            return rip(0x8D, a[0], text_va + off + 7, v + shift)
        return rip(0x8B, a[0], text_va + off + 7, v + shift)
    if o == "setmem":
        return rip(0x89, a[1], text_va + off + 7, a[0] + shift)
    if o == ".frame":
        n = a[0]
        opc = 5 if n >= 0 else 0                    # /5 sub, /0 add
        return alu_imm(_sp(), opc, abs(n))          # rsp, since [S-15 B1]
    if o == ".lea":                                  # movabs, fixed 10 bytes
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
        return rip(0x8D, a[0], text_va + off + 7, addr)   # lea
    if o == ".ld":
        return load_w(a[0], a[1], a[2], a[3])
    if o == ".st":
        return store_w(a[2], a[0], a[1], a[3])
    if o in FP_OPS:
        return _fp(o, a)
    if o == ".zero":
        # n bytes at [base+disp] <- 0, from r11 (never a tape register),
        # cleared once, in the widest pieces that fit.  It fell through to
        # ud2 and nobody noticed: no tape had reached a native image with it.
        out, k, n = b"\x4d\x31\xdb", 0, a[2]        # xor r11, r11
        while k < n:
            wd = 8
            while k + wd > n:
                wd //= 2
            out += store_w(SCR, a[0], a[1] + k, wd)
            k += wd
        return out
    if o == "callr":                                 # call r64: FF /2
        t = NUM[a[0]]
        return (rex(0, 0, 0, 1) if t >= 8 else b"") + b"\xff" + modrm(3, 2, t)
    if o == "push":
        return push(a[0])
    if o == "pop":
        return pop(a[0])
    if o in (".div", ".mod", ".udiv", ".umod"):
        # idiv writes rax/rdx, which are tape registers here, so they have to be
        # saved.  NOT with `push`/`pop`: `spinit` binds the tape SP to the real
        # rsp, so the tape stack starts exactly where a real push would write,
        # and the two clobber each other.  The symptom is a corrupted return
        # address -- every x86_64 program that printed an integer died, while
        # the interpreter and arm64 were fine.  [I-13]  Save through the TAPE
        # stack instead, which is what I-10 says the stack is.
        sp = _sp()
        out = _spadj(16, 5)
        out += mem(0x89, "rax", sp, 0)               # mov [SP], rax
        out += mem(0x89, "rdx", sp, 8)               # mov [SP+8], rdx
        out += mov_rr("r11", a[2])
        out += mov_rr("rax", a[1])
        if o[1] == "u":
            out += rex(1, 0, 0, 0) + b"\x31" + modrm(3, 2, 2)  # xor rdx, rdx
            out += rex(1, 0, 0, 1) + b"\xf7" + modrm(3, 6, 11)  # div r11
        else:
            out += b"\x48\x99"                       # cqo
            out += rex(1, 0, 0, 1) + b"\xf7" + modrm(3, 7, 11)  # idiv r11
        out += mov_rr("r11", "rax" if o in (".div", ".udiv") else "rdx")
        out += mem(0x8B, "rax", sp, 0)               # mov rax, [SP]
        out += mem(0x8B, "rdx", sp, 8)               # mov rdx, [SP+8]
        out += _spadj(16, 0)
        out += mov_rr(a[0], "r11")
        return out
    if o == "argsave":                               # SysV _start: argc at [rsp]
        if not (len(a) > 2 and a[2]):
            # Darwin: dyld CALLS the LC_MAIN entry -- argc in rdi, argv in
            # rsi, and [rsp] is a return address
            out = rip(0x89, "rdi", text_va + off + 7, a[0] + shift)
            return out + rip(0x89, "rsi", text_va + off + 14, a[1] + shift)
        out = rex(1, 0, 0, 0) + b"\x8b" + modrm(0, 0, 4) + b"\x24"
        out += rip(0x89, "rax", text_va + off + len(out) + 7, a[0] + shift)
        out += rex(1, 0, 0, 0) + b"\x8d" + modrm(1, 0, 4) + b"\x24\x08"
        out += rip(0x89, "rax", text_va + off + len(out) + 7, a[1] + shift)
        return out
    if o == "argvget":
        out = rip(0x8B, "r11", text_va + off + 7, a[2] + shift)
        t = NUM[a[1]]
        out += rex(1, 1, t >> 3, 1) + b"\x8b" + modrm(0, 11, 4) + \
            bytes([0xC3 | ((t & 7) << 3)])          # r11 = argv[t]
        return out + mov_rr(a[0], "r11")            # ...into the destination
    if o == "spinit":
        if len(a) > 1 and a[1] is not None:
            # Windows: the tape's own stack, reached rip-relative because the
            # image may slide
            return rip(0x8D, a[0], text_va + off + 7, a[1] + shift)
        return mov_rr(a[0], "rsp")               # mov rN, rsp
    if o == "winsave":
        out = rip(0x8D, SCR, text_va + off + 7, a[0] + shift)
        for k, r in enumerate(REGS8()):
            out += mem(0x89, r, SCR, 8 * k)
        return out
    if o == "winrest":
        out = mov_rr(SCR, "rax")                 # the call's result
        out += rip(0x8D, SCR2, text_va + off + len(out) + 7, a[0] + shift)
        for k, r in enumerate(REGS8()):
            if k:                                # r0 carries the result back
                out += mem(0x8B, r, SCR2, 8 * k)
        return out + mov_rr(a[1], SCR)
    if o == "winargs":                               # see emit_arm
        pre, _ = _align_pre(0)
        out = pre + _callimp(text_va + off + len(pre), imps, "GetCommandLineA")
        out += _align_post()
        out += bytes.fromhex("4889c64889c731c9")   # mov rsi,rax; mov rdi,rax; xor ecx,ecx
        out += rip(0x8D, "r8", text_va + off + len(out) + 7, a[2] + shift)
        out += WINARGS_BODY
        out += rip(0x89, "rcx", text_va + off + len(out) + 7, a[0] + shift)
        out += rip(0x89, "r8", text_va + off + len(out) + 7, a[1] + shift)
        return out
    if o == "winstdh":
        out = b""
        for k in range(3):
            out += mov_ri("rcx", (-10 - k) & MASK64)
            pre, _ = _align_pre(0)
            out += pre
            out += _callimp(text_va + off + len(out), imps, "GetStdHandle")
            out += _align_post()
            out += rip(0x8D, SCR, text_va + off + len(out) + 7, a[0] + shift)
            out += mem(0x89, "rax", SCR, 8 * k)
        return out
    if o == "ret":
        return b"\xc3"
    if o == "nop":
        return b"\x90"
    if o == "itoa":
        return _itoa(text_va + off, a[0] + shift, a[1] + shift, a[2] + shift)
    if o == "gate":
        if ins.meta.get("form") == "winapi":
            return _winapi(ins, off, shift, text_va, imps)
        out = b"\x0f\x05"                                  # syscall
        if ins.meta.get("carry"):
            # Darwin: CF set means failure, rax holds errno.  `jnc +3` over
            # `neg rax`, so the caller sees -errno like everywhere else [I-20]
            out += b"\x73\x03" + b"\x48\xf7\xd8"
        return out
    if o == "jump":
        if short:                                    # jmp rel8
            return b"\xeb" + _rel8(labels[a[0]] - (off + 2))
        return b"\xe9" + _rel(ins, labels[a[0]] - (off + 5))
    if o == "call":                                  # call rel32
        return b"\xe8" + _rel(ins, labels[a[0]] - (off + 5))
    if o == "jumpz":
        r = NUM[a[0]]
        test = rex(1, r >> 3, 0, r >> 3) + b"\x85" + modrm(3, r, r)
        if short:                                    # jz rel8
            return test + b"\x74" + _rel8(labels[a[1]] - (off + len(test) + 2))
        return test + b"\x0f\x84" + _rel(ins, labels[a[1]] - (off + len(test) + 6))
    return None


# the reloc stage's answer is the displacement's width (see emit_arm)
RELBYTES = {"rel32": 4}


def _rel(ins, d):
    n = RELBYTES[ins.meta["reloc"]]
    return (d & ((1 << (8 * n)) - 1)).to_bytes(n, "little")


class _Zero:
    """`labels` as the sizing pass sees it: every name present, every
    address 0.  A view, not a copy -- building `{k: 0 for k in labels}`
    per instruction made sizing unisacc.c (123k insns x 5k labels) cost
    17 s of a 28 s target; this costs nothing per call."""
    __slots__ = ("_l",)

    def __init__(self, labels):
        self._l = labels

    def __contains__(self, k):
        return k in self._l

    def __getitem__(self, k):
        if k in self._l:
            return 0
        raise KeyError(k)


def size(ins, labels, short=False):
    b = encode(ins, 0, _Zero(labels), "x86_64", {}, 0, 0, None, short)
    return len(b) if b is not None else len(UD2)


def short_size(ins):
    """The length of this branch's short form, or None if it has none.
    `jump` and `jumpz` do; `call` does not -- its jmp is part of a fixed
    sequence whose return address is computed from the lengths."""
    if ins.op == "jump":
        return 2
    if ins.op == "jumpz":
        return 3 + 2                     # test r, r (REX 85 modrm) + jz rel8
    return None


def branch_target(ins):
    return ins.args[0] if ins.op == "jump" else ins.args[1]


def _rel8(d):
    assert -128 <= d <= 127, "rel8 out of range: %d" % d
    return (d & 0xFF).to_bytes(1, "little")
