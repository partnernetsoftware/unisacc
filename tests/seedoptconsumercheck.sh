#!/bin/sh
# K5-1f (机房主任 03:43): exec/pipeline/prepare.sh's opt step goes through exec/opt/gen-delta.sh --o2 and shares the
# private SEED_GEN_DIR="$OUT/.seed-gen-cache" with the pp step after it.  Controlled check, lnx/x86_64, NETWORK=0:
#  * shims first on PATH.  python3: passes "-", "-c", tests/bound.py, gen.py opt, prune and pp to the real interpreter
#    (logged pass-gen-<stage>), short-circuits gen.py lex/parse2/lower/enc/enc/arm and exec/c/tbl.py
#    by touching their output, refuses anything else -- an unknown gen.py stage included -- with rc 97.
#    sh: a call whose first argument is exec/{opt,prune,pp}/gen-delta.sh is logged (helper-<stage>, full argv), the
#    cache $OUT/.seed-gen-cache is snapshotted (name, inode, mtime_ns, sha256; ABSENT when missing) into
#    $SNAPDIR/before-<stage> before and $SNAPDIR/after-<stage> after the real helper, each snapshot rc kept; the
#    helper rc is logged (helper-<stage>-rc).  Every other sh call goes to the real shell.  cc: stubs exec/c/run.c,
#    passes seed/gen.c (each compile logged pass-gen-c), tests/bound.c and a lone --version; refuses anything else.
#  * positive: prepare rc 0; helpers opt -> prune -> pp (0.0.40 K5-1g, 699d310d: prune shares the cache), each rc 0;
#    seed/gen.c compiled exactly once; helper key step three times, one per helper, in helper order; the cache
#    after opt equals the cache before and after prune and after pp (one binary + sidecar, same inode/mtime/sha:
#    prune and pp reused it); e4.json byte-equal to that seed-gen's "opt --o2"; no direct gen.py opt / prune / pp.
#  * negatives: red SEED_GEN_BIN (rc 3), missing SEED_GEN_BIN (rc 2), SEED_GEN=yes (rc 2) -- prepare fails with that
#    rc after the opt helper only (no prune / pp helper), no e4.json, no Python fallback; the old direct "gen.py opt"
#    line is caught (no opt helper, no cache before prune).
#  * SEED_GEN=0 named reference: helpers -> gen.py opt --o2 / prune / pp, no cache; e4.json byte-equal to the C one.
# usage: tests/seedoptconsumercheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
_td=${TMPDIR:-/tmp}; S=$(mktemp -d "${_td%/}/seedoptc.XXXXXX") || exit 2   # TMPDIR=/tmp/ would leave // in $W and the trajectory
# 0.0.40 (机房主任 00:04): SEEDOPTC_EVIDENCE=DIR (a new or empty directory outside the scratch) keeps the evidence
# the scratch would lose: every SAME/DIFF line, the o1/o2 JSON sha256s and the monotonic start/end (ns), written
# before the scratch is removed -- also on a failing or interrupted run.
EV=${SEEDOPTC_EVIDENCE:-}
# 0.0.40 (机房主任 00:25): the export fails closed.  Any failed write, timestamp or hash sets evfail; a run with
# evfail is never green (exit 3 when it would have been 0, the original status otherwise), and its scratch is kept
# (path printed) instead of being removed without a record.
evfail=0
mono() { python3 -c 'import time;print(time.monotonic_ns())'; }
evw() {   # evw LINE: append one line to lines.txt, or mark the export failed
    [ -n "$EV" ] || return 0
    printf '%s\n' "$1" >> "$EV/lines.txt" || { evfail=1; echo "seedoptc: EVIDENCE write failed: $1" >&2; }
}
evmono() {   # evmono KEY: one monotonic stamp, checked to be a number
    [ -n "$EV" ] || return 0
    t=$(mono) && case "$t" in ''|*[!0-9]*) false;; *) true;; esac && printf '%s %s\n' "$1" "$t" >> "$EV/mono.txt" \
        || { evfail=1; echo "seedoptc: EVIDENCE timestamp $1 failed" >&2; }
}
if [ -n "$EV" ]; then
    case "$EV" in "$S"|"$S"/*) echo "seedoptc: SEEDOPTC_EVIDENCE inside the scratch"; exit 2;; esac
    mkdir -p "$EV" && [ -z "$(ls -A "$EV")" ] || { echo "seedoptc: SEEDOPTC_EVIDENCE must be a new or empty directory: $EV"; exit 2; }
    evmono start_ns; [ "$evfail" = 0 ] || exit 2
fi
export_ev() {   # hashes of the four JSON files (all four must exist and hash) and the end stamp
    [ -n "$EV" ] || return 0
    rc=0
    for f in out-pos/e4.json out-sg0/e4.json; do
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
        echo "seedoptc: EVIDENCE INCOMPLETE -- not green; scratch kept at $S" >&2
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
echo "seedoptc  tree $H"
git archive -o "$S/tree.tar" "$H" exec seed unisa include tests/bound tests/bound.c tests/bound.py || { echo "seedoptc: git archive failed"; exit 2; }
tar -x -C "$W" -f "$S/tree.tar" || { echo "seedoptc: scratch tree failed"; exit 2; }
cd "$W" || exit 2
BOUND_CACHE=$S/boundcache; export BOUND_CACHE
REALPY=$(command -v python3) && REALCC=$(command -v cc) && REALSH=$(command -v sh) || { echo "seedoptc: python3, cc or sh missing"; exit 2; }
mkdir "$S/shim"
cat > "$S/shim/sh" <<SHIM || exit 2
#!$REALSH
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "sh shim: log write failed" >&2; exit 96; }; }
case "\${1:-}" in
    */exec/opt/gen-delta.sh) st=opt;;
    */exec/prune/gen-delta.sh) st=prune;;
    */exec/pp/gen-delta.sh) st=pp;;
    *) exec "$REALSH" "\$@";;
