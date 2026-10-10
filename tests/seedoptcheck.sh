#!/bin/sh
# K5-1: opt δ — seed/gen.c vs exec/build/gen.py byte-identical; a corrupted TSV is refused by name by both;
# exec/opt/gen-delta.sh prefers seed-gen with no silent Python fallback.  Does not change exec/opt/check.sh.
# 0.0.40 (机房主任 23:41): everything runs in a private scratch tree (git archive of HEAD), never the checkout:
# the negative mutates the scratch copy only, an EXIT/INT/TERM/HUP trap removes the scratch, and the checkout's
# rounds-result.tsv is hashed before and after (conserved).  23:47: default-route cache key is seed/*.[ch]
# content hash (not gen.c mtime); a json.h-only edit in the scratch must rebuild.  usage: tests/seedoptcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
S=$(mktemp -d "${TMPDIR:-/tmp}/seedopt.XXXXXX") || exit 2
# 0.0.40 (机房主任 00:04): SEEDOPT_EVIDENCE=DIR (a new or empty directory outside the scratch) keeps the evidence
# the scratch would lose: every SAME/DIFF line, the o1/o2 JSON sha256s and the monotonic start/end (ns), written
# before the scratch is removed -- also on a failing or interrupted run.
EV=${SEEDOPT_EVIDENCE:-}
# 0.0.40 (机房主任 00:25): the export fails closed.  Any failed write, timestamp or hash sets evfail; a run with
# evfail is never green (exit 3 when it would have been 0, the original status otherwise), and its scratch is kept
# (path printed) instead of being removed without a record.
evfail=0
mono() { python3 -c 'import time;print(time.monotonic_ns())'; }
evw() {   # evw LINE: append one line to lines.txt, or mark the export failed
    [ -n "$EV" ] || return 0
    printf '%s\n' "$1" >> "$EV/lines.txt" || { evfail=1; echo "seedopt: EVIDENCE write failed: $1" >&2; }
}
evmono() {   # evmono KEY: one monotonic stamp, checked to be a number
    [ -n "$EV" ] || return 0
    t=$(mono) && case "$t" in ''|*[!0-9]*) false;; *) true;; esac && printf '%s %s\n' "$1" "$t" >> "$EV/mono.txt" \
        || { evfail=1; echo "seedopt: EVIDENCE timestamp $1 failed" >&2; }
}
if [ -n "$EV" ]; then
    case "$EV" in "$S"|"$S"/*) echo "seedopt: SEEDOPT_EVIDENCE inside the scratch"; exit 2;; esac
    mkdir -p "$EV" && [ -z "$(ls -A "$EV")" ] || { echo "seedopt: SEEDOPT_EVIDENCE must be a new or empty directory: $EV"; exit 2; }
    evmono start_ns; [ "$evfail" = 0 ] || exit 2
fi
export_ev() {   # hashes of the four JSON files (all four must exist and hash) and the end stamp
    [ -n "$EV" ] || return 0
    rc=0
    for f in c-o1 c-o2 py-o1 py-o2; do
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
        echo "seedopt: EVIDENCE INCOMPLETE -- not green; scratch kept at $S" >&2
        [ -z "$EV" ] || printf 'EVIDENCE INCOMPLETE status %s scratch %s\n' "$st" "$S" >> "$EV/lines.txt" 2>/dev/null
        [ "$st" -ne 0 ] || st=3
    else
        rm -rf "$S"
    fi
    exit "$st"
}
trap 'finish $?' EXIT
trap 'exit 130' INT TERM HUP
NEG=exec/opt/rounds-result.tsv
before=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$NEG") || exit 2
W=$S/tree; mkdir "$W"
# archive to a file first: a failing git archive must not hide behind tar's status
H=$(git rev-parse --verify HEAD) || exit 2   # the one commit this run is about; every copy comes from it
echo "seedopt  tree $H"
git archive -o "$S/tree.tar" "$H" exec seed unisa tests/bound tests/bound.c tests/bound.py || { echo "seedopt: git archive failed"; exit 2; }
tar -x -C "$W" -f "$S/tree.tar" || { echo "seedopt: scratch tree failed"; exit 2; }
cd "$W" || exit 2
B=$W/tests/bound
BOUND_CACHE=$S/boundcache; export BOUND_CACHE   # the bound helper is built inside the scratch, not the shared cache
G=$S/seed-gen
"$B" 55 cc -std=c99 -O2 -w -Iseed -o "$G" seed/gen.c || { echo "seedopt: gen.c does not build"; exit 1; }
same=0; bad=0
ok() { echo "SAME $1"; evw "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; evw "DIFF $1"; bad=$((bad + 1)); }
# a refusal is a normal non-zero exit: not 0, not a timeout (124/142) and not a signal (>= 128)
refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }

run_pair() {   # run_pair LABEL [--o2]
    label=$1; shift
    rm -f "$S/c-$label.json" "$S/py-$label.json"
    "$B" 55 "$G" opt "$S/c-$label.json" "$@" 2>"$S/c-$label.err"; c=$?
    [ "$c" -eq 0 ] || { no "$label (C rc=$c) $(tail -1 "$S/c-$label.err")"; return 0; }
    "$B" 55 python3 exec/build/gen.py opt "$S/py-$label.json" "$@" 2>"$S/py-$label.err"; p=$?
    [ "$p" -eq 0 ] || { no "$label (Python rc=$p) $(tail -1 "$S/py-$label.err")"; return 0; }
    if cmp -s "$S/c-$label.json" "$S/py-$label.json"; then ok "$label"; else no "$label"; fi
}
run_pair o1
run_pair o2 --o2

# deterministic double-run (C)
for l in o1 o2; do
    rm -f "$S/c-$l-b.json"
    if [ "$l" = o2 ]; then "$B" 55 "$G" opt "$S/c-$l-b.json" --o2 2>"$S/c-$l-b.err"; else "$B" 55 "$G" opt "$S/c-$l-b.json" 2>"$S/c-$l-b.err"; fi
    r=$?
    if [ "$r" -ne 0 ]; then no "$l-det (second run rc=$r)"
    elif cmp -s "$S/c-$l.json" "$S/c-$l-b.json"; then ok "$l-det"; else no "$l nondeterministic"; fi
done

# negative: drop one column of one data row in the SCRATCH copy -> both refuse by name, write nothing
python3 - "$NEG" <<'PY' || { echo "seedopt: negative mutation failed"; exit 2; }
import sys
from pathlib import Path
p = Path(sys.argv[1])
lines = p.read_text().splitlines(True)
for i, l in enumerate(lines):
    if l and not l.startswith("#") and "\t" in l:
        lines[i] = "\t".join(l.rstrip("\n").split("\t")[:-1]) + "\n"
        break
else:
    raise SystemExit("no data row to mutate")
p.write_text("".join(lines))
PY
rm -f "$S/neg-py.json" "$S/neg-c.json"
"$B" 55 python3 exec/build/gen.py opt "$S/neg-py.json" 2>"$S/neg-py.err"; py_rc=$?
"$B" 55 "$G" opt "$S/neg-c.json" 2>"$S/neg-c.err"; c_rc=$?
neg0=$bad
refused "$py_rc" || no "NEG Python did not refuse normally (rc=$py_rc)"
refused "$c_rc" || no "NEG C did not refuse normally (rc=$c_rc)"
grep -q 'expected section plus four columns' "$S/neg-py.err" || no "NEG Python refuse not named"
grep -q 'section rule column count' "$S/neg-c.err" || no "NEG C refuse not named ($(tail -1 "$S/neg-c.err"))"
[ ! -e "$S/neg-py.json" ] && [ ! -e "$S/neg-c.json" ] || no "NEG wrote output despite refuse"
# the negative passes by name with both child statuses, never only through bad 0
[ "$bad" -eq "$neg0" ] && ok "NEG both refuse by name, no output (py_rc=$py_rc c_rc=$c_rc)"
evw "NEG rc py=$py_rc c=$c_rc"
tar -x -C "$W" -f "$S/tree.tar" "$NEG" || exit 2   # restore the scratch copy for the helper checks

# gen-delta.sh (the scratch copy, so its R is the scratch tree): no silent fallback, 0/1 only, no extra flags
D=$W/exec/opt/gen-delta.sh
gd() { out=$1; shift; rm -f "$out"; env "$@" sh "$D" "$out" 2>"$S/gd.err"; }
gd "$S/fb.json" SEED_GEN=1 SEED_GEN_BIN=/nonexistent/seed-gen-k5-missing; r=$?
{ [ "$r" -ne 0 ] && [ ! -e "$S/fb.json" ]; } && ok "no-fallback missing BIN" || no "missing BIN fell back (rc=$r)"
printf '#!/bin/sh\necho seed-gen-red >&2\nexit 3\n' > "$S/red-gen"; chmod +x "$S/red-gen"
gd "$S/fr.json" SEED_GEN=1 SEED_GEN_BIN="$S/red-gen"; r=$?
{ [ "$r" -eq 3 ] && [ ! -e "$S/fr.json" ] && grep -q seed-gen-red "$S/gd.err"; } && ok "no-fallback failing BIN (rc 3 kept)" || no "failing BIN fell back or lost rc (rc=$r)"
for v in 2 yes ''; do
    gd "$S/sv.json" SEED_GEN="$v"; r=$?
    { [ "$r" -eq 2 ] && [ ! -e "$S/sv.json" ]; } && ok "SEED_GEN='$v' refused" || no "SEED_GEN='$v' not refused (rc=$r)"
done
rm -f "$S/xf.json"; env SEED_GEN=1 SEED_GEN_BIN="$G" sh "$D" "$S/xf.json" --o2 --extra 2>"$S/gd.err"; r=$?
{ [ "$r" -eq 2 ] && [ ! -e "$S/xf.json" ]; } && ok "extra flag refused" || no "extra flag not refused (rc=$r)"
gd "$S/py-only.json" SEED_GEN=0; r=$?
{ [ "$r" -eq 0 ] && cmp -s "$S/py-only.json" "$S/py-o1.json"; } && ok "SEED_GEN=0 reaches Python" || no "SEED_GEN=0 (rc=$r)"
for l in o1 o2; do
    rm -f "$S/h-$l.json"
    if [ "$l" = o2 ]; then env SEED_GEN=1 SEED_GEN_BIN="$G" sh "$D" "$S/h-$l.json" --o2; else env SEED_GEN=1 SEED_GEN_BIN="$G" sh "$D" "$S/h-$l.json"; fi
    r=$?; { [ "$r" -eq 0 ] && cmp -s "$S/h-$l.json" "$S/py-$l.json"; } && ok "helper-$l" || no "helper-$l (rc=$r)"
done
# the default route: SEED_GEN_BIN removed from the environment, so gen-delta must build its own keyed seed-gen
# into a fresh private directory (exactly one seed-gen-KEY with its .sha256 appears) and match the reference
for l in o1 o2; do
    rm -rf "$S/seedbin-$l" "$S/dflt-$l.json"
    if [ "$l" = o2 ]; then env -u SEED_GEN_BIN SEED_GEN=1 SEED_GEN_DIR="$S/seedbin-$l" sh "$D" "$S/dflt-$l.json" --o2
    else env -u SEED_GEN_BIN SEED_GEN=1 SEED_GEN_DIR="$S/seedbin-$l" sh "$D" "$S/dflt-$l.json"; fi
    r=$?; n=$(ls "$S/seedbin-$l" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}$')
    { [ "$r" -eq 0 ] && [ "$n" -eq 1 ] && cmp -s "$S/dflt-$l.json" "$S/py-$l.json"; } && ok "default-route $l (own keyed build)" || no "default-route $l (rc=$r builds=$n)"
done
# cache key is content of seed/*.[ch] (not gen.c mtime).  All three calls run with SEED_GEN_BIN unset in one
# fresh directory: the first builds cold (exactly one binary), the second must reuse it (same name, same bytes),
# a json.h-only edit (scratch copy) must build a second one.  Binaries and .sha256 sidecars are counted apart.
nbin() { ls "$S/seedbin" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}$'; }
nsid() { ls "$S/seedbin" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}\.sha256$'; }
# a rebuild replaces the file (rm, then mv of a new one): name, inode, mtime_ns and bytes together tell a reuse
# from a rebuild even when the compiler reproduces the same bytes
snap() { python3 -c 'import hashlib,os,sys
d=sys.argv[1]
for f in sorted(os.listdir(d)):
    st=os.stat(os.path.join(d,f)); print(f, st.st_ino, st.st_mtime_ns, hashlib.sha256(open(os.path.join(d,f),"rb").read()).hexdigest())' "${1:-$S/seedbin}"; }
# the snapshot's own failure must be visible: a missing directory gives a non-zero status (controlled stub)
snap "$S/no-such-dir" >/dev/null 2>&1 && no "snapshot of a missing directory did not fail" || ok "snapshot failure is visible"
dg() { out=$1; rm -f "$out"; env -u SEED_GEN_BIN SEED_GEN=1 SEED_GEN_DIR="$S/seedbin" sh "$D" "$out" 2>"$S/gd.err"; }
rm -rf "$S/seedbin"
dg "$S/cold.json"; r=$?
{ [ "$r" -eq 0 ] && [ "$(nbin)" -eq 1 ] && [ "$(nsid)" -eq 1 ] && cmp -s "$S/cold.json" "$S/py-o1.json"; } && ok "default-route cold build" || no "default-route cold build (rc=$r bins $(nbin) sidecars $(nsid))"
# hit_ok SNAP ACTION: both snapshots stored with their own status; red on a non-zero snapshot, a non-zero action,
# an empty snapshot, or a changed directory
hit_ok() {
    a0=$($1); s0=$?
    $2; r=$?
    a1=$($1); s1=$?
    [ "$s0" -eq 0 ] && [ "$s1" -eq 0 ] && [ "$r" -eq 0 ] && [ -n "$a0" ] && [ "$a1" = "$a0" ]
}
# call-site negatives (no generator): identical partial output followed by a failure, and an empty snapshot
part_fail() { echo "seed-gen-0000000000000000 1 1 x"; return 1; }
empty_ok() { return 0; }
noop() { return 0; }
hit_ok part_fail noop && no "hit_ok accepted a snapshot that printed then failed" || ok "hit_ok rejects partial output + failure"
hit_ok empty_ok noop && no "hit_ok accepted an empty snapshot" || ok "hit_ok rejects an empty snapshot"
hit_dg() { dg "$S/hit.json"; }
hit_ok snap hit_dg && cmp -s "$S/hit.json" "$S/py-o1.json" && ok "default-route cache hit" || no "default-route cache hit (action rc=$r snapshot rc $s0/$s1; directory changed, empty, or output differs)"
printf '\n/* seedopt: json.h cache probe */\n' >> seed/json.h   # the scratch copy (cwd is the scratch tree)
dg "$S/hdr.json"; r=$?
{ [ "$r" -eq 0 ] && [ "$(nbin)" -eq 2 ] && [ "$(nsid)" -eq 2 ] && cmp -s "$S/hdr.json" "$S/py-o1.json"; } && ok "json.h-only rebuild" || no "json.h-only rebuild (rc=$r bins $(nbin) sidecars $(nsid))"

