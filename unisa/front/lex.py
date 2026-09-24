"""Lexer. [W-2]

Classic scanner; the single decision -- what does this character start, given
the next one -- goes through the lex table. [G-5]
"""
from ..gold import TOKS

KEYWORDS = ("if", "else", "while", "for", "do", "switch", "case", "default",
            "return", "break", "continue", "sizeof", "struct", "typedef",
            "enum", "goto", "union")
TYPEKW = ("int", "char", "long", "short", "void", "unsigned", "signed",
          "float", "double", "const", "static", "_Bool",
          # storage class and qualifiers: declspec skips them, but they have to
          # reach it as `type` tokens or they arrive as identifiers and the
          # declaration is rejected
          "extern", "volatile", "register", "auto", "inline", "restrict")
PUNCT = sorted([t for t in TOKS if not t[0].isalpha() and t != "eof"],
               key=len, reverse=True)
OPCHARS = set("".join(PUNCT))


class Tok:
    __slots__ = ("kind", "text", "val", "line", "pos")

    def __init__(self, kind, text, val=None, line=0, pos=-1):
        self.kind, self.text, self.val, self.line = kind, text, val, line
        self.pos = pos               # where in the source, for folding [W-12]

    def __repr__(self):
        return "%s(%s)" % (self.kind, self.text)


def _numval(t):
    """int(x, 0) rejects C's leading-zero octal: `022` is 18, not an error."""
    t = t.rstrip("uUlL")
    if len(t) > 1 and t[0] == "0" and t[1] not in "xX":
        return int(t, 8)
    return int(t, 0)


class FNum(float):
    """A floating constant's value.  `bits` is what the tape carries: the
    binary64 pattern, or for an `f` suffix the binary32 one (rounded once
    from the decimal, not via a double)."""
    def __new__(cls, text):
        from ..fp import bd, dec_to_f32
        body = text.rstrip("fFlL")
        # a hex body keeps its trailing hex digits: only strip a suffix that
        # follows the binary exponent
        if body[:2].lower() == "0x":
            body = text[:-1] if text[-1] in "fFlL" and "p" in text.lower() else text
            v = float.fromhex(body)     # exact: that is what hex floats are for
        else:
            v = float(body)
        self = float.__new__(cls, v)
        self.f32 = text[-1] in "fF" and not (text[:2].lower() == "0x" and "p" not in text.lower())
        if self.f32:
            import struct
            self.bits = (struct.unpack("<I", struct.pack("<f", v))[0]
                         if body[:2].lower() == "0x" else dec_to_f32(body))
        else:
            self.bits = bd(v)
        return self


def charclass(c):
    if c == "":
        return "eof"
    if c == ".":
        return "dot"                 # `.5` may start a number [G-5]
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
        j = i + 2
        while j < len(s) and s[j] in "0123456789abcdefABCDEF" and j < i + 4:
            j += 1
        return chr(int(s[i + 2:j], 16) & 0xFF), j
    if n in "01234567":                      # octal: one to three digits
        j = i + 1
        while j < len(s) and s[j] in "01234567" and j < i + 4:
            j += 1
        return chr(int(s[i + 1:j], 8) & 0xFF), j
    return ESC.get(n, n), i + 2


