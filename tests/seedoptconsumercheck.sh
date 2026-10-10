#!/bin/sh
# K5-1f (机房主任 03:43): exec/pipeline/prepare.sh's opt step goes through exec/opt/gen-delta.sh --o2 and shares the
# private SEED_GEN_DIR="$OUT/.seed-gen-cache" with the pp step after it.  Controlled check, lnx/x86_64, NETWORK=0:
#  * shims first on PATH.  python3: passes "-", "-c", tests/bound.py, gen.py opt and gen.py pp to the real interpreter
#    (logged pass-gen-opt / pass-gen-pp), short-circuits gen.py lex/parse2/prune/lower/enc/enc/arm and exec/c/tbl.py
#    by touching their output, refuses anything else -- an unknown gen.py stage included -- with rc 97.  On the
#    prune call (prepare.sh runs it between opt and pp) it snapshots $OUT/.seed-gen-cache (name, inode, mtime_ns,
#    sha256) into $SNAPMID, status checked.  cc: stubs exec/c/run.c, passes seed/gen.c (each compile logged
#    pass-gen-c), tests/bound.c and a lone --version; refuses anything else.
#  * positive: prepare rc 0; seed/gen.c compiled exactly once; helper key step twice (opt, pp); the cache snapshot
#    after opt equals the one after pp (pp reused the binary, no rebuild); e4.json byte-equal to that seed-gen's
#    "opt --o2"; no direct gen.py opt / pp; no unknown call.
#  * negatives: red SEED_GEN_BIN (rc 3), missing SEED_GEN_BIN (rc 2), SEED_GEN=yes (rc 2) -- prepare fails with that
#    rc before prune and pp, no e4.json, no Python fallback; the old direct "gen.py opt" line is caught.
#  * SEED_GEN=0 named reference: helper -> gen.py opt --o2; its e4.json is byte-equal to the C one.
# usage: tests/seedoptconsumercheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
S=$(mktemp -d "${TMPDIR:-/tmp}/seedoptc.XXXXXX") || exit 2
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
REALPY=$(command -v python3) && REALCC=$(command -v cc) || { echo "seedoptc: python3 or cc missing"; exit 2; }
mkdir "$S/shim"
cat > "$S/shim/python3" <<SHIM || exit 2
#!/bin/sh
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "python3 shim: log write failed" >&2; exit 96; }; }
case "\${1:-}" in
    -|-c) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    */tests/bound.py|tests/bound.py) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    exec/build/gen.py)
        case "\${2:-}" in
            opt) log pass-gen-opt "python3 \$*"; exec "$REALPY" "\$@";;
            pp) log pass-gen-pp "python3 \$*"; exec "$REALPY" "\$@";;
            prune) log short "python3 \$*"
                   d=\$(dirname "\$3")/.seed-gen-cache
                   "$REALPY" -c 'import hashlib,os,sys
