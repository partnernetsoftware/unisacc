"""Compare the E2 delta (minimum slice) with the reference, file by file.

    python3 exec/pp/compare.py delta.json FILE... | @LISTFILE

Per file (each child bounded, 10 s; each sim run bounded by steps):
  ref    : $E2REF -E FILE          (the reference as built)
  noauto : $E2NOAUTO -E FILE       (same, autoinc() removed: exec/pp/mknoauto.sh)
  sim    : exec/pp/sim.py          (accept(o) or reject(k))
The -E text IS the lexer's input buffer (fe_load writes src[0..nsrc) after
expandsrc(), the same buffer lex() reads; exec/lex/mkpre.sh's UA_LEXIN hook
dumps it), so one comparison covers both.
Verdicts:
  equal         sim accept, stdout == ref stdout, ref exit 0
  equal-noauto  sim accept, stdout == noauto stdout (ref differs: autoinc fired)
  reject-agree  sim reject k, ref exit 1 and k appears in ref stderr
  not-covered   sim reject `not covered: ...` (construct outside the slice)
  DIFF          anything else -- listed
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import json          # noqa: E402
import sim           # noqa: E402

REF = os.environ.get("E2REF", "/tmp/ua_ref")
NOAUTO = os.environ.get("E2NOAUTO", "/tmp/ua_noauto")


def sh(argv, t=10):
    try:
        p = subprocess.run(argv, capture_output=True, timeout=t)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return "timeout", b"", b""


def main():
    delta = json.load(open(sys.argv[1]))
    loaded = sim.load(delta)
    files = []
    for a in sys.argv[2:]:
        files += open(a[1:]).read().split() if a.startswith("@") else [a]
    tally = {}
    cov = set()
    steps = nbytes = 0
    fs = sim.Files()
    for f in files:
        rc, out, err = sh([REF, "-E", f])
        rc2, out2, _ = sh([NOAUTO, "-E", f])
        x = open(f, "rb").read()
        res, val, n = sim.run(delta, x, f, fs, cov, maxsteps=20000000, loaded=loaded)
        steps += n
        nbytes += len(x)
        if res == "accept" and rc == 0 and val == out:
            v = "equal"
        elif res == "accept" and rc2 == 0 and val == out2:
            v = "equal-noauto"
        elif res == "reject" and val[0].startswith("not covered"):
            v = "not-covered"
            print("  not-covered %s  (%s)" % (f, val[0]))
        elif res == "reject" and rc == 1 and val[0].encode() in err:
            v = "reject-agree"
        else:
            v = "DIFF"
            print("  DIFF %s  ref rc=%s sim=%s" % (f, rc, res if res != "reject" else val[0]))
            if res == "accept":
                a, b = out2.split(b"\n"), val.split(b"\n")
                for k in range(max(len(a), len(b))):
                    if k >= len(a) or k >= len(b) or a[k] != b[k]:
                        print("     noauto line %d ref %r\n                sim %r" % (
                            k + 1, a[k][:100] if k < len(a) else None, b[k][:100] if k < len(b) else None))
                        break
        tally[v] = tally.get(v, 0) + 1
    ent = sum(len(r) for _, r in delta["states"].values())
    print("delta states visited %d/%d  entries used %d/%d"
          % (len(set(q for q, _ in cov)), len(delta["states"]), len(cov), ent))
    print("files %d  %s  bytes %d  steps %d" % (len(files), "  ".join(
        "%s %d" % kv for kv in sorted(tally.items())), nbytes, steps))
    return 1 if tally.get("DIFF") else 0


if __name__ == "__main__":
    sys.exit(main())
