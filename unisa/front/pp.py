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
_ESC = {"n": 10, "t": 9, "r": 13, "a": 7, "b": 8, "f": 12, "v": 11}
_M64 = (1 << 64) - 1


def _pp_char(e, i):
    """e[i] is the opening quote of a character constant -> (value, end)."""
    j, v = i + 1, 0
    while j < len(e) and e[j] != "'":
        c = e[j]
        j += 1
        if c == "\\" and j < len(e):
            c = e[j]
            j += 1
            if c in _ESC:
                c = _ESC[c]
            elif c == "x":
                k = j
                while j < len(e) and e[j] in "0123456789abcdefABCDEF":
                    j += 1
                c = int(e[k:j] or "0", 16)
            elif c in "01234567":
                k = j - 1
                while j < len(e) and j - k < 3 and e[j] in "01234567":
                    j += 1
                c = int(e[k:j], 8)
            else:
                c = ord(c)
        else:
            c = ord(c)
        v = ((v << 8) | (c & 255)) & _M64
    return v, j + 1


def _pp_tokens(e):
    """Tokenise a preprocessor constant expression.  A number is
    ("num", (value, unsigned))."""
    out, i = [], 0
    ops = ("<<=", ">>=", "&&", "||", "==", "!=", "<=", ">=", "<<", ">>")
    while i < len(e):
        c = e[i]
        if c in " \t\n":
            i += 1
            continue
        if c == "'" or (c == "L" and e[i + 1:i + 2] == "'"):
            v, i = _pp_char(e, i + (c == "L"))
            out.append(("num", (v, False)))
            continue
        m = _PPNUM.match(e, i)
        if m:
            t = m.group(0)
            i = m.end()
            uns = False
            while i < len(e) and e[i] in "uUlL":
                uns = uns or e[i] in "uU"
                i += 1
            if len(t) > 1 and t[0] == "0" and t[1] not in "xX":
                v = int(re.match(r"[0-7]*", t).group(0) or "0", 8)
            else:
                v = int(t, 0)
            v &= _M64
            out.append(("num", (v, uns or v >> 63 != 0)))
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


def _s64(v):
    v &= _M64
    return v - (1 << 64) if v >> 63 else v


class _PPExpr:
    """C's integer constant expression in intmax_t / uintmax_t (C99 6.10.1).
    A value is (bits, unsigned) with bits in [0, 2**64).  Operands that are
    not evaluated (the right of a decided && or ||, the untaken arm of ?:)
    are parsed with `skip` raised, so a division by zero there is fine."""
    LEVELS = (("|",), ("^",), ("&",), ("==", "!="),
              ("<", ">", "<=", ">="), ("<<", ">>"), ("+", "-"),
              ("*", "/", "%"))

    def __init__(self, toks, macros=None):
        self.t, self.i, self.skip, self.div0 = toks, 0, 0, False
        self.macros = macros or {}

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

    def land(self):
        v = self.expr()
        while self.peek() == ("op", "&&"):
            self.take()
            self.skip += v[0] == 0
            r = self.expr()
            self.skip -= v[0] == 0
            v = (1 if (v[0] and r[0]) else 0, False)
        return v

    def lor(self):
        v = self.land()
        while self.peek() == ("op", "||"):
            self.take()
            self.skip += v[0] != 0
            r = self.land()
            self.skip -= v[0] != 0
            v = (1 if (v[0] or r[0]) else 0, False)
        return v

    def cond(self):
        v = self.lor()
        if self.peek() == ("op", "?"):
            self.take()
            self.skip += v[0] == 0
            a = self.cond()
            self.skip -= v[0] == 0
            if self.peek() == ("op", ":"):
                self.take()
            self.skip += v[0] != 0
            b = self.cond()
            self.skip -= v[0] != 0
            u = a[1] or b[1]
            return ((a if v[0] else b)[0], u)
        return v

    def apply(self, o, a, b):
        (x, ux), (y, uy) = a, b
        if o in ("<<", ">>"):
            n = y & 63
            if o == "<<":
                return ((x << n) & _M64, ux)
            return ((x >> n) if ux else (_s64(x) >> n) & _M64, ux)
        u = ux or uy
        sx, sy = (x, y) if u else (_s64(x), _s64(y))
        if o == "==":
            return (1 if x == y else 0, False)
        if o == "!=":
            return (1 if x != y else 0, False)
        if o in ("<", ">", "<=", ">="):
            r = {"<": sx < sy, ">": sx > sy, "<=": sx <= sy, ">=": sx >= sy}[o]
            return (1 if r else 0, False)
        if o == "|":
            return (x | y, u)
        if o == "^":
            return (x ^ y, u)
        if o == "&":
            return (x & y, u)
        if o == "+":
            return ((x + y) & _M64, u)
        if o == "-":
            return ((x - y) & _M64, u)
        if o == "*":
            return ((x * y) & _M64, u)
        if y == 0:
            if not self.skip:
                self.div0 = True
            return (0, u)
        q = sx // sy if u or (sx < 0) == (sy < 0) else -(abs(sx) // abs(sy))
        if o == "/":
            return (q & _M64, u)
        return ((sx - sy * q) & _M64, u)

    def unary(self):
        k, v = self.peek()
        if k == "op" and v in ("!", "~", "-", "+"):
            self.take()
            x, u = self.unary()
            if v == "!":
                return (1 if x == 0 else 0, False)
            if v == "~":
                return (~x & _M64, u)
            if v == "-":
                return (-x & _M64, u)
            return (x, u)
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
            if v == "defined":            # produced by a macro body
                p = self.peek() == ("op", "(")
                if p:
                    self.take()
                n = self.take()
                if p and self.peek() == ("op", ")"):
                    self.take()
                return (1 if n[0] == "id" and n[1] in self.macros else 0, False)
            return (0, False)        # C99 6.10.1: an unknown name is 0
        self.take()
        return (0, False)


