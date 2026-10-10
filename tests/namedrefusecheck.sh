#!/bin/bash
# namedrefusecheck (0.0.39 COV1): difftest_o's named-refusal ledger is strict.  A fake driver plays each case
# on the listed probe: the exact refusal at all three -O passes; wrong output, a signal, another diagnostic or a
# refusal at only some levels fails; agreeing everywhere is REVIVED and fails.  No product build is needed.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-namedrefuse.XXXXXX"); trap 'rm -rf "$T"' EXIT
P=tests/c/fb12-31-unused-static-refs-undefined.c
printf 'fb12-31-unused-static-refs-undefined\tnot covered: a branch to an undefined label\ttest\n' > "$T/refuse"
cat > "$T/ua" <<'UA'
#!/bin/sh
o=$1
case "$FAKE_MODE" in
  refuse) echo 'reject: not covered: a branch to an undefined label' >&2; exit 1;;
  wrong)  echo 'nope'; exit 0;;
  signal) kill -9 $$;;
  diag)   echo 'reject: not covered: something else' >&2; exit 1;;
  agree)  echo 'ok'; exit 0;;
  mixed)  [ "$o" = -O2 ] && { echo 'ok'; exit 0; }; echo 'reject: not covered: a branch to an undefined label' >&2; exit 1;;
esac
UA
chmod +x "$T/ua"
run() { FAKE_MODE=$1 UA="$T/ua" UA_RUN="$T/ua" DIFFO_REFUSE="$T/refuse" WF1_PRECHECK=1 PROBES=$P SHARD=1/1 \
        DIFFO_CACHE="$T/cache" python3 tests/bound.py 55 ./tests/difftest_o.sh > "$T/out.$1" 2>&1; echo $?; }
fail() { echo "namedrefuse: $*"; tail -3 "$T/out.$2"; exit 1; }
[ "$(run refuse)" = 0 ] && grep -q 'named 1' "$T/out.refuse" || fail "exact refusal at all levels not accepted" refuse
grep -q 'agree 0 ' "$T/out.refuse" || fail "a named refusal was counted as agree/PASS" refuse
for m in wrong signal diag mixed; do
  [ "$(run $m)" != 0 ] && grep -q "FAIL fb12-31-unused-static-refs-undefined named refusal broken" "$T/out.$m" || fail "$m was not a FAIL" $m
done
[ "$(run agree)" != 0 ] && grep -q 'revived 1' "$T/out.agree" || fail "agreeing everywhere was not REVIVED" agree
echo "namedrefuse  exact refusal at -O0/-O1/-O2 accepted and never counted as agree; wrong, signal, other diagnostic, partial refusal FAIL; agreement REVIVED"
