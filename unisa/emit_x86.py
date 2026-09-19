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
SETCC = {"slt64": 0x9C, "sle64": 0x9E, "eq": 0x94, "ne": 0x95}   # l, le, e, ne
SCRATCH = 11                                                      # r11


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


def rip(opc, reg, pc_next, target):
    """[rip+disp32], 7 bytes fixed."""
    r = NUM[reg]
    d = target - pc_next
    return rex(1, r >> 3, 0, 0) + bytes([opc]) + modrm(0, r, 5) + \
        (d & 0xFFFFFFFF).to_bytes(4, "little")


def encode(ins, off, labels, arch="x86_64", syms=None, shift=0, text_va=0):
    """-> bytes, or None when the op has no encoding here."""
    o, a = ins.op, ins.args
    if o == "mov":
        return mov_rr(a[0], a[1])
    if o == "imm":
        return mov_ri(a[0], a[1])
    if o in ALU2:
        pre = b"" if a[0] == a[1] else mov_rr(a[0], a[1])
        return pre + _alu(ALU2[o], a[0], a[2])
    if o in ("shl64", "shr64"):                      # shifts take cl
        pre = b"" if a[0] == a[1] else mov_rr(a[0], a[1])
        pre += mov_rr("rcx", a[2])
        d = NUM[a[0]]
        return pre + rex(1, 0, 0, d >> 3) + b"\xd3" + \
            modrm(3, 4 if o == "shl64" else 7, d)
    if o == "mul64":
        pre = b"" if a[0] == a[1] else mov_rr(a[0], a[1])
        d, s = NUM[a[0]], NUM[a[2]]
        return pre + rex(1, d >> 3, 0, s >> 3) + b"\x0f\xaf" + modrm(3, d, s)
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
    if o == "callr":
        lea = rip(0x8D, "r11", text_va + off + 7, text_va + off + 20)
        sub = rex(1, 0, 0, 1) + b"\x81" + modrm(3, 5, 10) + \
            (8).to_bytes(4, "little")
        st = rex(1, 1, 0, 1) + b"\x89" + modrm(0, 11, 10)
        t = NUM[a[0]]
        return lea + sub + st + rex(0, 0, 0, t >> 3) + b"\xff" + \
            modrm(3, 4, t)
    if o in (".div", ".mod"):
        # idiv writes rax/rdx, which are tape registers here, so bracket the
        # whole thing with real pushes and stage the divisor in r11.
        out = b"\x50\x52"                            # push rax, push rdx
        out += mov_rr("r11", a[2])
        out += mov_rr("rax", a[1])
        out += b"\x48\x99"                           # cqo
        out += rex(1, 0, 0, 1) + b"\xf7" + modrm(3, 7, 11)   # idiv r11
        out += mov_rr("r11", "rax" if o == ".div" else "rdx")
        out += b"\x5a\x58"                           # pop rdx, pop rax
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
    if o == "spinit":                                # mov rN, rsp
        return mov_rr(a[0], "rsp")
    if o == "ret":                                   # pop and jump
        ld = rex(1, 1, 0, 1) + b"\x8b" + modrm(0, 11, 10)
        add = rex(1, 0, 0, 1) + b"\x81" + modrm(3, 0, 10) + \
            (8).to_bytes(4, "little")
        return ld + add + rex(0, 0, 0, 1) + b"\xff" + modrm(3, 4, 11)
    if o == "nop":
        return b"\x90"
    if o == "gate":
        if ins.meta.get("form") == "winapi":
            return b"\xff\x15" + b"\x00\x00\x00\x00"      # call [rip+disp32]
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
