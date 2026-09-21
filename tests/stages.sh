#!/bin/sh
# No stage decided in code.  [A-33]
#
# The claim is that every table-shaped decision is made by a net.  The Python
# front end asks pp, lex, parse, type, scope and irsel; the self-hosted C
# front end once asked only the first three and decided the rest in
# hand-written code -- quietly, because every other suite checks ANSWERS, and
# a hand-written rule that agrees with the table gives the same answers.
#
# So this checks the thing itself: for every probe, each stage the Python
# front end asked at least once, the C front end must also have asked.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
UA=${UA:-/tmp/ua_ref}
[ -x "$UA" ] || "$R/tests/build_ref.sh" >/dev/null || exit 1
cd "$R"
python3 - "$UA" "$@" <<'PY'
import subprocess, sys
sys.path.insert(0, ".")
from unisa.__main__ import _built
from unisa.oracle import Oracle
from unisa.driver import compile_file
UA, files = sys.argv[1], sys.argv[2:]
FRONT = ("pp", "lex", "parse", "type", "scope", "irsel")
nets = _built()
# a probe the two front ends are KNOWN to disagree on is listed, with its
# reason, in one place -- ccrun.knownwrong -- and is not re-judged here
known = {l.split()[0] for l in open("tests/ccrun.knownwrong")
         if l.strip() and not l.startswith("#")}
ok = bad = skip = nk = 0
for f in files:
    if f.split("/")[-1][:-2] in known:
        nk += 1
        continue
    r = subprocess.run([UA, f, "-c", "-v"], capture_output=True, timeout=60)
    line = [l for l in r.stderr.decode("latin-1").splitlines()
            if l.startswith("asked ")]
    if r.returncode != 0 or not line:
        skip += 1                     # refused by unisacc: selfgap counts it
        continue
    w = line[-1].split()[1:]
    c = {w[i]: int(w[i + 1]) for i in range(0, len(w), 2)}
    o = Oracle(nets, drive="built")
    try:
        compile_file(f, o, "lnx/x86_64")
    except Exception:
        skip += 1
        continue
    py = {s for s in FRONT if o.stats.get(s, [0, 0]) != [0, 0]}
    missing = sorted(s for s in py if c.get(s, 0) == 0)
    if missing:
        bad += 1
        print("  FAIL %-14s python asks %s, unisacc never does" %
              (f.split("/")[-1], ",".join(missing)))
    else:
        ok += 1
print()
print("stages agree %d   missing %d   known %d   (refused %d)" % (ok, bad, nk, skip))
sys.exit(1 if bad else 0)
PY
