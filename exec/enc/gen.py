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


def emit_rules(phase):
    from finite_rules import install as install_rules
    sequences = {name: [tuple(a) for a in json.loads(actions)] for name, actions in
                 (line.rstrip('\n').split('\t') for line in open(os.path.join(HERE, 'x86-emit-sequences.tsv')) if not line.startswith('#'))}
    sequences.update({'byte'+line.strip(): byte(P('byte.binding'), int(line)).acts for line in
                      open(os.path.join(HERE, 'x86-emit-bytes.tsv')) if not line.startswith('#')})
    sequences.update({name: E.rej(reason) for name, reason in
                      (line.rstrip('\n').split('\t') for line in open(os.path.join(HERE, 'x86-emit-reject.tsv')) if not line.startswith('#'))})
    for line in open(os.path.join(HERE, 'x86-emit-instances.tsv')):
        if line.startswith('#'): continue
        selected, section, values, names, prepare = line.rstrip('\n').split('\t')
        if selected != phase: continue
        bindings = {name: globals()[name] for name in ('AOPC', 'ACC', 'SHX', 'BLB', 'SZ', 'KND', 'SCR', 'SCR2', 'SPREG')}
        bindings.update(RAX=NUM['rax'], RDX=NUM['rdx'])
        bindings.update(json.loads(values))
        bindings.update({name: P(owner).fresh(kind) for name, owner, kind in json.loads(names)})
        install_rules(g, HERE, 'x86-emit', section=section, bindings=bindings,
                      sequences={**sequences, 'prepare': sequences[prepare]},
                      classes={'division_registers': tuple(NUM[r] for r in REGMAP['x86_64'][:7])})


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
    emit_rules('pre')
    emit_rules('division')
    emit_rules('post')
    install_fp(E, byte)
    install_address(E, byte, KND, SZ, OFF, LABD)
    from x86itoa import install as install_itoa
    install_itoa(E,KND,SZ)
    from x86win import install as install_win
    install_win(E, byte, KND, SZ)
    relax()
    completion = {'done'+str(i): P('DONE').fresh('r') for i in range(3)}
    install_rules(g, HERE, 'x86-emit', section='done-write', bindings=completion)
    if image:
        from elfimage import install as install_elf
        install_elf(E, byte, OFF, LABD,image_format=image if isinstance(image,str) else "elf")
        completion['done3'] = P('DONE').fresh('r')
    install_rules(g, HERE, 'x86-emit', section='done-image' if image else 'done-raw', bindings=completion)
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
