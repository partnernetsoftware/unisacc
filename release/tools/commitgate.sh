#!/bin/bash
# commitgate.sh -m MSG [--suite NAME]... -- PATH... (0.0.39): the recommended commit path.  Runs the named gate
# suites (default: the light infra set) through checkrun.sh, so the decision is the gate's own exit status, and
# commits the given paths only when it is 0.  A red gate -- whatever its log tail says -- leaves no commit.
# COMMITGATE_GATE replaces tests/gate.sh only together with COMMITGATE_SELFTEST=1 (tests/commitgatecheck.sh).
set -u
R=$(cd "$(dirname "$0")/../.." && pwd)
msg=; suites=()
while [ $# -gt 0 ]; do
  case $1 in
    -m) msg=${2:?message}; shift 2;;
    --suite) suites+=(--suite "${2:?suite}"); shift 2;;
    --) shift; break;;
    *) echo "commitgate: unknown argument $1" >&2; exit 2;;
  esac
done
[ -n "$msg" ] && [ $# -gt 0 ] || { echo "commitgate: need -m MSG and -- PATH..." >&2; exit 2; }
[ ${#suites[@]} -gt 0 ] || suites=(--suite gate-layers --suite script-inventory --suite checkrun)
gate=$R/tests/gate.sh
if [ -n "${COMMITGATE_GATE:-}" ]; then
  [ "${COMMITGATE_SELFTEST:-}" = 1 ] || { echo "commitgate: COMMITGATE_GATE is for the self-test only" >&2; exit 2; }
  gate=$COMMITGATE_GATE
fi
log=$(mktemp "${TMPDIR:-/tmp}/commitgate.XXXXXX")
"$R/release/tools/checkrun.sh" "$log" -- "$gate" "${suites[@]}" || { rc=$?; echo "commitgate: gate rc=$rc, nothing committed (log $log)" >&2; exit 1; }
git commit -q -m "$msg" -- "$@" || exit 1
echo "commitgate: committed $(git rev-parse --short HEAD) after gate rc=0 (log $log)"
