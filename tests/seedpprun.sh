#!/bin/sh
# K5-1e (机房主任 02:59/03:00): exec/pp/run.sh builds d.json through exec/pp/gen-delta.sh.  Controlled check of that
# consumer on a private scratch tree (git archive of HEAD), each case with its own E2TMP and TMPDIR, run serially:
#  * tests/build_ref.sh and exec/pp/mknoauto.sh are replaced IN THE SCRATCH by stubs that only create their
#    outputs (the reference builds are not what this checks); a python3 shim first on PATH passes known calls to
#    the real interpreter (gen-delta's "python3 -" key and "-c" hashes, tests/bound.py, gen.py pp) and refuses any
#    other (rc 97); every call is logged with its class.
#  * cold `run.sh gen`: rc 0, d.json built through the helper (its key step logged, no direct gen.py pp), no
#    one-time seed-gen directory left; warm rerun: no rebuild, stamp unchanged, helper not called.
#  * from a copy of the warm E2TMP: SEED_GEN=yes, a red SEED_GEN_BIN, a missing SEED_GEN_CC (BIN unset) each miss
#    the warm stamp and fail: run.sh gen rc 1, no d.json.stamp left; an edited seed header rebuilds d.json.
#  * ready failing (stub build_ref exits 1): run.sh gen rc != 0, no d.json; a scratch copy without "|| exit 1"
#    ends 0 (the old false green); the old direct gen.py pp line is caught as not going through the helper.
#  * SEED_GEN=0 named reference: helper -> gen.py pp; its d.json is byte-equal to the cold seed-gen d.json.
# usage: tests/seedpprun.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
S=$(mktemp -d "${TMPDIR:-/tmp}/seedpprun.XXXXXX") || exit 2
# 0.0.40 (机房主任 00:04): SEEDPPRUN_EVIDENCE=DIR (a new or empty directory outside the scratch) keeps the evidence
# the scratch would lose: every SAME/DIFF line, the o1/o2 JSON sha256s and the monotonic start/end (ns), written
# before the scratch is removed -- also on a failing or interrupted run.
EV=${SEEDPPRUN_EVIDENCE:-}
# 0.0.40 (机房主任 00:25): the export fails closed.  Any failed write, timestamp or hash sets evfail; a run with
# evfail is never green (exit 3 when it would have been 0, the original status otherwise), and its scratch is kept
# (path printed) instead of being removed without a record.
evfail=0
mono() { python3 -c 'import time;print(time.monotonic_ns())'; }
evw() {   # evw LINE: append one line to lines.txt, or mark the export failed
    [ -n "$EV" ] || return 0
    printf '%s\n' "$1" >> "$EV/lines.txt" || { evfail=1; echo "seedpprun: EVIDENCE write failed: $1" >&2; }
}
evmono() {   # evmono KEY: one monotonic stamp, checked to be a number
    [ -n "$EV" ] || return 0
    t=$(mono) && case "$t" in ''|*[!0-9]*) false;; *) true;; esac && printf '%s %s\n' "$1" "$t" >> "$EV/mono.txt" \
        || { evfail=1; echo "seedpprun: EVIDENCE timestamp $1 failed" >&2; }
}
if [ -n "$EV" ]; then
    case "$EV" in "$S"|"$S"/*) echo "seedpprun: SEEDPPRUN_EVIDENCE inside the scratch"; exit 2;; esac
    mkdir -p "$EV" && [ -z "$(ls -A "$EV")" ] || { echo "seedpprun: SEEDPPRUN_EVIDENCE must be a new or empty directory: $EV"; exit 2; }
    evmono start_ns; [ "$evfail" = 0 ] || exit 2
fi
export_ev() {   # hashes of the four JSON files (all four must exist and hash) and the end stamp
    [ -n "$EV" ] || return 0
    rc=0
    for f in e2-cold/d.json e2-sg0/d.json; do
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
        echo "seedpprun: EVIDENCE INCOMPLETE -- not green; scratch kept at $S" >&2
        [ -z "$EV" ] || printf 'EVIDENCE INCOMPLETE status %s scratch %s\n' "$st" "$S" >> "$EV/lines.txt" 2>/dev/null
        [ "$st" -ne 0 ] || st=3
    else
        rm -rf "$S"
    fi
    exit "$st"
}
trap 'finish $?' EXIT
trap 'exit 130' INT TERM HUP
RUN=exec/pp/run.sh
before=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$RUN") || exit 2
W=$S/tree; mkdir "$W"
H=$(git rev-parse --verify HEAD) || exit 2
echo "seedpprun  tree $H"
git archive -o "$S/tree.tar" "$H" exec seed unisa kernel src include unisacc.c weights/gold/pp.tsv tests/bound tests/bound.c tests/bound.py \
    tests/build_ref.sh tests/export_ref.sh tests/sourceflat.py tests/refshim.h tests/reffoot.h || { echo "seedpprun: git archive failed"; exit 2; }
tar -x -C "$W" -f "$S/tree.tar" || { echo "seedpprun: scratch tree failed"; exit 2; }
cd "$W" || exit 2
BOUND_CACHE=$S/boundcache; export BOUND_CACHE
REALPY=$(command -v python3) || { echo "seedpprun: python3 missing"; exit 2; }
# stubs for the reference builds (scratch only)
printf '#!/bin/sh\n[ -z "${STUB_BUILD_REF_FAIL:-}" ] || { echo stub-build-ref-fail >&2; exit 1; }\n: > "$1"; printf "#!/bin/sh\\nexit 0\\n" > "$2"; chmod +x "$2"\n' > tests/build_ref.sh
printf '#!/bin/sh\n[ -z "${STUB_MKNOAUTO_FAIL:-}" ] || { echo stub-mknoauto-fail >&2; exit 1; }\nprintf "#!/bin/sh\\nexit 0\\n" > "$2"; chmod +x "$2"\n' > exec/pp/mknoauto.sh
chmod +x tests/build_ref.sh exec/pp/mknoauto.sh
mkdir "$S/shim"
cat > "$S/shim/python3" <<SHIM || exit 2
#!/bin/sh
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "python3 shim: log write failed" >&2; exit 96; }; }
case "\${1:-}" in
    -|-c) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    */tests/bound.py|tests/bound.py) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    exec/build/gen.py) case "\${2:-}" in pp) log pass-gen-pp "python3 \$*"; exec "$REALPY" "\$@";; esac;;
