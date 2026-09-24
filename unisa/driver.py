"""src -> tape. [W]"""
import os
import re

from .front.pp import preprocess, predefines, expand, expand_positional
from .front.lex import lex
from .front.parse import compile_units, CError, CErrors


def compile_c(src, oracle, target="lnx/x86_64", path=None, includes=()):
    return compile_sources([src], oracle, target, [path], includes)


def compile_sources(srcs, oracle, target="lnx/x86_64", paths=None,
                    includes=()):
    """Front end, with one retry per missing library function.

    There is no linker, so a program that calls `strlen` without including
    <string.h> -- which is most C ever written -- used to be refused.  We
    carry those functions as ordinary C in `include/`, so when one turns out
    to be missing we append the header that defines it and compile again.
    Calls are resolved at the end of the unit, so appending is enough and the
    user's line numbers do not move."""
    srcs = list(srcs)
    paths = list(paths or [None] * len(srcs))
    tried = set()
    while True:
        try:
            return _compile_once(srcs, oracle, target, paths, includes)
        except CError as e:
            m = re.search(r"undefined function '(\w+)'", str(e))
            hdr = _libc_index().get(m.group(1)) if m else None
            if hdr is None or hdr in tried:
                raise
            tried.add(hdr)
            # EVERY unit gets it, not just the one that asked -- and the error
            # does not say which one asked.  Our headers define their
            # functions `static`, so a unit that did not get the definition
            # would emit a call to a name that only another unit's renamed
            # copy answers to.  The include guard makes the extra appends
            # free for a unit that already had it.
            srcs = [c + "\n#include <%s>\n" % hdr for c in srcs]


def _compile_once(srcs, oracle, target, paths, includes):
    # Preprocessing is PER FILE (include guards, __FILE__, #define state);
    # only the walk is shared.
    streams, seen = [], []
    for src, path in zip(srcs, paths):
        text, macros = preprocess(src, oracle, predefines(target), path,
                                  tuple(includes) + (DEFAULT_INCLUDE,))
        from .front import pp as _pp
        origin = _pp.LAST_ORIGIN[0]
        ex = expand_positional(text, macros)
        seen.append((ex, origin))
        streams.append(lex(ex, oracle))
    try:
        t = compile_units(streams, oracle)
    except CErrors as e:
        n = len(e.errors)
        d = CDiag("\n".join(str(_located(x, seen)) for x in e.errors)
                  + "\n%d error%s generated." % (n, "" if n == 1 else "s"))
        raise d from None
    except CError as e:
        raise _located(e, seen) from None
    # Which OS the SOURCE was compiled for.  Not the same question as which
    # OS a lowering targets: `#ifdef _WIN32` is decided here, once, and an
    # interpreter has to read the tape's syscall arguments in that light.
    t.src_os = target.split("/")[0]
    return t


class CDiag(CError):
    """A CError that already reads `file:line:col: error: ...`, the shape the
    C front end prints [S-12] -- so it is printed as it stands."""


def _located(e, seen):
    """[S-12] the user's file, line and column, then the line, then a caret.

    The walker's line numbers count lines of the buffer it was handed, in
    which every #include has been spliced -- so they are the origin table's
    INDEX, not a line anyone can find.  The column is measured in the line
    the parser saw, which is the user's except where a macro expanded; the
    printed line is that same line, so the caret still points at the token."""
    tok = getattr(e, "tok", None)
    unit = getattr(e, "unit", 0)
    if tok is None or unit >= len(seen):
        return e
    text, origin = seen[unit]
    msg = str(e)
    # The message's own "line N" is the OFFENDING token's line; the walker
    # has usually moved past it by the time the error escapes.  Likewise
    # the token the message quotes is the one to point at.
    m = re.match(r"line (\d+): ", msg)
    bline = int(m.group(1)) if m else tok.line
    # "expected ';', got 'return'" is about the token it GOT; otherwise the
    # first quoted token is the one the message is about
    q = (re.search(r"got '([^']+)'", msg) or re.search(r'got "([^"]+)"', msg)
         or re.search(r"'([^']+)'", msg) or re.search(r'"([^"]+)"', msg))
    name = q.group(1) if q else None
    if not origin or bline < 1 or bline > len(origin):
        return e
    path, line = origin[bline - 1]
    lines = text.split("\n")
    src_line = lines[bline - 1] if bline - 1 < len(lines) else ""
    # The lexer records a position only for string literals (it needs one
    # to fold them), so the column is where the token's text first appears
    # on its line -- exact whenever the token is the line's only copy of
    # itself, which for the identifiers and punctuation errors name is the
    # usual case.
    col = 1
    for cand in (name, str(tok.text) if tok.text else None):
        if cand:
            k = src_line.find(cand)
            if k >= 0:
                col = k + 1
                break
    m = re.match(r"line \d+: (.*)$", msg, re.S)
    if m:
        msg = m.group(1)
    # C's wording, where the two front ends mean the same thing
    msg = re.sub(r"^unknown identifier '.*'$", "unknown identifier", msg)
    out = "%s:%d:%d: error: %s\n  %s\n  %s^" % (
        path or "<input>", line, col, msg, src_line, " " * (col - 1))
    d = CDiag(out)
    d.tok, d.unit = tok, unit
    return d


_LIBC = None
# a definition, not a declaration: the line must open a body
_DEFN = re.compile(r"^static\s[^\n(]*?(\w+)\s*\([^\n]*\)\s*\{", re.M)


def _libc_index():
    """function name -> the header of ours that defines it."""
    global _LIBC
    if _LIBC is None:
        _LIBC = {}
        for h in sorted(os.listdir(DEFAULT_INCLUDE)):
            if not h.endswith(".h"):
                continue
            with open(os.path.join(DEFAULT_INCLUDE, h), encoding="latin-1") as f:
                for nm in _DEFN.findall(f.read()):
                    _LIBC.setdefault(nm, h)
    return _LIBC


DEFAULT_INCLUDE = os.path.join(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))), "include")


def compile_file(path, oracle, target="lnx/x86_64", includes=()):
    """`path` is one file or several; several make ONE program."""
    paths = [path] if isinstance(path, str) else list(path)
    srcs = []
    for p in paths:
        # latin-1: every source byte maps to exactly one character, so a
        # string literal's bytes survive the front end unchanged.  Reading as
        # UTF-8 turns \x80..\xff into two bytes and silently corrupts
        # embedded binary data.
        with open(p, encoding="latin-1") as f:
            srcs.append(f.read())
    return compile_sources(srcs, oracle, target, paths, includes)