def _truth(expr, macros, live=True):
    """#if / #elif over a real integer constant expression: `defined(X)`
    first, then macro expansion, then any name left standing is 0. [W-1]"""
    e = re.sub(r"\bdefined\s*\(\s*(\w+)\s*\)",
               lambda m: "1" if m.group(1) in macros else "0", expr)
    e = re.sub(r"\bdefined\s+(\w+)",
               lambda m: "1" if m.group(1) in macros else "0", e)
    e = expand(e, macros)
    try:
        x = _PPExpr(_pp_tokens(e), macros)
        r = x.cond()[0] != 0
    except Exception:
        return False
    if x.div0 and live:
        raise ValueError("division by zero in #if")
    return r


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


# the origin map of the most recent preprocess() call: (file, line) per
# output line, see there
LAST_ORIGIN = [None]

# Positional macros.  C defines a macro FROM its #define TO its #undef, and a
# redefinition in between replaces it from that point on -- `#define V 1`,
# `a = V`, `#undef V`, `#define V 2`, `b = V` gives a = 1, b = 2.  This
# preprocessor used to process every directive first and expand the whole
# text with the FINAL table afterwards, so both were 2.  Now, whenever the
# table changes, the next line of code is prefixed with MARK N MARK, and N
# indexes SNAPS, the table as it stood there; expand_positional() expands
# each stretch with its own table and drops the markers.  A marker sits at
# the start of a line, so no line and no column moves.
SNAPS = []
MARK = "\x02"
_MARK_RE = re.compile(MARK + r"(\d+)" + MARK)
# #pragma push_macro / pop_macro: a stack of saved definitions per name
_PUSHED = {}


