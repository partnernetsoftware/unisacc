"""Segment nets: one model per compiler phase instead of one per decision. [E-18]

    s1  源码预处理     pp lex
    s2  代码到 IR      parse type scope irsel
    s3  IR 到多 ISA    isel abi enc reloc

Encoding: a `stage` field plus three generic slots whose vocabularies are the
stage-qualified union of that slot across the segment (qualification keeps two
stages from accidentally sharing an embedding row).  Stages of lower arity pad
with `-`.  Each sample supervises only its own stage's heads, so the segment is
a masked multi-task net, not a net that must also learn "not applicable".
"""
from ..gold import STAGES

SEGMENTS = {
    "s1": ("pp", "lex"),
    "s2": ("parse", "type", "scope", "irsel"),
    "s3": ("isel", "abi", "enc", "reloc"),
}
SEG_OF = {st: seg for seg, sts in SEGMENTS.items() for st in sts}
PAD = "-"
MAXSLOT = 3


def qual(stage, v):
    return "%s:%s" % (stage, v)


class SegmentStage:
    """Same interface as gold.Stage, so train/acc/oracle need no special case."""

    def __init__(self, seg, cfg):
        self.name = seg
        self.members = SEGMENTS[seg]
        self.cfg = cfg
        self.weight = None

        slots = [[] for _ in range(MAXSLOT)]
        for st in self.members:
            S = STAGES[st]
            for i in range(MAXSLOT):
                if i < len(S.fields):
                    slots[i] += [qual(st, v) for v in S.fields[i][1]]
        self.fields = [("stage", tuple(self.members))] + [
            ("k%d" % i, tuple([PAD] + slots[i])) for i in range(MAXSLOT)]

        self.heads = []
        for st in self.members:
            for (hn, cl, sh) in STAGES[st].heads:
                self.heads.append(("%s.%s" % (st, hn), cl,
                                   ("reg" if sh else None)))
        self._idx = [{v: i for i, v in enumerate(vo)} for (_, vo) in self.fields]
        self._hidx = [{c: i for i, c in enumerate(cl)}
                      for (_, cl, _) in self.heads]
        self._hpos = {h[0]: j for j, h in enumerate(self.heads)}

    def key_for(self, stage, kv):
        k = [stage] + [PAD] * MAXSLOT
        for i, v in enumerate(kv):
            k[1 + i] = qual(stage, v)
        return tuple(k)

    def index(self, key):
        return tuple(self._idx[i][v] for i, v in enumerate(key))

    def corpus(self):
        out = []
        for st in self.members:
            S = STAGES[st]
            for kv in S.keys():
                lab = S.label(*kv)
                li = {}
                for (hn, _, _) in S.heads:
                    full = "%s.%s" % (st, hn)
                    li[full] = self._hidx[self._hpos[full]][lab[hn]]
                out.append((self.index(self.key_for(st, kv)), li))
        return out

    def train_corpus(self):
        return [(k, l, 1.0) for (k, l) in self.corpus()]

    def rows(self):
        return sum(STAGES[st].rows() for st in self.members)


def build(widths=None):
    w = widths or {"s1": [24], "s2": [96, 64], "s3": [96, 64]}
    d = {"s1": 8, "s2": 12, "s3": 12}
    return {seg: SegmentStage(seg, dict(d=d[seg], hidden=w[seg],
                                        seed={"s1": 41, "s2": 43, "s3": 47}[seg]))
            for seg in SEGMENTS}
