"""Generic procedure assembler (K2, was the core of class P in exec/parse/gen.py, now exec/build/parsebase.py): a label
is a state, straight-line actions ride on the outgoing transition, a branch is a state reading r,
call pushes a fresh return label. No stage names, no domain facts. make_P(g, O, rej) returns a
fresh class (its own fresh-label counter n) bound to graph g, text encoder O and reject sequence rej.
"""


import os as _os

# T1d (c): optional fresh-order recorder.  UNISACC_FRESH_LOG=PATH appends one row per fresh
# "n<TAB>owner<TAB>kind<TAB>caller" at exit; unset (default) changes nothing.  caller is the Python
# frame outside this module (file:line); the manifest line is not available here because
# exec/assemble.py rows() drops line numbers when it parses a manifest.
_LOG = _os.environ.get("UNISACC_FRESH_LOG")
_ROWS = []
if _LOG:
    import atexit as _atexit, sys as _sys

    def _flush():
        with open(_LOG, "a") as f:
            f.writelines(_ROWS)
    _atexit.register(_flush)

    def _record(n, owner, kind):
        fr = _sys._getframe(2)
        while fr and fr.f_code.co_filename == __file__:
            fr = fr.f_back
        where = "%s:%d" % (_os.path.relpath(fr.f_code.co_filename), fr.f_lineno) if fr else "-"
        _ROWS.append("%d\t%s\t%s\t%s\n" % (n, owner, kind, where))


def make_P(g, O, rej):
    class P:
        """Procedures as op lists; a label is a state; straight-line actions ride
        on the outgoing transition; a branch is a state reading r."""
        n = 0

        def __init__(self, name):
            self.cur = name
            self.acts = []

        def fresh(self, h="k"):
            P.n += 1
            if _LOG:
                _record(P.n, self.cur.split(".")[0], h)
            return "%s.%s%d" % (self.cur.split(".")[0], h, P.n)

        def a(self, *acts):
            for x in acts:
                if isinstance(x, list):
                    self.acts += x
                else:
                    self.acts.append(x)
            return self

        def o(self, s):
            return self.a(O(s))

        def goto(self, lab):
            g.on(self.cur, range(257), lab, self.acts, "r")
            self.cur, self.acts = None, []

        def label(self, lab):
            if self.cur is not None:
                self.goto(lab)
            self.cur = lab
            return self

        def call(self, proc, ret=None):
            ret = ret or self.fresh("r")
            g.labels.add(ret)
            self.a(("PUSH", ret))
            self.goto(proc)
            self.cur = ret
            return self

        def ret(self):
            self.goto("RET")

        def branch(self, cases, other, acts=()):
            """cases: {r-value(s): label}; other: label or ('rej', k)"""
            b = self.fresh("b")
            self.a(*acts)
            self.goto(b)
            done = set()
            for ks, lab in cases.items():
                ks = ks if isinstance(ks, tuple) else (ks,)
                g.on(b, ks, lab, [], "r")
                done |= set(ks)
            if isinstance(other, tuple):
                g.on(b, [k for k in range(257) if k not in done], "DEAD", rej(other[1]), "r")
            else:
                g.on(b, [k for k in range(257) if k not in done], other, [], "r")

    return P
