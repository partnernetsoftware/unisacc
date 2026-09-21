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
    return rex(1, 0, 0, d >> 3) + bytes([0xB8 + (d & 7)]) + \
        (imm & ((1 << 64) - 1)).to_bytes(8, "little")


def mem(opc, reg, base, disp, w=1):
    r, b = NUM[reg], NUM[base]
    pre = rex(w, r >> 3, 0, b >> 3)
    op = bytes([opc]) if isinstance(opc, int) else opc
    return pre + op + modrm(2, r, b) + \
        (disp & 0xFFFFFFFF).to_bytes(4, "little")


def load_w(reg, base, disp, width):
    """sign-extending load of `width` bytes into a 64-bit register"""
    if width == 8:
        return mem(0x8B, reg, base, disp)
    if width == 4:
        return mem(0x63, reg, base, disp)             # movsxd
    return mem(b"\x0f\xbe" if width == 1 else b"\x0f\xbf",
               reg, base, disp)


def store_w(reg, base, disp, width):
    if width == 8:
        return mem(0x89, reg, base, disp)
    if width == 4:
        return mem(0x89, reg, base, disp, w=0)
    if width == 2:
        return b"\x66" + mem(0x89, reg, base, disp, w=0)
    return mem(0x88, reg, base, disp, w=0)


ALU2 = {"add64": 0x01, "sub64": 0x29, "xor64": 0x31,
        "and64": 0x21, "or64": 0x09}
SETCC = {"slt64": 0x9C, "sle64": 0x9E, "eq": 0x94, "ne": 0x95,
         "ult64": 0x92, "ule64": 0x96}        # l, le, e, ne, b, be
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


def abs_mem(opc, reg, addr):
    """mod=00 rm=100 + SIB 0x25 -> [disp32]"""
    r = NUM[reg]
    return rex(1, r >> 3, 0, 0) + bytes([opc]) + modrm(0, r, 4) + \
        b"\x25" + (addr & 0xFFFFFFFF).to_bytes(4, "little")


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


def _spadd(n):
    return rex(1, 0, 0, 0) + b"\x83" + modrm(3, 0, 4) + bytes([n])


# Win64 requires rsp to be 16-byte aligned at the call, and kernel32 uses
# aligned SSE moves, so getting it wrong is an access violation inside the
# callee rather than a polite error.  We cannot assume what rsp is -- the tape
# never touches it -- so align it explicitly and put it back from rbx, which
# the callee is required to preserve.  [I-18]
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
    m = ins.meta
    op = m.get("catop")
    hstd = m.get("hstd", 0) + shift
    written = m.get("written", 0) + shift
    pc = text_va + off
    if op == "exit":
        out, _ = _align_pre(0)
        out += _callimp(pc + len(out), imps, "ExitProcess")
        return out + _align_post()
    if op in ("write", "read"):
        out = _fd2handle_x86(pc, hstd)
        out += rip(0x8D, "r9", pc + len(out) + 7, written)   # r9 = &written
        pre, _ = _align_pre(1)
        out += pre
        out += _stackarg(32, 0)                              # lpOverlapped
        out += _callimp(pc + len(out), imps,
                        "WriteFile" if op == "write" else "ReadFile")
        out += _align_post()
        out += rip(0x8B, "rax", pc + len(out) + 7, written)
        return out
    if op == "close":
        out = _fd2handle_x86(pc, hstd)
        pre, _ = _align_pre(0)
        out += pre
        out += _callimp(pc + len(out), imps, "CloseHandle")
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
        out += _callimp(pc + len(out), imps, "CreateFileA")
        return bytes(out) + _align_post()
    return None


def _sp():
    from .catalog import REGMAP
    return REGMAP["x86_64"][7]          # the tape SP, not rsp


def _spadj(n, opc):
    """opc 5 = sub, 0 = add, on the tape SP"""
    d = NUM[_sp()]
    return rex(1, 0, 0, d >> 3) + b"\x81" + modrm(3, opc, d) + \
        (n & 0xFFFFFFFF).to_bytes(4, "little")


def rip(opc, reg, pc_next, target):
    """[rip+disp32], 7 bytes fixed."""
    r = NUM[reg]
    d = target - pc_next
    return rex(1, r >> 3, 0, 0) + bytes([opc]) + modrm(0, r, 5) + \
        (d & 0xFFFFFFFF).to_bytes(4, "little")


