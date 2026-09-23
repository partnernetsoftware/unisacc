"""UJS lexer. Classic scan; classification via lex table. [W]"""
from ..gold import TOKS

KEYWORDS = {
    "let", "const", "var", "function", "if", "else", "while", "for", "return",
    "break", "continue", "switch", "case", "default",
    "null", "true", "false",
    "typeof", "of", "in",
}
PUNCT = sorted(
    [t for t in TOKS if not t[0].isalpha() and t not in ("eof", "...")],
    key=len, reverse=True,
)
# JS surface + ensure ... is tried first; ===/!== normalize to ==/!=
PUNCT = ["...", "===", "!==", "=>", "??", "+=", "-=", "*=", "/=", "%="] + [
    p for p in PUNCT if p != "..."
]


class Tok:
    __slots__ = ("kind", "text", "val", "line")

    def __init__(self, kind, text, val=None, line=1):
        self.kind, self.text, self.val, self.line = kind, text, val, line

    def __repr__(self):
        return "%s(%r)" % (self.kind, self.text)


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
        return "D"
    if c == '"':
        return "Q"
    if c == "'":
        return "S"
    if c in "{}()[];,:.?":
        return "P"
    if c in "=<>!+-*/%&|.":
        return "O"
    return "other"


def lex(src, oracle):
    i, n, line = 0, len(src), 1
    out = []

    def peek(k=0):
        j = i + k
        return src[j] if j < n else ""

    while True:
        c = peek()
        act = oracle.ask("lex", (charclass(c), charclass(peek(1))))
        if act == "eof":
            out.append(Tok("eof", "", line=line))
            return out
        if act == "ws":
            while charclass(peek()) == "ws":
                i += 1
            continue
        if act == "nl":
            i += 1
            line += 1
            continue
        if act == "id":
            j = i
            while charclass(peek()) in ("A", "D"):
                i += 1
            text = src[j:i]
            if text in KEYWORDS:
                kind = "let" if text in ("const", "var") else text
                val = {"true": True, "false": False, "null": None}.get(text)
                out.append(Tok(kind, text, val, line))
            else:
                out.append(Tok("id", text, text, line))
            continue
        if act == "num":
            j = i
            while peek().isdigit():
                i += 1
            if peek() == "." and peek(1).isdigit():
                i += 1
                while peek().isdigit():
                    i += 1
                out.append(Tok("num", src[j:i], float(src[j:i]), line))
            else:
                out.append(Tok("num", src[j:i], int(src[j:i]), line))
            continue
        if act == "str":
            quote = peek()
            i += 1
            j = i
            buf = []
            while peek() and peek() != quote:
                if peek() == "\\" and peek(1):
                    i += 1
                    esc = peek()
                    buf.append({"n": "\n", "t": "\t", "r": "\r",
                                "\\": "\\", '"': '"', "'": "'"}.get(esc, esc))
                    i += 1
                else:
                    if peek() == "\n":
                        line += 1
                    buf.append(peek())
                    i += 1
            if peek() != quote:
                raise SyntaxError("unterminated string at line %d" % line)
            i += 1
            out.append(Tok("str", "".join(buf), "".join(buf), line))
            continue
        if act in ("punct", "op"):
            # // comment
            if peek() == "/" and peek(1) == "/":
                while peek() and peek() != "\n":
                    i += 1
                continue
            matched = None
            for p in PUNCT:
                if src.startswith(p, i):
                    matched = p
                    break
            if matched is None:
                raise SyntaxError("bad punct %r at %d" % (peek(), i))
            i += len(matched)
            kind = {"===": "==", "!==": "!="}.get(matched, matched)
            out.append(Tok(kind, matched, line=line))
            continue
        raise SyntaxError("lex bad at %d (%r)" % (i, c))
