#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Regression: per-file token numbers are not program-wide storage identities.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b cc tests/multi/static1.c tests/multi/static2.c -o "$T/reference"
b python3 - "$UA" "$T/reference" <<'PY'
import subprocess,sys
def check(cmd):
 r=subprocess.run(cmd,capture_output=True,timeout=15)
 assert (r.returncode,r.stdout,r.stderr)==(19,b'',b''),(cmd,r)
check([sys.argv[2]])
compiler=(['sh'] if sys.argv[1].endswith('.com') else [])+[sys.argv[1]]
for fs in [('static1','static2'),('static2','static1')]:
 paths=['tests/multi/'+f+'.c' for f in fs]
 for level in range(3):check([*compiler,'-O'+str(level),'-run',*paths])
 check(['python3','-m','unisa','run',*paths])
print('static units: cc, C O0/O1/O2 and Python, both orders return 19')
PY
