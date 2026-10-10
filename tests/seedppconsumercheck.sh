#!/bin/sh
# K5-1c (机房主任 01:23): exec/pipeline/prepare.sh's pp step goes through exec/pp/gen-delta.sh with a private
# SEED_GEN_DIR="$OUT/.seed-gen-cache".  Controlled check of that consumer -- not a model acceptance:
#  * prepare.sh runs for real (scratch tree from one commit) with NETWORK=0 and two shims first on PATH:
#    python3 short-circuits only the named non-pp steps (gen.py lex/parse2/opt/prune/lower/enc/enc/arm,
#    exec/c/tbl.py) by touching their output, passes through everything else that is known (gen-delta's
#    "python3 -" key and "python3 -c" hashes, tests/bound.py, gen.py pp) to the real interpreter, and refuses
#    anything unknown (rc 97); cc writes a stub for exec/c/run.c only and passes every other compile (seed/gen.c,
#    tests/bound.c) to the real compiler.  Every call is logged with its class.
#  * per target: prepare rc 0, e2.json made through the helper (one keyed seed-gen and its .sha256 in
#    $OUT/.seed-gen-cache, no direct gen.py pp), byte-equal to that seed-gen run with the target's expected
#    flags, and the next step (gen.py lower) reached.
#  * shard 1: lnx/x86_64 lnx/arm64 osx/x86_64; red seed-gen (rc 3 kept), missing SEED_GEN_BIN (rc 2), SEED_GEN=yes
#    (rc 2) each stop prepare before lower with no e2.json; the old direct "gen.py pp" line (mutated copy in the
#    scratch) is caught as not going through the helper.
#  * shard 2: osx/arm64 win/x86_64 win/arm64; SEED_GEN=0 named reference route for win/arm64 goes through the
#    helper to gen.py pp with --win --arm64 and is byte-equal to the C e2.
# usage: tests/seedppconsumercheck.sh 1|2
case "${1:-}" in
    1) TARGETS="lnx/x86_64 lnx/arm64 osx/x86_64";;
    2) TARGETS="osx/arm64 win/x86_64 win/arm64";;
    *) echo "usage: tests/seedppconsumercheck.sh 1|2"; exit 2;;
esac
SHARD=$1
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
S=$(mktemp -d "${TMPDIR:-/tmp}/seedppc.XXXXXX") || exit 2
# 0.0.40 (机房主任 00:04): SEEDPPC_EVIDENCE=DIR (a new or empty directory outside the scratch) keeps the evidence
# the scratch would lose: every SAME/DIFF line, the o1/o2 JSON sha256s and the monotonic start/end (ns), written
# before the scratch is removed -- also on a failing or interrupted run.
EV=${SEEDPPC_EVIDENCE:-}
# 0.0.40 (机房主任 00:25): the export fails closed.  Any failed write, timestamp or hash sets evfail; a run with
# evfail is never green (exit 3 when it would have been 0, the original status otherwise), and its scratch is kept
# (path printed) instead of being removed without a record.
evfail=0
mono() { python3 -c 'import time;print(time.monotonic_ns())'; }
evw() {   # evw LINE: append one line to lines.txt, or mark the export failed
    [ -n "$EV" ] || return 0
    printf '%s\n' "$1" >> "$EV/lines.txt" || { evfail=1; echo "seedppc: EVIDENCE write failed: $1" >&2; }
}
evmono() {   # evmono KEY: one monotonic stamp, checked to be a number
    [ -n "$EV" ] || return 0
    t=$(mono) && case "$t" in ''|*[!0-9]*) false;; *) true;; esac && printf '%s %s\n' "$1" "$t" >> "$EV/mono.txt" \
        || { evfail=1; echo "seedppc: EVIDENCE timestamp $1 failed" >&2; }
}
if [ -n "$EV" ]; then
    case "$EV" in "$S"|"$S"/*) echo "seedppc: SEEDPPC_EVIDENCE inside the scratch"; exit 2;; esac
    mkdir -p "$EV" && [ -z "$(ls -A "$EV")" ] || { echo "seedppc: SEEDPPC_EVIDENCE must be a new or empty directory: $EV"; exit 2; }
    evmono start_ns; [ "$evfail" = 0 ] || exit 2
fi
export_ev() {   # hashes of the four JSON files (all four must exist and hash) and the end stamp
    [ -n "$EV" ] || return 0
    rc=0
    for t in $TARGETS; do
        f=$S/out-$(echo "$t" | tr / -)/e2.json
        if [ -f "$f" ] && h=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$f") \
            && [ ${#h} -eq 64 ]; then line="$h e2 $t"; else line="MISSING e2 $t"; rc=1; fi
        printf '%s\n' "$line" >> "$EV/json.sha256" || rc=1
    done
    evmono end_ns; [ "$evfail" = 0 ] || rc=1
    return $rc
}
finish() {
    st=$1
    export_ev || evfail=1
    if [ "$evfail" != 0 ]; then
        echo "seedppc: EVIDENCE INCOMPLETE -- not green; scratch kept at $S" >&2
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
echo "seedppc  shard $SHARD  tree $H"
git archive -o "$S/tree.tar" "$H" exec seed unisa include tests/bound tests/bound.c tests/bound.py || { echo "seedppc: git archive failed"; exit 2; }
tar -x -C "$W" -f "$S/tree.tar" || { echo "seedppc: scratch tree failed"; exit 2; }
cd "$W" || exit 2
BOUND_CACHE=$S/boundcache; export BOUND_CACHE
REALPY=$(command -v python3) && REALCC=$(command -v cc) || { echo "seedppc: python3 or cc missing"; exit 2; }
mkdir "$S/shim"
cat > "$S/shim/python3" <<SHIM || exit 2
#!/bin/sh
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "python3 shim: log write failed" >&2; exit 96; }; }
case "\${1:-}" in
    -|-c) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    */tests/bound.py|tests/bound.py) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    exec/build/gen.py)
        case "\${2:-}" in
            pp) log pass-gen-pp "python3 \$*"; exec "$REALPY" "\$@";;
            lex|parse2|opt|prune|lower|enc|enc/arm) log short "python3 \$*"; : > "\$3"; exit 0;;
        esac;;
    exec/c/tbl.py) log short "python3 \$*"; : > "\$3"; exit 0;;
