"""src -> tape. [W]"""
import os
import re

from .front.pp import preprocess, predefines, expand
from .front.lex import lex
from .front.parse import compile_tokens, CError


def compile_c(src, oracle, target="lnx/x86_64", path=None, includes=()):
    """Front end, with one retry per missing library function.

    There is no linker, so a program that calls `strlen` without including
    <string.h> -- which is most C ever written -- used to be refused.  We
    carry those functions as ordinary C in `include/`, so when one turns out
    to be missing we append the header that defines it and compile again.
    Calls are resolved at the end of the unit, so appending is enough and the
    user's line numbers do not move."""
    tried = set()
    while True:
        try:
            return _compile_once(src, oracle, target, path, includes)
        except CError as e:
            m = re.search(r"undefined function '(\w+)'", str(e))
            hdr = _libc_index().get(m.group(1)) if m else None
            if hdr is None or hdr in tried:
                raise
            tried.add(hdr)
            src = src + "\n#include <%s>\n" % hdr


def _compile_once(src, oracle, target, path, includes):
    text, macros = preprocess(src, oracle, predefines(target), path,
                              tuple(includes) + (DEFAULT_INCLUDE,))
    text = expand(text, macros)
    toks = lex(text, oracle)
    t = compile_tokens(toks, oracle)
    # Which OS the SOURCE was compiled for.  Not the same question as which
    # OS a lowering targets: `#ifdef _WIN32` is decided here, once, and an
    # interpreter has to read the tape's syscall arguments in that light.
    t.src_os = target.split("/")[0]
    return t


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
    # latin-1: every source byte maps to exactly one character, so a string
    # literal's bytes survive the front end unchanged.  Reading as UTF-8 turns
    # \x80..\xff into two bytes and silently corrupts embedded binary data.
    with open(path, encoding="latin-1") as f:
        return compile_c(f.read(), oracle, target, path, includes)
