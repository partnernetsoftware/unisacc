"""Shared driver for check_<fmt>.py: main(line_ok, whole_ok) -> exit status."""
import re
import sys


def main(line_ok=None, whole_ok=None):
    if len(sys.argv) != 2:
        sys.stderr.write("usage: %s FILE  (checks the text of one dump)\n"
                         % (sys.argv[0] or "check"))
        return 2
    try:
        b = open(sys.argv[1], "rb").read()
    except OSError as e:
        sys.stderr.write("%s\n" % e)
        return 2
    if b"\0" in b:
        sys.stderr.write("NUL byte\n")
        return 1
    if whole_ok:
        m = whole_ok(b)
        if m:
            sys.stderr.write(m + "\n")
            return 1
    if line_ok:
        for n, l in enumerate(b.decode("latin-1").split("\n"), 1):
            if l and not line_ok(l):
                sys.stderr.write("line %d: %r\n" % (n, l[:80]))
                return 1
    return 0


TOK = re.compile(r"^[a-z_]+=.*$|^\S+$|^\d+ tokens$")
# kind -> (is the mutually-exclusive form present?, message)
_TOKEN_KINDS = {
    "plain": (lambda ls: any(l.startswith(b"type=") for l in ls),
              "a type token carries a spelling (that is tokens.typed)"),
    "typed": (lambda ls: b"type" in ls,
              "a bare `type` token without its spelling (that is tokens.plain)"),
}


def tokens(kind):
    """Exit status for a tokens.<kind> dump (kind: plain | typed)."""
    foreign, why = _TOKEN_KINDS[kind]

    def whole(b):
        ls = b.rstrip(b"\n").split(b"\n")
        if len(ls) < 2 or ls[-2] != b"eof" or ls[-1] != b"%d tokens" % (len(ls) - 1):
            return "does not end with `eof` and `N tokens` (N = token lines incl. eof)"
        if foreign(ls):
            return why

    sys.exit(main(lambda l: bool(TOK.match(l)), whole))
