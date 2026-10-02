"""Library bodies on demand: which carried static functions a program needs.

The C library travels as include/*.h source whose functions are `static`
bodies.  Each body is guarded

    #if !__UNISA_FTRIM_LIBC || __UN_<name>
    static ... name(...) { ... }
    #endif

so with __UNISA_FTRIM_LIBC undefined every body is compiled exactly as before.
A preprocessor that defines __UNISA_FTRIM_LIBC also defines __UN_<name>
for every name the translation unit uses, closed over this table: the bodies
and function-like macros each carried name reaches.  The table is derived
from the headers alone, so it cannot disagree with them.
"""
import os


def _headers(incdir):
    """Every carried header, subdirectories included (`sys/stat.h`, 0.0.18)."""
    return sorted(os.path.relpath(os.path.join(d, n), incdir)
                  for d, _, fs in os.walk(incdir) for n in fs if n.endswith(".h"))
import re

PREFIX = "__UN_"
# Names the compiler itself references, not the source: the startup calls
# exit after main returns whenever an exit function exists (src/front_parse.c
# `__main_ret`), so its closure is needed whenever pruning is on.
ROOTS = ("exit",)
MACRO_NAME_LIMIT = 63          # src/front_pp.c keeps NAMEW-1 = 63 bytes of a macro name (C99 5.2.4.1)
IDENT = re.compile(r"[A-Za-z_]\w*")
HEAD = re.compile(r"^static\b[^;{]*?\{", re.M)
KEYWORDS = {"void", "char", "short", "int", "long", "float", "double", "signed", "unsigned",
            "const", "volatile", "static", "inline", "struct", "union", "enum", "_Bool"}
CALLED = re.compile(r"([A-Za-z_]\w*)\s*\(")


def _name(head):
    """The declarator's name: the first non-keyword identifier followed by `(`,
    so `static void (*signal(int, void (*)(int)))(int) {` names signal."""
    for m in CALLED.finditer(head):
        if m.group(1) not in KEYWORDS:
            return m.group(1)
    return None
DEFINE = re.compile(r"^#\s*define\s+([A-Za-z_]\w*)(.*)$", re.M)


def _skip(text, i):
    """Index just past a string/char literal or comment starting at i, else i."""
    c = text[i]
    if c in "\"'":
        j = i + 1
        while j < len(text) and text[j] != c:
            j += 2 if text[j] == "\\" else 1
        return j + 1
    if text.startswith("/*", i):
        return text.index("*/", i + 2) + 2
    if text.startswith("//", i):
        return text.index("\n", i)
    return i


def bodies(text):
    """(name, start, end) of each `static` function body, end exclusive."""
    out = []
    for m in HEAD.finditer(text):
        name = _name(m.group(0))
        if name is None or "(" not in m.group(0):
            continue
        j = m.end() - 1
        depth = 0
        while True:
            k = _skip(text, j)
            if k != j:
                j = k
                continue
            if text[j] == "{":
                depth += 1
            elif text[j] == "}":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        out.append((name, m.start(), j + 1))
    return out


def table(incdir):
    """Sorted names, and for each the sorted closure of carried bodies it needs."""
    refs = {}
    kind = {}
    for h in _headers(incdir):
        text = open(os.path.join(incdir, h)).read()
        for name, a, b in bodies(text):
            assert name not in kind, "carried body defined twice: " + name
            refs[name], kind[name] = set(IDENT.findall(text[a:b])), "body"
        for m in DEFINE.finditer(text):
            name = m.group(1)
            if name.startswith("_UNISA_") or name.startswith("__UNISA_"):
                continue
            if name not in kind:
                refs[name], kind[name] = set(IDENT.findall(m.group(2))), "macro"
    names = sorted(n for n in kind if kind[n] == "body")
    body = set(names)
    closure = {}
    for n in sorted(kind):
        seen, stack = set(), [n]
        while stack:
            x = stack.pop()
            if x in seen:
                continue
            seen.add(x)
            stack.extend(t for t in refs.get(x, ()) if t in kind and t != x)
        need = sorted(seen & body)
        if need:
            closure[n] = need
    for n in names:
        assert len(PREFIX) + len(n) <= MACRO_NAME_LIMIT, "need macro too long: " + n
    assert len(names) <= 1024, "src/front_pp.c ftrim_libc_mark holds 1024 bodies"
    return sorted(closure), closure, names


def roots(incdir):
    """Sorted closure of the compiler's own roots (ROOTS) over the carried bodies."""
    keys, closure, names = table(incdir)
    return sorted({b for r in ROOTS for b in closure.get(r, ())})


def guard(name):
    # An identifier #if does not know is 0 (C99 6.10.1p4): with __UNISA_FTRIM_LIBC
    # undefined the body is always kept; the short form saves header bytes.
    return "#if !__UNISA_FTRIM_LIBC || %s%s" % (PREFIX, name)


def check_guards(incdir):
    """Every carried body sits alone inside its own guard; returns problems."""
    bad = []
    for h in _headers(incdir):
        text = open(os.path.join(incdir, h)).read()
        for name, a, b in bodies(text):
            before = text[:a].rstrip("\n").rsplit("\n", 1)[-1]
            after = text[b:].lstrip(" \t").split("\n", 2)
            nxt = after[1] if after and after[0] == "" and len(after) > 1 else after[0]
            if before != guard(name) or nxt.strip() != "#endif":
                bad.append("%s: %s" % (h, name))
    return bad


def apply_guards(incdir):
    """Wrap every unguarded carried body; idempotent."""
    changed = 0
    for h in _headers(incdir):
        path = os.path.join(incdir, h)
        text = open(path).read()
        out, last = [], 0
        for name, a, b in bodies(text):
            before = text[:a].rstrip("\n").rsplit("\n", 1)[-1]
            if before == guard(name):
                continue
            out.append(text[last:a] + guard(name) + "\n" + text[a:b] + "\n#endif")
            last = b
            changed += 1
        if out:
            open(path, "w").write("".join(out) + text[last:])
    return changed


if __name__ == "__main__":
    import sys
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    inc = os.path.join(root, "include")
    if sys.argv[1:] == ["--apply"]:
        print("guarded", apply_guards(inc), "bodies")
    problems = check_guards(inc)
    keys, closure, names = table(inc)
    print("carried bodies %d, table keys %d, unguarded %d" % (len(names), len(keys), len(problems)))
    for p in problems[:20]:
        print("  unguarded", p)
    sys.exit(1 if problems else 0)
