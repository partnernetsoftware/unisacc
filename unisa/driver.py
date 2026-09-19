"""src -> tape. [W]"""
from .front.pp import preprocess, predefines, expand
from .front.lex import lex
from .front.parse import compile_tokens


def compile_c(src, oracle, target="lnx/x86_64"):
    text, macros = preprocess(src, oracle, predefines(target))
    text = expand(text, macros)
    toks = lex(text, oracle)
    return compile_tokens(toks, oracle)


def compile_file(path, oracle, target="lnx/x86_64"):
    # latin-1: every source byte maps to exactly one character, so a string
    # literal's bytes survive the front end unchanged.  Reading as UTF-8 turns
    # \x80..\xff into two bytes and silently corrupts embedded binary data.
    with open(path, encoding="latin-1") as f:
        return compile_c(f.read(), oracle, target)
