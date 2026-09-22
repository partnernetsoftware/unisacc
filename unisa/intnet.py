"""Integer inference over constructed weights. [K-5] [E-22]

The hidden activation is always 0 or 1, so layer 2 needs no multiply: a firing
unit simply ADDS its W2 entries.  Layer 1 is |F| integer adds via an adjacency
list -- one-hot input never materialises a vector.

Nothing in this file touches a float.
"""
from .construct import build_net, verify_int
from .gold import STAGES


class IntNet:
    __slots__ = ("stage", "heads", "H", "feeds", "b1", "w2", "offs",
                 "vidx", "ncls", "exact", "maxlogit")

    def __init__(self, stage, p, stages=None):
        reg = stages if stages is not None else STAGES
        self.stage = stage if isinstance(stage, str) else stage.name
        S = reg[self.stage]
        self.heads = [(hn, list(cl)) for hn, cl in p["heads"]]
        self.H = p["H"]
        self.offs = p["offs"]
        self.b1 = list(p["b1"])
        self.vidx = [{v: i for i, v in enumerate(vo)} for (_, vo) in S.fields]
        # coord -> units it feeds (W1 is binary, so this is all of W1)
        self.feeds = [[] for _ in range(p["h0"])]
        for c in range(p["h0"]):
            row = p["W1"][c]
            for j in range(p["H"]):
                if row[j]:
                    self.feeds[c].append(j)
        # head -> unit -> [(class, weight)]   sparse, integers only
        self.w2 = {}
        self.ncls = {}
        for hn, cl in self.heads:
            M = p["W2"][hn]
            self.ncls[hn] = len(cl)
            self.w2[hn] = [[(k, M[j][k]) for k in range(len(cl)) if M[j][k]]
                           for j in range(p["H"])]

    def predict(self, key):
        """key: tuple of field VALUES -> {head: class name}.  Pure integer."""
        hit = [0] * self.H
        for i, v in enumerate(key):
            for j in self.feeds[self.offs[i] + self.vidx[i][v]]:
                hit[j] += 1
        b1, w2 = self.b1, self.w2
        out = {}
        for hn, cl in self.heads:
            z = [0] * self.ncls[hn]
            rows = w2[hn]
            for j in range(self.H):
                if hit[j] + b1[j] > 0:            # ReLU; activation is exactly 1
                    for (k, w) in rows[j]:
                        z[k] += w                 # no multiply, no shift
            best = 0
            bv = z[0]
            for k in range(1, len(z)):
                if z[k] > bv:
                    bv = z[k]
                    best = k
            out[hn] = cl[best]
        return out

    def nunits(self):
        return self.H

    def weight_values(self):
        vals = set(self.b1)
        for hn, _ in self.heads:
            for row in self.w2[hn]:
                for (_, w) in row:
                    vals.add(w)
        return sorted(vals)


def build(stage, verify=True, stages=None):
    # `stage` may be a name (looked up in `stages` or the unisa registry) or a
    # Stage object.  Construction always receives the Stage object so sibling
    # packages can share this kernel without registering into unisa.gold.
    reg = stages if stages is not None else STAGES
    st = stage if not isinstance(stage, str) else reg[stage]
    name = st.name
    p = build_net(st)
    n = IntNet(name, p, stages=reg)
    if verify:
        bad, mx_pre, mx_log = verify_int(p)
        n.exact = not bad
        n.maxlogit = mx_log
        assert n.exact, "%s: construction not exact (%d wrong)" % (name, len(bad))
    else:
        n.exact, n.maxlogit = None, None
    return n


def build_all(names=None, verify=True, stages=None):
    from .gold import ALL as _ALL
    reg = stages if stages is not None else STAGES
    names = names or (list(reg) if stages is not None else _ALL)
    return {n: build(n, verify, stages=reg) for n in names}


# -- cache -----------------------------------------------------------------
# Constructing all 11 nets takes ~6s; the result is deterministic, so cache it.
# Deliberately a plain sorted-key JSON so the cache is diffable and its
# byte-reproducibility is visible. [D-5]
import json
import os


def to_dict(n):
    return {
        "stage": n.stage,
        "H": n.H,
        "offs": n.offs,
        "b1": n.b1,
        "heads": [[hn, cl] for hn, cl in n.heads],
        "feeds": n.feeds,
        "w2": {hn: [[list(t) for t in row] for row in n.w2[hn]]
               for hn, _ in n.heads},
        "maxlogit": n.maxlogit,
    }


def from_dict(d, stages=None):
    reg = stages if stages is not None else STAGES
    S = reg[d["stage"]]
    n = IntNet.__new__(IntNet)
    n.stage = d["stage"]
    n.H = d["H"]
    n.offs = d["offs"]
    n.b1 = d["b1"]
    n.heads = [(hn, list(cl)) for hn, cl in d["heads"]]
    n.feeds = d["feeds"]
    n.w2 = {hn: [[tuple(t) for t in row] for row in d["w2"][hn]]
            for hn, _ in n.heads}
    n.ncls = {hn: len(cl) for hn, cl in n.heads}
    n.vidx = [{v: i for i, v in enumerate(vo)} for (_, vo) in S.fields]
    n.exact = True
    n.maxlogit = d.get("maxlogit")
    return n


def save_all(nets, path):
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    blob = {k: to_dict(v) for k, v in sorted(nets.items())}
    with open(path, "w") as f:
        json.dump(blob, f, sort_keys=True, separators=(",", ":"))
    return os.path.getsize(path)


def load_all(path, stages=None):
    with open(path) as f:
        return {k: from_dict(v, stages=stages) for k, v in json.load(f).items()}
