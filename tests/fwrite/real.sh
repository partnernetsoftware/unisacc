#!/bin/sh
# REAL fwrite probe: `ulimit -f 1`, SIGXFSZ ignored, 3000 B then 5000 B.
# Runs the program built by the reference compiler natively, through -run,
# and through the Python front end; passes when none of them claims both
# writes succeeded and the three files hold the same bytes.
# usage: tests/fwrite/real.sh [ref-compiler]   (default /tmp/ua_ref)
UA=${1:-/tmp/ua_ref}
D=$(cd "$(dirname "$0")" && pwd); R=$(cd "$D/../.." && pwd)
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm shift; exec @ARGV' "$@"; }
b 30 "$UA" "$D/real.c" -b osx/arm64 -o "$T/real" >/dev/null 2>&1 || { echo "FAIL build"; exit 1; }
bad=0
ulimit -f 1
trap '' XFSZ
o1=$(b 20 "$T/real" "$T/native.bin"); echo "  native: $o1"
o2=$(b 30 "$UA" -run "$D/real.c" "$T/run.bin"); echo "  -run:   $o2"
o3=$(cd "$R" && b 55 python3 -m unisa run "$D/real.c" --drive built -- "$T/py.bin" 2>&1 | tail -1); echo "  python: $o3"
for o in "$o1" "$o2" "$o3"; do case "$o" in *error-reported*) ;; *) bad=1;; esac; done
for f in native run py; do
    echo "  $f.bin $(wc -c < "$T/$f.bin" 2>/dev/null) B, $(tr -d 'ab' < "$T/$f.bin" 2>/dev/null | wc -c) stray B, b-count $(tr -cd 'b' < "$T/$f.bin" 2>/dev/null | wc -c)"
done
cmp -s "$T/native.bin" "$T/run.bin" && cmp -s "$T/native.bin" "$T/py.bin" && echo "  files identical" || { echo "  files DIFFER"; bad=1; }
exit $bad
