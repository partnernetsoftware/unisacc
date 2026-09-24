"""Preprocessor. [W-1]

Classic line walker; the only decision -- what to do with a directive given the
current truth flag -- goes through the pp table. [G-4]
"""
import os
import re

DIRECTIVE = re.compile(r"^\s*#\s*(\w+)\s*(.*)$")

PREDEF = {
    "lnx": ("__linux__", "__unix__", "__ELF__"),
    "osx": ("__APPLE__", "__MACH__", "__unix__"),
    "win": ("_WIN32", "_WIN64"),
    "x86_64": ("__x86_64__", "__LP64__"),
    "arm64": ("__aarch64__", "__LP64__"),
}


def predefines(target):
    os_, arch = target.split("/")
    d = {}
    for m in PREDEF[os_] + PREDEF[arch]:
        d[m] = "1"
    d["__UNISA__"] = "1"
    return d


_PPNUM = re.compile(r"0[xX][0-9a-fA-F]+|\d+")
_PPID = re.compile(r"[A-Za-z_]\w*")


def _pp_tokens(e):
    """Tokenise a preprocessor constant expression."""
    out, i = [], 0
    ops = ("<<=", ">>=", "&&", "||", "==", "!=", "<=", ">=", "<<", ">>")
    while i < len(e):
        c = e[i]
        if c in " \t":
            i += 1
            continue
        if c == "'":                            # a character constant
            j = i + 1
            v = 0
            while j < len(e) and e[j] != "'":
                if e[j] == "\\":
                    j += 1
                    v = {"n": 10, "t": 9, "r": 13, "0": 0}.get(e[j], ord(e[j]))
                else:
                    v = ord(e[j])
                j += 1
            out.append(("num", v))
            i = j + 1
            continue
        m = _PPNUM.match(e, i)
        if m:
            t = m.group(0)
            i = m.end()
            while i < len(e) and e[i] in "uUlL":
                i += 1
            out.append(("num", int(t, 8) if (len(t) > 1 and t[0] == "0"
                                             and t[1] not in "xX")
                        else int(t, 0)))
            continue
        m = _PPID.match(e, i)
        if m:
            out.append(("id", m.group(0)))
            i = m.end()
            continue
        for o in ops:
            if e.startswith(o, i):
                out.append(("op", o))
                i += len(o)
                break
        else:
            out.append(("op", c))
            i += 1
    out.append(("end", None))
    return out


