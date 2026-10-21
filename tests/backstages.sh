#!/bin/bash
# No BACK-END stage decided in code.  [A-33 extended to the full route]
#
# tests/stages.sh makes the claim for the front end (the `-c -v` route): every
# net the Python front end asks, the C front end must also ask.  This is the
# same claim for the six nets the back end asks -- opinfo and peep (opt),
# abi, regmap and reloc (lower), enc (encoding) -- which only a FULL build
# exercises: `unisacc f.c -O2 -o BIN -v` prints the asked line after
# bk_build, when the back end's asks have happened (src/main.c askedline).
#
# The Python side runs the same full route: front end (compile_file), then
# optimise at -O2 (with the module oracle pointed at this run's oracle, so
# the memoised opinfo asks are counted too), then lower.  Presence is
# compared, not counts: the two sides memoise differently (the C oracle is
# the DENSE cache, proved equal to the nets by oracle-cache), so "asked at
# all" is the invariant, exactly as in stages.sh.
#
# A fixed two-probe set, not the whole corpus: the Python back end costs ~2 s
# per probe, and the claim is per-net reachability, not per-probe parity --
# the per-probe answer parity is difftest's job.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
UA=${UA:-/tmp/ua_ref}
cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
python3 - "$UA" "$T" examples/fib.c examples/hello.c <<'PY'
import subprocess, sys
sys.path.insert(0, ".")
from unisa.__main__ import _built
from unisa.oracle import Oracle
from unisa.driver import compile_file
from unisa.lower import lower
from unisa import opt as _opt
UA, T, files = sys.argv[1], sys.argv[2], sys.argv[3:]
BACK = ("opinfo", "peep", "abi", "regmap", "reloc", "enc")
nets = _built()
ok = bad = 0
for f in files:
    r = subprocess.run([UA, f, "-O2", "-o", T + "/bin", "-v"],
                       capture_output=True, timeout=60)
    line = [l for l in r.stderr.decode("latin-1").splitlines()
            if l.startswith("asked ")]
    if r.returncode != 0 or not line:
        bad += 1
        print("  FAIL %-14s unisacc full build rc=%d, asked line %s"
              % (f.split("/")[-1], r.returncode, "present" if line else "absent"))
        continue
    w = line[-1].split()[1:]
    c = {w[i]: int(w[i + 1]) for i in range(0, len(w), 2)}
    o = Oracle(nets, drive="built")
    _opt._ORACLE[:] = [o]; _opt._INFO.clear()
    t = compile_file(f, o, "lnx/x86_64")
    _opt.optimise(t.to_text(), 2, o)
    lower(t, "lnx/x86_64", o, drive="built")
    py = {s for s in BACK if o.stats.get(s, [0, 0]) != [0, 0]}
    missing = sorted(s for s in py if c.get(s, 0) == 0)
    if missing:
        bad += 1
        print("  FAIL %-14s python asks %s, unisacc never does"
              % (f.split("/")[-1], ",".join(missing)))
    else:
        ok += 1
        print("  ok   %-14s back asks: %s" % (f.split("/")[-1],
              " ".join("%s %d" % (s, c[s]) for s in BACK)))
_opt._ORACLE[:] = []; _opt._INFO.clear()
print("backstages agree %d   missing %d" % (ok, bad))
sys.exit(1 if bad or not ok else 0)
PY
