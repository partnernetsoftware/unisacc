"""The E1 delta (gen.py's JSON) in the E0 executor's table format.

    python3 exec/lex/tbl.py delta.json out.tbl
    python3 exec/lex/tbl.py check delta.json fdump    (T1, see check())

exec/exec.c knows nothing about C; this only renames.  States become
numbers (DISPATCH first, so q0 = 0; the pseudo-state HALT, reached only by
actions that halt, becomes 0 too), stack symbols P D0..D9 become 0..10 with
BOT = NG = 11, R = 0..19, and each E1 action becomes a fixed sequence of the
executor's generic actions (exec/README.md):

    ADV          ADV
    OUT b        EMIT b
    MARK s       GETI s                      W[s] := i
    JUMP s       SETI s                      i := W[s]
    SPAN s       GETI t; SPAN s t            append x[W[s]..i)
    SPAN2 s e    SPAN s e
    INC s        LDI one 1; ALU add s s one
    COPYW d s    ALU or d s s
    DIVMOD10 s   LDI ten 10; ALU remu m s ten; ALU ltu f s ten; ALU mul f f ten;
                 ALU add m m f; ALU divu s s ten; SETRW m
    PUSH g/POP   PUSH g / POP
    ACCEPT       ACC
    REJECT k     REJ code(k)   (codes: REJECTS below, 1-based)

Per state: one all-wildcard row carries the state's most frequent entry, then
one row per exception on the component the state reads.
"""
import json
import sys

REJECTS = ["unexpected character", "missing terminating '\"' character",
           "missing terminating ' character", "stray char", "unreachable"]
WK = {"S": 0, "E": 1, "B": 2, "NT": 3, "T": 4}
TI, TEN, M, F, ONE = 10, 11, 12, 13, 14
GAM = ["P"] + ["D%d" % d for d in range(10)]
NG, NR = len(GAM), 20


def acts(a):
    op = a[0]
    if op == "ADV":
        return [[2]]
    if op == "OUT":
        return [[5, a[1]]]
    if op == "MARK":
        return [[15, WK[a[1]]]]
    if op == "JUMP":
        return [[16, WK[a[1]]]]
    if op == "SPAN":
        return [[15, TI], [17, WK[a[1]], TI]]
    if op == "SPAN2":
        return [[17, WK[a[1]], WK[a[2]]]]
    if op == "INC":
        k = WK[a[1]]
        return [[8, ONE, 1], [10, 0, k, k, ONE]]
    if op == "COPYW":
        return [[10, 6, WK[a[1]], WK[a[2]], WK[a[2]]]]
    if op == "DIVMOD10":
        k = WK[a[1]]
        return [[8, TEN, 10], [10, 4, M, k, TEN], [10, 10, F, k, TEN], [10, 2, F, F, TEN],
                [10, 0, M, M, F], [10, 3, k, k, TEN], [18, M]]
    if op == "PUSH":
        return [[3, GAM.index(a[1])]]
    if op == "POP":
        return [[4]]
    if op == "ACCEPT":
        return [[0]]
    if op == "REJECT":
        return [[1, REJECTS.index(a[1]) + 1]]
    raise ValueError(op)


def check(d, dump):
    """T1: the executor's loaded map (exec -fdump) equals delta on the whole
    domain -- every state, every value of the component it reads -- and every
    component it does not read is unread by the executor too."""
    names = list(d["states"])
    qix = {n: k for k, n in enumerate(names)}
    qix["HALT"] = 0
    got = {}
    for ln in open(dump):
        v = list(map(int, ln.split()))
        got[tuple(v[:4])] = v[4:]
    n = bad = 0
    for q, nm in enumerate(names):
        mode, row = d["states"][nm]
        for k, e in row.items():
            if mode == "b":
                key = (q, -1, int(k), -1)
            elif mode == "t":
                key = (q, -1, -1, NG if k == "BOT" else GAM.index(k))
            else:
                key = (q, int(k), -1, -1)
            if key not in got:          # the state's component is unread: one cell
                key = (q, -1, -1, -1)
            xs = [x for a in d["seqs"][e[1]] for x in acts(a)]
            want = [qix[e[0]], len(xs)] + [y for x in xs for y in x]
            n += 1
            if got.get(key) != want:
                bad += 1
    extra = set(k[0] for k in got) - set(range(len(names)))
    print("T1 %s: %d observations (states %d), wrong %d, stray states %d"
          % ("ok" if not bad and not extra else "FAIL", n, len(names), bad, len(extra)))
    return 1 if bad or extra or n == 0 else 0


def main():
    if sys.argv[1] == "check":
        return check(json.load(open(sys.argv[2])), sys.argv[3])
    d = json.load(open(sys.argv[1]))
    names = list(d["states"])
    assert names[0] == "DISPATCH"
    qix = {n: k for k, n in enumerate(names)}
    qix["HALT"] = 0
    seqs = []
    for s in d["seqs"]:
        xs = [x for a in s for x in acts(a)]
        seqs.append("%d " % len(xs) + " ".join(" ".join(map(str, x)) for x in xs))

    def ent(e):
        return "%d %s" % (qix[e[0]], seqs[e[1]])
    rows = []
    for n in names:
        mode, row = d["states"][n]
        q = qix[n]
        cnt = {}
        for v in row.values():
            cnt[tuple(v)] = cnt.get(tuple(v), 0) + 1
        dflt = max(cnt, key=lambda e: cnt[e])
        rows.append("%d -1 -1 -1 %s" % (q, ent(dflt)))
        for k, v in row.items():
            if tuple(v) == dflt:
                continue
            if mode == "b":
                rows.append("%d -1 %s -1 %s" % (q, k, ent(v)))
            elif mode == "t":
                rows.append("%d -1 -1 %d %s" % (q, NG if k == "BOT" else GAM.index(k), ent(v)))
            else:
                rows.append("%d %s -1 -1 %s" % (q, k, ent(v)))
    with open(sys.argv[2], "w") as f:
        f.write("# E1 lexer delta, generated by exec/lex/tbl.py from gen.py -- do not edit\n")
        f.write("%d %d %d 0\n0 1 1 %d\n%d\n" % (len(names), NR, NG, REJECTS.index("unreachable") + 1, len(rows)))
        f.write("\n".join(rows) + "\n")
    print("states %d rows %d seqs %d" % (len(names), len(rows), len(seqs)))


if __name__ == "__main__":
    sys.exit(main())