d=sys.argv[1]
for f in sorted(os.listdir(d)):
    st=os.stat(os.path.join(d,f)); print(f, st.st_ino, st.st_mtime_ns, hashlib.sha256(open(os.path.join(d,f),"rb").read()).hexdigest())' "\$d" > "\$SNAPMID"
                   mr=\$?; echo "\$mr" > "\$SNAPMID.rc"
                   [ "\$mr" -eq 0 ] || { echo "python3 shim: mid snapshot failed" >&2; exit 95; }
                   : > "\$3"; exit 0;;
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
chmod +x "$S/shim/python3" "$S/shim/cc"
same=0; bad=0
ok() { echo "SAME $1"; evw "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; evw "DIFF $1"; bad=$((bad + 1)); }
haslog() { awk -F '\t' -v c="$2" -v a="$3" '$1 == c && $2 == a { f = 1 } END { exit !f }' "$1"; }
hasclass() { awk -F '\t' -v c="$2" '$1 == c { f = 1 } END { exit !f }' "$1"; }
count() { awk -F '\t' -v c="$2" -v a="$3" '$1 == c && (a == "" || $2 == a) { n++ } END { print n + 0 }' "$1"; }
KEYSTEP="python3 - cc -std=c99 -O2 -w"
snap() { python3 -c 'import hashlib,os,sys
d=sys.argv[1]
for f in sorted(os.listdir(d)):
    st=os.stat(os.path.join(d,f)); print(f, st.st_ino, st.st_mtime_ns, hashlib.sha256(open(os.path.join(d,f),"rb").read()).hexdigest())' "$1"; }
prep() {   # prep NAME [SCRIPT] [ENV...]: prepare.sh OUT lnx/x86_64 0 cc under the shims; prints rc
    n=$1; sc=${2:-$PREP}; shift 2 2>/dev/null || shift $#
    o=$S/out-$n; rm -rf "$o"; mkdir -p "$o"; : > "$S/log-$n"; rm -f "$S/snapmid-$n" "$S/snapmid-$n.rc"
    env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC -u SEED_GEN_DIR PATH="$S/shim:$PATH" SHIMLOG="$S/log-$n" SNAPMID="$S/snapmid-$n" \
        "$@" sh "$sc" "$o" lnx/x86_64 0 cc > "$S/log-$n.out" 2>&1; echo $?
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
o=$S/out-pos; l=$S/log-pos
snap "$o/.seed-gen-cache" > "$S/snapend-pos"; se=$?
{ [ "$r" -eq 0 ] && [ -s "$o/e4.json" ] && [ -s "$o/e2.json" ] && ! hasclass "$l" UNKNOWN; } \
    && ok "prepare rc 0, e4.json and e2.json made, no unknown call" || no "prepare (rc=$r) $(tail -1 "$l.out")"
evw "POS rc=$r"
nc=$(count "$l" pass-gen-c ""); nk=$(count "$l" pass "$KEYSTEP")
{ [ "$nc" -eq 1 ] && [ "$nk" -eq 2 ]; } && ok "seed/gen.c compiled once; helper key step twice (opt, pp)" || no "compiles=$nc key steps=$nk"
evw "POS compiles=$nc keysteps=$nk"
mrc=$(cat "$S/snapmid-pos.rc" 2>/dev/null || echo none)
evw "POS snapmid_rc=$mrc snapend_rc=$se"
{ [ "$mrc" = 0 ] && [ -s "$S/snapmid-pos" ] && [ "$se" -eq 0 ] && [ -s "$S/snapend-pos" ] && cmp -s "$S/snapmid-pos" "$S/snapend-pos" \
    && [ "$(wc -l < "$S/snapend-pos")" -eq 2 ] && [ "$(grep -c '^seed-gen-[0-9a-f]\{16\} ' "$S/snapend-pos")" -eq 1 ] \
    && [ "$(grep -c '^seed-gen-[0-9a-f]\{16\}\.sha256 ' "$S/snapend-pos")" -eq 1 ]; } \
    && ok "cache after opt == cache after pp (one binary, same inode/mtime/sha): pp reused it" || no "cache changed between opt and pp (snapend rc=$se)"
# event order in the shim log: helper key step (opt) -> prune (mid snapshot taken here) -> helper key step (pp)
ord=$(awk -F '\t' -v k="$KEYSTEP" '$1 == "pass" && $2 == k { printf "K" } $1 == "short" && $2 ~ /^python3 exec\/build\/gen.py prune / { printf "P" }' "$l")
[ "$ord" = KPK ] && ok "event order: opt key step, then prune (mid snapshot), then pp key step" || no "event order '$ord' (want KPK)"
evw "POS order=$ord snapend_rc=$se"
{ ! hasclass "$l" pass-gen-opt && ! hasclass "$l" pass-gen-pp; } && ok "no direct gen.py opt / pp" || no "direct Python generator call"
g=$(ls "$o/.seed-gen-cache"/seed-gen-* 2>/dev/null | grep -v '\.sha256$' | head -1)
rm -f "$S/ref-e4.json"
rr=none; [ -n "$g" ] && { "$W/tests/bound" 55 "$g" opt "$S/ref-e4.json" --o2 2>"$S/ref-e4.err"; rr=$?; }
evw "POS ref_rc=$rr"
{ [ "$rr" = 0 ] && cmp -s "$o/e4.json" "$S/ref-e4.json"; } && ok "e4.json = seed-gen opt --o2 (reference rc 0)" || no "e4.json vs seed-gen opt --o2 (reference rc=$rr)"

# 2-4 negatives: the opt step fails, prepare stops before prune and pp
printf '#!/bin/sh\necho seed-gen-red >&2\nexit 3\n' > "$S/red-gen"; chmod +x "$S/red-gen"
for case_ in "red:3:SEED_GEN_BIN=$S/red-gen" "missing:2:SEED_GEN_BIN=/nonexistent/seed-gen" "badsg:2:SEED_GEN=yes"; do
    n=${case_%%:*}; rest=${case_#*:}; want=${rest%%:*}; e=${rest#*:}
    r=$(prep "$n" "$PREP" "$e")
    l=$S/log-$n
    { [ "$r" -eq "$want" ] && [ ! -e "$S/out-$n/e4.json" ] && [ ! -e "$S/out-$n/e2.json" ] \
        && ! awk -F '\t' '$2 ~ /^python3 exec\/build\/gen.py (prune|lower) / { f = 1 } END { exit !f }' "$l" \
        && ! hasclass "$l" pass-gen-opt && ! hasclass "$l" pass-gen-pp && [ -s "$l" ]; } \
        && ok "NEG-$n: prepare rc $r before prune and pp, no e4.json, no Python fallback" || no "NEG-$n rc=$r (want $want) $(tail -1 "$l.out")"
    evw "NEG-$n rc prepare=$r want=$want"
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
{ [ "$r" -eq 0 ] && haslog "$S/log-mut" pass-gen-opt "python3 exec/build/gen.py opt $S/out-mut/e4.json --o2"; } \
    && ok "mutant (old direct gen.py opt) is caught" || no "mutant not caught (rc=$r)"
evw "MUTANT rc=$r"
# 6 SEED_GEN=0 named reference
r=$(prep sg0 "$PREP" SEED_GEN=0)
{ [ "$r" -eq 0 ] && haslog "$S/log-sg0" pass-gen-opt "python3 exec/build/gen.py opt $S/out-sg0/e4.json --o2" && cmp -s "$S/out-sg0/e4.json" "$S/out-pos/e4.json"; } \
    && ok "SEED_GEN=0 reference: helper -> gen.py opt --o2, byte-equal to the C e4.json" || no "SEED_GEN=0 reference (rc=$r)"
evw "SEEDGEN0 rc=$r"

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$PREP") || exit 2
[ "$before" = "$after" ] && ok "checkout $PREP untouched" || no "checkout $PREP changed"
if [ -n "$EV" ]; then
    for f in shimneg log-pos log-pos.out snapmid-pos snapmid-pos.rc snapend-pos ref-e4.err log-red log-red.out log-missing log-missing.out log-badsg log-badsg.out \
             log-mut log-mut.out log-sg0 log-sg0.out; do
        cp "$S/$f" "$EV/$f" || { evfail=1; echo "seedoptc: EVIDENCE copy of $f failed" >&2; }
    done
fi
echo "seedoptc  same $same  bad $bad"
evw "seedoptc  same $same  bad $bad  tree $H"
[ "$bad" = 0 ] && [ "$same" -gt 0 ] && [ "$evfail" = 0 ]