esac
snapc() {   # snapc WHEN: the cache next to the helper's output into \$SNAPDIR/WHEN-<stage> (+ .rc)
    "$REALPY" -c 'import hashlib,os,sys
d=sys.argv[1]
try: names=sorted(os.listdir(d))
except FileNotFoundError: print("ABSENT"); sys.exit(0)
for f in names:
    st=os.stat(os.path.join(d,f)); print(f, st.st_ino, st.st_mtime_ns, hashlib.sha256(open(os.path.join(d,f),"rb").read()).hexdigest())' "\$(dirname "\$2")/.seed-gen-cache" > "\$SNAPDIR/\$1-\$st"
    sr=\$?; echo "\$sr" > "\$SNAPDIR/\$1-\$st.rc" || exit 95
    [ "\$sr" -eq 0 ] || { echo "sh shim: snapshot \$1 \$st failed" >&2; exit 95; }
}
log "helper-\$st" "\$*"
snapc before "\$2"
"$REALSH" "\$@"; hr=\$?
log "helper-\$st-rc" "\$hr"
snapc after "\$2"
exit "\$hr"
SHIM
cat > "$S/shim/python3" <<SHIM || exit 2
#!/bin/sh
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
#!/bin/sh
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
helpers() { awk -F '\t' '$1 ~ /^helper-(opt|prune|pp)$/ { sub(/^helper-/, "", $1); printf "%s ", $1 }' "$1"; }
KEYSTEP="python3 - cc -std=c99 -O2 -w"
snap() { python3 -c 'import hashlib,os,sys
d=sys.argv[1]
for f in sorted(os.listdir(d)):
    st=os.stat(os.path.join(d,f)); print(f, st.st_ino, st.st_mtime_ns, hashlib.sha256(open(os.path.join(d,f),"rb").read()).hexdigest())' "$1"; }
prep() {   # prep NAME [SCRIPT [ENV...]]: prepare.sh OUT lnx/x86_64 0 cc under the shims; prints rc
    # explicit arguments: no "shift N" (a POSIX special-builtin error ends the subshell when N > $#)
    [ $# -ge 1 ] && [ -n "$1" ] || { echo "seedoptc: prep needs a NAME" >&2; exit 2; }
    n=$1; shift
    sc=$PREP; if [ $# -gt 0 ]; then sc=$1; shift; fi
    o=$S/out-$n; rm -rf "$o" "$S/snap-$n"; mkdir -p "$o" "$S/snap-$n"; : > "$S/log-$n"
    env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC -u SEED_GEN_DIR PATH="$S/shim:$PATH" SHIMLOG="$S/log-$n" SNAPDIR="$S/snap-$n" \
        "$@" "$S/shim/sh" "$sc" "$o" lnx/x86_64 0 cc > "$S/log-$n.out" 2>&1; echo $?
}
# the shims refuse what they do not know (an unknown gen.py stage included)
SHIMLOG=$S/shimneg; export SHIMLOG; : > "$SHIMLOG"
for argv in "exec/c/other.py" "exec/build/gen.py nosuchstage /dev/null"; do
    "$S/shim/python3" $argv >/dev/null 2>&1; r=$?
    [ "$r" -eq 97 ] && ok "python3 shim refuses '$argv'" || no "python3 shim did not refuse '$argv' (rc=$r)"
done
"$S/shim/cc" -c foo.c >/dev/null 2>&1; r=$?
[ "$r" -eq 97 ] && ok "cc shim refuses an unknown source" || no "cc shim did not refuse (rc=$r)"
unset SHIMLOG

# 1 positive
r=$(prep pos)
o=$S/out-pos; l=$S/log-pos; sd=$S/snap-pos
snap "$o/.seed-gen-cache" > "$S/snapend-pos"; se=$?
{ [ "$r" = 0 ] && [ -s "$l" ] && [ -s "$l.out" ] && [ -s "$o/e4.json" ] && [ -s "$o/prune.json" ] && [ -s "$o/e2.json" ] && ! hasclass "$l" UNKNOWN; } \
    && ok "prepare rc 0, e4.json, prune.json and e2.json made, no unknown call" || no "prepare (rc=$r) $(tail -1 "$l.out")"
evw "POS rc=$r"
hs=$(helpers "$l")
evw "POS helpers=$hs"
{ [ "$hs" = "opt prune pp " ] \
    && haslog "$l" helper-opt "$W/exec/opt/gen-delta.sh $o/e4.json --o2" && haslog "$l" helper-opt-rc 0 \
    && haslog "$l" helper-prune "$W/exec/prune/gen-delta.sh $o/prune.json" && haslog "$l" helper-prune-rc 0 \
    && haslog "$l" helper-pp "$W/exec/pp/gen-delta.sh $o/e2.json" && haslog "$l" helper-pp-rc 0; } \
    && ok "helpers opt -> prune -> pp with exact argv, each rc 0" || no "helpers '$hs'"
nc=$(count "$l" pass-gen-c ""); nk=$(count "$l" pass "$KEYSTEP")
{ [ "$nc" -eq 1 ] && [ "$nk" -eq 3 ]; } && ok "seed/gen.c compiled once; helper key step three times (opt, prune, pp)" || no "compiles=$nc key steps=$nk"
evw "POS compiles=$nc keysteps=$nk"
# event order: each helper call is followed by its own key step; the one compile follows the opt key step
ord=$(awk -F '\t' -v k="$KEYSTEP" '$1 == "helper-opt" { printf "O" } $1 == "helper-prune" { printf "R" } $1 == "helper-pp" { printf "P" }
    $1 == "pass" && $2 == k { printf "K" } $1 == "pass-gen-c" { printf "C" }' "$l")
[ "$ord" = OKCRKPK ] && ok "event order: opt key step + the one compile, then prune key step, then pp key step" || no "event order '$ord' (want OKCRKPK)"
evw "POS order=$ord"
src=; for f in before-opt after-opt before-prune after-prune before-pp after-pp; do src="$src $f=$(cat "$sd/$f.rc" 2>/dev/null || echo none)"; done
evw "POS snap_rc$src snapend_rc=$se"
two() { [ -s "$1" ] && [ "$(wc -l < "$1")" -eq 2 ] && [ "$(grep -c '^seed-gen-[0-9a-f]\{16\} ' "$1")" -eq 1 ] \
    && [ "$(grep -c '^seed-gen-[0-9a-f]\{16\}\.sha256 ' "$1")" -eq 1 ]; }
{ [ "$src" = " before-opt=0 after-opt=0 before-prune=0 after-prune=0 before-pp=0 after-pp=0" ] && [ "$se" -eq 0 ] \
    && [ "$(cat "$sd/before-opt")" = ABSENT ] && two "$sd/after-opt" \
    && cmp -s "$sd/after-opt" "$sd/before-prune" && cmp -s "$sd/after-opt" "$sd/after-prune" \
    && cmp -s "$sd/after-opt" "$sd/before-pp" && cmp -s "$sd/after-opt" "$sd/after-pp" && cmp -s "$sd/after-opt" "$S/snapend-pos"; } \
    && ok "cache absent before opt; after opt == before/after prune == before/after pp (one binary + sidecar, same inode/mtime/sha): prune and pp reused it" \
    || no "cache snapshots differ or failed ($src snapend=$se)"
{ ! hasclass "$l" pass-gen-opt && ! hasclass "$l" pass-gen-prune && ! hasclass "$l" pass-gen-pp; } && ok "no direct gen.py opt / prune / pp" || no "direct Python generator call"
g=$(ls "$o/.seed-gen-cache"/seed-gen-* 2>/dev/null | grep -v '\.sha256$' | head -1)
rm -f "$S/ref-e4.json"
rr=none; [ -n "$g" ] && { "$W/tests/bound" 55 "$g" opt "$S/ref-e4.json" --o2 2>"$S/ref-e4.err"; rr=$?; }
evw "POS ref_rc=$rr"
{ [ "$rr" = 0 ] && cmp -s "$o/e4.json" "$S/ref-e4.json"; } && ok "e4.json = seed-gen opt --o2 (reference rc 0)" || no "e4.json vs seed-gen opt --o2 (reference rc=$rr)"

# 2-4 negatives: the opt step fails, prepare stops before the prune and pp helpers
printf '#!/bin/sh\necho seed-gen-red >&2\nexit 3\n' > "$S/red-gen"; chmod +x "$S/red-gen"
for case_ in "red:3:SEED_GEN_BIN=$S/red-gen" "missing:2:SEED_GEN_BIN=/nonexistent/seed-gen" "badsg:2:SEED_GEN=yes"; do
    n=${case_%%:*}; rest=${case_#*:}; want=${rest%%:*}; e=${rest#*:}
    r=$(prep "$n" "$PREP" "$e")
    l=$S/log-$n
    { [ "$r" -eq "$want" ] && [ ! -e "$S/out-$n/e4.json" ] && [ ! -e "$S/out-$n/e2.json" ] \
        && ! awk -F '\t' '$2 ~ /^python3 exec\/build\/gen.py (prune|lower) / { f = 1 } END { exit !f }' "$l" \
        && ! hasclass "$l" pass-gen-opt && ! hasclass "$l" pass-gen-prune && ! hasclass "$l" pass-gen-pp && [ -s "$l" ] \
        && [ "$(helpers "$l")" = "opt " ] && haslog "$l" helper-opt-rc "$want"; } \
        && ok "NEG-$n: opt helper rc $want, prepare rc $r, prune and pp helpers never started, no e4.json, no Python fallback" \
        || no "NEG-$n rc=$r (want $want) helpers '$(helpers "$l")' $(tail -1 "$l.out")"
    evw "NEG-$n rc prepare=$r want=$want helpers=$(helpers "$l")"
done
# 5 the old direct gen.py opt line is caught
python3 - "$PREP" exec/pipeline/prepare-oldopt.sh <<'PY' || { echo "seedoptc: mutation failed"; exit 2; }
import sys
s = open(sys.argv[1]).read()
new = 'b env SEED_GEN_DIR="$OUT/.seed-gen-cache" sh "$R/exec/opt/gen-delta.sh" "$OUT/e4.json" --o2\n'
if s.count(new) != 1: raise SystemExit("opt line not found exactly once")
open(sys.argv[2], "w").write(s.replace(new, 'b python3 exec/build/gen.py opt "$OUT/e4.json" --o2\n'))
PY
r=$(prep mut exec/pipeline/prepare-oldopt.sh)
# the opt step bypasses the helper, so no cache exists before prune (prune then builds it cold)
{ [ "$r" = 0 ] && [ -s "$S/log-mut" ] && haslog "$S/log-mut" pass-gen-opt "python3 exec/build/gen.py opt $S/out-mut/e4.json --o2" \
    && [ "$(helpers "$S/log-mut")" = "prune pp " ] \
    && [ "$(cat "$S/snap-mut/before-prune.rc" 2>/dev/null)" = 0 ] && [ "$(cat "$S/snap-mut/before-prune" 2>/dev/null)" = ABSENT ]; } \
    && ok "mutant (old direct gen.py opt) is caught: direct call logged, no opt helper, rc 0, no cache before prune" || no "mutant not caught (rc=$r)"
evw "MUTANT helpers=$(helpers "$S/log-mut") before_prune_rc=$(cat "$S/snap-mut/before-prune.rc" 2>/dev/null || echo none) before_prune=$(head -1 "$S/snap-mut/before-prune" 2>/dev/null || echo none)"
evw "MUTANT rc=$r"
# 6 SEED_GEN=0 named reference
r=$(prep sg0 "$PREP" SEED_GEN=0)
{ [ "$r" = 0 ] && [ -s "$S/log-sg0" ] && haslog "$S/log-sg0" pass-gen-opt "python3 exec/build/gen.py opt $S/out-sg0/e4.json --o2" \
    && haslog "$S/log-sg0" pass-gen-prune "python3 exec/build/gen.py prune $S/out-sg0/prune.json" \
    && haslog "$S/log-sg0" pass-gen-pp "python3 exec/build/gen.py pp $S/out-sg0/e2.json" \
    && [ "$(helpers "$S/log-sg0")" = "opt prune pp " ] \
    && [ "$(cat "$S/snap-sg0/after-pp.rc" 2>/dev/null)" = 0 ] && [ "$(cat "$S/snap-sg0/after-pp" 2>/dev/null)" = ABSENT ] \
    && cmp -s "$S/out-sg0/e4.json" "$S/out-pos/e4.json"; } \
    && ok "SEED_GEN=0 reference: helpers -> gen.py opt --o2 / prune / pp, no cache, e4.json byte-equal to the C one" || no "SEED_GEN=0 reference (rc=$r)"
evw "SEEDGEN0 rc=$r helpers=$(helpers "$S/log-sg0") after_pp_rc=$(cat "$S/snap-sg0/after-pp.rc" 2>/dev/null || echo none) after_pp=$(head -1 "$S/snap-sg0/after-pp" 2>/dev/null || echo none)"

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$PREP") || exit 2
[ "$before" = "$after" ] && ok "checkout $PREP untouched" || no "checkout $PREP changed"
if [ -n "$EV" ]; then
    for f in shimneg log-pos log-pos.out snap-pos snapend-pos ref-e4.err log-red log-red.out snap-red log-missing log-missing.out snap-missing \
             log-badsg log-badsg.out snap-badsg log-mut log-mut.out snap-mut log-sg0 log-sg0.out snap-sg0; do
        cp -R "$S/$f" "$EV/$f" || { evfail=1; echo "seedoptc: EVIDENCE copy of $f failed" >&2; }
    done
fi
echo "seedoptc  same $same  bad $bad"
evw "seedoptc  same $same  bad $bad  tree $H"
[ "$bad" = 0 ] && [ "$same" -gt 0 ] && [ "$evfail" = 0 ]
