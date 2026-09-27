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
C_ITOA = 29
from unisa.catalog import REGMAP     # noqa: E402  (generation time only)
SPREG = NUM[REGMAP["x86_64"][7]]     # the tape SP's machine register (rsp), read, not written here
SHX = 83 * 10 ** 6                           # the /digit of D3 for a shift op id (ENCSPEC shiftext)
SCR2 = 3                                     # rbx: emit_x86.SCR2
SCR = 11                                     # r11: emit_x86.SCR


def byte(p, v):
    return p.a(("LDI", "ob", v), ("OUTW", "ob"))


def procs():
    from pathlib import Path
    from finite_rules import install as install_rules
    labels = (('ALU', 'r'), ('ALU', 'r'), ('LB', 'b'), ('MEM', 'b'),
              ('ME', 'r'), ('ME', 'b'), ('ME', 'b'), ('ME', 'b'),
              ('ME', 'r'), ('ME', 'b'), ('ME', 'b'), ('ME', 'r'),
              ('ME', 'r'), ('ME', 'r'), ('ME', 'r'), ('ME', 'b'))
    bindings = {'label'+str(i): P(owner).fresh(kind)
                for i, (owner, kind) in enumerate(labels)}
    sequences = {'byte'+str(v): byte(P('byte.binding'),v).acts for v in (0x66,0x24)}
    install_rules(g, Path(__file__).parent, 'x86-procs', bindings=bindings,
                  sequences=sequences, section='procs')


def relax():
    from pathlib import Path
    from finite_rules import install as install_rules
    labels = (('RX', 'b'), ('RX', 'b'), ('RX', 'b'), ('RX', 'b'),
              ('RX', 'b'), ('RX', 'b'), ('RX', 'b'), ('RX', 'r'),
              ('RX', 'b'), ('RX', 'b'), ('RX', 'b'), ('RX', 'b'),
              ('RX', 'b'), ('RX', 'b'), ('LADDR', 'b'), ('LA', 'b'),
              ('WR', 'b'), ('WR', 'b'), ('WR', 'r'), ('WR', 'r'),
              ('WR', 'r'), ('WR', 'b'), ('WR', 'r'), ('WR', 'r'),
              ('WR', 'r'), ('WR', 'r'), ('WR', 'b'), ('WR', 'r'))
    bindings = {'label'+str(i): P(owner).fresh(kind)
                for i, (owner, kind) in enumerate(labels)}
    bindings.update({name: globals()[name] for name in
                     ('OFF', 'LABD', 'KND', 'BLB', 'SZ', 'TGT', 'BRG', 'SHT', 'FIT')})
    sequences = {'undefined': E.rej('not covered: a branch to an undefined label')}
    install_rules(g, Path(__file__).parent, 'x86-procs', bindings=bindings,
                  sequences=sequences, section='relax')


