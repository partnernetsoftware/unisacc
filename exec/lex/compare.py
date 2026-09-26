"""Compare the E1 delta against the reference lexer, file by file.

    python3 exec/lex/compare.py delta.json FILE... | @LISTFILE

Per file (every child process bounded, 10 s):
  ref : /tmp/ua_ref -dump-tokens FILE        -> stdout, stderr, exit status
  in  : UA_LEXIN hook of /tmp/ua_pre         -> the lexer's input buffer
  sim : exec/lex/sim.py on that buffer       -> accept(o) or reject(k, i)
        a reject is rendered by UA_LEXREJ=i  -> stderr, exit status
Equal means stdout, stderr and exit status all equal.
With E1EXEC=EXE:TABLE (exec/exec.c and tbl.py's table) every buffer is also
run through the C executor, bounded 10 s, and its result must equal sim's:
the same output bytes on accept, the same reject kind and position.
A file whose reference run fails BEFORE the lexer (the buffer hook is never
reached) is counted as `pre-lex` and not compared: that is the pp layer (E2).
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import json          # noqa: E402
import sim           # noqa: E402
import tbl           # noqa: E402

REF, PRE = "/tmp/ua_ref", "/tmp/ua_pre"


def sh(argv, env=None, t=10):
    e = dict(os.environ)
    e.update(env or {})
    try:
        p = subprocess.run(argv, capture_output=True, env=e, timeout=t)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return "timeout", b"", b""


def main():
    delta = json.load(open(sys.argv[1]))
    ok = bad = prelex = 0
    steps = nbytes = 0
    rejects = 0
    cov = set()
    ex = os.environ.get("E1EXEC")
    xok = xbad = 0
    tmp = tempfile.mkdtemp()
    buf = os.path.join(tmp, "in.i")
    files = []
    for a in sys.argv[2:]:
        files += open(a[1:]).read().split() if a.startswith("@") else [a]
    for f in files:
        rc, out, err = sh([REF, "-dump-tokens", f])
        if os.path.exists(buf):
            os.unlink(buf)
        rc2, _, _ = sh([PRE, "-dump-tokens", f], {"UA_LEXIN": buf})
        if not os.path.exists(buf):
            prelex += 1
            print("  pre-lex %s (rc %s)" % (f, rc))
            continue
        x = open(buf, "rb").read()
        res, val, n = sim.run(delta, x, cov)
        steps += n
        if ex:
            exe, tb = ex.split(":")
            erc, eout, eerr = sh([exe, tb, buf])
            if erc == 0:
                eg = ("accept", eout)
            elif erc == 1 and eerr.startswith(b"reject "):
                k, _, at = eerr.split()[1:4]
                eg = ("reject", (tbl.REJECTS[int(k) - 1], int(at)))
            else:
                eg = ("error", (erc, eerr[:80]))
            if eg == (res, val):
                xok += 1
            else:
                xbad += 1
                print("  EXEC-DIFF %s  sim %s exec %s" % (f, res if res == "accept" else val, eg[0] if eg[0] == "accept" else eg[1]))
        nbytes += len(x)
        if res == "accept":
            got = (0, val, b"")
        else:
            rejects += 1
            got = ("reject " + val[0], b"", b"")
            if val[0] in ("unexpected character", "missing terminating '\"' character",
                          "missing terminating ' character"):
                got = sh([PRE, "-dump-tokens", f], {"UA_LEXREJ": str(val[1]), "UA_LEXMSG": val[0]})
        if got == (rc, out, err):
            ok += 1
        else:
            bad += 1
            print("  DIFF %s  ref rc=%s sim=%s" % (f, rc, res if res == "accept" else val))
            a = out.split(b"\n")
            b = got[1].split(b"\n")
            for k in range(max(len(a), len(b))):
                if k >= len(a) or k >= len(b) or a[k] != b[k]:
                    print("     line %d ref %r sim %r" % (k + 1, a[k] if k < len(a) else None,
                                                     b[k] if k < len(b) else None))
                    break
            if err != got[2]:
                print("     stderr ref %r sim %r" % (err[:120], got[2][:120]))
    ent = sum(len(r) for _, r in delta["states"].values())
    print("delta states visited %d/%d  entries used %d/%d"
          % (len(set(q for q, _ in cov)), len(delta["states"]), len(cov), ent))
    print("equal %d  differ %d  pre-lex %d  sim-rejects %d  bytes %d  steps %d"
          % (ok, bad, prelex, rejects, nbytes, steps))
    if ex:
        print("exec-equal %d  exec-differ %d" % (xok, xbad))
    return 1 if bad or ok == 0 or xbad or (ex and xok == 0) else 0


if __name__ == "__main__":
    sys.exit(main())
