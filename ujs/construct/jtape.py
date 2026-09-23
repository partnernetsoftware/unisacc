"""jtape — target-independent instruction stream. [TP]"""


class Ins:
    __slots__ = ("op", "a", "b", "c")

    def __init__(self, op, a=None, b=None, c=None):
        self.op, self.a, self.b, self.c = op, a, b, c

    def __repr__(self):
        parts = [self.op]
        for x in (self.a, self.b, self.c):
            if x is not None:
                parts.append(repr(x))
        return "Ins(%s)" % ", ".join(parts)


class Fn:
    """Compiled unit. [A-2][A-3]"""
    __slots__ = ("src", "code", "localslot", "strings", "meta")

    def __init__(self, src, code, localslot, strings=None, meta=None):
        self.src = src
        self.code = list(code)
        self.localslot = list(localslot)   # ordered local names
        self.strings = list(strings or [])
        self.meta = dict(meta or {})

    def to_dict(self):
        return {
            "src": self.src,
            "code": [[i.op, i.a, i.b, i.c] for i in self.code],
            "localslot": self.localslot,
            "strings": self.strings,
            "meta": self.meta,
        }

    @classmethod
    def from_dict(cls, d):
        code = [Ins(op, a, b, c) for op, a, b, c in d["code"]]
        return cls(d.get("src", ""), code, d["localslot"],
                   d.get("strings"), d.get("meta"))
