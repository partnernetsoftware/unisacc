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
    R mode n  key next seq ... a state: mode 0 b / 1 t / 2 r; key -1 = BOT
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
       ("C64", "rr"), ("C64U", "rr")]
CODE = {n: k for k, (n, _) in enumerate(OPS)}
ALUOPS = ["add", "sub", "mul", "div", "rem", "and", "or", "xor", "shl", "sar",
          "sdiv", "srem", "udiv", "urem", "not", "shr"]


def main():
    d = json.load(open(sys.argv[1]))
    names = list(d["states"])
    six = {n: k for k, n in enumerate(names)}
    regs, strs = {}, {}

    def reg(n):
        return regs.setdefault(n, len(regs))

    def arg(kind, v):
        if kind == "r":
            return reg(v)
        if kind == "g":
            return six[v]
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
        mode, row = d["states"][n]
        m = {"b": 0, "t": 1, "r": 2}[mode]
        ents = []
        for k, (nx, sq) in row.items():
            key = (-1 if k == "BOT" else six[k]) if m == 1 else int(k)
            ents.append("%d %d %d" % (key, six[nx], sq))
        lines.append("R %d %d %s" % (m, len(ents), " ".join(ents)))
    with open(sys.argv[2], "w") as f:
        f.write("T %d %d %d %d %d\n" % (len(names), len(seqs), len(regs), len(strs), six[d["start"]]))
        for s in sorted(strs, key=strs.get):
            f.write("S %s\n" % (s.encode("latin-1", "replace").hex() or "-"))
        for q in seqs:
            f.write("Q %d %s\n" % (len(q), " ".join(q)))
        for ln in lines:
            f.write(ln + "\n")


if __name__ == "__main__":
    main()
