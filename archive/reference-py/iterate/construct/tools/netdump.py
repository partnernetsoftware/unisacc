"""Print a stage's constructed net in construct.c's canonical text form. [J10]

    python3 iterate/construct/tools/netdump.py [-d] weights/gold/prec.tsv

The net comes from the Python constructor, intnet.build(stage,
stages=tsvgold.load_all(...)); the lines mirror intnet.to_dict field for
field (stage, H, offs, b1, heads, feeds, w2, maxlogit), then `exact <keys>`.
-d adds the intermediate dumps construct.c -d prints; for a multi-head
stage (tyinfo) that includes T4's rebuilt decision lists and every selection.
"""
import os
import sys

R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__)))))
sys.path.insert(0, R)
from unisa import construct, intnet, tsvgold  # noqa: E402


def cube(D, c):
    out = []
    for i, s in enumerate(c):
        if len(s) == len(D.vocabs[i]):
            out.append("*")
        else:
            out.append("{" + ",".join(str(g) for g in sorted(s)) + "}")
    return " " + " ".join(out)


def multihead(st):
    """build_net for a multi-head stage, observed from outside: every
    decision_list call (the initial one per head, then T4's per head and
    round, in call order) and every selection (build_net's min over the
    picked starts) is printed as it happens.  construct.py is not changed:
    the module's decision_list and min are wrapped for the duration of one
    build_net call, and the candidate lists are read from the selection
    key's closure."""
    heads = [h[0] for h in st.heads]
    orig_dl = construct.decision_list
    calls = [0]

    def dl(D, classof, pool=None, *a):
        r = orig_dl(D, classof, pool, *a)
        hn = heads[calls[0] % len(heads)]
        calls[0] += 1
        lv = construct.ranks(D, r)
        pk = {construct.cube_key(c) for c in pool} if pool else set()
        print("dl %s pool %d rules %d" % (hn, len(pool) if pool else 0, len(r)))
        for j, ((c, L), l) in enumerate(zip(r, lv)):
            print("rule %d:%s => %s rank %d%s" % (
                j, cube(D, c), L, l,
                " pool" if construct.cube_key(c) in pk else ""))
        return r

    def mn(*args, **kw):
        if "key" not in kw or isinstance(args[0], range) or len(args) != 1:
            return min(*args, **kw)
        key = kw["key"]
        cl = dict(zip(key.__code__.co_freevars,
                      (c.cell_contents for c in key.__closure__)))
        cands = cl["cands"]
        print("select")
        for hn in heads:
            print("cands %s:%s" % (hn, "".join(" %d" % len(c) for c in cands[hn])))
        picked = list(args[0])
        for i, c in enumerate(picked):
            u, ln = key(c)
            print("pick %d:%s units %d len %d" % (
                i, "".join(" %d" % c[h] for h in heads), u, ln))
        r = min(picked, key=key)
        print("chosen:%s units %d" % ("".join(" %d" % r[h] for h in heads), key(r)[0]))
        return r

    construct.decision_list = dl
    construct.min = mn
    try:
        return construct.build_net(st)
    finally:
        construct.decision_list = orig_dl
        del construct.min


def main(argv):
    dbg = "-d" in argv
    path = [a for a in argv if a != "-d"][0]
    name = os.path.basename(path)[:-len(".tsv")]
    stages = tsvgold.load_all(os.path.dirname(path) or ".", [name])
    st = stages[name]
    if dbg:
        D = construct.Domain(st)
        for i, g in enumerate(D.groups):
            print("groups %d: %s" % (i, " ".join(
                "[" + ",".join(str(v) for v in m) + "]" for m in g)))
        if len(st.heads) > 1:
            p = multihead(st)
        else:
            hn = st.heads[0][0]
            labels = [st.label(*kv) for kv in D.keys]
            classof = [labels[j][hn] for j in range(D.n)]
            rules = construct.decision_list(D, classof, None, True, True)
            lv = construct.ranks(D, rules)
            for j, ((c, L), l) in enumerate(zip(rules, lv)):
                print("rule %d:%s => %s rank %d" % (j, cube(D, c), L, l))
            p = construct.build_net(st)
        for h, k in p["kinds"].items():
            print("kind %s %s" % (h, k))
        for j, c in enumerate(p["units"]):
            print("unit %d:%s" % (j, cube(D, c)))
    n = intnet.build(name, stages=stages)
    d = intnet.to_dict(n)
    print("stage %s" % d["stage"])
    print("H %d" % d["H"])
    print("offs" + "".join(" %d" % o for o in d["offs"]))
    print("b1" + "".join(" %d" % b for b in d["b1"]))
    for hn, cl in d["heads"]:
        print("head %s" % hn + "".join(" %s" % c for c in cl))
    print("feeds %d" % len(d["feeds"]))
    for c, js in enumerate(d["feeds"]):
        print("f %d:" % c + "".join(" %d" % j for j in js))
    for hn, _ in d["heads"]:
        print("w2 %s" % hn)
        for j, row in enumerate(d["w2"][hn]):
            print("r %d:" % j + "".join(" %d,%d" % (k, w) for k, w in row))
    print("maxlogit %d" % d["maxlogit"])
    assert n.exact
    print("exact %d" % len(st.keys()))


if __name__ == "__main__":
    main(sys.argv[1:])
