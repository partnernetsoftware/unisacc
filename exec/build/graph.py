"""Generic graph primitives shared by the stage entries (K2): no stage names, no domain facts.

G      the transition graph of pp/parse-based stages: interned action sequences, on/els/r edge
       writers, finish (RET pop table over the return-label set collected while templates run,
       DEAD totalisation over 257 keys). Was class G in exec/pp/gen.py.
Delta  the lexer's graph (states dict, interned sequences, on adapter for finite_rules).
       Was class Delta in exec/lex/gen.py.
"""


class G:
    def __init__(self):
        self.st = {}
        self.seqs, self.seqix = [], {}
        self.labels = set()
        self.unreach = 0

    def seq(self, acts):
        acts = tuple(tuple(a) for a in acts)
        if acts not in self.seqix:
            self.seqix[acts] = len(self.seqs)
            self.seqs.append(acts)
        return self.seqix[acts]

    def on(self, name, keys, nxt, acts=(), mode="b"):
        if name not in self.st:
            self.st[name] = [mode, {}]
        assert self.st[name][0] == mode, name
        row = self.st[name][1]
        for k in keys:
            if k not in row:
                row[k] = (nxt, self.seq(acts))

    def els(self, name, nxt, acts=(), mode="b"):
        self.on(name, range(257), nxt, acts, mode)

    def r(self, name, cases):          # a state that reads r
        for keys, (nxt, acts) in cases.items():
            self.on(name, keys if isinstance(keys, tuple) else (keys,), nxt, acts, "r")

    def finish(self):
        self.st["RET"] = ["t", {g: (g, self.seq([("POP",)])) for g in sorted(self.labels)}]
        self.st["RET"][1]["BOT"] = ("DEAD", self.seq([("REJECT", "unreachable")]))
        for name, (mode, row) in self.st.items():
            if mode in "br":
                for k in range(257):
                    if k not in row:
                        row[k] = ("DEAD", self.seq([("REJECT", "unreachable")]))
                        self.unreach += 1
        self.els("DEAD", "DEAD", [("REJECT", "unreachable")])


class Delta:
    def __init__(self):
        self.states = {}          # name -> (mode, {value: (next, seqid)})
        self.seqs = []            # action sequences (tuples)
        self.seqix = {}
        self.unreach = 0
        self.st = self.states     # the graph view finite_rules edits
        self.labels = set()

    def seq(self, acts):
        acts = tuple(tuple(a) if isinstance(a, list) else a for a in acts)
        if acts not in self.seqix:
            self.seqix[acts] = len(self.seqs)
            self.seqs.append(acts)
        return self.seqix[acts]

    def state(self, name, mode):
        if name not in self.states:
            self.states[name] = (mode, {})
        return self.states[name][1]

    # thin adapter for exec/finite_rules.install_template
    def on(self, name, keys, nxt, acts, mode):
        row = self.state(name, mode)
        for key in keys:
            row[key] = (nxt, self.seq(acts))
