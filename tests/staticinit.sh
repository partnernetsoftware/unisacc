#!/bin/sh
# Static initialisation must not refer to a live automatic object.
# Compilation only: never run the invalid input, even if a compiler accepts it.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 10; exec @ARGV' "$@"; }
cat > "$T/read.c" <<'SRC'
int f(int k) { int n = k; static int m = n; return m; }
int main(void) { return f(3); }
SRC
cat > "$T/address.c" <<'SRC'
int f(int k) { static int *p = &k; return *p; }
int main(void) { return f(3); }
SRC
cat > "$T/array.c" <<'SRC'
int f(void) { int a[2]; static int *p = a; return *p; }
int main(void) { return f(); }
SRC
ok=0
for f in read address array; do
    rc=0; b cc -fsyntax-only "$T/$f.c" > "$T/cc" 2>&1 || rc=$?
    [ "$rc" -eq 1 ] || { echo "host did not reject $f: $rc"; exit 1; }
    for opt in 0 1 2; do
        rc=0; b "$UA" -O"$opt" -S "$T/$f.c" -o "$T/out" > "$T/log" 2>&1 || rc=$?
        [ "$rc" -eq 1 ] && grep -q 'static initializer refers to an automatic object' "$T/log" || {
            cat "$T/log"; echo "staticinit $f -O$opt: exit $rc"; exit 1;
        }
        ok=$((ok+1))
    done
done
echo "staticinit: $ok rejections, no invalid program executed"