def preprocess(src, oracle, macros=None, path=None, includes=(), _depth=0,
               _seen=None):
    """Returns (text, macros).

    Skipped lines become blank so positions hold.  `#include` splices the named
    file in place, which does shift line numbers -- only diagnostics depend on
    them, and a header that is spliced is more useful than a line number that
    is exact."""
    macros = dict(macros or {})
    if _depth == 0:
        SNAPS[:] = [dict(macros)]          # the table the text starts with
        _PUSHED.clear()
    pending = [False]                      # the table changed; mark the next code
    _seen = _seen if _seen is not None else set()
    here = os.path.dirname(os.path.abspath(path)) if path else None
    out = []
    stack = []            # [(taking, seen_true)]
    # where[k] lists the (file, line) of every output line element out[k]
    # produces.  An #include splices a whole header into ONE element, so
    # without this every line after it was reported hundreds of lines off:
    # the Python front end said `line 553` for line 4 of a six-line file.
    where = []
    src = _decomment(_splice(src))
    for lineno, raw in enumerate(src.splitlines(), 1):
        # every element appended below came from this source line, unless
        # it is a spliced header, which brings its own origins
        while len(where) < len(out):
            where.append([(path, lineno - 1)])
        m = DIRECTIVE.match(raw)
        live = all(t for (t, _) in stack)
        if not m:
            if live and pending[0] and raw.strip():
                SNAPS.append(dict(macros))
                raw = MARK + str(len(SNAPS) - 1) + MARK + raw
                pending[0] = False
            out.append(raw if live else "")
            continue
        d, rest = m.group(1), m.group(2)
        if d == "pragma" and live:
            # #pragma push_macro("X") / pop_macro("X"): save X's definition
            # (or its absence) and restore it later
            pm = re.match(r'^\s*(push_macro|pop_macro)\s*\(\s*"(\w+)"\s*\)', rest)
            if pm:
                name = pm.group(2)
                if pm.group(1) == "push_macro":
                    _PUSHED.setdefault(name, []).append(macros.get(name))
                elif _PUSHED.get(name):
                    saved = _PUSHED[name].pop()
                    if saved is None:
                        macros.pop(name, None)
                    else:
                        macros[name] = saved
                    pending[0] = True
        if d not in ("ifdef", "ifndef", "if", "elif", "else", "endif",
                     "define", "include", "undef"):
            out.append("")
            continue

        # the flag the table keys on
        if d in ("ifdef", "ifndef"):
            flag = "1" if rest.split()[0] in macros else "0"
        elif d in ("if", "elif"):
            ol = live if d == "if" else (
                bool(stack) and not stack[-1][1] and
                all(x for (x, _) in stack[:-1]))
            flag = "1" if _truth(rest, macros, ol) else "0"
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
                if not found:
                    raise ValueError("%s: no such file for #include: %s"
                                     % (path or "<input>", m2.group(2)))
                if found and found not in _seen:
                    _seen.add(found)
                    # the header's code is expanded with the table as it
                    # stands HERE, so a waiting marker goes in first ...
                    if pending[0]:
                        SNAPS.append(dict(macros))
                        lead = MARK + str(len(SNAPS) - 1) + MARK
                        pending[0] = False
                    else:
                        lead = ""
                    with open(found, encoding="latin-1") as f:
                        sub, macros = preprocess(f.read(), oracle, macros,
                                                 found, includes,
                                                 _depth + 1, _seen)
                    out.append(lead + sub)
                    # ... and whatever it defined applies from here on
                    pending[0] = True
                    where.append(LAST_ORIGIN[0] or
                                 [(found, 0)] * (sub.count("\n") + 1))
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
                            parts[1].strip() if len(parts) > 1 else ""
            elif d == "undef" and rest.split():
                macros.pop(rest.split()[0], None)
            pending[0] = True
        out.append("")
    while len(where) < len(out):
        where.append([(path, len(where) + 1)])
    LAST_ORIGIN[0] = [o for w in where for o in w]
    return "\n".join(out), macros


# ---- macro expansion: tokens with hide sets (C99 6.10.3) ------------------
#
# Expansion works on preprocessing tokens, each carrying a HIDE SET: the
# names of the macros whose expansion produced it.  A name in its own hide
# set is never replaced again (6.10.3.4p2, "painted blue"), so
# `#define foo foo + 1` gives `foo + 1` and stops -- the text engine this
# replaces rescanned in rounds and gave `foo + 1 + 1 ... + 1`, eight times.
# An object-like expansion's tokens get HS(name) + {name}; a function-like
# one's get (HS(name) & HS(`)`)) + {name}, which is what makes
# `#define f(a) a*g` / `#define g(a) f(a)` / `f(2)(9)` give `2*9*g`.
# Arguments are fully expanded on their own before substitution, except as
# operands of # and ##, and the result is rescanned together with the rest
# of the source.  Text outside a macro invocation is copied through
# untouched, so lines and columns hold; an expansion is written with one
# space between its tokens, so no two can re-lex as one.