esac
log UNKNOWN "python3 \$*"; exit 97
SHIM
cat > "$S/shim/cc" <<SHIM || exit 2
#!/bin/sh
for a in "\$@"; do case "\$a" in exec/c/run.c|*/exec/c/run.c) run=1;; esac; done
if [ -n "\${run:-}" ]; then
    o=; prev=; for a in "\$@"; do [ "\$prev" = -o ] && o=\$a; prev=\$a; done
    printf '%s\t%s\n' short "cc \$*" >> "\$SHIMLOG" || { echo "cc shim: log write failed" >&2; exit 96; }
    [ -n "\$o" ] && printf '#!/bin/sh\nexit 0\n' > "\$o" && chmod +x "\$o"; exit \$?
fi
printf '%s\t%s\n' pass "cc \$*" >> "\$SHIMLOG" || { echo "cc shim: log write failed" >&2; exit 96; }
exec "$REALCC" "\$@"
SHIM
chmod +x "$S/shim/python3" "$S/shim/cc"
same=0; bad=0
ok() { echo "SAME $1"; evw "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; evw "DIFF $1"; bad=$((bad + 1)); }
flags() { case $1 in lnx/x86_64) ;; lnx/arm64) echo --arm64;; osx/x86_64) echo --osx;; osx/arm64) echo --osx --arm64;; win/x86_64) echo --win;; win/arm64) echo --win --arm64;; esac; }
slug() { echo "$1" | tr / -; }
prep() {   # prep TARGET OUT LOG [SCRIPT] [ENV...]: run prepare.sh under the shims; prints its rc
    t=$1; o=$2; l=$3; sc=${4:-$PREP}; shift 4 2>/dev/null || shift $#
    rm -rf "$o"; mkdir -p "$o"; : > "$l"
    env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC -u SEED_GEN_DIR PATH="$S/shim:$PATH" SHIMLOG="$l" "$@" \
        sh "$sc" "$o" "$t" 0 cc > "$l.out" 2>&1; echo $?
}
haslog() {   # haslog LOG CLASS ARGV: an exact (class, argv) record
    awk -F '\t' -v c="$2" -v a="$3" '$1 == c && $2 == a { f = 1 } END { exit !f }' "$1"
}
hasclass() { awk -F '\t' -v c="$2" '$1 == c { f = 1 } END { exit !f }' "$1"; }
via_helper() {   # via_helper OUT LOG: exactly one keyed seed-gen (+ .sha256) in the private cache, no direct gen.py pp
    nb=$(ls "$1/.seed-gen-cache" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}$')
    ns=$(ls "$1/.seed-gen-cache" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}\.sha256$')
    # the log must be live: gen-delta's key step is a recorded pass-through
    [ "$nb" -eq 1 ] && [ "$ns" -eq 1 ] && ! hasclass "$2" pass-gen-pp && haslog "$2" pass "python3 - cc -std=c99 -O2 -w"
}
for t in $TARGETS; do
    s=$(slug "$t"); o=$S/out-$s; l=$S/log-$s
    r=$(prep "$t" "$o" "$l" "$PREP")
    [ "$r" -eq 0 ] || { no "$t prepare rc=$r $(tail -1 "$l.out")"; continue; }
    via_helper "$o" "$l" && ok "$t pp through gen-delta (private keyed seed-gen, no direct gen.py pp)" || no "$t pp not through the helper"
    g=$(ls "$o/.seed-gen-cache"/seed-gen-* 2>/dev/null | grep -v '\.sha256$' | head -1)
    rm -f "$S/ref-$s.json"
    [ -n "$g" ] && "$W/tests/bound" 55 "$g" pp "$S/ref-$s.json" $(flags "$t") 2>/dev/null && cmp -s "$o/e2.json" "$S/ref-$s.json" \
        && ok "$t e2 = seed-gen pp $(flags "$t")" || no "$t e2 differs from seed-gen with the expected flags"
    f=$(flags "$t")
    haslog "$l" short "python3 exec/build/gen.py lower $o/lower.json --full${f:+ $f}" && ok "$t reaches lower with the same flags" || no "$t lower not reached as expected"
    ! hasclass "$l" UNKNOWN && ok "$t no unknown tool call" || no "$t unknown tool call: $(grep '^UNKNOWN' "$l" | head -1)"