def build(image=False):
    E.prn()
    procs()
    p = P("START")
    classes = {".div": C_DIV, ".mod": C_MOD, ".udiv": C_UDIV, ".umod": C_UMOD, "setreg": C_SETREG, "spinit": C_SPINIT, ".zero": C_ZERO, "push": C_PUSH, "pop": C_POP, "nop": C_NOP, ".frame": C_FRAME, "callr": C_CALLR, "mov": C_MOV, "imm": C_IMM, "mul64": C_MUL, "load64": C_LD8, "store64": C_ST8, ".ld": C_LD, ".st": C_ST, "ret": C_RET}
    classes.update(FP_IDS)
    from x86win import IDS as WIN_IDS
    classes.update(WIN_IDS)
    from x86win import init as win_init, reset as win_reset, META as WIN_META
    win_init(p)
    classes.update({"itoa":C_ITOA, "gate": C_GATE, ".lea": C_LEA, "setmem": C_SETMEM, "argsave":C_ARGSAVE, "argvget":C_ARGVGET})
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
    for w, nm in [("mem", "tagmem"), ("addr", "tagaddr"), ("lnx/x86_64", "target1"), ("osx/x86_64", "target2"), ("win/x86_64", "target3")] + [("@"+k, "h_"+k) for k in ("target","data","sym","src_os","data_len","bss","relocs","argc","argv")]:
        p.a(("SBCLR",), [("SBOUT", ch) for ch in w.encode()], ("SBINTERN", "id_" + nm))
    p.a(("LDI", "target_os", 1), ("LDI", "has_relocs", 0))
    p.a(("SBCLR",), [("SBOUT", ch) for ch in b"_start"], ("SBINTERN", "id_entry"))
    for w in ("jump", "jumpz", "call"):
        p.a(("SBCLR",), [("SBOUT", ch) for ch in w.encode()], ("SBINTERN", "id_" + w))
    p.a(("LDI", "npc", 0), ("LDI", "lnum", 0)).goto("LINE")
    from finite_rules import install as install_rules
    from functools import partial
    line_rules = partial(install_rules, g, HERE, 'x86-line')
    line_bindings = {name: globals()[name] for name in
                     ('LABD', 'OPC', 'KND', 'SZ', 'REGN', 'BRG', 'TGT', 'MSN', 'C_GATE')}
    line_names = [line.rstrip('\n').split('\t') for line in
                  open(os.path.join(HERE, 'x86-line-names.tsv')) if not line.startswith('#')]
    line_sequences = {name: E.rej(reason) for name, reason in
                      (line.rstrip('\n').split('\t') for line in
                       open(os.path.join(HERE, 'x86-line-reject.tsv')) if not line.startswith('#'))}
    line_sequences['reset'] = win_reset(P('reset.binding')).acts
    line_bindings.update({name: P(owner).fresh(kind) for part, name, owner, kind in line_names if part == 'line'})
    line_rules(section='line', bindings=line_bindings, sequences=line_sequences)
    for prefix in ('BRM', 'META'):
        line_rules(section='value', bindings=dict(entry=prefix + '.v', body=prefix + '.vv', end=prefix + '.e'))
    bindings = {name: globals()[name] for name in ('REGN', 'C_ITOA', 'C_LEA', 'C_ARGSAVE')}
    for line in open(os.path.join(HERE, 'x86-operand-names.tsv')):
        if not line.startswith('#'):
            name, owner, kind = line.rstrip('\n').split('\t')
            bindings[name] = P(owner).fresh(kind)
    install_rules(g, HERE, 'x86-operand', section='scan', bindings=bindings,
                  sequences={'register_reject': E.rej('not covered: an operand that is not a register or an integer')})
    for i in range(4):
        entry = 'ARG.put' if i == 0 else 'AP.n%d' % (i - 1)
        install_rules(g, HERE, 'x86-operand', section='put', bindings=dict(
            entry=entry, test=P(entry).fresh('b'), hit='AP.%d' % i, next='AP.n%d' % i,
            index=i, value='a%d' % i, kind='ak%d' % i))
    install_rules(g, HERE, 'x86-operand', section='tail')
    line_bindings.update({name: P(owner).fresh(kind) for part, name, owner, kind in line_names if part == 'meta-head'})
    line_rules(section='meta-head', bindings=line_bindings, sequences=line_sequences)
    entry = 'META.gother'
    for key in META_KEYS:
        if key in ('role', 'form', 'reloc', 'carry'): continue
        nxt = 'META.after.' + key
        line_rules(section='key', bindings=dict(entry=entry, test=P(entry).fresh('b'),
            key='id_' + key, yes='WX.meta.' + key if key in WIN_META else 'META.ok', next=nxt))
        entry = nxt
    line_rules(section='key-end', bindings={'entry': entry})
    line_bindings.update({name: P(owner).fresh(kind) for part, name, owner, kind in line_names if part == 'meta-tail'})
    line_rules(section='meta-tail', bindings=line_bindings, sequences=line_sequences)
    # ARGS.d: at the end of the line: the class decides
    p = P("ARGS.d")
    p.branch({C_MOV + 1 - 1: "E.mov", C_IMM: "E.imm", C_ALU: "E.alu", C_MUL: "E.mul", C_LD8: "E.ld8", C_ST8: "E.st8",
              C_LD: "E.ld", C_ST: "E.st", C_SET: "E.set", C_RET: "E.ret", C_SHF: "E.shf", C_CALLR: "E.callr",
              C_PUSH: "E.push", C_POP: "E.pop", C_NOP: "E.nop", C_FRAME: "E.frame", C_ZERO: "E.zero",
              C_ITOA:"ITO.store", C_SETREG: "E.setreg", C_SPINIT: "E.spinit", C_GATE: "E.gate", C_LEA:"AD.lea", C_SETMEM:"AD.setmem", C_ARGSAVE:"AD.argsave", C_ARGVGET:"AD.argvget",
              C_DIV: "E.div", C_MOD: "E.mod", C_UDIV: "E.udiv", C_UMOD: "E.umod",
              **{v: "FP." + k for k, v in FP_IDS.items()}, **{v:"WX.store" for v in WIN_IDS.values()}}, "DEAD.op", [("RLD", "cls")])
    g.on("DEAD.op", range(257), "DEAD", E.rej("not covered: an op outside the first encoder slice"), "r")
    P("EG.choose").branch({1:"WX.store"},"EG.emit",[("CMPI","gwin",1)])
    P("E.gate").branch({1: "EG.choose"}, "DEAD.meta", [("CMPI", "na", 0)])
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
    from x86itoa import install as install_itoa
    install_itoa(E,KND,SZ)
    from x86win import install as install_win
    install_win(E, byte, KND, SZ)
    relax()
    p = P("DONE").call("RELAX").call("LAYOUT").call("WRITE")
    if image:
        from elfimage import install as install_elf
        install_elf(E, byte, OFF, LABD,image_format=image if isinstance(image,str) else "elf")
        p.call("ELF")
    p.a(("ACCEPT",)).goto("DEAD")
    g.finish()
    states = {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in g.st.items()}
    return {"start": "START", "states": states, "seqs": [list(map(list, s)) for s in g.seqs]}


if __name__ == "__main__":
    if len(sys.argv) not in (2,3) or (len(sys.argv)==3 and sys.argv[2] not in ("--elf","--macho","--pe")):
        sys.exit("usage: gen.py OUT.json [--elf|--macho|--pe]")
    d = build(image=sys.argv[2][2:] if len(sys.argv)==3 else False)
    s = json.dumps(d, separators=(",", ":"))
    open(sys.argv[1], "w").write(s)
    st, ent, live, ns, na = E.sizes(d)
    sys.stderr.write("states %d  entries %d  action seqs %d (%d actions)  json %d B\n" % (st, ent, ns, na, len(s)))
