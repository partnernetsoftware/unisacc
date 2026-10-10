#!/bin/sh
# K5-1g (机房主任 04:09): exec/pipeline/prepare.sh's prune step goes through exec/prune/gen-delta.sh and shares the
# private SEED_GEN_DIR="$OUT/.seed-gen-cache" with opt (before it) and pp (after it).  Controlled, lnx/x86_64,
# NETWORK=0.  Shims first on PATH:
#  * sh: a call whose first argument is exec/{parse2gen,opt,prune,pp}/gen-delta.sh is logged (stage, full argv), run with the
#    real shell, its rc logged, and AFTER it returns the cache is snapshotted into $SNAPDIR/after-<stage> (name,
#    inode, mtime_ns, sha256; ABSENT only when the directory does not exist; snapshot rc into after-<stage>.rc; a
#    failed snapshot ends the call with 95).  NEG_PRUNE_BIN=<path> makes only the prune helper run with that
#    SEED_GEN_BIN.  Every other sh call goes to the real shell unchanged.
#  * python3: passes "-", "-c", tests/bound.py and gen.py opt / prune / pp (logged pass-gen-<stage>); short-circuits
#    gen.py lex/parse2/lower/enc/enc/arm and exec/c/tbl.py; refuses anything else, an unknown gen.py stage included.
#  * cc: stubs exec/c/run.c; passes seed/gen.c (logged pass-gen-c), tests/bound.c and a lone --version; refuses else.
# 0.0.40 K5-1h (机房主任 06:01): parse2 now runs first through exec/parse2gen/gen-delta.sh and builds the cache cold;
# gen.py parse2 stays short-circuited here (the parse2 consumer is tests/seedparse2consumercheck.sh).
# Checks: positive (helpers parse2 -> opt -> prune -> pp with exact argv and rc 0; after-parse2 = after-opt = after-prune = after-pp, each rc 0,
# exactly the binary and its sidecar; one seed/gen.c compile; prune.json = seed-gen prune); prune-only red and missing
# BIN (opt green, prune fails, prepare stops before pp); helper argument refusals; the old direct gen.py prune line;
# SEED_GEN=0 reference; shim refusals.  Schema: /tmp/cc40-prep/k5-1g/schema.md.
# usage: tests/seedpruneconsumercheck.sh 1|2|3   (0.0.40 K5-1h 机房主任 06:12: three slices, each <= 58 s; every case keeps its own OUT and
#   builds seed-gen cold -- no shared SEED_GEN_BIN / SEED_GEN_DIR)
set -u
case "${1:-}" in 1|2|3) SLICE=$1;; *) echo "usage: tests/seedpruneconsumercheck.sh 1|2|3"; exit 2;; esac
case $SLICE in 1) JSONS="out-pos/prune.json";; 2) JSONS=;; 3) JSONS="out-sg0/prune.json c3/prune.json";; esac
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
_td=${TMPDIR:-/tmp}; S=$(mktemp -d "${_td%/}/seedprunec.XXXXXX") || exit 2   # TMPDIR=/tmp/ would leave // in $W and the trajectory
# 0.0.40 (机房主任 00:04): SEEDPRUNEC_EVIDENCE=DIR (a new or empty directory outside the scratch) keeps the evidence
# the scratch would lose: every SAME/DIFF line, the o1/o2 JSON sha256s and the monotonic start/end (ns), written
# before the scratch is removed -- also on a failing or interrupted run.
EV=${SEEDPRUNEC_EVIDENCE:-}
# 0.0.40 (机房主任 00:25): the export fails closed.  Any failed write, timestamp or hash sets evfail; a run with
# evfail is never green (exit 3 when it would have been 0, the original status otherwise), and its scratch is kept
# (path printed) instead of being removed without a record.
evfail=0
mono() { python3 -c 'import time;print(time.monotonic_ns())'; }
evw() {   # evw LINE: append one line to lines.txt, or mark the export failed
    [ -n "$EV" ] || return 0
    printf '%s\n' "$1" >> "$EV/lines.txt" || { evfail=1; echo "seedprunec: EVIDENCE write failed: $1" >&2; }
}
evmono() {   # evmono KEY: one monotonic stamp, checked to be a number
    [ -n "$EV" ] || return 0
    t=$(mono) && case "$t" in ''|*[!0-9]*) false;; *) true;; esac && printf '%s %s\n' "$1" "$t" >> "$EV/mono.txt" \
        || { evfail=1; echo "seedprunec: EVIDENCE timestamp $1 failed" >&2; }
}
if [ -n "$EV" ]; then
    case "$EV" in "$S"|"$S"/*) echo "seedprunec: SEEDPRUNEC_EVIDENCE inside the scratch"; exit 2;; esac
    mkdir -p "$EV" && [ -z "$(ls -A "$EV")" ] || { echo "seedprunec: SEEDPRUNEC_EVIDENCE must be a new or empty directory: $EV"; exit 2; }
    evmono start_ns; [ "$evfail" = 0 ] || exit 2
fi
export_ev() {   # hashes of the four JSON files (all four must exist and hash) and the end stamp
    [ -n "$EV" ] || return 0
    rc=0
    for f in $JSONS; do
        if [ -f "$S/$f" ] && h=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$S/$f") \
            && [ ${#h} -eq 64 ]; then line="$h $f"; else line="MISSING $f"; rc=1; fi
        printf '%s\n' "$line" >> "$EV/json.sha256" || rc=1
    done
    evmono end_ns; [ "$evfail" = 0 ] || rc=1
    return $rc
}
finish() {
    st=$1
    export_ev || evfail=1
    if [ "$evfail" != 0 ]; then
        echo "seedprunec: EVIDENCE INCOMPLETE -- not green; scratch kept at $S" >&2
        [ -z "$EV" ] || printf 'EVIDENCE INCOMPLETE status %s scratch %s\n' "$st" "$S" >> "$EV/lines.txt" 2>/dev/null
        [ "$st" -ne 0 ] || st=3
    else
        rm -rf "$S"
    fi
    exit "$st"
}
trap 'finish $?' EXIT
trap 'exit 130' INT TERM HUP
PREP=exec/pipeline/prepare.sh
before=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$PREP") || exit 2
W=$S/tree; mkdir "$W"
H=$(git rev-parse --verify HEAD) || exit 2
echo "seedprunec  tree $H"
git archive -o "$S/tree.tar" "$H" exec seed unisa include tests/bound tests/bound.c tests/bound.py || { echo "seedprunec: git archive failed"; exit 2; }
tar -x -C "$W" -f "$S/tree.tar" || { echo "seedprunec: scratch tree failed"; exit 2; }
cd "$W" || exit 2
BOUND_CACHE=$S/boundcache; export BOUND_CACHE
REALPY=$(command -v python3) && REALCC=$(command -v cc) && REALSH=$(command -v sh) || { echo "seedprunec: python3, cc or sh missing"; exit 2; }
mkdir "$S/shim"
cat > "$S/shim/sh" <<SHIM || exit 2
#!$REALSH
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "sh shim: log write failed" >&2; exit 96; }; }
case "\${1:-}" in
    */exec/parse2gen/gen-delta.sh) st=parse2;;
    */exec/opt/gen-delta.sh) st=opt;;
    */exec/prune/gen-delta.sh) st=prune;;
    */exec/pp/gen-delta.sh) st=pp;;
    *) exec "$REALSH" "\$@";;
