"""Preprocessor. [W-1]

Classic line walker; the only decision -- what to do with a directive given the
current truth flag -- goes through the pp table. [G-4]
"""
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


def _truth(expr, macros):
    """#if / #elif: only the forms the subset needs -- defined(X), !defined(X),
    a bare macro name, or an integer literal."""
    e = expr.strip()
    neg = False
    while e.startswith("!"):
        neg = not neg
        e = e[1:].strip()
    m = re.match(r"^defined\s*\(?\s*(\w+)\s*\)?$", e)
    if m:
        v = m.group(1) in macros
    elif re.match(r"^\d+$", e):
        v = int(e) != 0
    elif re.match(r"^\w+$", e):
        v = macros.get(e, "0") not in ("0", "")
    else:
        v = False
    return v != neg


def preprocess(src, oracle, macros=None):
    """Returns (text, macros).  Skipped lines become blank so line numbers hold."""
    macros = dict(macros or {})
    out = []
    stack = []            # [(taking, seen_true)]
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


def expand(text, macros):
    """Object-like and function-like substitution, fixed point up to 8 rounds."""
    if not macros:
        return text
    obj = {k: v for k, v in macros.items() if not isinstance(v, tuple)}
    fn = {k: v for k, v in macros.items() if isinstance(v, tuple)}
    for _ in range(8):
        new = text
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
                if args is None or len(args) != len(params):
                    out.append(new[m.start():m.end()])
                    i = m.end()
                    continue
                b = body
                for pn, av in zip(params, args):
                    b = re.sub(r"\b%s\b" % re.escape(pn), "(" + av + ")", b)
                out.append(b)
                i = end
            new = "".join(out)
        if obj:
            pat = re.compile(r"\b(" + "|".join(re.escape(k) for k in obj) + r")\b")
            new = pat.sub(lambda m: obj[m.group(1)], new)
        if new == text:
            break
        text = new
    return text
