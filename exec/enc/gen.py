"""E5, first slice (research/e5-slice.md): the x86_64 encoder for straight-line
lowered instructions, as a delta for the generic executor.

    python3 exec/enc/gen.py delta.json

Input: TIns text, one instruction per line (`op arg, ...`; machine register
names; integers).  Output: the machine code bytes, as unisa/emit_x86.encode
writes them.  Ops: mov, imm, add64/sub64/xor64/and64/or64, mul64, load64,
store64, .ld/.st (1, 2, 4, 8 bytes), the six setcc ops, ret; anything else is
rejected as not covered.

Read, not copied: catalog.ENCSPEC's alu2 opcodes and setcc bytes (generated
into memory at START).  Hand structure, stated: the register numbers
(emit_x86.NUM) and the REX / ModRM / SIB / displacement packing below.
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
OPC, REGN = 70 * 10 ** 6, 71 * 10 ** 6       # OPC[op id] = class + 1; REGN[name id] = register + 1
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
    p.goto("LINE")
    # LINE: the op word, then up to four comma-separated arguments into a0..a3 (a register's number or an integer)
    g.on("LINE", [EOF], "DONE", [])
    g.on("LINE", [10], "LINE", [("ADV",)])
    g.els("LINE", "LW", [("MARK", "ws")])
    g.on("LW", [32] + NL, "LW.e", [("MARK", "we")])
    g.els("LW", "LW", [("ADV",)])
    p = P("LW.e")
    p.a(("INTERN", "opid", "ws", "we"), ("LDX", "cls", "opid", OPC), ("LDI", "na", 0)).goto("ARG")
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
    g.on("NEXTL", [10], "LINE", [("ADV",)])
    g.on("NEXTL", [EOF], "DONE", [])
    g.els("NEXTL", "NEXTL", [("ADV",)])
    P("DONE").a(("ACCEPT",)).goto("DEAD")
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": "START", "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    d = build()
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d  action seqs %d (%d actions)  json %d B\n" % (st, ent, ns, na, len(s)))
