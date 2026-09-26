"""E5, first slice (research/e5-slice.md): the x86_64 encoder for straight-line
lowered instructions, as a delta for the generic executor.

    python3 exec/enc/gen.py delta.json

Input: TIns text, one instruction per line (`op arg, ...`; machine register
names; integers).  Output: the machine code bytes, as unisa/emit_x86.encode
writes them.  Ops: mov, imm, add64/sub64/xor64/and64/or64, mul64, load64,
store64, .ld/.st (1, 2, 4, 8 bytes), the six setcc ops, ret; anything else is
rejected as not covered.

Read, not copied: catalog.ENCSPEC's alu2 opcodes and setcc bytes, and
emit_x86.NUM's register numbers (the reference's declaration, read at generation
and put into memory at START).  Local constants: SCR = 11 (r11).  Hand structure,
stated: the REX / ModRM / SIB / displacement packing below.

Third slice: call L inside the fixture -- E8 rel32 from the call's final offset
(after relaxation), always 5 bytes; only the encoding, not call semantics.

Second slice: jump L and jumpz rX, L inside the fixture, with `name:` label
lines.  Every instruction is first encoded (a branch in no form yet), then the
branches are relaxed as unisa/assemble.py does -- all in their long form (jmp
rel32: 5 bytes; test + jz rel32: 9), each round lays the code out and marks at
once every branch whose short form (jmp rel8: 2; test + jz rel8: 5) would reach
its target, measured from the end of the short instruction; repeat until no
more fit -- and the code is written.  The offsets and the short set are the
delta's own; a duplicate or an undefined label is rejected.  The rel32 width
(4 bytes) is a local constant here, equal to emit_x86.RELBYTES["rel32"]; the reloc
table is not read by this slice.
"""
import importlib.util
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location("e3gen", os.path.join(HERE, "..", "parse", "gen.py"))
E = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(E)
g, P, EOF = E.g, E.P, 256
sys.path.insert(0, os.path.join(HERE, "..", ".."))
from unisa.catalog import ENCSPEC   # noqa: E402  (generation time only)
from unisa.emit_x86 import NUM      # noqa: E402

X86 = ENCSPEC["x86_64"]
DIGIT = list(range(48, 58))
NL = [10, EOF]
OPC, REGN = 70 * 10 ** 6, 71 * 10 ** 6       # OPC[op id] = class; REGN[name id] = register + 1
LABD = 74 * 10 ** 6                          # LABD[label id] = the index of the next instruction + 1
KND, BLB, SZ, TGT, BRG, SHT, OFF, FIT = (75 * 10 ** 6, 76 * 10 ** 6, 77 * 10 ** 6, 78 * 10 ** 6, 79 * 10 ** 6,
                                         80 * 10 ** 6, 81 * 10 ** 6, 82 * 10 ** 6)    # per instruction
AOPC, ACC = 72 * 10 ** 6, 73 * 10 ** 6       # the alu2 opcode / setcc byte of an op id
C_MOV, C_IMM, C_ALU, C_MUL, C_LD8, C_ST8, C_LD, C_ST, C_SET, C_RET = range(1, 11)
SCR = 11                                     # r11: emit_x86.SCR


def byte(p, v):
    return p.a(("LDI", "ob", v), ("OUTW", "ob"))