# fault branches of the export, each in a subshell with its own directories (the real export is not touched):
# a failed write, a bad timestamp and a missing JSON each mark the run; finish turns a would-be 0 into 3, keeps a
# non-zero status, and keeps the scratch
( EV=$S/fault-w; mkdir -p "$EV" && chmod a-w "$EV"; evfail=0; evw probe 2>/dev/null; [ "$evfail" = 1 ] ) && ok "fault: evidence write failure marks the run" || no "fault: evidence write failure not caught"
( EV=$S/fault-m; mkdir -p "$EV"; mono() { echo not-a-number; }; evfail=0; evmono probe 2>/dev/null; [ "$evfail" = 1 ] ) && ok "fault: bad timestamp marks the run" || no "fault: bad timestamp not caught"
( EV=$S/fault-j; mkdir -p "$EV" "$S/fault-empty"; S=$S/fault-empty; evfail=0; export_ev 2>/dev/null; [ $? -ne 0 ] && grep -q '^MISSING c-o1.json$' "$EV/json.sha256" ) && ok "fault: missing JSON fails the export" || no "fault: missing JSON not caught"
( mkdir -p "$S/fault-f0"; EV=; S=$S/fault-f0; evfail=1; finish 0 ) 2>/dev/null; r=$?
{ [ "$r" -eq 3 ] && [ -d "$S/fault-f0" ]; } && ok "fault: finish turns 0 into 3 and keeps the scratch" || no "fault: finish 0 -> $r"
( mkdir -p "$S/fault-f5"; EV=; S=$S/fault-f5; evfail=1; finish 5 ) 2>/dev/null; r=$?
[ "$r" -eq 5 ] && ok "fault: finish keeps a non-zero status" || no "fault: finish 5 -> $r"

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$NEG") || exit 2
[ "$before" = "$after" ] && ok "checkout $NEG untouched" || no "checkout $NEG changed"
echo "seedopt  same $same  bad $bad"
evw "seedopt  same $same  bad $bad  tree $H"
[ "$bad" = 0 ] && [ "$same" -gt 0 ] && [ "$evfail" = 0 ]