esac
log "helper-\$st" "\$*"
if [ "\$st" = prune ] && [ -n "\${NEG_PRUNE_BIN:-}" ]; then SEED_GEN_BIN=\$NEG_PRUNE_BIN; export SEED_GEN_BIN; fi
"$REALSH" "\$@"; hr=\$?
log "helper-\$st-rc" "\$hr"
d=\$(dirname "\$2")/.seed-gen-cache
"$REALPY" -c 'import hashlib,os,sys
d=sys.argv[1]
try: names=sorted(os.listdir(d))
except FileNotFoundError: print("ABSENT"); sys.exit(0)
for f in names:
    st=os.stat(os.path.join(d,f)); print(f, st.st_ino, st.st_mtime_ns, hashlib.sha256(open(os.path.join(d,f),"rb").read()).hexdigest())' "\$d" > "\$SNAPDIR/after-\$st"
sr=\$?; echo "\$sr" > "\$SNAPDIR/after-\$st.rc" || exit 95
[ "\$sr" -eq 0 ] || { echo "sh shim: snapshot after \$st failed" >&2; exit 95; }
exit "\$hr"
SHIM
cat > "$S/shim/python3" <<SHIM || exit 2
#!$REALSH
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "python3 shim: log write failed" >&2; exit 96; }; }
case "\${1:-}" in
    -|-c) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    */tests/bound.py|tests/bound.py) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    exec/build/gen.py)
        case "\${2:-}" in
            opt|prune|pp) log "pass-gen-\$2" "python3 \$*"; exec "$REALPY" "\$@";;
            lex|parse2|lower|enc|enc/arm) log short "python3 \$*"; : > "\$3"; exit 0;;
        esac;;
    exec/c/tbl.py) log short "python3 \$*"; : > "\$3"; exit 0;;
