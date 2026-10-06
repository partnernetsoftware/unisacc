#!/bin/sh
# 0.0.32 D2: the product memcpy/memset word paths are defined C -- the bodies from
# include/string.h run tests/memalign/memalign.c (every src/dst offset 0..15, length 0..80) under
# UBSan alignment checks on the host cc, and the shipped .com runs the same probe.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound; T=${TMPDIR:-/tmp}/unisacc-memalign.$$; mkdir -p "$T"; trap 'rm -rf "$T"' EXIT
python3 - "$T/ma.c" <<'PY' || { echo "memalign: cannot extract"; exit 1; }
import sys
s = open('include/string.h').read()
def grab(name):
    i = s.index('static void *%s(' % name); j = s.index('\n}\n', i) + 3
    return s[i:j].replace(name + '(', 'u_' + name + '(')
t = open('tests/memalign/memalign.c').read()
t = t.replace('#include <string.h>\n', '').replace('#include <stdio.h>\n', '').replace('printf("%d\\n", bad);', '')
t = t.replace('memcpy(', 'u_memcpy(').replace('memset(', 'u_memset(')
open(sys.argv[1], 'w').write('#define NULL 0\n' + grab('memset') + grab('memcpy') + t)
PY
"$B" 30 cc -std=c99 -w -fsanitize=alignment -fno-sanitize-recover=alignment "$T/ma.c" -o "$T/ma" &&
  "$B" 20 "$T/ma" 2> "$T/err" || { echo "memalign: UBSan $(head -1 "$T/err")"; exit 1; }
out=$("$B" 30 sh "${MODEL_COM:-$R/unisacc.com}" -run tests/memalign/memalign.c) && [ "$out" = 0 ] ||
  { echo "memalign: product run gave '$out'"; exit 1; }
echo "memalign  UBSan clean over 16x16 offsets x 81 lengths; product run 0 wrong"
