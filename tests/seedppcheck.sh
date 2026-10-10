#!/bin/sh
# K5-1b (机房主任 00:42): pp δ -- seed/gen.c vs exec/build/gen.py byte-identical for the six flag sets the
# Python call sites use (none, --osx, --win, --arm64, --osx --arm64, --win --arm64); a corrupted
# exec/pp/autoinc-result.tsv (line 2, last column dropped) is refused by name by both; --osx --win is refused by
# both (C's reason is "manifest let option is not yet covered", not a named exclusion -- a finding, the generator
# is not changed here); exec/pp/gen-delta.sh prefers seed-gen with no silent Python fallback.  No consumer is
# changed.  Same harness as tests/seedoptcheck.sh: private scratch tree from one commit, fail-closed evidence
# export (SEEDPP_EVIDENCE), checkout conservation.  Split beforehand (00:42 §5):
#   tests/seedppcheck.sh 1   none --osx --win; both negatives; helper flag/fallback checks; export fault branches
#   tests/seedppcheck.sh 2   --arm64 --osx-arm64 --win-arm64; default route cold build / cache hit / json.h rebuild
case "${1:-}" in
    1) GROUPS_="none osx win";;
    2) GROUPS_="arm64 osx-arm64 win-arm64";;
    *) echo "usage: tests/seedppcheck.sh 1|2"; exit 2;;