esac
log UNKNOWN "python3 \$*"; exit 97
SHIM
cat > "$S/shim/cc" <<SHIM || exit 2
#!$REALSH
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "cc shim: log write failed" >&2; exit 96; }; }
srcs=0; run=; gen=; bnd=
for a in "\$@"; do
    case "\$a" in *.c) srcs=\$((srcs + 1));; esac
    case "\$a" in exec/c/run.c|*/exec/c/run.c) run=1;; seed/gen.c|*/seed/gen.c) gen=1;; tests/bound.c|*/tests/bound.c) bnd=1;; esac
done
if [ "\$#" -eq 1 ] && [ "\$1" = --version ]; then log pass "cc \$*"; exec "$REALCC" "\$@"; fi
if [ "\$srcs" -eq 1 ] && [ -n "\$run" ]; then
    o=; prev=; for a in "\$@"; do [ "\$prev" = -o ] && o=\$a; prev=\$a; done
    [ -n "\$o" ] || { log UNKNOWN "cc \$*"; exit 97; }
    log short "cc \$*"; printf '#!/bin/sh\nexit 0\n' > "\$o" && chmod +x "\$o"; exit \$?
fi
if [ "\$srcs" -eq 1 ] && [ -n "\$gen" ]; then log pass-gen-c "cc \$*"; exec "$REALCC" "\$@"; fi
if [ "\$srcs" -eq 1 ] && [ -n "\$bnd" ]; then log pass "cc \$*"; exec "$REALCC" "\$@"; fi
log UNKNOWN "cc \$*"; exit 97
SHIM
chmod +x "$S/shim/sh" "$S/shim/python3" "$S/shim/cc"
same=0; bad=0
ok() { echo "SAME $1"; evw "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; evw "DIFF $1"; bad=$((bad + 1)); }
haslog() { awk -F '\t' -v c="$2" -v a="$3" '$1 == c && $2 == a { f = 1 } END { exit !f }' "$1"; }
hasclass() { awk -F '\t' -v c="$2" '$1 == c { f = 1 } END { exit !f }' "$1"; }
count() { awk -F '\t' -v c="$2" -v a="$3" '$1 == c && (a == "" || $2 == a) { n++ } END { print n + 0 }' "$1"; }
helpers() { awk -F '\t' '$1 ~ /^helper-(parse2|opt|prune|pp)$/ { sub(/^helper-/, "", $1); printf "%s ", $1 }' "$1"; }
KEYSTEP="python3 - cc -std=c99 -O2 -w"
prep() {   # prep NAME [SCRIPT [ENV...]]: prepare.sh OUT lnx/x86_64 0 cc under the shims; prints rc
    [ $# -ge 1 ] && [ -n "$1" ] || { echo "seedprunec: prep needs a NAME" >&2; exit 2; }
    n=$1; shift
    sc=$PREP; if [ $# -gt 0 ]; then sc=$1; shift; fi
    o=$S/out-$n; rm -rf "$o" "$S/snap-$n"; mkdir -p "$o" "$S/snap-$n"; : > "$S/log-$n"
    env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC -u SEED_GEN_DIR -u NEG_PRUNE_BIN PATH="$S/shim:$PATH" SHIMLOG="$S/log-$n" \
        SNAPDIR="$S/snap-$n" "$@" "$S/shim/sh" "$sc" "$o" lnx/x86_64 0 cc > "$S/log-$n.out" 2>&1; echo $?
}
# shim refusals (no generator)
SHIMLOG=$S/shimneg; export SHIMLOG; : > "$SHIMLOG"
if [ "$SLICE" = 1 ]; then
for argv in "exec/c/other.py" "exec/build/gen.py nosuchstage /dev/null"; do
    "$S/shim/python3" $argv >/dev/null 2>&1; r=$?
    [ "$r" -eq 97 ] && ok "python3 shim refuses '$argv'" || no "python3 shim did not refuse '$argv' (rc=$r)"
done
"$S/shim/cc" -c foo.c >/dev/null 2>&1; r=$?
[ "$r" -eq 97 ] && ok "cc shim refuses an unknown source" || no "cc shim did not refuse (rc=$r)"
fi
unset SHIMLOG

two() { [ "$(cat "$1.rc" 2>/dev/null)" = 0 ] && [ "$(wc -l < "$1")" -eq 2 ] \
    && [ "$(grep -c '^seed-gen-[0-9a-f]\{16\} ' "$1")" -eq 1 ] && [ "$(grep -c '^seed-gen-[0-9a-f]\{16\}\.sha256 ' "$1")" -eq 1 ]; }
shaf() { python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$1" 2>/dev/null || echo none; }

if [ "$SLICE" = 1 ]; then
# 1 positive
r=$(prep pos)
o=$S/out-pos; l=$S/log-pos; sd=$S/snap-pos
evw "POS rc=$r"
{ [ "$r" = 0 ] && [ -s "$l" ] && [ -s "$o/e4.json" ] && [ -s "$o/prune.json" ] && [ -s "$o/e2.json" ] && ! hasclass "$l" UNKNOWN; } \
    && ok "prepare rc 0; e4.json, prune.json, e2.json made; no unknown call" || no "prepare (rc=$r) $(tail -1 "$l.out")"
hs=$(helpers "$l")
evw "POS helpers=$hs"
{ [ "$hs" = "parse2 opt prune pp " ] \
    && haslog "$l" helper-parse2 "$W/exec/parse2gen/gen-delta.sh $o/e3.json" && haslog "$l" helper-parse2-rc 0 \
    && haslog "$l" helper-opt "$W/exec/opt/gen-delta.sh $o/e4.json --o2" && haslog "$l" helper-opt-rc 0 \
    && haslog "$l" helper-prune "$W/exec/prune/gen-delta.sh $o/prune.json" && haslog "$l" helper-prune-rc 0 \
    && haslog "$l" helper-pp "$W/exec/pp/gen-delta.sh $o/e2.json" && haslog "$l" helper-pp-rc 0; } \
    && ok "helpers in order parse2 -> opt -> prune -> pp, exact argv, each rc 0" || no "helper trajectory '$hs'"
nc=$(count "$l" pass-gen-c ""); nk=$(count "$l" pass "$KEYSTEP")
evw "POS compiles=$nc keysteps=$nk"
[ "$nc" -eq 1 ] && ok "seed/gen.c compiled exactly once" || no "seed/gen.c compiles=$nc"
evw "POS snap_rc parse2=$(cat "$sd/after-parse2.rc" 2>/dev/null || echo none) opt=$(cat "$sd/after-opt.rc" 2>/dev/null || echo none) prune=$(cat "$sd/after-prune.rc" 2>/dev/null || echo none) pp=$(cat "$sd/after-pp.rc" 2>/dev/null || echo none)"
{ two "$sd/after-parse2" && two "$sd/after-opt" && two "$sd/after-prune" && two "$sd/after-pp" \
    && cmp -s "$sd/after-parse2" "$sd/after-opt" && cmp -s "$sd/after-opt" "$sd/after-prune" && cmp -s "$sd/after-prune" "$sd/after-pp"; } \
    && ok "after-parse2 = after-opt = after-prune = after-pp: one binary and its sidecar, same inode/mtime/sha (opt, prune and pp reused it)" \
    || no "cache snapshots differ or are not exactly two members"
{ ! hasclass "$l" pass-gen-opt && ! hasclass "$l" pass-gen-prune && ! hasclass "$l" pass-gen-pp; } && ok "no Python generator call" || no "Python generator called"
g=$(ls "$o/.seed-gen-cache"/seed-gen-* 2>/dev/null | grep -v '\.sha256$' | head -1)
rr=none; rm -f "$S/ref-prune.json"
[ -n "$g" ] && { "$W/tests/bound" 55 "$g" prune "$S/ref-prune.json" 2>"$S/ref-prune.err"; rr=$?; }
evw "POS ref_rc=$rr"
{ [ "$rr" = 0 ] && cmp -s "$o/prune.json" "$S/ref-prune.json"; } && ok "prune.json = seed-gen prune (reference rc 0)" || no "prune.json vs seed-gen prune (reference rc=$rr)"

evw "C_PRUNE_SHA $(shaf "$o/prune.json")"
fi

if [ "$SLICE" = 2 ]; then
# 2 prune-only failures: opt green, prune fails, prepare stops before pp
printf '#!/bin/sh\necho seed-gen-red >&2\nexit 3\n' > "$S/red-gen"; chmod +x "$S/red-gen"
for case_ in "red:3:$S/red-gen" "missing:2:/nonexistent/seed-gen"; do
    n=${case_%%:*}; rest=${case_#*:}; want=${rest%%:*}; bin=${rest#*:}
    r=$(prep "nprune-$n" "$PREP" NEG_PRUNE_BIN="$bin")
    l=$S/log-nprune-$n; o=$S/out-nprune-$n
    hs=$(helpers "$l")
    { [ "$r" = "$want" ] && [ "$hs" = "parse2 opt prune " ] && haslog "$l" helper-parse2-rc 0 && haslog "$l" helper-opt-rc 0 && [ -s "$o/e4.json" ] \
        && haslog "$l" helper-prune-rc "$want" && [ ! -e "$o/prune.json" ] && [ ! -e "$o/e2.json" ] \
        && ! awk -F '\t' '$2 ~ /^python3 exec\/build\/gen.py (lower|enc) / { f = 1 } END { exit !f }' "$l" \
        && ! hasclass "$l" pass-gen-opt && ! hasclass "$l" pass-gen-prune && ! hasclass "$l" pass-gen-pp; } \
        && ok "prune-only $n: parse2 and opt rc 0 with e4.json, prune helper rc $want, prepare rc $r before pp, no prune.json/e2.json, no Python" \
        || no "prune-only $n (prepare rc=$r want $want, helpers '$hs') $(tail -1 "$l.out")"
    evw "NEG-prune-$n prepare=$r helpers=$hs"
done

# 3 helper argument refusals (scratch copy; outputs cleared first)
D=$W/exec/prune/gen-delta.sh
for args in "--x" "--o2"; do
    rm -f "$S/ha.json"; env -u SEED_GEN -u SEED_GEN_DIR SEED_GEN_BIN="$S/red-gen" "$REALSH" "$D" "$S/ha.json" $args 2>/dev/null; r=$?
    { [ "$r" -eq 2 ] && [ ! -e "$S/ha.json" ]; } && ok "helper refuses OUT $args (rc 2, no output)" || no "helper did not refuse OUT $args (rc=$r)"
done
env -u SEED_GEN "$REALSH" "$D" 2>/dev/null; r=$?
[ "$r" -eq 2 ] && ok "helper refuses no argument (rc 2)" || no "helper did not refuse no argument (rc=$r)"
for v in 2 yes; do
    rm -f "$S/sv.json"; env SEED_GEN="$v" "$REALSH" "$D" "$S/sv.json" 2>/dev/null; r=$?
    { [ "$r" -eq 2 ] && [ ! -e "$S/sv.json" ]; } && ok "helper refuses SEED_GEN='$v'" || no "helper did not refuse SEED_GEN='$v' (rc=$r)"
done
rm -f "$S/sv.json"; env SEED_GEN= "$REALSH" "$D" "$S/sv.json" 2>/dev/null; r=$?
{ [ "$r" -eq 2 ] && [ ! -e "$S/sv.json" ]; } && ok "helper refuses SEED_GEN set but empty (unset is the default route, covered by the positive)" || no "helper did not refuse empty SEED_GEN (rc=$r)"

fi

if [ "$SLICE" = 3 ]; then
# 4 the old direct gen.py prune line is caught
python3 - "$PREP" exec/pipeline/prepare-oldprune.sh <<'PY' || { echo "seedprunec: mutation failed"; exit 2; }
import sys
s = open(sys.argv[1]).read()
new = 'b env SEED_GEN_DIR="$OUT/.seed-gen-cache" sh "$R/exec/prune/gen-delta.sh" "$OUT/prune.json"\n'
if s.count(new) != 1: raise SystemExit("prune line not found exactly once")
open(sys.argv[2], "w").write(s.replace(new, 'b python3 exec/build/gen.py prune "$OUT/prune.json"\n'))
PY
r=$(prep mut exec/pipeline/prepare-oldprune.sh)
hs=$(helpers "$S/log-mut")
{ [ "$r" = 0 ] && [ -s "$S/log-mut" ] && haslog "$S/log-mut" pass-gen-prune "python3 exec/build/gen.py prune $S/out-mut/prune.json" && [ "$hs" = "parse2 opt pp " ]; } \
    && ok "mutant (old direct gen.py prune) is caught: Python prune logged, no prune helper" || no "mutant not caught (rc=$r helpers '$hs')"
evw "MUTANT rc=$r helpers=$hs"
# 5 SEED_GEN=0 named reference
r=$(prep sg0 "$PREP" SEED_GEN=0)
l=$S/log-sg0; sd=$S/snap-sg0
{ [ "$r" = 0 ] && [ "$(helpers "$l")" = "parse2 opt prune pp " ] && haslog "$l" pass-gen-prune "python3 exec/build/gen.py prune $S/out-sg0/prune.json" \
    && haslog "$l" pass-gen-opt "python3 exec/build/gen.py opt $S/out-sg0/e4.json --o2" && haslog "$l" pass-gen-pp "python3 exec/build/gen.py pp $S/out-sg0/e2.json" \
    && [ "$(cat "$sd/after-prune" 2>/dev/null)" = ABSENT ] && [ "$(cat "$sd/after-prune.rc" 2>/dev/null)" = 0 ] \
    ; } \
    && ok "SEED_GEN=0 reference: helpers -> Python (parse2 short-circuited), no cache" || no "SEED_GEN=0 reference (rc=$r)"
evw "SEEDGEN0 rc=$r"

# the C side in this slice: the prune helper itself, default route, private cache in the scratch (cold build)
mkdir -p "$S/c3"
env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC SEED_GEN_DIR="$S/c3/cache" "$REALSH" "$W/exec/prune/gen-delta.sh" "$S/c3/prune.json" 2>"$S/c3.err"; cr=$?
evw "C3 rc=$cr"
evw "C_PRUNE_SHA $(shaf "$S/c3/prune.json")"
{ [ "$cr" = 0 ] && cmp -s "$S/out-sg0/prune.json" "$S/c3/prune.json"; } && ok "SEED_GEN=0 prune.json byte-equal to the C prune built in this slice (helper rc 0)" || no "SEED_GEN=0 prune.json vs C (helper rc=$cr)"
fi

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$PREP") || exit 2
[ "$before" = "$after" ] && ok "checkout $PREP untouched" || no "checkout $PREP changed"
if [ -n "$EV" ]; then
    case $SLICE in
        1) files="shimneg log-pos log-pos.out snap-pos/after-parse2 snap-pos/after-parse2.rc snap-pos/after-opt snap-pos/after-opt.rc snap-pos/after-prune snap-pos/after-prune.rc snap-pos/after-pp snap-pos/after-pp.rc ref-prune.err";;
        2) files="log-nprune-red log-nprune-red.out log-nprune-missing log-nprune-missing.out";;
        3) files="log-mut log-mut.out log-sg0 log-sg0.out snap-sg0/after-prune snap-sg0/after-prune.rc c3.err";;
    esac
    for f in $files; do
        mkdir -p "$EV/$(dirname "$f")" && cp "$S/$f" "$EV/$f" || { evfail=1; echo "seedprunec: EVIDENCE copy of $f failed" >&2; }
    done
fi
echo "seedprunec  slice $SLICE  same $same  bad $bad"
evw "seedprunec  slice $SLICE  same $same  bad $bad  tree $H"
[ "$bad" = 0 ] && [ "$same" -gt 0 ] && [ "$evfail" = 0 ]