def procs():
    # REX(w, rr, bb) with x = 0: 0x40 | w<<3 | (rr>>3)<<2 | (bb>>3)
    p = P("REX")
    p.a(("ALUI", "shl", "rx_t", "rx_w", 3), ("ALUI", "or", "rx_t", "rx_t", 0x40),
        ("ALUI", "sar", "rx_u", "rx_r", 3), ("ALUI", "shl", "rx_u", "rx_u", 2), ("ALU", "or", "rx_t", "rx_t", "rx_u"),
        ("ALUI", "sar", "rx_u", "rx_b", 3), ("ALU", "or", "rx_t", "rx_t", "rx_u"), ("OUTW", "rx_t")).ret()
    # MODRM(mod, reg, rm)
    p = P("MODRM")
    p.a(("ALUI", "shl", "rx_t", "mr_m", 6), ("ALUI", "and", "rx_u", "mr_r", 7), ("ALUI", "shl", "rx_u", "rx_u", 3),
        ("ALU", "or", "rx_t", "rx_t", "rx_u"), ("ALUI", "and", "rx_u", "mr_b", 7), ("ALU", "or", "rx_t", "rx_t", "rx_u"),
        ("OUTW", "rx_t")).ret()
    # ALU(opc: al_o, dst: al_d, src: al_s): rex(1, s>>3, 0, d>>3) opc modrm(3, s, d)
    p = P("ALU")
    p.a(("LDI", "rx_w", 1), ("COPYW", "rx_r", "al_s"), ("COPYW", "rx_b", "al_d")).call("REX").a(("OUTW", "al_o"),
        ("LDI", "mr_m", 3), ("COPYW", "mr_r", "al_s"), ("COPYW", "mr_b", "al_d")).call("MODRM").ret()
    # LEBYTES(lb_v, lb_n): lb_n little-endian bytes of the 64-bit value
    p = P("LEBYTES")
    p.label("LB.l")
    p.branch({2: "LB.o"}, "RET", [("CMPI", "lb_n", 0)])
    P("LB.o").a(("OUTW", "lb_v"), ("A64I", "shr", "lb_v", "lb_v", 8), ("ALUI", "sub", "lb_n", "lb_n", 1)).goto("LB.l")
    # MEM(opcode bytes already chosen by the caller in me_o1/me_o2 (me_two), reg me_r, base me_b, disp me_d, w me_w;
    # me_66: a 0x66 prefix first)
    p = P("MEM")
    p.branch({1: "ME.66"}, "ME.rex", [("CMPI", "me_66", 1)])
    byte(P("ME.66"), 0x66).goto("ME.rex")
    p = P("ME.rex")
    p.a(("COPYW", "rx_w", "me_w"), ("COPYW", "rx_r", "me_r"), ("COPYW", "rx_b", "me_b")).call("REX")
    p.a(("OUTW", "me_o1")).branch({1: "ME.o2"}, "ME.fm", [("CMPI", "me_two", 1)])
    P("ME.o2").a(("OUTW", "me_o2")).goto("ME.fm")
    p = P("ME.fm")          # the form: disp 0 (base not rbp/r13), 8-bit, 32-bit
    p.a(("ALUI", "and", "me_b7", "me_b", 7), ("LDI", "me_z", 0)).branch({1: "ME.z0"}, "ME.nz", [("C64", "me_d", "me_z")])
    P("ME.z0").branch({1: "ME.nz"}, "ME.m0", [("CMPI", "me_b7", 5)])
    p = P("ME.m0")
    p.a(("LDI", "mr_m", 0)).call("ME.mrsib").ret()
    p = P("ME.nz")
    p.a(("LDI", "me_z", -128)).branch({0: "ME.m2"}, "ME.nz2", [("C64", "me_d", "me_z")])
    p = P("ME.nz2")
    p.a(("LDI", "me_z", 127)).branch({2: "ME.m2"}, "ME.m1", [("C64", "me_d", "me_z")])
    p = P("ME.m1")
    p.a(("LDI", "mr_m", 1)).call("ME.mrsib").a(("OUTW", "me_d")).ret()
    p = P("ME.m2")
    p.a(("LDI", "mr_m", 2)).call("ME.mrsib").a(("COPYW", "lb_v", "me_d"), ("LDI", "lb_n", 4)).call("LEBYTES").ret()
    p = P("ME.mrsib")       # modrm(mod, r, b), then the SIB 0x24 when b&7 == 4
    p.a(("COPYW", "mr_r", "me_r"), ("COPYW", "mr_b", "me_b")).call("MODRM").branch({1: "ME.sib"}, "RET", [("CMPI", "me_b7", 4)])
    byte(P("ME.sib"), 0x24).ret()


