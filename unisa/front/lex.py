"""Lexer. [W-2]

Classic scanner; the single decision -- what does this character start, given
the next one -- goes through the lex table. [G-5]
"""
from ..gold import TOKS

KEYWORDS = ("if", "else", "while", "for", "do", "switch", "case", "default",
            "return", "break", "continue", "sizeof", "struct", "typedef",
            "enum", "goto", "union")
TYPEKW = ("int", "char", "long", "short", "void", "unsigned", "signed",
          "float", "double", "const", "static",
          # storage class and qualifiers: declspec skips them, but they have to
          # reach it as `type` tokens or they arrive as identifiers and the
          # declaration is rejected
          "extern", "volatile", "register", "auto", "inline", "restrict")
PUNCT = sorted([t for t in TOKS if not t[0].isalpha() and t != "eof"],
               key=len, reverse=True)
OPCHARS = set("".join(PUNCT))


class Tok:
    __slots__ = ("kind", "text", "val", "line")

    def __init__(self, kind, text, val=None, line=0):
        self.kind, self.text, self.val, self.line = kind, text, val, line

    def __repr__(self):
        return "%s(%s)" % (self.kind, self.text)


def _numval(t):
    """int(x, 0) rejects C's leading-zero octal: `022` is 18, not an error."""
    t = t.rstrip("uUlL")
    if len(t) > 1 and t[0] == "0" and t[1] not in "xX":
        return int(t, 8)
    return int(t, 0)


def charclass(c):
    if c == "":
        return "eof"
    if c == "\n":
        return "nl"
    if c in " \t\r\f\v":
        return "ws"
    if c.isalpha() or c == "_":
        return "A"
    if c.isdigit():
        return "d"
    if c == '"':
        return "q"
    if c == "'":
        return "sq"
    if c == "/":
        return "slash"
    if c == "*":
        return "star"
    if c in OPCHARS:
        return "punct"
    return "other"


ESC = {"n": "\n", "t": "\t", "r": "\r", "0": "\0", "\\": "\\",
       '"': '"', "'": "'"}


def _escape(s, i):
    c = s[i]
    if c != "\\":
        return c, i + 1
    n = s[i + 1]
    if n == "x":
        return chr(int(s[i + 2:i + 4], 16)), i + 4
    return ESC.get(n, n), i + 2


def lex(src, oracle):
    toks = []
    i, n, line = 0, len(src), 1
    while True:
        c = src[i] if i < n else ""
        p = src[i + 1] if i + 1 < n else ""
        act = oracle.ask("lex", (charclass(c), charclass(p)))   # [W-2]

        if act == "skip":
            if c == "":
                break
            i += 1
        elif act == "nl":
            line += 1
            i += 1
        elif act == "linecmt":
            while i < n and src[i] != "\n":
                i += 1
        elif act == "cmt":
            j = src.find("*/", i + 2)
            line += src.count("\n", i, j if j >= 0 else n)
            i = (j + 2) if j >= 0 else n
        elif act == "ident":
            j = i
            while j < n and (src[j].isalnum() or src[j] == "_"):
                j += 1
            w = src[i:j]
            if w in KEYWORDS:
                toks.append(Tok(w, w, None, line))
            elif w in TYPEKW:
                toks.append(Tok("type", w, None, line))
            else:
                toks.append(Tok("id", w, None, line))
            i = j
        elif act == "num":
            j = i
            if src[j:j + 2].lower() in ("0x",):
                j += 2
                while j < n and src[j] in "0123456789abcdefABCDEF":
                    j += 1
            else:
                while j < n and src[j].isdigit():
                    j += 1
            while j < n and src[j] in "uUlL":
                j += 1
            toks.append(Tok("num", src[i:j], _numval(src[i:j]), line))
            i = j
        elif act == "str":
            i += 1
            buf = ""
            while i < n and src[i] != '"':
                ch, i = _escape(src, i)
                buf += ch
            i += 1
            toks.append(Tok("str", buf, buf, line))
        elif act == "charlit":
            i += 1
            ch, i = _escape(src, i)
            i += 1                      # closing quote
            toks.append(Tok("num", repr(ch), ord(ch), line))
        elif act == "op":
            for p3 in PUNCT:
                if src.startswith(p3, i):
                    toks.append(Tok(p3, p3, None, line))
                    i += len(p3)
                    break
            else:
                raise SyntaxError("line %d: stray %r" % (line, c))
        else:                            # bad
            raise SyntaxError("line %d: bad character %r" % (line, c))
    toks.append(Tok("eof", "", None, line))
    # C concatenates adjacent string literals; the scanner emits one token per
    # literal, so fold them here.  [W-12]
    out = []
    for t in toks:
        if t.kind == "str" and out and out[-1].kind == "str":
            out[-1].text = out[-1].text + t.text
            out[-1].val = out[-1].val + t.val
            continue
        out.append(t)
    return out