def encode(ins, off, labels, arch="x86_64", syms=None, shift=0,
           text_va=0, imps=None):
    """-> bytes, or None when the op has no encoding here."""
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
    if o in ("shl64", "shr64", "lshr64"):
        # The count has to be in cl -- and rcx is a TAPE register (r4, also
        # arg3), so it must be saved and put back.  Do the whole thing in the
        # scratches so neither operand can be the register we are about to
        # clobber.  [I-14]
        out = mov_rr(SCR, a[1]) + mov_rr(SCR2, a[2])
        out += _spadj(8, 5) + mem(0x89, "rcx", _sp(), 0)     # save tape rcx
        out += mov_rr("rcx", SCR2)
        out += rex(1, 0, 0, 1) + b"\xd3" + \
            modrm(3, {"shl64": 4, "shr64": 7, "lshr64": 5}[o], NUM[SCR])
        out += mem(0x8B, "rcx", _sp(), 0) + _spadj(8, 0)     # restore
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
        return rip(0x8B, a[0], text_va + off + 7, v + shift)
    if o == "setmem":
        return rip(0x89, a[1], text_va + off + 7, a[0] + shift)
    if o == ".frame":
        d = NUM[a[0]] if False else NUM["r10"]
        n = a[0]
        opc = 5 if n >= 0 else 0                    # /5 sub, /0 add
        return rex(1, 0, 0, d >> 3) + b"\x81" + modrm(3, opc, d) + \
            (abs(n) & 0xFFFFFFFF).to_bytes(4, "little")
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
    if o == "callr":
        lea = rip(0x8D, "r11", text_va + off + 7, text_va + off + 20)
        sub = rex(1, 0, 0, 1) + b"\x81" + modrm(3, 5, 10) + \
            (8).to_bytes(4, "little")
        st = rex(1, 1, 0, 1) + b"\x89" + modrm(0, 11, 10)
        t = NUM[a[0]]
        return lea + sub + st + rex(0, 0, 0, t >> 3) + b"\xff" + \
            modrm(3, 4, t)
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
        out = rex(1, 0, 0, 0) + b"\x8b" + modrm(0, 0, 4) + b"\x24"
        out += rip(0x89, "rax", text_va + off + len(out) + 7, a[0] + shift)
        out += rex(1, 0, 0, 0) + b"\x8d" + modrm(1, 0, 4) + b"\x24\x08"
        out += rip(0x89, "rax", text_va + off + len(out) + 7, a[1] + shift)
        return out
    if o == "argvget":
        out = rip(0x8B, "r11", text_va + off + 7, a[2] + shift)
        t = NUM[a[1]]
        out += rex(1, 3, t >> 3, 3) + b"\x8b" + modrm(0, 11, 4) + \
            bytes([0xC3 | ((t & 7) << 3)])
        return out
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
    if o == "winstdh":
        out = b""
        for k in range(3):
            out += mov_ri("rcx", (-10 - k) & 0xFFFFFFFFFFFFFFFF)
            pre, _ = _align_pre(0)
            out += pre
            out += _callimp(text_va + off + len(out), imps, "GetStdHandle")
            out += _align_post()
            out += rip(0x8D, SCR, text_va + off + len(out) + 7, a[0] + shift)
            out += mem(0x89, "rax", SCR, 8 * k)
        return out
    if o == "ret":                                   # pop and jump
        ld = rex(1, 1, 0, 1) + b"\x8b" + modrm(0, 11, 10)
        add = rex(1, 0, 0, 1) + b"\x81" + modrm(3, 0, 10) + \
            (8).to_bytes(4, "little")
        return ld + add + rex(0, 0, 0, 1) + b"\xff" + modrm(3, 4, 11)
    if o == "nop":
        return b"\x90"
    if o == "gate":
        if ins.meta.get("form") == "winapi":
            return _winapi(ins, off, shift, text_va, imps)
        return b"\x0f\x05"                                 # syscall
    if o == "jump":
        d = labels[a[0]] - (off + 5)
        return b"\xe9" + (d & 0xFFFFFFFF).to_bytes(4, "little")
    if o == "call":                                  # push ret addr on r10
        lea = rip(0x8D, "r11", text_va + off + 7, text_va + off + 22)
        sub = rex(1, 0, 0, 1) + b"\x81" + modrm(3, 5, 10) + \
            (8).to_bytes(4, "little")
        st = rex(1, 1, 0, 1) + b"\x89" + modrm(0, 11, 10)
        d = labels[a[0]] - (off + 22)
        return lea + sub + st + b"\xe9" + (d & 0xFFFFFFFF).to_bytes(4, "little")
    if o == "jumpz":
        r = NUM[a[0]]
        test = rex(1, r >> 3, 0, r >> 3) + b"\x85" + modrm(3, r, r)
        d = labels[a[1]] - (off + len(test) + 6)
        return test + b"\x0f\x84" + (d & 0xFFFFFFFF).to_bytes(4, "little")
    return None


def size(ins, labels):
    b = encode(ins, 0, {k: 0 for k in labels}, "x86_64", {}, 0, 0)
    return len(b) if b is not None else len(UD2)