class _PPExpr:
    """C's integer constant expression, the subset `#if` can contain."""
    LEVELS = (("||",), ("&&",), ("|",), ("^",), ("&",), ("==", "!="),
              ("<", ">", "<=", ">="), ("<<", ">>"), ("+", "-"),
              ("*", "/", "%"))

    def __init__(self, toks):
        self.t, self.i = toks, 0

    def peek(self):
        return self.t[self.i]

    def take(self):
        self.i += 1
        return self.t[self.i - 1]

    def expr(self, lvl=0):
        if lvl >= len(self.LEVELS):
            return self.unary()
        v = self.expr(lvl + 1)
        while self.peek()[0] == "op" and self.peek()[1] in self.LEVELS[lvl]:
            o = self.take()[1]
            r = self.expr(lvl + 1)
            v = self.apply(o, v, r)
        return v

    def cond(self):
        v = self.expr()
        if self.peek() == ("op", "?"):
            self.take()
            a = self.cond()
            if self.peek() == ("op", ":"):
                self.take()
            b = self.cond()
            return a if v else b
        return v

    @staticmethod
    def apply(o, a, b):
        if o == "||":
            return 1 if (a or b) else 0
        if o == "&&":
            return 1 if (a and b) else 0
        if o == "|":
            return a | b
        if o == "^":
            return a ^ b
        if o == "&":
            return a & b
        if o == "==":
            return 1 if a == b else 0
        if o == "!=":
            return 1 if a != b else 0
        if o == "<":
            return 1 if a < b else 0
        if o == ">":
            return 1 if a > b else 0
        if o == "<=":
            return 1 if a <= b else 0
        if o == ">=":
            return 1 if a >= b else 0
        if o == "<<":
            return a << (b & 63)
        if o == ">>":
            return a >> (b & 63)
        if o == "+":
            return a + b
        if o == "-":
            return a - b
        if o == "*":
            return a * b
        if b == 0:
            return 0
        if o == "/":
            return int(a / b) if (a < 0) != (b < 0) else a // b
        return a - b * (int(a / b) if (a < 0) != (b < 0) else a // b)

    def unary(self):
        k, v = self.peek()
        if k == "op" and v in ("!", "~", "-", "+"):
            self.take()
            x = self.unary()
            return {"!": lambda y: 1 if not y else 0, "~": lambda y: ~y,
                    "-": lambda y: -y, "+": lambda y: y}[v](x)
        if k == "op" and v == "(":
            self.take()
            x = self.cond()
            if self.peek() == ("op", ")"):
                self.take()
            return x
        if k == "num":
            self.take()
            return v
        if k == "id":
            self.take()
            return 0                 # C99 6.10.1: an unknown name is 0
        self.take()
        return 0


def _truth(expr, macros):
    """#if / #elif over a real integer constant expression: `defined(X)`
    first, then macro expansion, then any name left standing is 0. [W-1]"""
    e = re.sub(r"defined\s*\(\s*(\w+)\s*\)",
               lambda m: "1" if m.group(1) in macros else "0", expr)
    e = re.sub(r"defined\s+(\w+)",
               lambda m: "1" if m.group(1) in macros else "0", e)
    e = expand(e, macros)
    try:
        return _PPExpr(_pp_tokens(e)).cond() != 0
    except Exception:
        return False


def _find_header(name, angled, here, paths):
    cand = []
    if not angled and here:
        cand.append(os.path.join(here, name))
    for p in paths:
        cand.append(os.path.join(p, name))
    for c in cand:
        if os.path.isfile(c):
            return c
    return None


def _splice(src):
    """C99 phase 2: a backslash-newline pair is deleted, joining the lines.
    Blank lines are pushed back in so line numbers survive."""
    if "\\\n" not in src:
        return src
    out, pending = [], 0
    for line in src.split("\n"):
        if line.endswith("\\"):
            out.append(line[:-1])
            pending += 1
            continue
        out.append(line)
        if pending:
            joined = "".join(out[-pending - 1:])
            del out[-pending - 1:]
            out.append(joined)
            out.extend([""] * pending)
            pending = 0
    return "\n".join(out)


def _decomment(src):
    """C99 translation phase 3: every comment becomes one space, and it
    happens BEFORE directives are processed in phase 4.

    Leaving it to the lexer -- which is phase 7 -- meant that
    `#define N 32   // a note` captured the note as part of the replacement
    list, so every later use of N commented out the rest of ITS line.  Real
    code found it: `BYTE hash[SHA256_BLOCK_SIZE] = {...}` lost its `]` and the
    parser ran to end of file.  A block comment is replaced by a space plus
    its own newlines, so nothing below it moves."""
    out, i, n = [], 0, len(src)
    while i < n:
        c = src[i]
        if c == '"' or c == "'":
            # a comment opener inside a literal is not a comment
            j = i + 1
            while j < n and src[j] != c:
                j += 2 if src[j] == "\\" else 1
            out.append(src[i:j + 1])
            i = j + 1
        elif c == "/" and i + 1 < n and src[i + 1] == "/":
            j = src.find("\n", i)
            i = n if j < 0 else j            # the newline itself stays
            out.append(" ")
        elif c == "/" and i + 1 < n and src[i + 1] == "*":
            j = src.find("*/", i + 2)
            body = src[i:n if j < 0 else j + 2]
            out.append(" " + "\n" * body.count("\n"))
            i = n if j < 0 else j + 2
        else:
            out.append(c)
            i += 1
    return "".join(out)


def preprocess(src, oracle, macros=None, path=None, includes=(), _depth=0,
               _seen=None):
    """Returns (text, macros).

    Skipped lines become blank so positions hold.  `#include` splices the named
    file in place, which does shift line numbers -- only diagnostics depend on
    them, and a header that is spliced is more useful than a line number that
    is exact."""
    macros = dict(macros or {})
    _seen = _seen if _seen is not None else set()
    here = os.path.dirname(os.path.abspath(path)) if path else None
    out = []
    stack = []            # [(taking, seen_true)]
    src = _decomment(_splice(src))
    for raw in src.splitlines():
        m = DIRECTIVE.match(raw)
        live = all(t for (t, _) in stack)
        if not m:
            out.append(raw if live else "")
            continue
        d, rest = m.group(1), m.group(2)
        if d not in ("ifdef", "ifndef", "if", "elif", "else", "endif",
                     "define", "include", "undef"):
            out.append("")
            continue

        # the flag the table keys on
        if d in ("ifdef", "ifndef"):
            flag = "1" if rest.split()[0] in macros else "0"
        elif d in ("if", "elif"):
            flag = "1" if _truth(rest, macros) else "0"
        elif d == "else":
            flag = "0" if (stack and stack[-1][1]) else "1"
        else:
            flag = "1"

        act = oracle.ask("pp", (d, flag))      # [W-1]

        if d in ("ifdef", "ifndef", "if"):
            take = (act == "take") and live
            stack.append((take, take))
        elif d == "elif":
            if stack:
                t, seen = stack[-1]
                take = (act == "take") and not seen and \
                    all(x for (x, _) in stack[:-1])
                stack[-1] = (take, seen or take)
        elif d == "else":
            if stack:
                t, seen = stack[-1]
                take = (act == "take") and not seen and \
                    all(x for (x, _) in stack[:-1])
                stack[-1] = (take, seen or take)
        elif act == "pop":
            if stack:
                stack.pop()
        elif act == "macro" and live and d == "include":
            m2 = re.match(r'^\s*([<"])([^>"]+)[>"]', rest)
            if m2 and _depth < 32:
                angled = m2.group(1) == "<"
                found = _find_header(m2.group(2), angled, here, includes)
                if found and found not in _seen:
                    _seen.add(found)
                    with open(found, encoding="latin-1") as f:
                        sub, macros = preprocess(f.read(), oracle, macros,
                                                 found, includes,
                                                 _depth + 1, _seen)
                    out.append(sub)
                    continue
            out.append("")
        elif act == "macro" and live:
            if d == "define":
                m2 = re.match(r"^(\w+)\(([^)]*)\)\s*(.*)$", rest)
                if m2:                                   # function-like
                    params = [x.strip() for x in m2.group(2).split(",")
                              if x.strip()]
                    macros[m2.group(1)] = (params, m2.group(3).strip())
                else:
                    parts = rest.split(None, 1)
                    if parts:
                        macros[parts[0]] = \
                            parts[1].strip() if len(parts) > 1 else "1"
            elif d == "undef" and rest.split():
                macros.pop(rest.split()[0], None)
        out.append("")
    return "\n".join(out), macros


def _subst(body, params, args):
    """Substitute a function-like macro's arguments, honouring `#` and `##`.

    Stringize and paste are the reason a `#` can show up outside a directive,
    which the lexer refuses -- six corpus programs died on `bad character '#'`
    with no other problem."""
    if not params:
        return body
    amap = dict(zip(params, args))
    pat = "|".join(re.escape(p) for p in params)
    body = re.sub(r"(?<!#)#\s*(" + pat + r")\b",
                  lambda m: '"%s"' % amap[m.group(1)]
                  .replace("\\", "\\\\").replace('"', '\\"'), body)
    if "##" in body:
        return _paste(body, amap)
    # ONE pass, all parameters at once.  Substituting them in turn rewrites
    # text that an earlier argument had just put there, and the names in
    # question are usually the same few letters:
    #
    #   #define FF(a,b,c,d,...) { a += F(b,c,d); a = b + ROT(a,s); }
    #   FF(d,a,b,c,...)
    #
    # a->d makes every `a` a `d`, and then d->c turns all of them into `c`.
    # MD5 is written exactly like that, and the rounds that should have
    # written b and d wrote c instead -- a wrong digest, no diagnostic.
    return re.sub(r"\b(" + pat + r")\b", lambda m: amap[m.group(1)], body)



# One preprocessing token, plus whitespace runs so that `a ## b` can find its
# operands.  Literals are already placeholders by this point.
_PTOK = re.compile(r"##|[A-Za-z_]\w*|[0-9][\w.]*|\x00\d+\x01|\s+|.")


def _paste(body, amap):
    """Substitute a macro body containing `##`.

    `##` joins the token immediately before it with the token immediately
    after -- not the two halves of the body.  Splitting on `##` and
    concatenating the halves is what turned `#define Q(A,B) A ## B+` with an
    empty B into `++`: the trailing `+` is an ordinary token that merely
    happens to follow the pasted one, and once B vanishes it ends up glued to
    it.  So we emit token by token and put a space between neighbours, except
    across a paste, where the whole point is that there is none.

    A pasted operand substitutes BARE -- `x ## y` must not become `(a) ## (b)`,
    which pastes to garbage."""
    toks = _PTOK.findall(body)
    pieces, glue = [], []               # glue[i]: join piece i to piece i-1
    i, join_next = 0, False
    while i < len(toks):
        t = toks[i]
        if t == "##":
            join_next = True
            i += 1
            while i < len(toks) and toks[i].isspace():
                i += 1
            continue
        if t.isspace():
            i += 1
            continue
        # `, ## __VA_ARGS__` with no variable arguments: the comma goes too.
        # A GNU extension, but in every logging macro; the C front end does
        # the same, so the two keep compiling the same programs.
        if (t == "__VA_ARGS__" and join_next and amap.get(t, None) == ""
                and pieces and pieces[-1].strip() == ","):
            pieces.pop()
            glue.pop()
            join_next = False
            i += 1
            continue
        pasted = join_next or (i + 1 < len(toks) and
                               _next_is_paste(toks, i + 1))
        v = amap.get(t, t)
        if t in amap and not pasted:
            v = _paren(v)
        pieces.append(v)
        glue.append(join_next)
        join_next = False
        i += 1
    out = []
    for k, v in enumerate(pieces):
        if k and not glue[k]:
            out.append(" ")
        out.append(v)
    return "".join(out)


def _next_is_paste(toks, i):
    """Is the next non-space token a `##`?  Then toks[i-1] is its left operand."""
    while i < len(toks) and toks[i].isspace():
        i += 1
    return i < len(toks) and toks[i] == "##"


# `\x00N\x01` is a string or character literal that _protect() has stashed:
# by the time a macro is expanded the literals are already placeholders, so an
# argument that IS one has to be recognised in that form.
_ATOM = re.compile(r'^\s*(?:[A-Za-z_]\w*|[0-9][\w.]*|\x00\d+\x01)\s*$')


def _paren(av):
    """Wrap a macro argument, unless wrapping would change its meaning.

    A real preprocessor never adds parentheses; we do, because the walker has
    no re-scan.  But `__VA_ARGS__` is an argument LIST -- parenthesising it
    turns N arguments into one comma expression -- and a bare literal needs no
    help, which matters because `printf`'s format has to stay a `str` token."""
    if not av.strip():
        return ""              # an empty argument substitutes to nothing
    if _ATOM.match(av) or _top_comma(av):
        return av
    return "(" + av + ")"


def _top_comma(s):
    d = 0
    for c in s:
        if c in "([":
            d += 1
        elif c in ")]":
            d -= 1
        elif c == "," and d == 0:
            return True
    return False


def _split_args(s, i):
    """s[i] == '(' -> (args, index just past the matching ')')"""
    depth, start, args = 0, i + 1, []
    j = i
    while j < len(s):
        c = s[j]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                args.append(s[start:j])
                return [a.strip() for a in args], j + 1
        elif c == "," and depth == 1:
            args.append(s[start:j])
            start = j + 1
        j += 1
    return None, i


# A macro name inside a string or character constant is not a macro.  With
# `#define NULL 0` in scope, printf("c is NULL\n") printed "c is 0".  [E-32]
# Neither alternative may cross a newline, and a character constant is at
# most four items long: an apostrophe in an English comment (`the key's
# value`) otherwise opens a literal that swallows the rest of the file.
_LIT = re.compile(r'"(?:\\.|[^"\\\n])*"' + r"|'(?:\\.|[^'\\\n]){1,4}'")
_HOLE = re.compile("\x00(\\d+)\x01")


def _protect(text):
    """Stash literals behind placeholders, numbered past any already there.

    Expansion nests -- a macro argument is expanded on its own, and that text
    already carries the outer level's placeholders.  Numbering from 0 again
    would make the inner _restore() hand back the wrong literal, or run off
    the end of its own list."""
    base = max((int(m.group(1)) for m in _HOLE.finditer(text)), default=-1) + 1
    lits = []

    def keep(m):
        lits.append(m.group(0))
        return "\x00%d\x01" % (base + len(lits) - 1)
    return _LIT.sub(keep, text), lits, base


def _restore(text, lits, base=0):
    # re.sub does not rescan what the replacement inserts, so a literal whose
    # own bytes look like a placeholder is safe.  A placeholder outside our
    # own range belongs to an enclosing level and is left alone.
    def back(m):
        k = int(m.group(1)) - base
        return lits[k] if 0 <= k < len(lits) else m.group(0)
    return _HOLE.sub(back, text)



def _raw_operands(body, params):
    """Which parameters appear as an operand of `#` or `##` in this body.

    Those are the only ones that substitute unexpanded (C99 6.10.3.1p1)."""
    toks = _PTOK.findall(body)
    raw, prev = set(), None
    for i, t in enumerate(toks):
        if t.isspace():
            continue
        if t in ("#", "##"):
            j = i + 1
            while j < len(toks) and toks[j].isspace():
                j += 1
            if j < len(toks) and toks[j] in params:
                raw.add(toks[j])
            if t == "##" and prev in params:
                raw.add(prev)
        prev = t
    return raw


def _preexpand(body, params, args, macros, _depth=0):
    """Macro-expand each argument before it is substituted.

    This is what makes the `#define XSTR(x) STR(x)` idiom work: `x` is not an
    operand of `#` in XSTR's body, so it is expanded first, and only the
    result reaches STR's `#`.  Without it the rescan loop reaches STR one
    round later and stringizes the unexpanded text."""
    if _depth > 8:
        return args
    raw = _raw_operands(body, params)
    return [a if p in raw else expand(a, macros)
            for p, a in zip(params, args)]


def expand(text, macros):
    """Object-like and function-like substitution, fixed point up to 8 rounds."""
    if not macros:
        return text
    obj = {k: v for k, v in macros.items() if not isinstance(v, tuple)}
    fn = {k: v for k, v in macros.items() if isinstance(v, tuple)}
    for _ in range(8):
        new, lits, base = _protect(text)
        if fn:
            out, i = [], 0
            while i < len(new):
                m = re.compile(r"\b(" + "|".join(re.escape(k) for k in fn) +
                               r")\s*\(").search(new, i)
                if not m:
                    out.append(new[i:])
                    break
                out.append(new[i:m.start()])
                params, body = fn[m.group(1)]
                args, end = _split_args(new, m.end() - 1)
                if args == [""] and not params:
                    args = []          # `F()` for `#define F() ...`
                if args is not None and params and params[-1] == "...":
                    # C99 variadic macro: the rest becomes __VA_ARGS__
                    fixed = len(params) - 1
                    if len(args) >= fixed:
                        args = args[:fixed] + [", ".join(args[fixed:])]
                        params = params[:fixed] + ["__VA_ARGS__"]
                if args is None or len(args) != len(params):
                    out.append(new[m.start():m.end()])
                    i = m.end()
                    continue
                out.append(_subst(body, params,
                                  _preexpand(body, params, args, macros)))
                i = end
            new = "".join(out)
        # `#x` has just MADE a string literal, and a macro name inside it is
        # not a macro: without this second pass `#define VER 3` turned
        # STR(VER) into "3" instead of "VER".
        new, lits2, base2 = _protect(new)
        if obj:
            pat = re.compile(r"\b(" + "|".join(re.escape(k) for k in obj) + r")\b")
            new = pat.sub(lambda m: obj[m.group(1)], new)
        new = _restore(_restore(new, lits2, base2), lits, base)
        if new == text:
            break
        text = new
    return text
