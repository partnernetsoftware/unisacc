#!/usr/bin/env python3
"""A delta (the JSON exec/pp/sim.py runs) as a flat integer table for exec/c/run.c.

    python3 exec/c/tbl.py delta.json out.tbl

Every name becomes a number -- states, registers, action codes, ALU ops;
REJECT reasons become entries of a string table.  Nothing is decided here:
the table is the delta, renumbered.  Text format, one record per line:
    T nstates nseqs nregs nstrs start
    S hex                      a string (REJECT reason)
    Q n  op a1 a2 ...  ...     an action sequence: n actions, each an opcode
                               followed by exactly ARITY[op] integers
    R mode n dnext dseq  key next seq ...
                               a state: mode 0 b / 1 t / 2 r; key -1 = BOT;
                               (dnext, dseq) answers every key not listed
                               (dnext -1: none -- a t row, or an empty one)
"""
import json
import sys

# opcode: (name, argument kinds) -- r register, i immediate, g state, s string, a alu op
OPS = [("ADV", ""), ("MARK", "r"), ("JUMP", "r"), ("LDI", "ri"), ("COPYW", "rr"), ("ALU", "arrr"),
       ("ALUI", "arri"), ("CMP", "rr"), ("CMPI", "ri"), ("RLD", "r"), ("LDX", "rri"), ("STX", "rir"),
       ("OUT", "i"), ("OUTW", "r"), ("COPY", ""), ("COPYT", ""), ("SPAN", "r"), ("SPANT", "r"),
       ("SPAN2", "rr"), ("OLAST", ""), ("ODROP", ""), ("OLEN", "r"), ("OCUT", "rr"), ("ORES", "ri"),
       ("OFILL", "rri"), ("OCLR", ""), ("OSEL", "i"), ("SETOT", "r"), ("XATTR", "r"), ("PUSH", "g"),
       ("POP", ""), ("INTERN", "rrr"), ("BLOBSAVE", "rrr"), ("INPUSH", "r"), ("INPUSHX", "r"),
       ("INPUSHXE", "rr"), ("INPOP", ""), ("SBCLR", ""), ("SBOUT", "i"), ("SBSPAN", "rr"), ("SBBLOB", "r"),
       ("SBINTERN", "r"), ("SBSAVE", "r"), ("SBFIND", "r"), ("BLEN", "rr"), ("BYTE", "r"), ("XLEN", "r"),
       ("DIVMOD10", "r"), ("SWAP", ""), ("ACCEPT", ""), ("REJECT", "s"), ("A64", "arrr"), ("A64I", "arri"),
       ("C64", "rr"), ("C64U", "rr"), ("INC", "r")]   # INC: exec/lex/sim.py's, W := W + 1 mod 2^32
CODE = {n: k for k, (n, _) in enumerate(OPS)}
ALUOPS = ["add", "sub", "mul", "div", "rem", "and", "or", "xor", "shl", "sar",
          "sdiv", "srem", "udiv", "urem", "not", "shr"]


def crosscheck():
    """run.c's enum and ARITY are a second copy of OPS: they must agree, name for name"""
    import os
    import re
    c = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "run.c")).read()
    enum = re.search(r"enum \{(.*?)\};", c, re.S).group(1).replace("\n", " ")
    names = [x.strip() for x in enum.split(",") if x.strip()]
    ar = [int(x) for x in re.search(r"ARITY\[NOP_\] = \{(.*?)\};", c, re.S).group(1).replace("\n", " ").split(",")]
    assert names[-1] == "NOP_" and names[:-1] == [n for n, _ in OPS], "run.c enum differs from OPS"
    assert ar == [len(k) for _, k in OPS], "run.c ARITY differs from OPS"


def main():
    crosscheck()
    d = json.load(open(sys.argv[1]))
    names = list(d["states"])
    # a target with no row (E1's HALT: always entered by a REJECT) gets an empty one
    for mode, row in list(d["states"].values()):
        for nx, _ in row.values():
            if nx not in d["states"] and nx not in names:
                names.append(nx)
    six = {n: k for k, n in enumerate(names)}
    regs, strs, syms = {}, {}, {}

    def sym(v):
        """a stack symbol: a state's index, or a number past the states (E1 pushes its own)"""
        return six[v] if v in six else len(names) + syms.setdefault(v, len(syms))

    def reg(n):
        return regs.setdefault(n, len(regs))

    def arg(kind, v):
        if kind == "r":
            return reg(v)
        if kind == "g":
            return sym(v)
        if kind == "s":
            if isinstance(v, int):
                v = str(v)
            return strs.setdefault(v, len(strs))
        if kind == "a":
            return ALUOPS.index(v)
        return int(v)

    seqs = []
    for s in d["seqs"]:
        out = []
        for a in s:
            name, kinds = OPS[CODE[a[0]]]
            assert len(a) - 1 == len(kinds), a
            out.append(" ".join([str(CODE[a[0]])] + [str(arg(k, v)) for k, v in zip(kinds, a[1:])]))
        seqs.append(out)
    lines = []
    for n in names:
        mode, row = d["states"].get(n, ["b", {}])
        m = {"b": 0, "t": 1, "r": 2}[mode]
        ents, dflt = [], (-1, 0)
        if m != 1:
            # all 257 keys, a missing one as (-1, 0) -- no transition -- so the default
            # can never turn a missing key into a transition; only the others are listed
            full = [(six[row[str(k)][0]], row[str(k)][1]) if str(k) in row else (-1, 0) for k in range(257)]
            cnt = {}
            for e in full:
                cnt[e] = cnt.get(e, 0) + 1
            dflt = max(sorted(cnt), key=cnt.get)
            ents = ["%d %d %d" % (k, e[0], e[1]) for k, e in enumerate(full) if e != dflt]
        else:
            for k, (nx, sq) in row.items():
                ents.append("%d %d %d" % (-1 if k == "BOT" else sym(k), six[nx], sq))
        lines.append("R %d %d %d %d %s" % (m, len(ents), dflt[0], dflt[1], " ".join(ents)))
    with open(sys.argv[2], "w") as f:
        f.write("T %d %d %d %d %d\n" % (len(names), len(seqs), len(regs), len(strs), six[d.get("start", "DISPATCH")]))
        for s in sorted(strs, key=strs.get):
            f.write("S %s\n" % (s.encode("latin-1", "replace").hex() or "-"))
        for q in seqs:
            f.write("Q %d %s\n" % (len(q), " ".join(q)))
        for ln in lines:
            f.write(ln + "\n")


if __name__ == "__main__":
    main()