def _utf8_cps(b):
    """Raw source bytes -> code points.  A byte that is not part of a valid
    UTF-8 sequence keeps its own value, which is what a compiler reading a
    latin-1 source would do."""
    if not b:
        return []
    try:
        return [ord(c) for c in bytes(b).decode("utf-8")]
    except UnicodeDecodeError:
        return list(b)


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
            HEX = "0123456789abcdefABCDEF"
            while j < n:
                if src[j].isalnum() or src[j] == "_":
                    j += 1
                    continue
                # a universal character name, C99 6.4.3 -- `caf\u00e9` is
                # one identifier, kept by its spelling, as the C lexer does
                if src[j] == "\\" and j + 1 < n and src[j + 1] in "uU":
                    w = 4 if src[j + 1] == "u" else 8
                    if all(c in HEX for c in src[j + 2:j + 2 + w]) and j + 2 + w <= n:
                        j += 2 + w
                        continue
                break
            w = src[i:j]
            # A wide CHARACTER constant is just an int, so the prefix is
            # dropped.  A wide STRING is not: its elements are wider than a
            # byte, and pretending otherwise would miscompile silently.
            # GCC spellings that carry no meaning for this subset.  They are
            # everywhere in real headers, so they are dropped in the lexer
            # rather than threaded through the grammar.
            if w == "__attribute__" or w == "__asm__" or w == "asm":
                k = j
                while k < n and src[k] in " \t\n\r":
                    k += 1
                if k < n and src[k] == "(":
                    depth = 0
                    while k < n:
                        ch = src[k]
                        if ch in "\"'":           # a `)` inside a literal is
                            q, k = ch, k + 1      # not a paren
                            while k < n and src[k] != q:
                                k += 2 if src[k] == "\\" else 1
                            k += 1
                            continue
                        if ch == "(":
                            depth += 1
                        elif ch == ")":
                            depth -= 1
                            if depth == 0:
                                k += 1
                                break
                        elif ch == "\n":
                            line += 1
                        k += 1
                    i = k
                    continue
            if w in ("__extension__", "__inline", "__inline__", "__restrict",
                     "__restrict__", "__const", "__volatile__", "__signed__"):
                i = j
                continue
            if w in ("L", "u", "U", "u8") and j < n and src[j] == "'":
                i = j
                continue
            if w in ("L", "u", "U", "u8") and j < n and src[j] == '"':
                # A wide string's elements are wider than a byte, so it gets
                # its own token kind and carries CODE POINTS, not bytes.  The
                # source was read as latin-1 (one char per byte), so the
                # literal text is still UTF-8 and has to be decoded here --
                # this is the only place that knows it is a literal at all.
                start, k = i, j + 1
                cps, raw = [], bytearray()
                while k < n and src[k] != '"':
                    if src[k] == "\\":
                        cps.extend(_utf8_cps(raw)); raw = bytearray()
                        ch, k = _escape(src, k)
                        cps.append(ord(ch))
                    elif ord(src[k]) > 255:
                        # already decoded: the caller handed us text, not the
                        # latin-1 bytes `compile_file` reads
                        cps.extend(_utf8_cps(raw)); raw = bytearray()
                        cps.append(ord(src[k]))
                        k += 1
                    else:
                        raw.append(ord(src[k]))
                        k += 1
                cps.extend(_utf8_cps(raw))
                k += 1                        # the closing quote
                # The KIND is still `str`: grammatically a wide string is a
                # string, and the parse table's token axis should not grow a
                # class the grammar cannot tell apart.  What distinguishes it
                # is the value -- code points instead of bytes.
                toks.append(Tok("str", src[start:k], cps, line, start))
                i = k
                continue
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
            # A floating constant (C99 6.4.4.2): a fraction, an exponent, or
            # both, then at most one of f F l L.  Scanning the extent is
            # structure; that it IS a number was the table's answer.
            isf = False
            # the hex floating form, 0x HEX [. HEX] p [+-] DEC -- the same
            # extent the C lexer takes, so the two token streams stay equal
            if src[i:i + 2].lower() == "0x":
                k = j
                if k < n and src[k] == ".":
                    k += 1
                    while k < n and src[k] in "0123456789abcdefABCDEF":
                        k += 1
                if k < n and src[k] in "pP":
                    k2 = k + 1
                    if k2 < n and src[k2] in "+-":
                        k2 += 1
                    if k2 < n and src[k2].isdigit():
                        isf = True
                        j = k2
                        while j < n and src[j].isdigit():
                            j += 1
            if src[i:i + 2].lower() != "0x":
                if j < n and src[j] == ".":
                    isf = True
                    j += 1
                    while j < n and src[j].isdigit():
                        j += 1
                if j < n and src[j] in "eE":
                    k = j + 1
                    if k < n and src[k] in "+-":
                        k += 1
                    if k < n and src[k].isdigit():
                        isf = True
                        j = k
                        while j < n and src[j].isdigit():
                            j += 1
            if isf:
                if j < n and src[j] in "fFlL":
                    j += 1
                toks.append(Tok("num", src[i:j], FNum(src[i:j]), line))
                i = j
                continue
            while j < n and src[j] in "uUlL":
                j += 1
            toks.append(Tok("num", src[i:j], _numval(src[i:j]), line))
            i = j
        elif act == "str":
            start = i
            i += 1
            buf = ""
            while i < n and src[i] != '"':
                ch, i = _escape(src, i)
                buf += ch
            i += 1
            # `text` is the SOURCE spelling and `val` the decoded bytes; the
            # parser uses val, and lexdiff compares spellings against the
            # self-hosted lexer, which has only the slice
            toks.append(Tok("str", src[start:i], buf, line, start))
        elif act == "charlit":
            start = i
            i += 1
            ch, i = _escape(src, i)
            i += 1                      # closing quote
            # `text` is the SOURCE spelling, like every other token: the
            # self-hosted lexer prints the same slice, and lexdiff compares
            # the two token streams by text
            toks.append(Tok("num", src[start:i], ord(ch), line))
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
            a, b = out[-1].val, t.val
            if isinstance(a, list) != isinstance(b, list):
                # C99 6.4.5p4: one wide operand makes the result wide
                a = a if isinstance(a, list) else [ord(c) for c in a]
                b = b if isinstance(b, list) else [ord(c) for c in b]
            # the SPELLING is the whole source span, whitespace and all: the
            # self-hosted lexer can only record (position, length), and
            # lexdiff compares the two streams by text
            out[-1].text = src[out[-1].pos:t.pos + len(t.text)]
            out[-1].val = a + b
            continue
        out.append(t)
    return out
