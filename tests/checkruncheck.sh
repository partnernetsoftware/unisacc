#!/bin/sh
# checkruncheck: checkrun.sh exits with the check's status even when the check's log ends in a success-looking
# line, and keeps the whole log; a passing check exits 0.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-checkrun.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "checkrun: $*"; exit 1; }
"$R/release/tools/checkrun.sh" "$T/a" -- sh -c 'echo early; echo "rc=0  all green"; exit 1' > "$T/out" 2>/dev/null
[ $? -eq 1 ] || fail "a red check with a green-looking log tail did not exit 1"
grep -q early "$T/a" && grep -q 'rc=0  all green' "$T/out" || fail "log or tail lost"
tail -1 "$T/a" | grep -q "^checkrun: rc=1 " || fail "the log does not end with the check's own rc"
"$R/release/tools/checkrun.sh" "$T/b" -- sh -c 'exit 0' > /dev/null 2>&1 || fail "a green check did not exit 0"
"$R/release/tools/checkrun.sh" "$T/c" -- sh -c 'kill -9 $$' > /dev/null 2>&1; [ $? -eq 137 ] || fail "a killed check did not keep its status"
"$R/release/tools/checkrun.sh" "$T/d" > /dev/null 2>&1; [ $? -eq 2 ] || fail "no command was not rc 2"
echo "checkrun  check status kept (red with green-looking tail = 1, killed = 137), full log kept, no command = 2"