esac
log UNKNOWN "python3 \$*"; exit 97
SHIM
chmod +x "$S/shim/python3"
same=0; bad=0
ok() { echo "SAME $1"; evw "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; evw "DIFF $1"; bad=$((bad + 1)); }
haslog() { awk -F '\t' -v c="$2" -v a="$3" '$1 == c && $2 == a { f = 1 } END { exit !f }' "$1"; }
hasclass() { awk -F '\t' -v c="$2" '$1 == c { f = 1 } END { exit !f }' "$1"; }
KEYSTEP="python3 - cc -std=c99 -O2 -w"
gen() {   # gen NAME SCRIPT [ENV...]: run SCRIPT gen with E2TMP=$S/e2-NAME, TMPDIR=$S/tmp-NAME; prints rc
    n=$1; sc=$2; shift 2
    mkdir -p "$S/e2-$n" "$S/tmp-$n"; : > "$S/log-$n"
    env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC -u SEED_GEN_DIR PATH="$S/shim:$PATH" SHIMLOG="$S/log-$n" \
        E2TMP="$S/e2-$n" TMPDIR="$S/tmp-$n" "$@" sh "$sc" gen > "$S/log-$n.out" 2>&1; echo $?
}
rebuilt() { grep -qx "fresh: (re)building $S/e2-$1/d.json" "$S/log-$1.out"; }
leftover() { ls "$S/tmp-$1" 2>/dev/null | grep -c '^seedgen-pprun\.'; }
stampsha() { python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$1" 2>/dev/null || echo none; }
# the shim refuses what it does not know
SHIMLOG=$S/shimneg; export SHIMLOG; : > "$SHIMLOG"
"$S/shim/python3" exec/c/other.py >/dev/null 2>&1; r=$?
[ "$r" -eq 97 ] && ok "python3 shim refuses an unknown script" || no "python3 shim did not refuse (rc=$r)"
unset SHIMLOG

# 1 cold
r=$(gen cold "$RUN")
{ [ "$r" -eq 0 ] && [ -s "$S/e2-cold/d.json" ] && [ -f "$S/e2-cold/d.json.stamp" ] && rebuilt cold \
    && haslog "$S/log-cold" pass "$KEYSTEP" && ! hasclass "$S/log-cold" pass-gen-pp && ! hasclass "$S/log-cold" UNKNOWN \
    && [ "$(leftover cold)" -eq 0 ]; } && ok "cold gen through the helper, stamp written, one-time seed-gen dir removed" \
    || no "cold gen (rc=$r leftover=$(leftover cold)) $(tail -1 "$S/log-cold.out")"
evw "COLD rc=$r"
s0=$(stampsha "$S/e2-cold/d.json.stamp")
cp -a "$S/e2-cold" "$S/warm-template" || exit 2
# 2 warm
cp -a "$S/warm-template/." "$S/e2-warm/" 2>/dev/null || { mkdir -p "$S/e2-warm" && cp -a "$S/warm-template/." "$S/e2-warm/"; } || exit 2
r=$(gen warm "$RUN")
{ [ "$r" -eq 0 ] && ! rebuilt warm && [ "$(stampsha "$S/e2-warm/d.json.stamp")" = "$s0" ] && ! haslog "$S/log-warm" pass "$KEYSTEP"; } \
    && ok "warm gen: d.json not rebuilt, stamp unchanged, helper not called" || no "warm gen (rc=$r)"
evw "WARM rc=$r"
# 3-5 warm stamp, then a bad helper configuration: must miss and fail
printf '#!/bin/sh\necho seed-gen-red >&2\nexit 3\n' > "$S/red-gen"; chmod +x "$S/red-gen"
for case_ in "badsg:SEED_GEN=yes" "redbin:SEED_GEN_BIN=$S/red-gen" "badcc:SEED_GEN_CC=/nonexistent/cc"; do
    n=${case_%%:*}; e=${case_#*:}
    mkdir -p "$S/e2-$n" && cp -a "$S/warm-template/." "$S/e2-$n/" || exit 2
    r=$(gen "$n" "$RUN" "$e")
    { [ "$r" -ne 0 ] && [ "$r" -ne 124 ] && [ "$r" -lt 128 ] && rebuilt "$n" && [ ! -f "$S/e2-$n/d.json.stamp" ] \
        && ! hasclass "$S/log-$n" pass-gen-pp; } \
        && ok "warm then $e: stamp missed, gen rc $r, no stamp left, no Python fallback" || no "warm then $e (rc=$r) $(tail -1 "$S/log-$n.out")"
    evw "NEG-$n rc=$r"
done
# 6 an edited seed header rebuilds a warm d.json
mkdir -p "$S/e2-hdr" && cp -a "$S/warm-template/." "$S/e2-hdr/" || exit 2
printf '\n/* seedpprun: header probe */\n' >> seed/json.h
r=$(gen hdr "$RUN")
tar -x -C "$W" -f "$S/tree.tar" seed/json.h || exit 2
{ [ "$r" -eq 0 ] && rebuilt hdr && haslog "$S/log-hdr" pass "$KEYSTEP" && [ "$(stampsha "$S/e2-hdr/d.json.stamp")" != "$s0" ]; } \
    && ok "edited seed header rebuilds d.json through the helper" || no "seed header edit (rc=$r)"
evw "HDR rc=$r"
# 7 ready failing: gen must fail; the copy without "|| exit 1" shows the old false green
for case_ in "rdyfail:STUB_BUILD_REF_FAIL=1" "noautofail:STUB_MKNOAUTO_FAIL=1"; do
    n=${case_%%:*}; e=${case_#*:}
    r=$(gen "$n" "$RUN" "$e")
    { [ "$r" -ne 0 ] && [ "$r" -lt 128 ] && [ ! -e "$S/e2-$n/d.json" ] && [ ! -e "$S/e2-$n/d.json.stamp" ] \
        && ! haslog "$S/log-$n" pass "$KEYSTEP" && ! hasclass "$S/log-$n" pass-gen-pp; } \
        && ok "$e: run.sh gen rc $r, no d.json, helper and gen.py never started" || no "$e (rc=$r)"
    evw "NEG-$n rc=$r"
done
python3 - "$RUN" exec/pp/run-oldgen.sh <<'PY' || { echo "seedpprun: mutation failed"; exit 2; }
import sys
s = open(sys.argv[1]).read()
new = "gen)      ready || exit 1 ;;"
i = s.find(new)
if i < 0 or s.count(new) != 1: raise SystemExit("gen line not found exactly once")
e = s.index("\n", i)
open(sys.argv[2], "w").write(s[:i] + "gen)      ready ;;" + s[e:])
PY
r=$(gen rdyold exec/pp/run-oldgen.sh STUB_BUILD_REF_FAIL=1)
[ "$r" -eq 0 ] && ok "without '|| exit 1' the same failure ends 0 (the false green the fix closes)" || no "old gen line did not end 0 (rc=$r)"
evw "RDYOLD rc=$r"
# 8 the old direct gen.py pp line is caught
python3 - "$RUN" exec/pp/run-oldpp.sh <<'PY' || { echo "seedpprun: mutation failed"; exit 2; }
import sys, re
s = open(sys.argv[1]).read()
i = s.index("    fresh $T/d.json $B sh -c ")
e = s.index("\n", i)
open(sys.argv[2], "w").write(s[:i] + "    fresh $T/d.json $B python3 exec/build/gen.py pp $T/d.json -- exec/finite_rules.py" + s[e:])
PY
r=$(gen oldpp exec/pp/run-oldpp.sh)
{ [ "$r" -eq 0 ] && hasclass "$S/log-oldpp" pass-gen-pp && ! haslog "$S/log-oldpp" pass "$KEYSTEP"; } \
    && ok "mutant (old direct gen.py pp) is caught: not through the helper" || no "mutant not caught (rc=$r)"
evw "MUTANT rc=$r"
# 9 SEED_GEN=0 named reference, byte-equal to the cold C d.json
r=$(gen sg0 "$RUN" SEED_GEN=0)
{ [ "$r" -eq 0 ] && haslog "$S/log-sg0" pass-gen-pp "python3 exec/build/gen.py pp $S/e2-sg0/d.json" && cmp -s "$S/e2-sg0/d.json" "$S/e2-cold/d.json"; } \
    && ok "SEED_GEN=0 reference: helper -> gen.py pp, byte-equal to the cold seed-gen d.json" || no "SEED_GEN=0 reference (rc=$r)"
evw "SEEDGEN0 rc=$r"

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$RUN") || exit 2
[ "$before" = "$after" ] && ok "checkout $RUN untouched" || no "checkout $RUN changed"
if [ -n "$EV" ]; then
    for f in shimneg log-cold log-warm log-badsg log-redbin log-badcc log-hdr log-rdyfail log-noautofail log-rdyold log-oldpp log-sg0 \
             log-cold.out log-warm.out log-badsg.out log-redbin.out log-badcc.out log-hdr.out log-rdyfail.out log-noautofail.out log-rdyold.out log-oldpp.out log-sg0.out; do
        cp "$S/$f" "$EV/$f" || { evfail=1; echo "seedpprun: EVIDENCE copy of $f failed" >&2; }
    done
fi
echo "seedpprun  same $same  bad $bad"
evw "seedpprun  same $same  bad $bad  tree $H"
[ "$bad" = 0 ] && [ "$same" -gt 0 ] && [ "$evfail" = 0 ]
