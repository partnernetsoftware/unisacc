"""Oracle for UJS. Same P-1 / P-2 contract as unisa. [P-1] [P-2]"""
from .gold import STAGES

NET_READY = 0.85


class Oracle:
    def __init__(self, nets=None, drive="gold"):
        self.drive = drive
        self.nets = nets or {}
        self.stats = {}
        self.built = (drive == "built")
        self._use_net = {}
        for name in STAGES:
            net = self.nets.get(name)
            if self.built:
                self._use_net[name] = net is not None
            else:
                self._use_net[name] = bool(
                    net is not None
                    and getattr(net, "exact", False)
                    and drive != "gold")

    def ask(self, stage, key):
        st = STAGES[stage]
        assert len(key) == len(st.fields), \
            "P-2: %s arity %d != %d" % (stage, len(key), len(st.fields))
        for i, v in enumerate(key):
            if v not in st._idx[i]:
                raise AssertionError(
                    "P-2: %s field %s got %r, not in K_s"
                    % (stage, st.fields[i][0], v))
        s = self.stats.setdefault(stage, [0, 0])
        if self._use_net.get(stage):
            s[0] += 1
            lab = self.nets[stage].predict(key)
            return lab["y"] if len(st.heads) == 1 else lab
        s[1] += 1
        lab = st.label(*key)
        return lab["y"] if len(st.heads) == 1 else lab

    def summary(self):
        d = sum(1 for k, v in self._use_net.items() if v)
        return "nets: %d/%d driven" % (d, len(STAGES))
