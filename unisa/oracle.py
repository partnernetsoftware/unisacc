"""The one dispatch point between learned tables and classic code. [O] [P-1] [P-2]

    oracle.ask(stage, key) -> class_name

Hard constraints, both of them load-bearing for the equivalence argument:

  [P-1]  Only the class name crosses this boundary.  No logits, no margin, no
         confidence, no ready-flag.  No ensembling, no low-confidence fallback.
         If any of that leaks, P-5 (compositional equivalence) fails.
  [P-1b] The NET_READY gate is resolved ONCE at load time, not per decision --
         given a weights snapshot the compiler is a fixed function.
  [P-2]  key must be in K_s.  Asserted unconditionally; proving the assert
         unreachable is the one obligation enumeration cannot discharge.
"""
from .gold import STAGES
from .linalg import argmax
from .control.train import NET_READY


def _ablations():
    """UNISA_ABLATE=stage[.head],... -- see Oracle.ask"""
    import os
    d = {}
    for item in os.environ.get("UNISA_ABLATE", "").split(","):
        if item:
            st, _, hd = item.partition(".")
            d[st] = hd
    return d


_ABLATE = _ablations()


class Oracle:
    def __init__(self, nets=None, drive="spec"):
        self.drive = drive
        self.nets = nets or {}
        self.stats = {}
        # `built` = constructed integer nets (K-5): exact by construction, so
        # the readiness gate is vacuous -- there is no acc to fall short of.
        self.built = (drive == "built")
        # [P-1b] resolved once, here, and never re-read during compilation
        self._use_net = {}
        for name in STAGES:
            net = self.nets.get(name)
            if self.built:
                self._use_net[name] = net is not None
            else:
                self._use_net[name] = bool(
                    net is not None and net.acc >= NET_READY and drive != "gold")

    def ask(self, stage, key):
        out = self._ask(stage, key)
        ab = _ABLATE.get(stage)
        if ab is None:
            return out
        # ablation [A-36]: rotate one head's answer to the next class, so a
        # suite can check the answer is USED -- a stage whose answer can be
        # wrong without changing any output is asked, not obeyed
        st = STAGES[stage]
        if len(st.heads) == 1:
            cl = list(st.heads[0][1])
            return cl[(cl.index(out) + 1) % len(cl)]
        out = dict(out)
        for hn, cl, *_ in st.heads:
            if ab in ("", hn):
                cl = list(cl)
                out[hn] = cl[(cl.index(out[hn]) + 1) % len(cl)]
        return out

    def _ask(self, stage, key):
        st = STAGES[stage]
        # [P-2] key totality -- unconditional, never compiled out
        assert len(key) == len(st.fields), \
            "P-2: %s arity %d != %d" % (stage, len(key), len(st.fields))
        for i, v in enumerate(key):
            if v not in st._idx[i]:
                raise AssertionError("P-2: %s field %s got %r, not in K_s"
                                     % (stage, st.fields[i][0], v))
        s = self.stats.setdefault(stage, [0, 0])
        if self._use_net[stage]:
            s[0] += 1
            net = self.nets[stage]
            if self.built:                       # pure-integer path [K-5]
                lab = net.predict(key)
                return lab["y"] if len(st.heads) == 1 else lab
            ki = tuple(st._idx[i][v] for i, v in enumerate(key))
            out, _ = net.forward(ki)
            if len(st.heads) == 1:
                return st.heads[0][1][argmax(out["y"])]
            return {h.name: h.classes[argmax(out[h.name])] for h in net.heads}
        s[1] += 1
        lab = st.label(*key)
        return lab["y"] if len(st.heads) == 1 else lab

    def driven(self):
        n = sum(1 for k, v in self._use_net.items() if v and k != "combo")
        return n, len([k for k in STAGES if k != "combo"])

    def summary(self):
        d, t = self.driven()
        return "nets: %d/%d driven" % (d, t)
