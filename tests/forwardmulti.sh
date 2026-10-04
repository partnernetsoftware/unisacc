#!/bin/sh
# 0.0.26 N5: host forwarding across translation units.  A prototype in one unit, its caller's
# helper in another: `unisacc a.c b.c`, -run and -o must forward like the one-file program does.
# The product failed here up to 0.0.25 (fwdwant required one source); the reference never did.
#   UA=compiler ./tests/forwardmulti.sh        (default /tmp/ua_ref; the gate's com- variant passes the product)
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
_BOUND=$("$R/tests/bound" --helper) || exit 2
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
[ "$(uname -s)" = Darwin ] || [ "$(uname -s)" = Linux ] || { echo "forwardmulti: POSIX hosts only"; exit 2; }
printf 'long sysconf(int);\nint side(void);\nint main(void){ return sysconf(29) > 0 && side() == 7 ? 0 : 3; }\n' > "$T/m1.c"
printf 'int side(void){ return 7; }\n' > "$T/m2.c"
ok=0; bad=0
check() { if [ "$2" -eq 0 ]; then ok=$((ok+1)); echo "  ok   $1"; else bad=$((bad+1)); echo "  FAIL $1 (rc $2)"; head -2 "$T/err" | sed 's/^/       /'; fi; }
(cd "$T" && "$_BOUND" 30 "$UA_RUN" m1.c m2.c) 2>"$T/err"; check "two units, default run" $?
(cd "$T" && "$_BOUND" 30 "$UA_RUN" -run m1.c m2.c) 2>"$T/err"; check "two units, -run" $?
if (cd "$T" && "$_BOUND" 30 "$UA_RUN" -o m.out m1.c m2.c) 2>"$T/err"; then
    chmod +x "$T/m.out"; "$_BOUND" 10 "$T/m.out" 2>"$T/err"; check "two units, -o image" $?
else check "two units, -o compile" 1; fi
echo; echo "forwardmulti  ok $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