def relax():
    """RELAX: the rounds of unisa/assemble.py; WRITE: every instruction in its final form"""
    p = P("RELAX")
    p.a(("LDI", "rround", 0)).label("RX.r")
    p.a(("ALUI", "add", "rround", "rround", 1), ("LDI", "q", 0), ("LDI", "off", 0)).label("RX.o")      # offsets
    p.branch({0: "RX.o1"}, "RX.f", [("CMP", "q", "npc")])
    P("RX.o1").a(("STX", "q", OFF, "off"), ("LDX", "t", "q", SZ), ("ALU", "add", "off", "off", "t"),
                 ("ALUI", "add", "q", "q", 1)).goto("RX.o")
    p = P("RX.f")           # every long branch whose short form would reach
    p.a(("COPYW", "endo", "off"), ("LDI", "q", 0), ("LDI", "nfit", 0)).label("RX.fl")
    p.branch({0: "RX.f1"}, "RX.m", [("CMP", "q", "npc")])
    p = P("RX.f1")
    p.a(("LDX", "k", "q", KND)).branch({1: "RX.nx"}, "RX.f15", [("CMPI", "k", 0)])
    P("RX.f15").branch({1: "RX.nx"}, "RX.f2", [("CMPI", "k", 3)])          # a call never shortens
    p = P("RX.f2")
    p.a(("LDX", "t", "q", SHT)).branch({1: "RX.f3"}, "RX.nx", [("CMPI", "t", 0)])
    p = P("RX.f3")
    p.a(("LDI", "ns", 2)).branch({1: "RX.f4"}, "RX.f35", [("CMPI", "k", 1)])
    P("RX.f35").a(("LDI", "ns", 5)).goto("RX.f4")
    p = P("RX.f4")
    p.call("LADDR").a(("LDX", "t", "q", OFF), ("ALU", "add", "t", "t", "ns"), ("ALU", "sub", "d", "la", "t"))
    p.branch({0: "RX.nx"}, "RX.f5", [("CMPI", "d", -128)])
    p = P("RX.f5")
    p.branch({2: "RX.nx"}, "RX.fit", [("CMPI", "d", 127)])
    P("RX.fit").a(("STX", "q", FIT, "rround"), ("ALUI", "add", "nfit", "nfit", 1)).goto("RX.nx")
    P("RX.nx").a(("ALUI", "add", "q", "q", 1)).goto("RX.fl")
    p = P("RX.m")           # none fit: done; else all of this round's fits become short
    p.branch({1: "RET"}, "RX.m0", [("CMPI", "nfit", 0)])
    p = P("RX.m0")
    p.a(("LDI", "q", 0)).label("RX.ml")
    p.branch({0: "RX.m1"}, "RX.r", [("CMP", "q", "npc")])
    p = P("RX.m1")
    p.a(("LDX", "t", "q", FIT)).branch({1: "RX.m2"}, "RX.m3", [("CMP", "t", "rround")])
    p = P("RX.m2")
    p.a(("LDI", "t", 1), ("STX", "q", SHT, "t"), ("LDX", "k", "q", KND), ("LDI", "t", 2)).branch({1: "RX.m2s"}, "RX.m2z", [("CMPI", "k", 1)])
    P("RX.m2z").a(("LDI", "t", 5)).goto("RX.m2s")
    P("RX.m2s").a(("STX", "q", SZ, "t")).goto("RX.m3")
    P("RX.m3").a(("ALUI", "add", "q", "q", 1)).goto("RX.ml")
    # LADDR(q) -> la: the offset of branch q's target label (the end past the last instruction)
    p = P("LADDR")
    p.a(("LDX", "t", "q", TGT), ("LDX", "t", "t", LABD)).branch({1: "DEAD.undef"}, "LA.1", [("CMPI", "t", 0)])
    g.on("DEAD.undef", range(257), "DEAD", E.rej("not covered: a branch to an undefined label"), "r")
    p = P("LA.1")
    p.a(("ALUI", "sub", "t", "t", 1)).branch({0: "LA.in"}, "LA.end", [("CMP", "t", "npc")])
    P("LA.in").a(("LDX", "la", "t", OFF)).ret()
    P("LA.end").a(("COPYW", "la", "endo")).ret()
    # WRITE
    p = P("WRITE")
    p.a(("LDI", "q", 0)).label("WR.l")
    p.branch({0: "WR.i"}, "RET", [("CMP", "q", "npc")])
    p = P("WR.i")
    p.a(("LDX", "k", "q", KND)).branch({0: "WR.blob", 1: "WR.j", 2: "WR.z", 3: "WR.c"}, "WR.blob", [("RLD", "k")])
    p = P("WR.c")           # call rel32: E8, target - (the call's final offset + 5)
    p.call("LADDR").a(("LDX", "o_", "q", OFF), ("ALUI", "add", "o_", "o_", 5), ("ALU", "sub", "d", "la", "o_"),
                     ("LDI", "t", 0xE8), ("OUTW", "t"), ("COPYW", "lb_v", "d"), ("LDI", "lb_n", 4)).call("LEBYTES").goto("WR.nx")
    p = P("WR.blob")
    p.a(("LDX", "t", "q", BLB), ("INPUSH", "t")).goto("WR.cp")
    g.on("WR.cp", [EOF], "WR.cpd", [("INPOP",)])
    g.els("WR.cp", "WR.cp", [("COPY",), ("ADV",)])
    P("WR.cpd").goto("WR.nx")
    p = P("WR.j")           # jmp: short EB rel8 / long E9 rel32, from the instruction's end
    p.call("LADDR").a(("LDX", "o_", "q", OFF), ("LDX", "t", "q", SHT)).branch({1: "WR.js"}, "WR.jl", [("CMPI", "t", 1)])
    P("WR.js").a(("ALUI", "add", "o_", "o_", 2), ("ALU", "sub", "d", "la", "o_"), ("LDI", "t", 0xEB), ("OUTW", "t"), ("OUTW", "d")).goto("WR.nx")
    p = P("WR.jl")
    p.a(("ALUI", "add", "o_", "o_", 5), ("ALU", "sub", "d", "la", "o_"), ("LDI", "t", 0xE9), ("OUTW", "t"),
        ("COPYW", "lb_v", "d"), ("LDI", "lb_n", 4)).call("LEBYTES").goto("WR.nx")
    p = P("WR.z")           # test r, r (rex(1, r>>3, 0, r>>3) 85 modrm(3, r, r)); jz rel8 74 / rel32 0F 84
    p.a(("LDX", "zr", "q", BRG), ("LDI", "rx_w", 1), ("COPYW", "rx_r", "zr"), ("COPYW", "rx_b", "zr")).call("REX")
    p.a(("LDI", "t", 0x85), ("OUTW", "t"), ("LDI", "mr_m", 3), ("COPYW", "mr_r", "zr"), ("COPYW", "mr_b", "zr")).call("MODRM")
    p.call("LADDR").a(("LDX", "o_", "q", OFF), ("LDX", "t", "q", SHT)).branch({1: "WR.zs"}, "WR.zl", [("CMPI", "t", 1)])
    P("WR.zs").a(("ALUI", "add", "o_", "o_", 5), ("ALU", "sub", "d", "la", "o_"), ("LDI", "t", 0x74), ("OUTW", "t"), ("OUTW", "d")).goto("WR.nx")
    p = P("WR.zl")
    p.a(("ALUI", "add", "o_", "o_", 9), ("ALU", "sub", "d", "la", "o_"), ("LDI", "t", 0x0F), ("OUTW", "t"), ("LDI", "t", 0x84), ("OUTW", "t"),
        ("COPYW", "lb_v", "d"), ("LDI", "lb_n", 4)).call("LEBYTES").goto("WR.nx")
    P("WR.nx").a(("ALUI", "add", "q", "q", 1)).goto("WR.l")