_PPTOK = re.compile(
    r'(?P<ws>[ \t\r\f\v]+)|(?P<nl>\n)'
    r'|(?P<str>(?:u8|[LuU])?"(?:\\.|[^"\\\n])*"?)'
    r"|(?P<chr>[LuU]?'(?:\\.|[^'\\\n])*'?)"
    r'|(?P<num>\.?[0-9](?:[eEpP][+-]|[A-Za-z0-9_.])*)'
    r'|(?P<id>[A-Za-z_](?:[A-Za-z0-9_]|\\u[0-9a-fA-F]{4}|\\U[0-9a-fA-F]{8})*)'
    r'|(?P<p>\.\.\.|<<=|>>=|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||'
    r'[*/%+\-&^|]=|##|.)', re.S)
_EMPTY = frozenset()
_IDSTART = re.compile(r"[A-Za-z_]")
_LITERAL = re.compile(r"(?:u8|[LuU])?[\"']")


def _pieces(text):
    """The text as a list of token spellings, whitespace runs and newlines
    included, so that joining them gives the text back."""
    return [m.group(0) for m in _PPTOK.finditer(text)]


def _body_tokens(body):
    """A replacement list as [(spelling, preceded-by-whitespace)]."""
    out, ws = [], False
    for s in _pieces(body):
        if s.isspace():
            ws = True
        else:
            out.append((s, ws))
            ws = False
    return out


def _stringize(arg):
    """`#param`: the argument's spelling, one space where it had any."""
    parts = []
    for k, (s, _, ws) in enumerate(arg):
        if k and ws:
            parts.append(" ")
        if _LITERAL.match(s):
            s = s.replace("\\", "\\\\").replace('"', '\\"')
        parts.append(s)
    return '"' + "".join(parts) + '"'