esac
SHARD=$1
JSONS=""; for g in $GROUPS_; do JSONS="$JSONS c-$g py-$g"; done
flags() { case $1 in none) ;; osx) echo --osx;; win) echo --win;; arm64) echo --arm64;; osx-arm64) echo --osx --arm64;; win-arm64) echo --win --arm64;; esac; }
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
S=$(mktemp -d "${TMPDIR:-/tmp}/seedpp.XXXXXX") || exit 2
# 0.0.40 (机房主任 00:04): SEEDPP_EVIDENCE=DIR (a new or empty directory outside the scratch) keeps the evidence
# the scratch would lose: every SAME/DIFF line, the o1/o2 JSON sha256s and the monotonic start/end (ns), written
# before the scratch is removed -- also on a failing or interrupted run.
EV=${SEEDPP_EVIDENCE:-}
# 0.0.40 (机房主任 00:25): the export fails closed.  Any failed write, timestamp or hash sets evfail; a run with
# evfail is never green (exit 3 when it would have been 0, the original status otherwise), and its scratch is kept
# (path printed) instead of being removed without a record.
evfail=0
mono() { python3 -c 'import time;print(time.monotonic_ns())'; }
evw() {   # evw LINE: append one line to lines.txt, or mark the export failed
    [ -n "$EV" ] || return 0
    printf '%s\n' "$1" >> "$EV/lines.txt" || { evfail=1; echo "seedpp: EVIDENCE write failed: $1" >&2; }
}
evmono() {   # evmono KEY: one monotonic stamp, checked to be a number
    [ -n "$EV" ] || return 0
    t=$(mono) && case "$t" in ''|*[!0-9]*) false;; *) true;; esac && printf '%s %s\n' "$1" "$t" >> "$EV/mono.txt" \
        || { evfail=1; echo "seedpp: EVIDENCE timestamp $1 failed" >&2; }
}
if [ -n "$EV" ]; then
    case "$EV" in "$S"|"$S"/*) echo "seedpp: SEEDPP_EVIDENCE inside the scratch"; exit 2;; esac
    mkdir -p "$EV" && [ -z "$(ls -A "$EV")" ] || { echo "seedpp: SEEDPP_EVIDENCE must be a new or empty directory: $EV"; exit 2; }
    evmono start_ns; [ "$evfail" = 0 ] || exit 2
fi
export_ev() {   # hashes of the four JSON files (all four must exist and hash) and the end stamp
    [ -n "$EV" ] || return 0
    rc=0
    for f in $JSONS; do
        if [ -f "$S/$f.json" ] && h=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$S/$f.json") \
            && [ ${#h} -eq 64 ]; then line="$h $f.json"; else line="MISSING $f.json"; rc=1; fi
        printf '%s\n' "$line" >> "$EV/json.sha256" || rc=1
    done
    evmono end_ns; [ "$evfail" = 0 ] || rc=1
    return $rc
}
finish() {
    st=$1
    export_ev || evfail=1
    if [ "$evfail" != 0 ]; then
        echo "seedpp: EVIDENCE INCOMPLETE -- not green; scratch kept at $S" >&2
        [ -z "$EV" ] || printf 'EVIDENCE INCOMPLETE status %s scratch %s\n' "$st" "$S" >> "$EV/lines.txt" 2>/dev/null
        [ "$st" -ne 0 ] || st=3
    else
        rm -rf "$S"
    fi
    exit "$st"
}
trap 'finish $?' EXIT
trap 'exit 130' INT TERM HUP
NEG=exec/pp/autoinc-result.tsv
before=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$NEG") || exit 2
W=$S/tree; mkdir "$W"
H=$(git rev-parse --verify HEAD) || exit 2   # the one commit this run is about; every copy comes from it
echo "seedpp  shard $SHARD  tree $H"
git archive -o "$S/tree.tar" "$H" exec seed unisa tests/bound tests/bound.c tests/bound.py || { echo "seedpp: git archive failed"; exit 2; }
tar -x -C "$W" -f "$S/tree.tar" || { echo "seedpp: scratch tree failed"; exit 2; }
cd "$W" || exit 2
B=$W/tests/bound
BOUND_CACHE=$S/boundcache; export BOUND_CACHE
G=$S/seed-gen
"$B" 55 cc -std=c99 -O2 -w -Iseed -o "$G" seed/gen.c || { echo "seedpp: gen.c does not build"; exit 1; }
same=0; bad=0
ok() { echo "SAME $1"; evw "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; evw "DIFF $1"; bad=$((bad + 1)); }
refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }

for g in $GROUPS_; do   # C vs Python, then a second C run (determinism)
    f=$(flags "$g"); rm -f "$S/c-$g.json" "$S/py-$g.json" "$S/c-$g-b.json"
    "$B" 55 "$G" pp "$S/c-$g.json" $f 2>"$S/c-$g.err"; c=$?
    [ "$c" -eq 0 ] || { no "$g (C rc=$c) $(tail -1 "$S/c-$g.err")"; continue; }
    "$B" 55 python3 exec/build/gen.py pp "$S/py-$g.json" $f 2>"$S/py-$g.err"; p=$?
    [ "$p" -eq 0 ] || { no "$g (Python rc=$p) $(tail -1 "$S/py-$g.err")"; continue; }
    if cmp -s "$S/c-$g.json" "$S/py-$g.json"; then ok "$g"; else no "$g"; fi
    "$B" 55 "$G" pp "$S/c-$g-b.json" $f 2>"$S/c-$g-b.err"; r=$?
    if [ "$r" -ne 0 ]; then no "$g-det (second run rc=$r)"
    elif cmp -s "$S/c-$g.json" "$S/c-$g-b.json"; then ok "$g-det"; else no "$g nondeterministic"; fi
done

if [ "$SHARD" = 1 ]; then
# negative 1: exec/pp/autoinc-result.tsv line 2 loses its last column in the SCRATCH copy
python3 - "$NEG" <<'PY' || { echo "seedpp: negative mutation failed"; exit 2; }
import sys
from pathlib import Path
p = Path(sys.argv[1])
lines = p.read_text().splitlines(True)
if len(lines) < 2 or lines[1].startswith("#") or "\t" not in lines[1]:
    raise SystemExit("line 2 is not a data row")
lines[1] = "\t".join(lines[1].rstrip("\n").split("\t")[:-1]) + "\n"
p.write_text("".join(lines))
PY
rm -f "$S/neg-py.json" "$S/neg-c.json"
"$B" 55 python3 exec/build/gen.py pp "$S/neg-py.json" 2>"$S/neg-py.err"; py_rc=$?
"$B" 55 "$G" pp "$S/neg-c.json" 2>"$S/neg-c.err"; c_rc=$?
neg0=$bad
refused "$py_rc" || no "NEG-tsv Python did not refuse normally (rc=$py_rc)"
refused "$c_rc" || no "NEG-tsv C did not refuse normally (rc=$c_rc)"
grep -q 'autoinc-result.tsv:2: expected four columns' "$S/neg-py.err" || no "NEG-tsv Python refuse not named ($(tail -1 "$S/neg-py.err"))"
grep -q 'seed-gen: plain rule column count' "$S/neg-c.err" || no "NEG-tsv C refuse not named ($(tail -1 "$S/neg-c.err"))"
[ ! -e "$S/neg-py.json" ] && [ ! -e "$S/neg-c.json" ] || no "NEG-tsv wrote output despite refuse"
[ "$bad" -eq "$neg0" ] && ok "NEG-tsv both refuse by name, no output (py_rc=$py_rc c_rc=$c_rc)"
evw "NEG-tsv rc py=$py_rc c=$c_rc"
tar -x -C "$W" -f "$S/tree.tar" "$NEG" || exit 2   # restore the scratch copy

# negative 2: --osx --win.  Python names the exclusion; C refuses through an uncovered let option (finding)
rm -f "$S/ow-py.json" "$S/ow-c.json"
"$B" 55 python3 exec/build/gen.py pp "$S/ow-py.json" --osx --win 2>"$S/ow-py.err"; py_rc=$?
"$B" 55 "$G" pp "$S/ow-c.json" --osx --win 2>"$S/ow-c.err"; c_rc=$?
neg0=$bad
refused "$py_rc" || no "NEG-osxwin Python did not refuse normally (rc=$py_rc)"
refused "$c_rc" || no "NEG-osxwin C did not refuse normally (rc=$c_rc)"
grep -q 'choose one OS' "$S/ow-py.err" || no "NEG-osxwin Python refuse not named ($(tail -1 "$S/ow-py.err"))"
grep -q 'manifest let option is not yet covered' "$S/ow-c.err" || no "NEG-osxwin C refuse changed ($(tail -1 "$S/ow-c.err"))"
[ ! -e "$S/ow-py.json" ] && [ ! -e "$S/ow-c.json" ] || no "NEG-osxwin wrote output despite refuse"
[ "$bad" -eq "$neg0" ] && ok "NEG-osxwin both refuse, no output (py_rc=$py_rc c_rc=$c_rc; C reason is an uncovered let option, not a named exclusion)"
evw "NEG-osxwin rc py=$py_rc c=$c_rc  kind py=semantic-exclusion c=unsupported-let (not a C exclusion check)"

# exec/pp/gen-delta.sh (the scratch copy): flags, no silent fallback, 0/1 only
D=$W/exec/pp/gen-delta.sh
gd() { out=$1; shift; rm -f "$out"; env "$@" sh "$D" "$out" 2>"$S/gd.err"; }
gda() { out=$1; shift; rm -f "$out"; env SEED_GEN=1 SEED_GEN_BIN="$G" sh "$D" "$out" "$@" 2>"$S/gd.err"; }
for bad_flags in "--o2" "--osx --osx" "--osx --win" "--win --arm64 --osx" "osx"; do
    gda "$S/bf.json" $bad_flags; r=$?
    { [ "$r" -eq 2 ] && [ ! -e "$S/bf.json" ]; } && ok "helper refuses '$bad_flags'" || no "helper did not refuse '$bad_flags' (rc=$r)"
done
gd "$S/fb.json" SEED_GEN=1 SEED_GEN_BIN=/nonexistent/seed-gen-k5-missing; r=$?
{ [ "$r" -ne 0 ] && [ ! -e "$S/fb.json" ]; } && ok "no-fallback missing BIN" || no "missing BIN fell back (rc=$r)"
printf '#!/bin/sh\necho seed-gen-red >&2\nexit 3\n' > "$S/red-gen"; chmod +x "$S/red-gen"
gd "$S/fr.json" SEED_GEN=1 SEED_GEN_BIN="$S/red-gen"; r=$?
{ [ "$r" -eq 3 ] && [ ! -e "$S/fr.json" ] && grep -q seed-gen-red "$S/gd.err"; } && ok "no-fallback failing BIN (rc 3 kept)" || no "failing BIN fell back or lost rc (rc=$r)"
for v in 2 yes ''; do
    gd "$S/sv.json" SEED_GEN="$v"; r=$?
    { [ "$r" -eq 2 ] && [ ! -e "$S/sv.json" ]; } && ok "SEED_GEN='$v' refused" || no "SEED_GEN='$v' not refused (rc=$r)"
done
gd "$S/py-only.json" SEED_GEN=0; r=$?
{ [ "$r" -eq 0 ] && cmp -s "$S/py-only.json" "$S/py-none.json"; } && ok "SEED_GEN=0 reaches Python" || no "SEED_GEN=0 (rc=$r)"
for g in $GROUPS_; do
    gda "$S/h-$g.json" $(flags "$g"); r=$?
    { [ "$r" -eq 0 ] && cmp -s "$S/h-$g.json" "$S/py-$g.json"; } && ok "helper-$g" || no "helper-$g (rc=$r)"
done

# fault branches of the export, each in a subshell with its own directories (the real export is not touched)
( EV=$S/fault-w; mkdir -p "$EV" && chmod a-w "$EV"; evfail=0; evw probe 2>/dev/null; [ "$evfail" = 1 ] ) && ok "fault: evidence write failure marks the run" || no "fault: evidence write failure not caught"
( EV=$S/fault-m; mkdir -p "$EV"; mono() { echo not-a-number; }; evfail=0; evmono probe 2>/dev/null; [ "$evfail" = 1 ] ) && ok "fault: bad timestamp marks the run" || no "fault: bad timestamp not caught"
( EV=$S/fault-j; mkdir -p "$EV" "$S/fault-empty"; S=$S/fault-empty; evfail=0; export_ev 2>/dev/null; [ $? -ne 0 ] && grep -q '^MISSING ' "$EV/json.sha256" ) && ok "fault: missing JSON fails the export" || no "fault: missing JSON not caught"
( mkdir -p "$S/fault-f0"; EV=; S=$S/fault-f0; evfail=1; finish 0 ) 2>/dev/null; r=$?
{ [ "$r" -eq 3 ] && [ -d "$S/fault-f0" ]; } && ok "fault: finish turns 0 into 3 and keeps the scratch" || no "fault: finish 0 -> $r"
( mkdir -p "$S/fault-f5"; EV=; S=$S/fault-f5; evfail=1; finish 5 ) 2>/dev/null; r=$?
[ "$r" -eq 5 ] && ok "fault: finish keeps a non-zero status" || no "fault: finish 5 -> $r"
fi

if [ "$SHARD" = 2 ]; then
# the default route (SEED_GEN_BIN unset) in one fresh directory, run with --arm64: cold build, cache hit, json.h
D=$W/exec/pp/gen-delta.sh
nbin() { ls "$S/seedbin" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}$'; }
nsid() { ls "$S/seedbin" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}\.sha256$'; }
snap() { python3 -c 'import hashlib,os,sys
d=sys.argv[1]
for f in sorted(os.listdir(d)):
    st=os.stat(os.path.join(d,f)); print(f, st.st_ino, st.st_mtime_ns, hashlib.sha256(open(os.path.join(d,f),"rb").read()).hexdigest())' "${1:-$S/seedbin}"; }
snap "$S/no-such-dir" >/dev/null 2>&1 && no "snapshot of a missing directory did not fail" || ok "snapshot failure is visible"
dg() { out=$1; rm -f "$out"; env -u SEED_GEN_BIN SEED_GEN=1 SEED_GEN_DIR="$S/seedbin" sh "$D" "$out" --arm64 2>"$S/gd.err"; }
rm -rf "$S/seedbin"
dg "$S/cold.json"; r=$?
{ [ "$r" -eq 0 ] && [ "$(nbin)" -eq 1 ] && [ "$(nsid)" -eq 1 ] && cmp -s "$S/cold.json" "$S/py-arm64.json"; } && ok "default-route cold build" || no "default-route cold build (rc=$r bins $(nbin) sidecars $(nsid))"
hit_ok() {
    a0=$($1); s0=$?
    $2; r=$?
    a1=$($1); s1=$?
    [ "$s0" -eq 0 ] && [ "$s1" -eq 0 ] && [ "$r" -eq 0 ] && [ -n "$a0" ] && [ "$a1" = "$a0" ]
}
part_fail() { echo "seed-gen-0000000000000000 1 1 x"; return 1; }
empty_ok() { return 0; }
noop() { return 0; }
hit_ok part_fail noop && no "hit_ok accepted a snapshot that printed then failed" || ok "hit_ok rejects partial output + failure"
hit_ok empty_ok noop && no "hit_ok accepted an empty snapshot" || ok "hit_ok rejects an empty snapshot"
hit_dg() { dg "$S/hit.json"; }
hit_ok snap hit_dg && cmp -s "$S/hit.json" "$S/py-arm64.json" && ok "default-route cache hit" || no "default-route cache hit (action rc=$r snapshot rc $s0/$s1)"
printf '\n/* seedpp: json.h cache probe */\n' >> seed/json.h   # the scratch copy
dg "$S/hdr.json"; r=$?
{ [ "$r" -eq 0 ] && [ "$(nbin)" -eq 2 ] && [ "$(nsid)" -eq 2 ] && cmp -s "$S/hdr.json" "$S/py-arm64.json"; } && ok "json.h-only rebuild" || no "json.h-only rebuild (rc=$r bins $(nbin) sidecars $(nsid))"
fi

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$NEG") || exit 2
[ "$before" = "$after" ] && ok "checkout $NEG untouched" || no "checkout $NEG changed"
echo "seedpp  shard $SHARD  same $same  bad $bad"
evw "seedpp  shard $SHARD  same $same  bad $bad  tree $H"
[ "$bad" = 0 ] && [ "$same" -gt 0 ] && [ "$evfail" = 0 ]
