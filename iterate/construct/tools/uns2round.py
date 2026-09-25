"""Decode a UNS2 blob with the deployment loader and run it over every key. [J10]

    python3 iterate/construct/tools/uns2round.py blob.uns2 weights/gold/prec.tsv ...

The decoder is unisa/uns2.py `load` (the loader tests/artifacts.sh round-trips
the shipped pack through); it rebuilds IntNets, deriving b1 from W1 as the
deployment does.  Inference is IntNet.predict, the deployed integer
arithmetic (a unit ADDS its W2 once when hit + b1 > 0).  For every key of the
original domain, from the TSV, and every head, predict must return the TSV
label.  predict's argmax takes the first maximum, so a tie would pass
silently; the logits are therefore also summed here with predict's exact
rule (no multiply) and the maximum is required to be unique.  The stage list
in the blob must equal the TSVs given.
"""
import os
import sys

R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__)))))
sys.path.insert(0, R)
from unisa import tsvgold, uns2  # noqa: E402


def logits(n, key, hn):
    hit = [0] * n.H
    for i, v in enumerate(key):
        for j in n.feeds[n.offs[i] + n.vidx[i][v]]:
            hit[j] += 1
    z = [0] * n.ncls[hn]
    for j in range(n.H):
        if hit[j] + n.b1[j] > 0:
            for (k, w) in n.w2[hn][j]:
                z[k] += w
    return z


def main(argv):
    blob = open(argv[0], "rb").read()
    paths = argv[1:]
    d = os.path.dirname(paths[0]) or "."
    names = [os.path.basename(p)[:-len(".tsv")] for p in paths]
    reg = tsvgold.load_all(d, names)
    nets = uns2.load(blob, reg)
    if sorted(nets) != sorted(names):
        print("round trip: blob holds %s, asked %s" % (sorted(nets), sorted(names)))
        return 1
    bad = 0
    for name in sorted(names):
        S, n = reg[name], nets[name]
        nk = 0
        for kv in S.keys():
            lab = S.label(*kv)
            got = n.predict(kv)
            for hn, cl in n.heads:
                z = logits(n, kv, hn)
                top = max(z)
                if got[hn] != lab[hn] or z.count(top) != 1 or cl[z.index(top)] != lab[hn]:
                    bad += 1
                    if bad <= 5:
                        print("round trip: %s %r head %s: got %s want %s z %s"
                              % (name, kv, hn, got[hn], lab[hn], z))
            nk += 1
        print("round trip %s: %d keys x %d heads, deployed argmax %s"
              % (name, nk, len(n.heads), "all equal the TSV, no tie" if not bad else "WRONG"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