done

if [ "$SHARD" = 1 ]; then
t=lnx/x86_64
printf '#!/bin/sh\necho seed-gen-red >&2\nexit 3\n' > "$S/red-gen"; chmod +x "$S/red-gen"
for case_ in "red:3:SEED_GEN=1 SEED_GEN_BIN=$S/red-gen" "missing:2:SEED_GEN=1 SEED_GEN_BIN=/nonexistent/seed-gen" "badsg:2:SEED_GEN=yes"; do
    n=${case_%%:*}; rest=${case_#*:}; want=${rest%%:*}; envs=${rest#*:}
    o=$S/neg-$n; l=$S/neglog-$n
    r=$(prep "$t" "$o" "$l" "$PREP" $envs)
    { [ "$r" -eq "$want" ] && [ ! -e "$o/e2.json" ] && ! awk -F '\t' '$2 ~ /^python3 exec\/build\/gen.py lower / { f = 1 } END { exit !f }' "$l" && ! hasclass "$l" pass-gen-pp && [ -s "$l" ]; } \
        && ok "NEG-$n prepare stops rc $r before lower, no e2.json, no Python fallback" || no "NEG-$n rc=$r (want $want) $(tail -1 "$l.out")"
    evw "NEG-$n rc prepare=$r want=$want"
done
# mutation: the old direct line in a scratch copy of prepare.sh must be caught by via_helper
M=exec/pipeline/prepare-old.sh
python3 - "$PREP" "$M" <<'PY' || { echo "seedppc: mutation failed"; exit 2; }
import sys
s = open(sys.argv[1]).read()
new = 'b env SEED_GEN_DIR="$OUT/.seed-gen-cache" sh "$R/exec/pp/gen-delta.sh" "$OUT/e2.json" $OSFLAG $ARCHFLAG\n'
if s.count(new) != 1: raise SystemExit("new pp line not found exactly once")
open(sys.argv[2], "w").write(s.replace(new, 'b python3 exec/build/gen.py pp "$OUT/e2.json" $OSFLAG $ARCHFLAG\n'))
PY
o=$S/mut; l=$S/mutlog
r=$(prep "$t" "$o" "$l" "$M")
{ [ "$r" -eq 0 ] && ! via_helper "$o" "$l" && haslog "$l" pass-gen-pp "python3 exec/build/gen.py pp $o/e2.json"; } && ok "mutant (old direct gen.py pp) is caught: not through the helper" || no "mutant not caught (rc=$r)"
evw "MUTANT rc prepare=$r"
fi

if [ "$SHARD" = 2 ]; then
t=win/arm64; s=$(slug "$t"); o=$S/py-$s; l=$S/pylog-$s
r=$(prep "$t" "$o" "$l" "$PREP" SEED_GEN=0)
{ [ "$r" -eq 0 ] && haslog "$l" pass-gen-pp "python3 exec/build/gen.py pp $o/e2.json --win --arm64" && cmp -s "$o/e2.json" "$S/out-$s/e2.json"; } \
    && ok "SEED_GEN=0 named reference: helper -> gen.py pp --win --arm64, byte-equal to the C e2" || no "SEED_GEN=0 route (rc=$r)"
evw "SEEDGEN0 rc prepare=$r"
fi

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$PREP") || exit 2
[ "$before" = "$after" ] && ok "checkout $PREP untouched" || no "checkout $PREP changed"
if [ -n "$EV" ]; then   # the shard's own logs, by name; a failed copy marks the export
    logs=""; for t in $TARGETS; do logs="$logs log-$(slug "$t")"; done
    if [ "$SHARD" = 1 ]; then logs="$logs neglog-red neglog-missing neglog-badsg mutlog"; else logs="$logs pylog-win-arm64"; fi
    for f in $logs; do cp "$S/$f" "$EV/$f" || { evfail=1; echo "seedppc: EVIDENCE copy of $f failed" >&2; }; done
fi
echo "seedppc  shard $SHARD  same $same  bad $bad"
evw "seedppc  shard $SHARD  same $same  bad $bad  tree $H"
[ "$bad" = 0 ] && [ "$same" -gt 0 ] && [ "$evfail" = 0 ]
