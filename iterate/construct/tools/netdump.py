"""Print a stage's constructed net in construct.c's canonical text form. [J10]

    python3 iterate/construct/tools/netdump.py [-d] weights/gold/prec.tsv

The net comes from the Python constructor, intnet.build(stage,
stages=tsvgold.load_all(...)); the lines mirror intnet.to_dict field for
field (stage, H, offs, b1, heads, feeds, w2, maxlogit), then `exact <keys>`.
-d adds the intermediate dumps construct.c -d prints (single-head stages).
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
