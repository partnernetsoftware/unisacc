#!/bin/bash
# gatesummarycheck (0.0.40, 机房主任 20:54): the gate summary reads only the leading "suite rc=N" field of each
# result line, never a number that appears later in a suite's own output.  Fixture result files in the exact
# format gate.sh writes ('%-14s rc=%-3s %3ss :: %s') go through tests/gatesummary.sh.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-gatesum.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "gatesummary: $*"; exit 1; }
case_() {   # case_ NAME WANT_RC WANT_FAILED WANT_UNVERIFIED LINE...
  local name=$1 want=$2 wf=$3 wu=$4; shift 4; local d=$T/$name; mkdir -p "$d"; local i=0
  for l in "$@"; do i=$((i+1)); printf '%s\n' "$l" > "$d/$(printf %03d $i).x"; done
  out=$(bash "$R/tests/gatesummary.sh" "$d" 0); rc=$?
  [ "$rc" = "$want" ] || fail "$name: exit $rc, want $want ($out)"
  echo "$out" | grep -q "failed $wf   unverified $wu " || fail "$name: summary '$out', want failed $wf unverified $wu"
}
L() { printf '%-14s rc=%-3s %3ss :: %s' "$1" "$2" "$3" "$4"; }
case_ pass            0 0 0 "$(L a 0 1 'ok')" "$(L b 0 2 'fine')"
case_ red-tail-rc0    1 1 0 "$(L queuetimeout 1 0 'queuetimeout: one window, 142: rc=0 launches=1')"
case_ red-tail-rc77   1 1 0 "$(L s 1 0 'expected rc=77 here')"
case_ pass-tail-rc77  0 0 0 "$(L s 0 1 'note: a child said rc=77 once')"
case_ real-77         4 0 1 "$(L host 77 0 'UNVERIFIED: required=darwin')"
case_ kill-142-tail0  1 1 0 "$(L slow 142 46 'last line rc=0 before the watchdog')"
case_ kill-137-tail0  1 1 0 "$(L oom 137 3 'child rc=0 then killed')"
case_ long-name-red   1 1 0 "$(L a-very-long-suite-name-x 1 0 'rc=0')"
case_ malformed       1 1 0 "garbage line with rc=0 in it"
case_ real-77-tail0    4 0 1 "$(L host 77 0 'probe printed rc=0 first')"
case_ kill-142-tail77 1 1 0 "$(L slow 142 46 'rc=77 seen before the watchdog')"
mkdir -p "$T/empty"; out=$(bash "$R/tests/gatesummary.sh" "$T/empty" 0); [ $? = 1 ] || fail "an empty result dir was not a failure: $out"
mkdir -p "$T/unreadable"; printf 'x rc=0 1s :: ok\n' > "$T/unreadable/001.x"; chmod 000 "$T/unreadable/001.x"
if [ "$(id -u)" != 0 ]; then out=$(bash "$R/tests/gatesummary.sh" "$T/unreadable" 0 2>&1); [ $? = 1 ] || fail "an unreadable result was not a failure: $out"; fi
chmod 644 "$T/unreadable/001.x"
# one green file beside an empty (zero-byte) result is not green; a two-line file and a repeated suite fail too
d=$T/green-empty; mkdir -p "$d"; L a 0 1 ok > "$d/001.a"; printf '\n' >> "$d/001.a"; : > "$d/002.b"
out=$(bash "$R/tests/gatesummary.sh" "$d" 0); [ $? = 1 ] || fail "green + empty result summarised as passed: $out"
d=$T/two-lines; mkdir -p "$d"; printf '%s\n%s\n' "$(L a 0 1 ok)" "$(L b 0 1 ok)" > "$d/001.a"
out=$(bash "$R/tests/gatesummary.sh" "$d" 0); [ $? = 1 ] || fail "a two-line result file passed: $out"
d=$T/dup; mkdir -p "$d"; L a 0 1 ok > "$d/001.a"; L a 0 1 ok > "$d/002.a"
out=$(bash "$R/tests/gatesummary.sh" "$d" 0); [ $? = 1 ] || fail "a suite reported twice passed: $out"
case_ mixed           1 1 1 "$(L a 0 1 ok)" "$(L b 77 0 unv)" "$(L c 2 0 'rc=0 rc=77')"
echo "gatesummary  only the leading suite rc field counts: a red suite whose output mentions rc=0/77 is failed; rc 0 with rc=77 in its output stays PASS (not unverified); a real 77 exits 4; watchdog 142 and kill 137 are red; long names and malformed lines handled; a real 77 whose output says rc=0 is unverified, a 142 whose output says rc=77 is red; an empty or unreadable result set fails; a zero-byte result beside a green one, a multi-line result file and a repeated suite fail"
