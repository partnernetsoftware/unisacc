"""Generate x86 FP encoding transitions, not host-encoded instruction bytes.

Opcode/predicate declarations are read from emit_x86. SSE layout, register
moves and unsigned conversion sequences are hand rules compiled into delta
transitions. The executor has no FP-encoding primitive. Operand registers
are the seven non-stack tape registers; xmm0/1 and r11/rbx are scratch.
"""
from unisa.emit_x86 import FARITH, FCMP, FP_OPS, NUM
from unisa.catalog import REGMAP

FP_IDS = {op: 100 + i for i, op in enumerate(FP_OPS)}


def install(E, byte):
    P, g = E.P, E.g

    def put(p, name, value):
        p.a(("LDI" if isinstance(value, int) else "COPYW", name, value))

    def sg(p, prefix, opcode, xmm, reg, width, reverse=False):
        for name, val in (("sg_p", prefix), ("sg_o", opcode), ("sg_x", xmm),
                          ("sg_g", reg), ("sg_w", width), ("sg_rev", int(reverse))):
            put(p, name, val)
        p.call("SG")

    def sx(p, prefix, opcode, dst=0, src=0, imm=None):
        for b in ([prefix] if prefix else []) + [0x0F, opcode, 0xC0 | dst << 3 | src]:
            byte(p, b)
        if imm is not None:
            byte(p, imm)

    def mov(p, dst, src):
        put(p, "al_o", 0x89); put(p, "al_d", dst); put(p, "al_s", src)
        p.call("ALU")

    def bits(p, reg, width, load=True, xmm=0):
        sg(p, 0x66, 0x6E if load else 0x7E, xmm, reg, width)

    def and1(p, reg):
        for name, val in (("rx_w", 1), ("rx_r", 0), ("rx_b", reg)):
            put(p, name, val)
        p.call("REX"); byte(p, 0x83)
        put(p, "mr_m", 3); put(p, "mr_r", 4); put(p, "mr_b", reg)
        p.call("MODRM"); byte(p, 1)

    def append(p, blob):
        p.a(("COPYW", "fp_blob", blob)).call("FP.copy")

    # Shared SSE instruction with a GPR: optional REX, direction-specific
    # extension bit and ModRM. 0x40 must be omitted for low 32-bit registers.
    p = P("SG")
    p.a(("OUTW", "sg_p"), ("ALUI", "shl", "sg_rex", "sg_w", 3),
        ("ALUI", "or", "sg_rex", "sg_rex", 0x40),
        ("ALUI", "sar", "sg_ext", "sg_g", 3)).branch({1: "SG.rev"}, "SG.fwd", [("RLD", "sg_rev")])
    P("SG.rev").a(("ALUI", "shl", "sg_ext", "sg_ext", 2),
        ("COPYW", "mr_r", "sg_g"), ("COPYW", "mr_b", "sg_x")).goto("SG.rex")
    P("SG.fwd").a(("COPYW", "mr_r", "sg_x"), ("COPYW", "mr_b", "sg_g")).goto("SG.rex")
    P("SG.rex").a(("ALU", "or", "sg_rex", "sg_rex", "sg_ext")).branch({1: "SG.op"}, "SG.pre", [("CMPI", "sg_rex", 0x40)])
    P("SG.pre").a(("OUTW", "sg_rex")).goto("SG.op")
    p = P("SG.op"); byte(p, 0x0F)
    p.a(("OUTW", "sg_o"), ("LDI", "mr_m", 3)).call("MODRM").ret()
    P("FP.copy").a(("INPUSH", "fp_blob")).goto("FP.cp")
    g.on("FP.cp", [256], "FP.cpend", [("INPOP",)])
    g.els("FP.cp", "FP.cp", [("COPY",), ("ADV",)])
    P("FP.cpend").ret()

    allowed = tuple(NUM[r] for r in REGMAP["x86_64"][:7])
    for op in FP_OPS:
        binary = op[:4] in FARITH or op[:3] in FCMP
        count = 3 if binary else 2
        p = P("FP." + op)
        p.branch({count: "FP." + op + ".r0"}, "DEAD.op", [("RLD", "na")])
        for i in range(count):
            P("FP." + op + ".r%d" % i).branch(
                {allowed: "FP." + op + ".r%d" % (i + 1) if i + 1 < count else "FP." + op + ".body"},
                "DEAD.op", [("RLD", "a%d" % i)])
        p = P("FP." + op + ".body")
        width = int(op.endswith("64"))
        prefix = 0xF2 if width else 0xF3
        if binary:
            bits(p, "a1", width); bits(p, "a2", width, xmm=1)
            if op[:4] in FARITH:
                sx(p, prefix, FARITH[op[:4]], src=1)
            else:
                sx(p, prefix, 0xC2, src=1, imm=FCMP[op[:3]])
            bits(p, "a0", width, False)
            if op[:3] in FCMP:
                and1(p, "a0")
        elif op in ("cvtid", "cvtis"):
            width = int(op == "cvtid")
            sg(p, 0xF2 if width else 0xF3, 0x2A, 0, "a1", 1)
            bits(p, "a0", width, False)
        elif op == "cvtdi":
            bits(p, "a1", 1); sg(p, 0xF2, 0x2C, 0, "a0", 1, True)
        elif op in ("cvtsd", "cvtds"):
            width = int(op == "cvtsd")
            bits(p, "a1", 1 - width)
            sx(p, 0xF3 if width else 0xF2, 0x5A)
            bits(p, "a0", width, False)
        elif op in ("fsqrt64", "fsqrt32"):
            bits(p, "a1", width); sx(p, prefix, 0x51); bits(p, "a0", width, False)
        elif op in ("cvtud", "cvtus"):
            width = int(op == "cvtud"); prefix = 0xF2 if width else 0xF3
            # Construct both blocks with delta operations and measure them;
            # branch displacements are never supplied by the Python referee.
            p.a(("OLEN", "fp_mark"))
            mov(p, 11, "a1")
            for b in (0x49, 0xD1, 0xEB): byte(p, b)  # shr r11,1
            mov(p, 3, "a1"); and1(p, 3)
            p.a(("LDI", "al_o", 0x09), ("LDI", "al_d", 11), ("LDI", "al_s", 3)).call("ALU")
            sg(p, prefix, 0x2A, 0, 11, 1); sx(p, prefix, 0x58)
            p.a(("OCUT", "fp_big", "fp_mark"))
            sg(p, prefix, 0x2A, 0, "a1", 1)
            byte(p, 0xEB); p.a(("BLEN", "fp_len", "fp_big"), ("OUTW", "fp_len"), ("OCUT", "fp_small", "fp_mark"))
            p.a(("LDI", "rx_w", 1), ("COPYW", "rx_r", "a1"), ("COPYW", "rx_b", "a1")).call("REX")
            byte(p, 0x85)
            p.a(("LDI", "mr_m", 3), ("COPYW", "mr_r", "a1"), ("COPYW", "mr_b", "a1")).call("MODRM")
            byte(p, 0x78); p.a(("BLEN", "fp_len", "fp_small"), ("OUTW", "fp_len"))
            append(p, "fp_small"); append(p, "fp_big"); bits(p, "a0", width, False)
        elif op == "cvtdu":
            bits(p, "a1", 1)
            for b in (0x49, 0xBB, 0, 0, 0, 0, 0, 0, 0xE0, 0x43): byte(p, b)  # r11 = double 2^63
            bits(p, 11, 1, xmm=1); sx(p, 0x66, 0x2E, src=1)
            p.a(("OLEN", "fp_mark"))
            sx(p, 0xF2, 0x5C, src=1); sg(p, 0xF2, 0x2C, 0, "a0", 1, True)
            for b in (0x49, 0xBB, 0, 0, 0, 0, 0, 0, 0, 0x80): byte(p, b)  # r11 = 1<<63
            p.a(("LDI", "al_o", 0x31), ("COPYW", "al_d", "a0"), ("LDI", "al_s", 11)).call("ALU")
            p.a(("OCUT", "fp_big", "fp_mark"))
            sg(p, 0xF2, 0x2C, 0, "a0", 1, True)
            byte(p, 0xEB); p.a(("BLEN", "fp_len", "fp_big"), ("OUTW", "fp_len"), ("OCUT", "fp_small", "fp_mark"))
            byte(p, 0x73); p.a(("BLEN", "fp_len", "fp_small"), ("OUTW", "fp_len"))
            append(p, "fp_small"); append(p, "fp_big")
        else:
            raise AssertionError("FP opcode has no encoding rule: " + op)
        p.goto("NEXTL")
