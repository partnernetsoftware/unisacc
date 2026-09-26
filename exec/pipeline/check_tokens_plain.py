import re
import sys
import _check
TOK = re.compile(r"^[a-z_]+=.*$|^\S+$|^\d+ tokens$")


def whole(b):
    ls = b.rstrip(b"\n").split(b"\n")
    if len(ls) < 2 or ls[-2] != b"eof" or ls[-1] != b"%d tokens" % (len(ls) - 1):
        return "does not end with `eof` and `N tokens` (N = token lines incl. eof)"
    if any(l.startswith(b"type=") for l in ls):
        return "a type token carries a spelling (that is tokens.typed)"


sys.exit(_check.main(lambda l: bool(TOK.match(l)), whole))