class _Expander:
    """One expansion.  `stack` holds tokens waiting to be rescanned (the top
    is next); below it, if `src` is set, is the rest of the source."""

    def __init__(self, macros, src=None, pos=0):
        self.m, self.stack, self.src, self.pos, self.nl = macros, [], src, pos, 0

    def next(self):
        if self.stack:
            return self.stack.pop()
        if self.src is None:
            return None
        ws = False
        while self.pos < len(self.src):
            s = self.src[self.pos]
            self.pos += 1
            if s == "\n":
                self.nl += 1
                ws = True
            elif s.isspace():
                ws = True
            else:
                return (s, _EMPTY, ws)
        return None

    def next_is_paren(self):
        if self.stack:
            return self.stack[-1][0] == "("
        if self.src is None:
            return False
        p = self.pos
        while p < len(self.src) and self.src[p].isspace():
            p += 1
        return p < len(self.src) and self.src[p] == "("

    def run(self, out, until_empty):
        while not (until_empty and not self.stack):
            t = self.next()
            if t is None:
                return
            self.step(t, out)

    def step(self, t, out):
        name, hs, ws = t
        if name == "_Pragma" and self.next_is_paren():
            # C99 6.10.9: the operator form of #pragma; no pragma is
            # honoured, so it goes, whole
            args = self.call()
            if args is not None:
                return
        d = self.m.get(name) if _IDSTART.match(name) else None
        if d is None or name in hs:
            out.append(t)
            return
        if not isinstance(d, tuple):
            self.push(self.subst(_body_tokens(d), None, None, hs | {name}, ws))
            return
        params, body = d
        if not self.next_is_paren():
            out.append(t)
            return
        got = self.call(params)
        if got is None:
            out.append(t)
            return
        args, rp = got
        self.push(self.subst(_body_tokens(body), params, args,
                             (hs & rp[1]) | {name}, ws))

    def call(self, params=None):
        """At the `(` of a call: its arguments and its `)`, or None -- and
        then nothing is consumed."""
        pos, nl, saved = self.pos, self.nl, list(self.stack)
        variadic = bool(params) and params[-1] == "..."
        n = len(params) if params is not None else 0
        self.next()                                  # the `(`
        args, depth = [[]], 0
        while True:
            t = self.next()
            if t is None:
                break
            s = t[0]
            if s == ")" and depth == 0:
                if params is None:
                    return args, t
                if not params and args == [[]]:
                    args = []
                if variadic and len(args) == n - 1:
                    args.append([])
                if len(args) == n:
                    return args, t
                break
            if s == "(":
                depth += 1
            elif s == ")":
                depth -= 1
            elif s == "," and depth == 0 and not (variadic and len(args) >= n):
                args.append([])
                continue
            args[-1].append(t)
        # not a call after all: put back what was taken
        self.pos, self.nl, self.stack = pos, nl, saved
        return None

    def push(self, toks):
        self.stack.extend(reversed(toks))

    def expand_arg(self, arg):
        sub = _Expander(self.m)
        sub.push(arg)
        out = []
        sub.run(out, False)
        return out

    def subst(self, body, params, args, hs, ws0):
        pidx = {}
        if params is not None:
            for k, p in enumerate(params):
                pidx["__VA_ARGS__" if p == "..." else p] = k
        va = pidx.get("__VA_ARGS__", -1) if params and params[-1] == "..." else -1
        done = {}
        R, paste, lastempty, i = [], False, False, 0
        while i < len(body):
            s, w = body[i]
            if s == "##" and 0 < i < len(body) - 1:
                paste = True
                i += 1
                continue
            if s == "#" and params is not None and i + 1 < len(body) \
                    and body[i + 1][0] in pidx:
                L = [(_stringize(args[pidx[body[i + 1][0]]]), _EMPTY, w)]
                i += 2
            elif s in pidx:
                k = pidx[s]
                raw = paste or (i + 1 < len(body) and body[i + 1][0] == "##")
                if raw:
                    L = list(args[k])
                else:
                    if k not in done:
                        done[k] = self.expand_arg(args[k])
                    L = list(done[k])
                if L:
                    L[0] = (L[0][0], L[0][1], w)
                i += 1
            else:
                L = [(s, _EMPTY, w)]
                i += 1
            if paste:
                paste = False
                if s in pidx and pidx[s] == va and R and R[-1][0] == "," \
                        and not lastempty:
                    # `, ## __VA_ARGS__`: with no variable arguments the
                    # comma goes too (GNU, in every logging macro); with
                    # some, nothing is pasted
                    if not args[va]:
                        R.pop()
                    else:
                        R.extend(L)
                    lastempty = False
                    continue
                if lastempty or not R:
                    R.extend(L)
                    lastempty = not L
                elif L:
                    lhs = R.pop()
                    glued = _body_tokens(lhs[0] + L[0][0])
                    R.extend((g, _EMPTY, lhs[2] if j == 0 else gw)
                             for j, (g, gw) in enumerate(glued))
                    R.extend(L[1:])
                continue
            R.extend(L)
            lastempty = not L
        out = [(s, h | hs, w) for (s, h, w) in R]
        if out:
            out[0] = (out[0][0], out[0][1], ws0)
        return out


def expand(text, macros):
    """Macro-expand `text` with the table `macros`."""
    if not macros:
        return text
    src = _pieces(text)
    out, i, n = [], 0, len(src)
    while i < n:
        s = src[i]
        d = macros.get(s) if s[:1].isalpha() or s[:1] == "_" else None
        if s == "_Pragma" or (d is not None and (not isinstance(d, tuple) or
                                                 _paren_follows(src, i + 1))):
            x = _Expander(macros, src, i)
            toks = []
            t = x.next()
            x.step(t, toks)
            x.run(toks, True)
            if x.pos == i + 1 and len(toks) == 1 and toks[0][0] == s:
                out.append(s)
                i += 1
                continue
            out.append(" " + " ".join(t[0] for t in toks) + " ")
            out.append("\n" * x.nl)
            i = x.pos
            continue
        out.append(s)
        i += 1
    return "".join(out)


def _paren_follows(src, i):
    while i < len(src) and src[i].isspace():
        i += 1
    return i < len(src) and src[i] == "("


def expand_positional(text, macros):
    """Expand each stretch of text with the macro table in force there (see
    SNAPS).  Text with no markers is one stretch, expanded with `macros`."""
    parts = _MARK_RE.split(text)
    if len(parts) == 1:
        return expand(text, macros)
    out = [expand(parts[0], SNAPS[0] if SNAPS else macros)]
    for i in range(1, len(parts), 2):
        out.append(expand(parts[i + 1], SNAPS[int(parts[i])]))
    return "".join(out)
