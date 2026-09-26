"""E5, first slice (research/e5-slice.md): the x86_64 encoder for straight-line
lowered instructions, as a delta for the generic executor.

    python3 exec/enc/gen.py delta.json

Input: TIns text, one instruction per line (`op arg, ...`; machine register
names; integers).  Output: the machine code bytes, as unisa/emit_x86.encode
writes them.  Ops: mov, imm, add64/sub64/xor64/and64/or64, mul64, load64,
store64, .ld/.st (1, 2, 4, 8 bytes), setcc, register shifts, ret, jump/jumpz,
call/callr, push/pop, nop, .frame, .zero, setreg imm/reg, spinit without an
address, .div/.mod/.udiv/.umod, FP_OPS (via fp.py), non-WinAPI gate,
.lea, setreg mem/addr, setmem, argsave and argvget. Other forms are rejected as not covered.

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
from address import install as install_address
from tins import META as META_KEYS
from fp import FP_IDS, install as install_fp  # local delta generator, not an encoder oracle

X86 = ENCSPEC["x86_64"]
DIGIT = list(range(48, 58))
NL = [10, EOF]
OPC, REGN = 70 * 10 ** 6, 71 * 10 ** 6       # OPC[op id] = class; REGN[name id] = register + 1
MSN = 84 * 10 ** 6                           # MSN[meta key id] = the line it was last given on
LABD = 74 * 10 ** 6                          # LABD[label id] = the index of the next instruction + 1
KND, BLB, SZ, TGT, BRG, SHT, OFF, FIT = (75 * 10 ** 6, 76 * 10 ** 6, 77 * 10 ** 6, 78 * 10 ** 6, 79 * 10 ** 6,
                                         80 * 10 ** 6, 81 * 10 ** 6, 82 * 10 ** 6)    # per instruction
AOPC, ACC = 72 * 10 ** 6, 73 * 10 ** 6       # the alu2 opcode / setcc byte of an op id
C_MOV, C_IMM, C_ALU, C_MUL, C_LD8, C_ST8, C_LD, C_ST, C_SET, C_RET, C_SHF, C_CALLR, C_PUSH, C_POP, C_NOP, C_FRAME, C_ZERO, C_SETREG, C_SPINIT, C_DIV, C_MOD, C_UDIV, C_UMOD, C_GATE, C_LEA, C_SETMEM, C_ARGSAVE, C_ARGVGET = range(1, 29)
from unisa.catalog import REGMAP     # noqa: E402  (generation time only)
SPREG = NUM[REGMAP["x86_64"][7]]     # the tape SP's machine register (rsp), read, not written here
SHX = 83 * 10 ** 6                           # the /digit of D3 for a shift op id (ENCSPEC shiftext)
SCR2 = 3                                     # rbx: emit_x86.SCR2
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
    p.a(("LDX", "k", "q", KND)).branch({1: "RX.nx"}, "RX.addr", [("CMPI", "k", 0)])
    P("RX.addr").branch({(1,2):"RX.nx"}, "RX.f15", [("CMPI","k",4)])
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
    p.a(("LDX", "k", "q", KND)).branch({0: "WR.blob", 1: "WR.j", 2: "WR.z", 3: "WR.c", 4:"WR.addr", 5:"WR.argsave", 6:"WR.argvget"}, "WR.blob", [("RLD", "k")])
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


def build(image=False):
    E.prn()
    procs()
    p = P("START")
    classes = {".div": C_DIV, ".mod": C_MOD, ".udiv": C_UDIV, ".umod": C_UMOD, "setreg": C_SETREG, "spinit": C_SPINIT, ".zero": C_ZERO, "push": C_PUSH, "pop": C_POP, "nop": C_NOP, ".frame": C_FRAME, "callr": C_CALLR, "mov": C_MOV, "imm": C_IMM, "mul64": C_MUL, "load64": C_LD8, "store64": C_ST8, ".ld": C_LD, ".st": C_ST, "ret": C_RET}
    classes.update(FP_IDS)
    classes.update({"gate": C_GATE, ".lea": C_LEA, "setmem": C_SETMEM, "argsave":C_ARGSAVE, "argvget":C_ARGVGET})
    for op, c in X86["alu2"].items():
        classes[op] = C_ALU
    for op in X86["setcc"]:
        classes[op] = C_SET
    for op in X86["shiftext"]:
        classes[op] = C_SHF
    for op, c in classes.items():
        p.a(("SBCLR",), [("SBOUT", ch) for ch in op.encode()], ("SBINTERN", "t"), ("LDI", "u", c), ("STX", "t", OPC, "u"))
        if op in X86["alu2"]:
            p.a(("LDI", "u", X86["alu2"][op]), ("STX", "t", AOPC, "u"))
        if op in X86["setcc"]:
            p.a(("LDI", "u", X86["setcc"][op]), ("STX", "t", ACC, "u"))
        if op in X86["shiftext"]:
            p.a(("LDI", "u", X86["shiftext"][op]), ("STX", "t", SHX, "u"))
    for nm, n in NUM.items():
        p.a(("SBCLR",), [("SBOUT", ch) for ch in nm.encode()], ("SBINTERN", "t"), ("LDI", "u", n + 1), ("STX", "t", REGN, "u"))
    for w, nm in (("imm", "tagimm"), ("reg", "tagreg"), ("role", "role"), ("form", "form"), ("reloc", "reloc"), ("rel32", "rel32")):
        p.a(("SBCLR",), [("SBOUT", ch) for ch in w.encode()], ("SBINTERN", "id_" + nm))
    for w in ("true", "false", "winapi", "carry", *[k for k in META_KEYS if k not in ("role", "form", "reloc", "carry", "winapi")]):
        p.a(("SBCLR",), [("SBOUT", ch) for ch in w.encode()], ("SBINTERN", "id_" + w))
    for w, nm in [("mem", "tagmem"), ("addr", "tagaddr"), ("lnx/x86_64", "target1"), ("osx/x86_64", "target2")] + [("@"+k, "h_"+k) for k in ("target","data","sym","src_os","data_len","bss","relocs")]:
        p.a(("SBCLR",), [("SBOUT", ch) for ch in w.encode()], ("SBINTERN", "id_" + nm))
    p.a(("LDI", "target_os", 1), ("LDI", "has_relocs", 0))
    p.a(("SBCLR",), [("SBOUT", ch) for ch in b"_start"], ("SBINTERN", "id_entry"))
    for w in ("jump", "jumpz", "call"):
        p.a(("SBCLR",), [("SBOUT", ch) for ch in w.encode()], ("SBINTERN", "id_" + w))
    p.a(("LDI", "npc", 0), ("LDI", "lnum", 0)).goto("LINE")
    # LINE: the op word, then up to four comma-separated arguments into a0..a3 (a register's number or an integer)
    g.on("LINE", [64], "HDR.key", [("MARK", "ws"), ("ADV",)])
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
    p.a(("INTERN", "opid", "ws", "we"), ("LDX", "cls", "opid", OPC), ("LDI", "na", 0), ("LDI", "stag", 0), ("LDI", "gcarry", 0), ("LDI", "anamed", 0), ("LDI", "a2", 0), ("OLEN", "omark"),
        ("ALUI", "add", "lnum", "lnum", 1))
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
    g.on("BR.name", [32] + NL, "BR.ne", [("MARK", "te")])       # (meta, if any, follows: see BR.ne)
    g.els("BR.name", "BR.name", [("ADV",)])
    p = P("BR.ne")
    p.a(("INTERN", "t", "ts", "te"), ("STX", "npc", TGT, "t"), ("ALUI", "add", "npc", "npc", 1)).goto("BR.m")
    g.on("BR.m", [32], "BR.m", [("ADV",)])
    g.on("BR.m", NL, "SKIPL", [])
    g.els("BR.m", "BR.mk", [("MARK", "ts")])
    g.on("BR.mk", [61], "BRM.v", [("MARK", "ke"), ("ADV",)])
    g.on("BR.mk", [32] + NL, "DEAD.meta", [])
    g.els("BR.mk", "BR.mk", [("ADV",)])
    g.on("BRM.v", [32] + NL, "BRM.e", [("MARK", "vs"), ("MARK", "ve")])
    g.els("BRM.v", "BRM.vv", [("MARK", "vs")])
    g.on("BRM.vv", [32] + NL, "BRM.e", [("MARK", "ve")])
    g.els("BRM.vv", "BRM.vv", [("ADV",)])
    p = P("BRM.e")
    p.a(("INTERN", "mk", "ts", "ke")).call("MSEEN").branch({1: "BR.m"}, "BRM.k2", [("CMP", "mk", "id_role")])
    # MSEEN: a meta key given twice on one instruction is rejected (tins.parse does the same)
    p = P("MSEEN")
    p.a(("LDX", "t", "mk", MSN)).branch({1: "DEAD.mdup"}, "MS.set", [("CMP", "t", "lnum")])
    P("MS.set").a(("STX", "mk", MSN, "lnum")).ret()
    g.on("DEAD.mdup", range(257), "DEAD", E.rej("not covered: a meta key given twice"), "r")
    P("BRM.k2").branch({1: "BR.m"}, "BRM.k3", [("CMP", "mk", "id_form")])
    P("BRM.k3").branch({1: "BRM.rl"}, "DEAD.meta", [("CMP", "mk", "id_reloc")])
    P("BRM.rl").a(("INTERN", "mv", "vs", "ve")).branch({1: "BR.m"}, "DEAD.meta", [("CMP", "mv", "id_rel32")])
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
    g.on("AR", [61], "META.v", [("MARK", "ke"), ("ADV",)])        # key=value: meta, not an operand
    g.els("AR", "AR", [("ADV",)])
    p = P("AR.e")
    p.a(("INTERN", "rid", "ts", "te")).branch({1: "AR.tag"}, "AR.e1", [("CMP", "rid", "id_tagimm")])
    P("AR.e1").branch({1: "AR.tag"}, "AR.mem", [("CMP", "rid", "id_tagreg")])
    P("AR.mem").branch({1:"AR.tag"}, "AR.addr", [("CMP","rid","id_tagmem")])
    P("AR.addr").branch({1:"AR.tag"}, "AR.lea", [("CMP","rid","id_tagaddr")])
    P("AR.lea").branch({1:"AR.l1"}, "AR.bool", [("CMPI","cls",C_LEA)])
    P("AR.bool").branch({1:"AR.b1"}, "AR.e2", [("CMPI","cls",C_ARGSAVE)])
    P("AR.b1").branch({1:"AR.bt"}, "AR.b2", [("CMP","rid","id_true")])
    P("AR.bt").a(("LDI","av",1)).goto("ARG.put")
    P("AR.b2").branch({1:"AR.bf"}, "AR.e2", [("CMP","rid","id_false")])
    P("AR.bf").a(("LDI","av",0)).goto("ARG.put")
    P("AR.l1").branch({1:"AR.name"}, "AR.e2", [("CMPI","na",1)])
    P("AR.name").a(("COPYW","av","rid"),("LDI","anamed",1)).goto("ARG.put")
    P("AR.tag").a(("COPYW", "stag", "rid")).goto("ARG")          # setreg's tag word: the value follows
    p = P("AR.e2")
    p.a(("LDX", "av", "rid", REGN)).branch({1: "DEAD.reg"}, "AR.r", [("CMPI", "av", 0)])
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
    g.els("ARG.sep", "ARG", [])                 # a token after a space: meta (checked there) or an error
    # META: `key=value` after the args.  role: informational, ignored.  form: informational for the
    # ordinary ops. For gate, winapi is rejected and carry must be true/false.
    # Other declared gate metadata is informational for the non-WinAPI encoder;
    # reloc must be rel32. Unknown keys and duplicate keys are rejected.
    g.on("META.v", [32] + NL, "META.e", [("MARK", "vs"), ("MARK", "ve")])
    g.els("META.v", "META.vv", [("MARK", "vs")])
    g.on("META.vv", [32] + NL, "META.e", [("MARK", "ve")])
    g.els("META.vv", "META.vv", [("ADV",)])
    p = P("META.e")
    p.a(("INTERN", "mk", "ts", "ke")).call("MSEEN").branch({1: "META.ok"}, "META.k2", [("CMP", "mk", "id_role")])
    P("META.k2").branch({1: "META.form"}, "META.k3", [("CMP", "mk", "id_form")])
    P("META.k3").branch({1: "META.rl"}, "META.gate", [("CMP", "mk", "id_reloc")])
    P("META.rl").a(("INTERN", "mv", "vs", "ve")).branch({1: "META.ok"}, "DEAD.meta", [("CMP", "mv", "id_rel32")])
    g.on("DEAD.meta", range(257), "DEAD", E.rej("not covered: meta this slice does not take (or reloc other than rel32)"), "r")
    P("META.form").branch({1: "META.gf"}, "META.ok", [("CMPI", "cls", C_GATE)])
    P("META.gf").a(("INTERN", "mv", "vs", "ve")).branch({1: "DEAD.meta"}, "META.ok", [("CMP", "mv", "id_winapi")])
    P("META.gate").branch({1: "META.gkeys"}, "DEAD.meta", [("CMPI", "cls", C_GATE)])
    P("META.gkeys").branch({1: "META.carry"}, "META.gother", [("CMP", "mk", "id_carry")])
    p = P("META.gother")
    for k in META_KEYS:
        if k in ("role", "form", "reloc", "carry"): continue
        nx = "META.after." + k
        p.branch({1: "META.ok"}, nx, [("CMP", "mk", "id_" + k)])
        p = P(nx)
    p.goto("DEAD.meta")
    P("META.carry").a(("INTERN", "mv", "vs", "ve")).branch({1: "META.ct"}, "META.cf", [("CMP", "mv", "id_true")])
    P("META.ct").a(("LDI", "gcarry", 1)).goto("META.ok")
    P("META.cf").branch({1: "META.ok"}, "DEAD.meta", [("CMP", "mv", "id_false")])
    P("META.ok").goto("ARG.sep")
    # ARGS.d: at the end of the line: the class decides
    p = P("ARGS.d")
    p.branch({C_MOV + 1 - 1: "E.mov", C_IMM: "E.imm", C_ALU: "E.alu", C_MUL: "E.mul", C_LD8: "E.ld8", C_ST8: "E.st8",
              C_LD: "E.ld", C_ST: "E.st", C_SET: "E.set", C_RET: "E.ret", C_SHF: "E.shf", C_CALLR: "E.callr",
              C_PUSH: "E.push", C_POP: "E.pop", C_NOP: "E.nop", C_FRAME: "E.frame", C_ZERO: "E.zero",
              C_SETREG: "E.setreg", C_SPINIT: "E.spinit", C_GATE: "E.gate", C_LEA:"AD.lea", C_SETMEM:"AD.setmem", C_ARGSAVE:"AD.argsave", C_ARGVGET:"AD.argvget",
              C_DIV: "E.div", C_MOD: "E.mod", C_UDIV: "E.udiv", C_UMOD: "E.umod",
              **{v: "FP." + k for k, v in FP_IDS.items()}}, "DEAD.op", [("RLD", "cls")])
    g.on("DEAD.op", range(257), "DEAD", E.rej("not covered: an op outside the first encoder slice"), "r")
    P("E.gate").branch({1: "EG.emit"}, "DEAD.meta", [("CMPI", "na", 0)])
    p = byte(byte(P("EG.emit"), 0x0f), 0x05)
    p.branch({1: "EG.carry"}, "NEXTL", [("CMPI", "gcarry", 1)])
    p = P("EG.carry")
    for b in (0x73, 3, 0x48, 0xf7, 0xd8): byte(p, b)
    p.goto("NEXTL")
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
    # Integer division/remainder: hand sequence from emit_x86, with the existing
    # ALU/MEM encoders reused. Save rax/rdx in the tape stack (not push/pop).
    # Operand domain is the non-stack tape registers; r11 is reserved scratch.
    for nm, unsigned, remainder in (("div", 0, 0), ("mod", 0, 1), ("udiv", 1, 0), ("umod", 1, 1)):
        P("E." + nm).a(("LDI", "dv_unsigned", unsigned), ("LDI", "dv_rem", remainder)).goto("DV.check")
    p = P("DV.check")
    p.branch({3: "DV.reg0"}, "DEAD.op", [("RLD", "na")])
    allowed = tuple(NUM[r] for r in REGMAP["x86_64"][:7])
    for i in range(3):
        P("DV.reg%d" % i).branch({allowed: "DV.reg%d" % (i + 1) if i < 2 else "DV.save"}, "DEAD.op", [("RLD", "a%d" % i)])

    def dmov(p, dst, src):
        p.a(("LDI", "al_o", 0x89), ("LDI" if isinstance(dst, int) else "COPYW", "al_d", dst),
            ("LDI" if isinstance(src, int) else "COPYW", "al_s", src)).call("ALU")

    def dmem(p, opcode, reg, disp):
        p.a(("LDI", "me_o1", opcode), ("LDI", "me_two", 0), ("LDI", "me_o2", 0),
            ("LDI", "me_w", 1), ("LDI", "me_66", 0), ("LDI", "me_r", reg),
            ("LDI", "me_b", SPREG), ("LDI", "me_d", disp)).call("MEM")

    def dadj(p, ext):
        p.a(("LDI", "rx_w", 1), ("LDI", "rx_r", 0), ("LDI", "rx_b", SPREG)).call("REX")
        byte(p, 0x83)
        p.a(("LDI", "mr_m", 3), ("LDI", "mr_r", ext), ("LDI", "mr_b", SPREG)).call("MODRM")
        byte(p, 16)

    p = P("DV.save")
    dadj(p, 5)
    dmem(p, 0x89, NUM["rax"], 0)
    dmem(p, 0x89, NUM["rdx"], 8)
    dmov(p, SCR, "a2")
    dmov(p, NUM["rax"], "a1")
    p.branch({1: "DV.u"}, "DV.s", [("RLD", "dv_unsigned")])
    p = P("DV.u")
    for b in (0x48, 0x31, 0xD2, 0x49, 0xF7, 0xF3): byte(p, b)
    p.goto("DV.result")
    p = P("DV.s")
    for b in (0x48, 0x99, 0x49, 0xF7, 0xFB): byte(p, b)
    p.goto("DV.result")
    P("DV.result").branch({1: "DV.rem"}, "DV.quot", [("RLD", "dv_rem")])
    p = P("DV.rem"); dmov(p, SCR, NUM["rdx"]); p.goto("DV.restore")
    p = P("DV.quot"); dmov(p, SCR, NUM["rax"]); p.goto("DV.restore")
    p = P("DV.restore")
    dmem(p, 0x8B, NUM["rax"], 0)
    dmem(p, 0x8B, NUM["rdx"], 8)
    dadj(p, 0)
    dmov(p, "a0", SCR)
    p.goto("NEXTL")
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
    # push/pop r: 50+r / 58+r, a REX.B (0x41) first for r8..r15 (emit_x86 push/pop)
    for nm, base in (("E.push", 0x50), ("E.pop", 0x58)):
        p = P(nm)
        p.branch({(1, 2): nm + ".x"}, nm + ".o", [("CMPI", "a0", 8)])
        byte(P(nm + ".x"), 0x41).goto(nm + ".o")
        P(nm + ".o").a(("ALUI", "and", "t", "a0", 7), ("ALUI", "add", "t", "t", base), ("OUTW", "t")).goto("NEXTL")
    byte(P("E.nop"), 0x90).goto("NEXTL")
    # .frame n: on the tape SP's register, sub (/5) for n >= 0, add (/0) for n < 0, by |n|:
    # 83 /r ib when |n| <= 127, else 81 /r id (emit_x86 alu_imm) -- hand rules, stated
    p = P("E.frame")
    p.a(("LDI", "z0", 0), ("LDI", "fx", 5), ("COPYW", "fn", "a0")).branch({0: "EF.neg"}, "EF.r", [("C64", "a0", "z0")])
    P("EF.neg").a(("LDI", "fx", 0), ("A64", "sub", "fn", "z0", "a0")).goto("EF.r")
    p = P("EF.r")
    p.a(("LDI", "rx_w", 1), ("LDI", "rx_r", 0), ("LDI", "rx_b", SPREG)).call("REX")
    p.a(("LDI", "z0", 127)).branch({2: "EF.l"}, "EF.s", [("C64", "fn", "z0")])
    p = P("EF.s")
    byte(p, 0x83)
    p.a(("LDI", "mr_m", 3), ("COPYW", "mr_r", "fx"), ("LDI", "mr_b", SPREG)).call("MODRM").a(("OUTW", "fn")).goto("NEXTL")
    p = P("EF.l")
    byte(p, 0x81)
    p.a(("LDI", "mr_m", 3), ("COPYW", "mr_r", "fx"), ("LDI", "mr_b", SPREG)).call("MODRM").a(("COPYW", "lb_v", "fn"), ("LDI", "lb_n", 4)).call("LEBYTES").goto("NEXTL")
    # .zero base, disp, n: xor r11, r11 once, then the widest store (8, 4, 2, 1) that fits the rest,
    # at disp + k, until n bytes are zero (emit_x86) -- the split is a hand rule; r11 may not be the base
    p = P("E.zero")
    p.branch({1: "DEAD.zb"}, "EZ.x", [("CMPI", "a0", SCR)])
    g.on("DEAD.zb", range(257), "DEAD", E.rej("not covered: .zero with the scratch r11 as its base"), "r")
    p = P("EZ.x")
    for c in (0x4D, 0x31, 0xDB):
        byte(p, c)
    p.a(("LDI", "zk", 0)).label("EZ.l")
    p.branch({0: "EZ.w"}, "NEXTL", [("C64", "zk", "a2")])
    p = P("EZ.w")
    p.a(("LDI", "zw", 8)).label("EZ.fit")
    p.a(("A64", "add", "t", "zk", "zw")).branch({2: "EZ.half"}, "EZ.st", [("C64", "t", "a2")])
    P("EZ.half").a(("ALUI", "sar", "zw", "zw", 1)).goto("EZ.fit")
    p = P("EZ.st")
    p.a(("LDI", "me_r", SCR), ("COPYW", "me_b", "a0"), ("A64", "add", "me_d", "a1", "zk"), ("LDI", "me_two", 0), ("LDI", "me_o2", 0))
    p.branch({8: "EZ.8", 4: "EZ.4", 2: "EZ.2", 1: "EZ.1"}, "DEAD.op", [("RLD", "zw")])
    for w, o1, ww, p66 in ((8, 0x89, 1, 0), (4, 0x89, 0, 0), (2, 0x89, 0, 1), (1, 0x88, 0, 0)):
        P("EZ.%d" % w).a(("LDI", "me_o1", o1), ("LDI", "me_w", ww), ("LDI", "me_66", p66)).call("MEM").a(("A64", "add", "zk", "zk", "zw")).goto("EZ.l")
    # setreg rX, imm V -> the imm path; setreg rX, reg rY -> the mov path (emit_x86: mov_ri / mov_rr);
    # mem/addr is deferred until layout; imm/reg reuse existing encoders
    p = P("E.setreg")
    p.branch({1: "E.imm"}, "ESR.r", [("CMP", "stag", "id_tagimm")])
    P("ESR.r").branch({1: "E.mov"}, "ESR.mem", [("CMP", "stag", "id_tagreg")])
    P("ESR.mem").branch({1:"AD.mem"}, "ESR.addr", [("CMP","stag","id_tagmem")])
    P("ESR.addr").branch({1:"AD.addr"}, "DEAD.op", [("CMP","stag","id_tagaddr")])
    # spinit rN (lowering's spinit(rN, None), the None normalised away): mov rN, rsp
    p = P("E.spinit")
    p.branch({1: "ESI.m"}, "DEAD.op", [("CMPI", "na", 1)])
    P("ESI.m").a(("LDI", "a1", SPREG)).goto("E.mov")
    # callr r: FF /2 modrm(3, 2, r), a REX.B (0x41, no W) first for r8..r15
    p = P("E.callr")
    p.branch({(1, 2): "ECR.x"}, "ECR.o", [("CMPI", "a0", 8)])
    byte(P("ECR.x"), 0x41).goto("ECR.o")
    p = P("ECR.o")
    byte(p, 0xFF)
    p.a(("LDI", "mr_m", 3), ("LDI", "mr_r", 2), ("COPYW", "mr_b", "a0")).call("MODRM").goto("NEXTL")
    # shl64/shr64/lshr64 d, s, c (emit_x86, [I-14]): mov r11, s; mov rbx, c; push rcx; mov rcx, rbx;
    # rex.WB D3 /ext r11; pop rcx; mov d, r11 -- the count must be in cl, and rcx is a tape register (r4).
    # r11 and rbx are no tape register (catalog.REGMAP), so no tape operand can alias them.
    p = P("E.shf")
    p.a(("LDI", "al_o", 0x89), ("LDI", "al_d", SCR), ("COPYW", "al_s", "a1")).call("ALU")
    p.a(("LDI", "al_o", 0x89), ("LDI", "al_d", SCR2), ("COPYW", "al_s", "a2")).call("ALU")
    byte(p, 0x51)                                                     # push rcx
    p.a(("LDI", "al_o", 0x89), ("LDI", "al_d", 1), ("LDI", "al_s", SCR2)).call("ALU")
    byte(p, 0x49)                                                     # rex(1, 0, 0, 1)
    byte(p, 0xD3)
    p.a(("LDI", "mr_m", 3), ("LDX", "mr_r", "opid", SHX), ("LDI", "mr_b", SCR)).call("MODRM")
    byte(p, 0x59)                                                     # pop rcx
    p.a(("LDI", "al_o", 0x89), ("COPYW", "al_d", "a0"), ("LDI", "al_s", SCR)).call("ALU").goto("NEXTL")
    p = P("NEXTL")          # a non-branch: its bytes become a blob, its size known
    p.a(("OCUT", "blob", "omark"), ("STX", "npc", BLB, "blob"), ("BLEN", "t", "blob"), ("STX", "npc", SZ, "t"),
        ("LDI", "t", 0), ("STX", "npc", KND, "t"), ("ALUI", "add", "npc", "npc", 1)).goto("SKIPL")
    install_fp(E, byte)
    install_address(E, byte, KND, SZ, OFF, LABD)
    relax()
    p = P("DONE").call("RELAX").call("LAYOUT").call("WRITE")
    if image:
        from elfimage import install as install_elf
        install_elf(E, byte, OFF, LABD,image_format="macho" if image=="macho" else "elf")
        p.call("ELF")
    p.a(("ACCEPT",)).goto("DEAD")
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": "START", "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    if len(sys.argv) not in (2,3) or (len(sys.argv)==3 and sys.argv[2] not in ("--elf","--macho")):
        sys.exit("usage: gen.py OUT.json [--elf|--macho]")
    d = build(image=("macho" if sys.argv[2]=="--macho" else True) if len(sys.argv)==3 else False)
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d  action seqs %d (%d actions)  json %d B\n" % (st, ent, ns, na, len(s)))