def build():
    E.prn()
    procs()
    p = P("START")
    classes = {"mov": C_MOV, "imm": C_IMM, "mul64": C_MUL, "load64": C_LD8, "store64": C_ST8, ".ld": C_LD, ".st": C_ST, "ret": C_RET}
    for op, c in X86["alu2"].items():
        classes[op] = C_ALU
    for op in X86["setcc"]:
        classes[op] = C_SET
    for op, c in classes.items():
        p.a(("SBCLR",), [("SBOUT", ch) for ch in op.encode()], ("SBINTERN", "t"), ("LDI", "u", c), ("STX", "t", OPC, "u"))
        if op in X86["alu2"]:
            p.a(("LDI", "u", X86["alu2"][op]), ("STX", "t", AOPC, "u"))
        if op in X86["setcc"]:
            p.a(("LDI", "u", X86["setcc"][op]), ("STX", "t", ACC, "u"))
    for nm, n in NUM.items():
        p.a(("SBCLR",), [("SBOUT", ch) for ch in nm.encode()], ("SBINTERN", "t"), ("LDI", "u", n + 1), ("STX", "t", REGN, "u"))
    for w in ("jump", "jumpz", "call"):
        p.a(("SBCLR",), [("SBOUT", ch) for ch in w.encode()], ("SBINTERN", "id_" + w))
    p.a(("LDI", "npc", 0)).goto("LINE")
    # LINE: the op word, then up to four comma-separated arguments into a0..a3 (a register's number or an integer)
    g.on("LINE", [EOF], "DONE", [])
    g.on("LINE", [10], "LINE", [("ADV",)])
    g.els("LINE", "LW", [("MARK", "ws"), ("LDI", "lc", 0)])
    g.on("LW", [32] + NL, "LW.e", [("MARK", "we")])
    g.els("LW", "LW", [("BYTE", "lc"), ("ADV",)])
    p = P("LW.e")
    p.branch({1: "LAB"}, "LW.i", [("CMPI", "lc", 58)])
    p = P("LAB")            # `name:` -- the next instruction's index; a name defined twice is rejected
    p.a(("ALUI", "sub", "t", "we", 1), ("INTERN", "lid", "ws", "t"), ("LDX", "t", "lid", LABD)).branch({1: "LAB.s"}, "DEAD.dup", [("CMPI", "t", 0)])
    P("LAB.s").a(("ALUI", "add", "t", "npc", 1), ("STX", "lid", LABD, "t")).goto("SKIPL")
    g.on("DEAD.dup", range(257), "DEAD", E.rej("not covered: a label defined twice"), "r")
    p = P("LW.i")
    p.a(("INTERN", "opid", "ws", "we"), ("LDX", "cls", "opid", OPC), ("LDI", "na", 0), ("OLEN", "omark"))
    p.branch({1: "BR.j"}, "LW.i1", [("CMP", "opid", "id_jump")])
    P("LW.i1").branch({1: "BR.c"}, "LW.i2", [("CMP", "opid", "id_call")])
    # call NAME: E8 rel32, always 5 bytes (no short form: assemble's short_size is None)
    g.els("BR.c", "BR.j0", [("LDI", "t", 3), ("STX", "npc", KND, "t"), ("LDI", "t", 5), ("STX", "npc", SZ, "t")])
    P("LW.i2").branch({1: "BR.z"}, "ARG", [("CMP", "opid", "id_jumpz")])
    # jump NAME / jumpz rX, NAME: recorded, encoded later
    g.els("BR.j", "BR.j0", [("LDI", "t", 1), ("STX", "npc", KND, "t"), ("LDI", "t", 5), ("STX", "npc", SZ, "t")])
    g.on("BR.j0", [32], "BR.j0", [("ADV",)])
    g.els("BR.j0", "BR.name", [("MARK", "ts")])
    g.els("BR.z", "BR.z0", [("LDI", "t", 2), ("STX", "npc", KND, "t"), ("LDI", "t", 9), ("STX", "npc", SZ, "t")])
    g.on("BR.z0", [32], "BR.z0", [("ADV",)])
    g.els("BR.z0", "BR.zr", [("MARK", "ts")])
    g.on("BR.zr", [44, 32] + NL, "BR.zr2", [("MARK", "te")])
    g.els("BR.zr", "BR.zr", [("ADV",)])
    p = P("BR.zr2")
    p.a(("INTERN", "rid", "ts", "te"), ("LDX", "t", "rid", REGN)).branch({1: "DEAD.reg"}, "BR.zr3", [("CMPI", "t", 0)])
    P("BR.zr3").a(("ALUI", "sub", "t", "t", 1), ("STX", "npc", BRG, "t")).goto("BR.zs")
    g.on("BR.zs", [32, 44], "BR.zs", [("ADV",)])
    g.els("BR.zs", "BR.name", [("MARK", "ts")])
    g.on("BR.name", [32] + NL, "BR.ne", [("MARK", "te")])
    g.els("BR.name", "BR.name", [("ADV",)])
    p = P("BR.ne")
    p.a(("INTERN", "t", "ts", "te"), ("STX", "npc", TGT, "t"), ("ALUI", "add", "npc", "npc", 1)).goto("SKIPL")
    g.on("SKIPL", [10], "LINE", [("ADV",)])
    g.on("SKIPL", [EOF], "DONE", [])
    g.els("SKIPL", "SKIPL", [("ADV",)])
    g.on("ARG", [32], "ARG", [("ADV",)])
    g.on("ARG", NL, "ARGS.d", [])
    g.on("ARG", [45] + DIGIT, "AN", [("LDI", "neg", 0), ("LDI", "av", 0)])
    g.els("ARG", "AR", [("MARK", "ts")])
    g.on("AN", [45], "AN.d", [("LDI", "neg", 1), ("ADV",)])
    g.els("AN", "AN.d", [])
    g.on("AN.d", DIGIT, "AN.d", [("BYTE", "bt"), ("ALUI", "sub", "bt", "bt", 48), ("A64I", "mul", "av", "av", 10),
                                 ("A64", "add", "av", "av", "bt"), ("ADV",)])
    g.els("AN.d", "AN.s", [])
    p = P("AN.s")
    p.branch({1: "AN.neg"}, "ARG.put", [("CMPI", "neg", 1)])
    P("AN.neg").a(("LDI", "z0", 0), ("A64", "sub", "av", "z0", "av")).goto("ARG.put")
    g.on("AR", [44, 32] + NL, "AR.e", [("MARK", "te")])
    g.els("AR", "AR", [("ADV",)])
    p = P("AR.e")
    p.a(("INTERN", "rid", "ts", "te"), ("LDX", "av", "rid", REGN)).branch({1: "DEAD.reg"}, "AR.r", [("CMPI", "av", 0)])
    g.on("DEAD.reg", range(257), "DEAD", E.rej("not covered: an operand that is not a register or an integer"), "r")
    P("AR.r").a(("ALUI", "sub", "av", "av", 1)).goto("ARG.put")
    p = P("ARG.put")
    for k in range(4):
        hit, nx = "AP.%d" % k, "AP.n%d" % k
        p.branch({1: hit}, nx, [("CMPI", "na", k)])
        P(hit).a(("COPYW", "a%d" % k, "av"), ("ALUI", "add", "na", "na", 1)).goto("ARG.sep")
        p = P(nx)
    p.goto("DEAD.reg")
    g.on("ARG.sep", [32], "ARG.sep", [("ADV",)])
    g.on("ARG.sep", [44], "ARG", [("ADV",)])
    g.on("ARG.sep", NL, "ARGS.d", [])
    g.els("ARG.sep", "DEAD.reg", [])
    # ARGS.d: at the end of the line: the class decides
    p = P("ARGS.d")
    p.branch({C_MOV + 1 - 1: "E.mov", C_IMM: "E.imm", C_ALU: "E.alu", C_MUL: "E.mul", C_LD8: "E.ld8", C_ST8: "E.st8",
              C_LD: "E.ld", C_ST: "E.st", C_SET: "E.set", C_RET: "E.ret"}, "DEAD.op", [("RLD", "cls")])
    g.on("DEAD.op", range(257), "DEAD", E.rej("not covered: an op outside the first encoder slice"), "r")
    # mov d, s
    p = P("E.mov")
    p.a(("LDI", "al_o", 0x89), ("COPYW", "al_d", "a0"), ("COPYW", "al_s", "a1")).call("ALU").goto("NEXTL")
    # imm d, v: `mov r32, imm32` when v >> 32 == 0 (0x41 first for r8..r15), else movabs
    p = P("E.imm")
    p.a(("A64I", "shr", "t", "a1", 32), ("LDI", "z0", 0)).branch({1: "EI.s"}, "EI.l", [("C64", "t", "z0")])
    p = P("EI.s")
    p.branch({(1, 2): "EI.s41"}, "EI.s2", [("CMPI", "a0", 8)])
    byte(P("EI.s41"), 0x41).goto("EI.s2")
    p = P("EI.s2")
    p.a(("ALUI", "and", "t", "a0", 7), ("ALUI", "add", "t", "t", 0xB8), ("OUTW", "t"), ("COPYW", "lb_v", "a1"), ("LDI", "lb_n", 4)).call("LEBYTES").goto("NEXTL")
    p = P("EI.l")
    p.a(("LDI", "rx_w", 1), ("LDI", "rx_r", 0), ("COPYW", "rx_b", "a0")).call("REX")
    p.a(("ALUI", "and", "t", "a0", 7), ("ALUI", "add", "t", "t", 0xB8), ("OUTW", "t"), ("COPYW", "lb_v", "a1"), ("LDI", "lb_n", 8)).call("LEBYTES").goto("NEXTL")
    # alu2 / mul64 d, s1, s2 (_alias: d == s1 -> op d, s2; s2 == d -> mov r11, s2 first, op on r11)
    for nm, mul in (("E.alu", False), ("E.mul", True)):
        p = P(nm)
        p.a(("COPYW", "src2", "a2")).branch({1: nm + ".op"}, nm + ".a2", [("CMP", "a0", "a1")])
        p = P(nm + ".a2")
        p.branch({1: nm + ".sc"}, nm + ".mv", [("CMP", "a2", "a0")])
        p = P(nm + ".sc")
        p.a(("LDI", "al_o", 0x89), ("LDI", "al_d", SCR), ("COPYW", "al_s", "a2")).call("ALU").a(("LDI", "src2", SCR)).goto(nm + ".mv")
        p = P(nm + ".mv")
        p.a(("LDI", "al_o", 0x89), ("COPYW", "al_d", "a0"), ("COPYW", "al_s", "a1")).call("ALU").goto(nm + ".op")
        p = P(nm + ".op")
        if not mul:
            p.a(("LDX", "al_o", "opid", AOPC), ("COPYW", "al_d", "a0"), ("COPYW", "al_s", "src2")).call("ALU").goto("NEXTL")
        else:       # rex(1, d>>3, 0, s2>>3) 0F AF modrm(3, d, s2)
            p.a(("LDI", "rx_w", 1), ("COPYW", "rx_r", "a0"), ("COPYW", "rx_b", "src2")).call("REX")
            byte(p, 0x0F)
            byte(p, 0xAF)
            p.a(("LDI", "mr_m", 3), ("COPYW", "mr_r", "a0"), ("COPYW", "mr_b", "src2")).call("MODRM").goto("NEXTL")
    # memory: load64 r, base, disp / store64 base, disp, r / .ld r, base, disp, w / .st base, disp, r, w
    def mem(p, o1, o2=None, w=1, p66=0):
        p.a(("LDI", "me_o1", o1), ("LDI", "me_two", 1 if o2 is not None else 0), ("LDI", "me_o2", o2 or 0),
            ("LDI", "me_w", w), ("LDI", "me_66", p66)).call("MEM").goto("NEXTL")
    p = P("E.ld8")
    p.a(("COPYW", "me_r", "a0"), ("COPYW", "me_b", "a1"), ("COPYW", "me_d", "a2"))
    mem(p, 0x8B)
    p = P("E.st8")
    p.a(("COPYW", "me_r", "a2"), ("COPYW", "me_b", "a0"), ("COPYW", "me_d", "a1"))
    mem(p, 0x89)
    p = P("E.ld")
    p.a(("COPYW", "me_r", "a0"), ("COPYW", "me_b", "a1"), ("COPYW", "me_d", "a2"))
    p.branch({1: "EL.1", 2: "EL.2", 4: "EL.4", 8: "EL.8"}, "DEAD.op", [("RLD", "a3")])
    mem(P("EL.8"), 0x8B)
    mem(P("EL.4"), 0x63)                   # movsxd
    mem(P("EL.1"), 0x0F, 0xBE)
    mem(P("EL.2"), 0x0F, 0xBF)
    p = P("E.st")
    p.a(("COPYW", "me_r", "a2"), ("COPYW", "me_b", "a0"), ("COPYW", "me_d", "a1"))
    p.branch({1: "ES.1", 2: "ES.2", 4: "ES.4", 8: "ES.8"}, "DEAD.op", [("RLD", "a3")])
    mem(P("ES.8"), 0x89)
    mem(P("ES.4"), 0x89, w=0)
    mem(P("ES.2"), 0x89, w=0, p66=1)
    mem(P("ES.1"), 0x88, w=0)
    # setcc d, ra, rb: cmp ra, rb / setcc r11b (41 0F cc modrm(3,0,r11)) / movzx d, r11b
    p = P("E.set")
    p.a(("LDI", "al_o", 0x39), ("COPYW", "al_d", "a1"), ("COPYW", "al_s", "a2")).call("ALU")
    byte(p, 0x41)
    byte(p, 0x0F)
    p.a(("LDX", "t", "opid", ACC), ("OUTW", "t"), ("LDI", "mr_m", 3), ("LDI", "mr_r", 0), ("LDI", "mr_b", SCR)).call("MODRM")
    p.a(("LDI", "rx_w", 1), ("COPYW", "rx_r", "a0"), ("LDI", "rx_b", 8)).call("REX")   # rex(1, d>>3, 0, 1): b bit set (REX sees rx_b >> 3)
    byte(p, 0x0F)
    byte(p, 0xB6)
    p.a(("LDI", "mr_m", 3), ("COPYW", "mr_r", "a0"), ("LDI", "mr_b", SCR)).call("MODRM").goto("NEXTL")
    byte(P("E.ret"), 0xC3).goto("NEXTL")
    p = P("NEXTL")          # a non-branch: its bytes become a blob, its size known
    p.a(("OCUT", "blob", "omark"), ("STX", "npc", BLB, "blob"), ("BLEN", "t", "blob"), ("STX", "npc", SZ, "t"),
        ("LDI", "t", 0), ("STX", "npc", KND, "t"), ("ALUI", "add", "npc", "npc", 1)).goto("SKIPL")
    relax()
    P("DONE").call("RELAX").call("WRITE").a(("ACCEPT",)).goto("DEAD")
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": "START", "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    d = build()
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d  action seqs %d (%d actions)  json %d B\n" % (st, ent, ns, na, len(s)))
