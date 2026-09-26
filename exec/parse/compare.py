"""Compare the E3 delta (minimum slice) with the reference, file by file.

    python3 exec/parse/compare.py delta.json FILE...

Per file (each child bounded, 10 s; each sim run bounded by steps):
  x    : UA_TYPESPELL=1 $E3DUMP -dump-tokens FILE   (the reference's token stream)
  ref  : $E3REF FILE -S -o -         (the reference's tape)
  sim  : exec/pp/sim.py on x         (accept(o) or reject(k); the executor is E2's, unchanged)
x comes from $E3DUMP (exec/parse/mkdump.sh, default /tmp/ua_tdump) with
UA_TYPESPELL=1, so a type token carries its spelling (`type=char`); the delta
itself decides what it covers -- there is no filter outside it.
Verdicts:
  equal         sim accept, o == ref stdout, ref exit 0
  reject-both   sim reject k (not `not covered`), ref exit in 1..127: both ends did
                not succeed -- NOT confirmed to be the same rejection (the
                diagnostics and exit codes are not compared); an observation only
  not-covered   sim reject `not covered: ...`, or src-type
  tool-fail     the token dump did not exit 0, or the reference timed out or died
                on a signal -- a harness failure, never a pass
  DIFF          anything else -- listed with the first differing line
Exit status 1 on any DIFF or tool-fail, on an empty file list, and -- with
E3KEEP=LIST (one path per line) -- when a listed file is not `equal`.
"""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "pp"))
import sim           # noqa: E402

REF = os.environ.get("E3REF", "/tmp/ua_ref")
DUMP = os.environ.get("E3DUMP", "/tmp/ua_tdump")


def sh(argv, t=10, env=None):
    try:
        p = subprocess.run(argv, capture_output=True, timeout=t, env=env)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return "timeout", b"", b""


def main():
    delta = json.load(open(sys.argv[1]))
    loaded = sim.load(delta)
    tally, why = {}, {}
    cov = set()
    steps = 0
    if len(sys.argv) < 3:
        print("no input files")
        return 1
    keep = set(open(os.environ["E3KEEP"]).read().split()) if os.environ.get("E3KEEP") else set()
    if os.environ.get("E3KEEP") and not keep:
        print("E3KEEP names an empty list")
        return 1
    lost = []
    for f in sys.argv[2:]:
        rc0, x, _ = sh([DUMP, "-dump-tokens", f], env=dict(os.environ, UA_TYPESPELL="1"))
        rc, out, err = sh([REF, f, "-S", "-o", "-"])
        k = None
        if rc0 != 0 or not isinstance(rc, int) or rc < 0 or rc >= 128:
            v = "tool-fail"
            print("  TOOL-FAIL %s  dump rc=%s ref rc=%s" % (f, rc0, rc))
        else:
            res, val, n = sim.run(delta, x, f, None, cov, maxsteps=50000000, loaded=loaded)
            steps += n
            if res == "accept" and rc == 0 and val == out:
                v = "equal"
            elif res == "reject" and val[0].startswith("not covered"):
                v, k = "not-covered", val[0]
            elif res == "reject" and rc != 0:
                v = "reject-both"
            else:
                v = "DIFF"
                print("  DIFF %s  ref rc=%s sim=%s" % (f, rc, res if res != "reject" else val[0]))
                if res == "accept":
                    a, b = out.split(b"\n"), val.split(b"\n")
                    for j in range(max(len(a), len(b))):
                        if j >= len(a) or j >= len(b) or a[j] != b[j]:
                            print("     line %d ref %r\n             sim %r" % (
                                j + 1, a[j] if j < len(a) else None, b[j] if j < len(b) else None))
                            break
        if k:
            why[k] = why.get(k, 0) + 1
        if os.environ.get("E3V"):
            print("  %-12s %s %s" % (v, f, k or ""))
        tally[v] = tally.get(v, 0) + 1
        if f in keep and v != "equal":
            lost.append((f, v))
    ent = sum(len(r) for _, r in delta["states"].values())
    print("not-covered reasons: " + "; ".join("%s %d" % kv for kv in sorted(why.items(), key=lambda t: -t[1])))
    print("delta states visited %d/%d  entries used %d/%d"
          % (len(set(q for q, _ in cov)), len(delta["states"]), len(cov), ent))
    print("files %d  %s  steps %d" % (len(sys.argv) - 2, "  ".join("%s %d" % kv for kv in sorted(tally.items())), steps))
    for f, v in lost:
        print("  LOST %s  (kept set; now %s)" % (f, v))
    missing = sorted(keep - set(sys.argv[2:]))
    for f in missing:
        print("  LOST %s  (kept set; not in the input list)" % f)
    return 1 if tally.get("DIFF") or tally.get("tool-fail") or lost or missing else 0


if __name__ == "__main__":
    sys.exit(main())
