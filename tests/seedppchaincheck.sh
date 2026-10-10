#!/bin/sh
# K5-1d (机房主任 02:35): exec/c/chain.sh's pp step goes through exec/pp/gen-delta.sh with a private
# SEED_GEN_DIR="$T/.seed-gen-cache" and stderr/stdout captured to $T/e2-gen.{err,out}.  Controlled
# consumer check -- not exec-chain acceptance:
#  * scratch tree from one commit; python3/cc shims log every call (pass-through known; refuse unknown rc 97)
#  * harness-only: chain's EXIT trap is extended in the scratch copy to snapshot e2.json / e2-gen.err /
#    .seed-gen-cache into $CHAINCAP before rm (production trap unchanged on disk under test for the gen line)
#  * positive: CHAINSHARD=1/1 NETWORK=0 one tiny probe; chain rc 0; via helper; e2 SAME as independent
#    seed-gen and gen.py pp (no flags); no direct gen.py pp; lex reached after E2
#  * NEG-red / NEG-missing / NEG-badsg: chain rc 1 ("E2 gen failed"); no lex; no pass-gen-pp; helper_rc 3/2/2
#  * MUTANT: old direct gen.py pp line caught as not through the helper
#  * SEED_GEN=0 named reference: helper -> gen.py pp, byte-equal to C e2
# 0.0.40 K5-1i (机房主任 07:46/07:53): chain.sh's parse2 step goes through exec/parse2gen/gen-delta.sh and reuses the
# seed-gen the pp step built in $T/.seed-gen-cache.  An sh shim logs both helpers (helper-<stage>, argv, rc) and
# snapshots the cache after each returns ($SNAPDIR/after-<stage>, rc kept); seed/gen.c compiles are logged pass-gen-c.
# Three slices (each <= 58 s; every slice builds its own seed-gen cold, nothing shared between slices):
#  1 shim refusals; positive (pp -> parse2 helpers rc 0, one compile, two key steps, after-pp = after-parse2 with
#    exactly the binary and its sidecar, e2 / e3 = seed-gen and e2 = gen.py pp, no Python parse2); inherited
#    SEED_GEN_DIR override.  Prints C_E2_SHA / C_E3_SHA.
#  2 pp negatives (helper rc 3/2/2; chain stops at E2, the parse2 helper never starts); parse2-only red / missing
#    BIN (NEG_PARSE2_BIN: pp rc 0 with e2/e1, parse2 rc 3/2, chain rc 1 "E3 gen failed", no e3, no call after the
#    parse2 helper); the old direct gen.py pp and gen.py parse2 lines (mutants; Python parse2 short-circuited).
#  3 SEED_GEN=0 named reference run to E3 (stops at tbl): pp and parse2 really through Python (PARSE2_PY=pass),
#    byte-equal to C e2 / e3 built in this slice by the two helpers (private cache).  Prints C_E2_SHA / C_E3_SHA.
# usage: tests/seedppchaincheck.sh 1|2|3
set -u
case "${1:-}" in 1|2|3) SLICE=$1;; *) echo "usage: tests/seedppchaincheck.sh 1|2|3"; exit 2;; esac
case $SLICE in 1) JSONS="e2.json e2-py.json e2-c-ref.json e3.json e3-c-ref.json";; 2) JSONS=;; 3) JSONS="e2.json e3.json e2-sg0.json e3-py.json";; esac
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
_td=${TMPDIR:-/tmp}; S=$(mktemp -d "${_td%/}/seedppch.XXXXXX") || exit 2
EV=${SEEDPPCH_EVIDENCE:-}
evfail=0
mono() { python3 -c 'import time;print(time.monotonic_ns())'; }
evw() {
    [ -n "$EV" ] || return 0
    printf '%s\n' "$1" >> "$EV/lines.txt" || { evfail=1; echo "seedppch: EVIDENCE write failed: $1" >&2; }
}
evmono() {
    [ -n "$EV" ] || return 0
    t=$(mono) && case "$t" in ''|*[!0-9]*) false;; *) true;; esac && printf '%s %s\n' "$1" "$t" >> "$EV/mono.txt" \
        || { evfail=1; echo "seedppch: EVIDENCE timestamp $1 failed" >&2; }
}
if [ -n "$EV" ]; then
    case "$EV" in "$S"|"$S"/*) echo "seedppch: SEEDPPCH_EVIDENCE inside the scratch"; exit 2;; esac
    mkdir -p "$EV" && [ -z "$(ls -A "$EV")" ] || { echo "seedppch: SEEDPPCH_EVIDENCE must be a new or empty directory: $EV"; exit 2; }
    evmono start_ns; [ "$evfail" = 0 ] || exit 2
fi
export_ev() {
    [ -n "$EV" ] || return 0
    rc=0
    for f in $JSONS; do
        p=$S/$f
        if [ -f "$p" ] && h=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$p") \
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
        echo "seedppch: EVIDENCE INCOMPLETE -- not green; scratch kept at $S" >&2
        [ -z "$EV" ] || printf 'EVIDENCE INCOMPLETE status %s scratch %s\n' "$st" "$S" >> "$EV/lines.txt" 2>/dev/null
        [ "$st" -ne 0 ] || st=3
    else
        rm -rf "$S"
    fi
    exit "$st"
}
trap 'finish $?' EXIT
trap 'exit 130' INT TERM HUP
CHAIN=exec/c/chain.sh
before=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$CHAIN") || exit 2
before_gd=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' exec/pp/gen-delta.sh) || exit 2
before_p2=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' exec/parse2gen/gen-delta.sh) || exit 2
W=$S/tree; mkdir "$W"
H=$(git rev-parse --verify HEAD) || exit 2
echo "seedppch  tree $H"
git archive -o "$S/tree.tar" "$H" \
    exec seed unisa include \
    tests/bound tests/bound.c tests/bound.py tests/lib.sh tests/knownfail.py \
    tests/c/a_char.c \
    || { echo "seedppch: git archive failed"; exit 2; }
tar -x -C "$W" -f "$S/tree.tar" || { echo "seedppch: scratch tree failed"; exit 2; }
cd "$W" || exit 2
# overlay live wiring from the checkout under test (HEAD archive alone misses a dirty worktree)
cp -f "$R/exec/c/chain.sh" "$W/exec/c/chain.sh" || exit 2
cp -f "$R/exec/pp/gen-delta.sh" "$W/exec/pp/gen-delta.sh" || exit 2
cp -f "$R/exec/parse2gen/gen-delta.sh" "$W/exec/parse2gen/gen-delta.sh" || exit 2
BOUND_CACHE=$S/boundcache; export BOUND_CACHE
REALPY=$(command -v python3) && REALCC=$(command -v cc) && REALSH=$(command -v sh) || { echo "seedppch: python3, cc or sh missing"; exit 2; }
mkdir "$S/shim"
cat > "$S/shim/sh" <<SHIM || exit 2
#!$REALSH
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "sh shim: log write failed" >&2; exit 96; }; }
case "\${1:-}" in
    */exec/pp/gen-delta.sh) st=pp;;
    */exec/parse2gen/gen-delta.sh) st=parse2;;
    *) exec "$REALSH" "\$@";;
esac
log "helper-\$st" "\$*"
if [ "\$st" = parse2 ] && [ -n "\${NEG_PARSE2_BIN:-}" ]; then SEED_GEN_BIN=\$NEG_PARSE2_BIN; export SEED_GEN_BIN; fi
"$REALSH" "\$@"; hr=\$?
log "helper-\$st-rc" "\$hr"
if [ -n "\${SNAPDIR:-}" ]; then
    "$REALPY" -c 'import hashlib,os,sys
d=sys.argv[1]
try: names=sorted(os.listdir(d))
except FileNotFoundError: print("ABSENT"); sys.exit(0)
for f in names:
    st=os.stat(os.path.join(d,f)); print(f, st.st_ino, st.st_mtime_ns, hashlib.sha256(open(os.path.join(d,f),"rb").read()).hexdigest())' "\$(dirname "\$2")/.seed-gen-cache" > "\$SNAPDIR/after-\$st"
    sr=\$?; echo "\$sr" > "\$SNAPDIR/after-\$st.rc" || exit 95
    [ "\$sr" -eq 0 ] || { echo "sh shim: snapshot after \$st failed" >&2; exit 95; }
fi
exit "\$hr"
SHIM
cat > "$S/shim/python3" <<SHIM || exit 2
#!/bin/sh
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "python3 shim: log write failed" >&2; exit 96; }; }
case "\${1:-}" in
    -|-c) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    */tests/bound.py|tests/bound.py) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    */tests/knownfail.py|tests/knownfail.py) log pass "python3 \$*"; exec "$REALPY" "\$@";;
    exec/build/gen.py)
        case "\${2:-}" in
            pp) log pass-gen-pp "python3 \$*"; exec "$REALPY" "\$@";;
            lex)
                log pass-lex "python3 \$*"
                if [ "\${SEEDPPCH_STOP_AFTER_E2:-}" = 1 ]; then echo "seedppch: stop after E2" >&2; exit 1; fi
                exec "$REALPY" "\$@";;
            parse2) case "\${PARSE2_PY:-}" in
                        pass) log pass-parse2 "python3 \$*"; exec "$REALPY" "\$@";;
                        '') log short-parse2 "python3 \$*"; : > "\$3"; exit 0;;
                        *) log UNKNOWN "python3 \$* (PARSE2_PY=\$PARSE2_PY)"; exit 97;;
                    esac;;
            *) log pass "python3 \$*"; exec "$REALPY" "\$@";;
        esac;;
    exec/c/tbl.py|exec/c/net.py)
        log pass "python3 \$*"
        if [ "\${SEEDPPCH_STOP_AFTER_E3:-}" = 1 ]; then echo "seedppch: stop after E3" >&2; exit 1; fi
        exec "$REALPY" "\$@";;
esac
log UNKNOWN "python3 \$*"; exit 97
SHIM
cat > "$S/shim/cc" <<SHIM || exit 2
#!/bin/sh
log() { [ -n "\${SHIMLOG:-}" ] && printf '%s\t%s\n' "\$1" "\$2" >> "\$SHIMLOG" || { echo "cc shim: log write failed" >&2; exit 96; }; }
srcs=0; run=; known=
for a in "\$@"; do
    case "\$a" in *.c) srcs=\$((srcs + 1));; esac
    case "\$a" in
        exec/c/run.c|*/exec/c/run.c) run=1;;
        seed/gen.c|*/seed/gen.c|tests/bound.c|*/tests/bound.c) known=1;;
    esac
done
if [ "\$#" -eq 1 ] && [ "\$1" = --version ]; then log pass "cc \$*"; exec "$REALCC" "\$@"; fi
if [ -n "\$run" ] && [ "\$srcs" -eq 1 ]; then log pass "cc \$*"; exec "$REALCC" "\$@"; fi
case " \$* " in *seed/gen.c*) if [ -z "\$run" ] && [ "\$srcs" -eq 1 ]; then log pass-gen-c "cc \$*"; exec "$REALCC" "\$@"; fi;; esac
if [ -n "\$known" ] && [ -z "\$run" ] && [ "\$srcs" -eq 1 ]; then log pass "cc \$*"; exec "$REALCC" "\$@"; fi
log UNKNOWN "cc \$*"; exit 97
SHIM
chmod +x "$S/shim/sh" "$S/shim/python3" "$S/shim/cc"
same=0; bad=0
ok() { echo "SAME $1"; evw "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; evw "DIFF $1"; bad=$((bad + 1)); }
# shim negatives
SHIMLOG=$S/shimneg; export SHIMLOG; : > "$SHIMLOG"
if [ "$SLICE" = 1 ]; then
"$S/shim/python3" exec/c/other.py >/dev/null 2>&1; r=$?
[ "$r" -eq 97 ] && ok "python3 shim refuses an unknown script" || no "python3 shim did not refuse (rc=$r)"
"$S/shim/cc" -o x >/dev/null 2>&1; r=$?
[ "$r" -eq 97 ] && ok "cc shim refuses unknown argv" || no "cc shim did not refuse (rc=$r)"
PARSE2_PY=bogus "$S/shim/python3" exec/build/gen.py parse2 "$S/x.json" >/dev/null 2>&1; r=$?
{ [ "$r" -eq 97 ] && [ ! -e "$S/x.json" ]; } && ok "python3 shim refuses gen.py parse2 under an unknown PARSE2_PY" || no "python3 shim PARSE2_PY=bogus (rc=$r)"
fi
unset SHIMLOG

# harness: extend EXIT trap in the scratch chain only (capture before rm)
python3 - "$CHAIN" <<'PY' || { echo "seedppch: trap harness failed"; exit 2; }
import sys
p = sys.argv[1]
s = open(p).read()
old = "trap 'rm -rf \"$T\"' EXIT"
new = ("trap 'mkdir -p \"$CHAINCAP\"; "
       "cp -f \"$T/e2.json\" \"$CHAINCAP/e2.json\" 2>/dev/null; "
       "cp -f \"$T/e1.json\" \"$CHAINCAP/e1.json\" 2>/dev/null; "
       "cp -f \"$T/e3.json\" \"$CHAINCAP/e3.json\" 2>/dev/null; "
       "cp -f \"$T/e3-gen.err\" \"$CHAINCAP/e3-gen.err\" 2>/dev/null; "
       "cp -f \"$T/e2-gen.err\" \"$CHAINCAP/e2-gen.err\" 2>/dev/null; "
       "cp -f \"$T/e2-gen.out\" \"$CHAINCAP/e2-gen.out\" 2>/dev/null; "
       "rm -rf \"$CHAINCAP/cache\"; cp -a \"$T/.seed-gen-cache\" \"$CHAINCAP/cache\" 2>/dev/null; "
       "rm -rf \"$T\"' EXIT")
if s.count(old) != 1:
    raise SystemExit("trap line not found exactly once")
open(p, "w").write(s.replace(old, new))
PY

# UA: copy current reference so ua_ready short-circuits (UA != /tmp/ua_ref)
if [ ! -x /tmp/ua_ref ]; then echo "seedppch: /tmp/ua_ref missing"; exit 2; fi
cp -f /tmp/ua_ref "$S/ua" && chmod +x "$S/ua" || exit 2

helpers() { awk -F '\t' '$1 ~ /^helper-(pp|parse2)$/ { sub(/^helper-/, "", $1); printf "%s ", $1 }' "$1"; }
KEYSTEP="python3 - cc -std=c99 -O2 -w"
cnt() { awk -F '\t' -v c="$2" -v a="$3" '$1 == c && (a == "" || $2 == a) { n++ } END { print n + 0 }' "$1"; }
two() { [ "$(cat "$1.rc" 2>/dev/null)" = 0 ] && [ "$(wc -l < "$1")" -eq 2 ] \
    && [ "$(grep -c '^seed-gen-[0-9a-f]\{16\} ' "$1")" -eq 1 ] && [ "$(grep -c '^seed-gen-[0-9a-f]\{16\}\.sha256 ' "$1")" -eq 1 ]; }
shaf() { python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$1" 2>/dev/null || echo none; }
haslog() { awk -F '\t' -v c="$2" -v a="$3" '$1 == c && $2 == a { f = 1 } END { exit !f }' "$1"; }
hasclass() { awk -F '\t' -v c="$2" '$1 == c { f = 1 } END { exit !f }' "$1"; }
via_helper() {   # via_helper CAP LOG: keyed seed-gen in captured cache, no direct gen.py pp
    nb=$(ls "$1/cache" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}$')
    ns=$(ls "$1/cache" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}\.sha256$')
    [ "$nb" -eq 1 ] && [ "$ns" -eq 1 ] && ! hasclass "$2" pass-gen-pp && haslog "$2" pass "python3 - cc -std=c99 -O2 -w"
}
run_chain() {   # run_chain CAP LOG [ENV...]: run scratch chain; prints rc; CAP gets snapshots
    cap=$1; log=$2; shift 2
    rm -rf "$cap" "$cap.snap"; mkdir -p "$cap" "$cap.snap"; : > "$log"
    env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC -u SEED_GEN_DIR -u NEG_PARSE2_BIN -u PARSE2_PY -u SEEDPPCH_STOP_AFTER_E3 \
        PATH="$S/shim:$PATH" SHIMLOG="$log" CHAINCAP="$cap" SNAPDIR="$cap.snap" UA="$S/ua" \
        CHAINSHARD=1/1 NETWORK=0 "$@" \
        "$S/shim/sh" "$CHAIN" tests/c/a_char.c > "$log.out" 2>&1
    echo $?
}

if [ "$SLICE" = 1 ]; then
# --- positive default C path ---
cap=$S/cap-pos; log=$S/log-pos
r=$(run_chain "$cap" "$log")
[ "$r" -eq 0 ] && ok "positive chain rc 0" || no "positive chain rc=$r $(tail -3 "$log.out" | tr '\n' ' ')"
via_helper "$cap" "$log" && ok "positive pp through gen-delta (private keyed seed-gen, no direct gen.py pp)" || no "positive not through helper"
hasclass "$log" pass-lex && ok "positive reaches lex after E2" || no "positive lex not reached"
[ -f "$cap/e2.json" ] || { no "positive missing captured e2.json"; }
# independent refs
g=$(ls "$cap/cache"/seed-gen-* 2>/dev/null | grep -v '\.sha256$' | head -1)
rm -f "$S/e2-c-ref.json" "$S/e2-py.json"
[ -n "$g" ] && "$W/tests/bound" 55 "$g" pp "$S/e2-c-ref.json" 2>/dev/null
"$W/tests/bound" 55 "$REALPY" exec/build/gen.py pp "$S/e2-py.json" 2>/dev/null
cp -f "$cap/e2.json" "$S/e2.json" 2>/dev/null || :
[ -f "$S/e2.json" ] && [ -f "$S/e2-c-ref.json" ] && cmp -s "$S/e2.json" "$S/e2-c-ref.json" \
    && ok "positive e2 = seed-gen pp (no flags)" || no "positive e2 differs from seed-gen"
[ -f "$S/e2.json" ] && [ -f "$S/e2-py.json" ] && cmp -s "$S/e2.json" "$S/e2-py.json" \
    && ok "positive e2 = gen.py pp (no flags)" || no "positive e2 differs from gen.py"
! hasclass "$log" UNKNOWN && ok "positive no unknown tool call" || no "positive UNKNOWN: $(grep '^UNKNOWN' "$log" | head -1)"
# K5-1i: parse2 through its helper, reusing the seed-gen the pp helper built
cap=$S/cap-pos; log=$S/log-pos; sd=$cap.snap
hs=$(helpers "$log"); evw "POS helpers=$hs"
a_pp=$(awk -F '\t' '$1 == "helper-pp" { print $2; exit }' "$log"); a_p2=$(awk -F '\t' '$1 == "helper-parse2" { print $2; exit }' "$log")
evw "POS argv pp=$a_pp"; evw "POS argv parse2=$a_p2"
{ [ "$hs" = "pp parse2 " ] && haslog "$log" helper-pp-rc 0 && haslog "$log" helper-parse2-rc 0 \
    && case "$a_pp" in */exec/pp/gen-delta.sh\ */e2.json) true;; *) false;; esac \
    && case "$a_p2" in */exec/parse2gen/gen-delta.sh\ */e3.json) true;; *) false;; esac; } \
    && ok "positive helpers pp -> parse2 (OUT only, no flags), each rc 0" || no "positive helper trajectory '$hs' ($a_pp | $a_p2)"
nc=$(cnt "$log" pass-gen-c ""); nk=$(cnt "$log" pass "$KEYSTEP"); evw "POS compiles=$nc keysteps=$nk"
{ [ "$nc" -eq 1 ] && [ "$nk" -eq 2 ]; } && ok "positive seed/gen.c compiled once, two key steps (pp, parse2)" || no "positive compiles=$nc key steps=$nk"
evw "POS snap_rc pp=$(cat "$sd/after-pp.rc" 2>/dev/null || echo none) parse2=$(cat "$sd/after-parse2.rc" 2>/dev/null || echo none)"
{ two "$sd/after-pp" && two "$sd/after-parse2" && cmp -s "$sd/after-pp" "$sd/after-parse2"; } \
    && ok "positive after-pp = after-parse2: one binary and its sidecar, same inode/mtime/sha (parse2 reused it)" \
    || no "positive cache snapshots differ or are not exactly two members"
{ ! hasclass "$log" pass-parse2 && ! hasclass "$log" short-parse2; } && ok "positive no Python parse2" || no "positive Python parse2 called"
rm -f "$S/e3-c-ref.json"; rr=none
[ -n "$g" ] && { "$W/tests/bound" 55 "$g" parse2 "$S/e3-c-ref.json" 2>/dev/null; rr=$?; }
cp -f "$cap/e3.json" "$S/e3.json" 2>/dev/null || :
evw "POS e3_ref_rc=$rr"
{ [ "$rr" = 0 ] && [ -f "$S/e3.json" ] && cmp -s "$S/e3.json" "$S/e3-c-ref.json"; } \
    && ok "positive e3 = seed-gen parse2 (reference rc 0)" || no "positive e3 vs seed-gen parse2 (reference rc=$rr)"
evw "C_E2_SHA $(shaf "$S/e2.json")"; evw "C_E3_SHA $(shaf "$S/e3.json")"
# inherited SEED_GEN_DIR overridden (sentinel untouched)
mkdir -p "$S/sentinel-dir" && echo sentinel > "$S/sentinel-dir/sentinel"
cap=$S/cap-inh; log=$S/log-inh
r=$(run_chain "$cap" "$log" SEED_GEN_DIR="$S/sentinel-dir" SEEDPPCH_STOP_AFTER_E2=1)
[ "$(ls -A "$S/sentinel-dir")" = sentinel ] && [ "$(cat "$S/sentinel-dir/sentinel")" = sentinel ] \
    && ok "inherited SEED_GEN_DIR overridden (sentinel unchanged)" || no "inherited SEED_GEN_DIR was used"
# early-stop: E2 ok then lex shim exits 1 -> chain "E1 gen failed" (rc 1); wiring still via helper
{ [ "$r" -eq 1 ] && grep -q 'E1 gen failed' "$log.out" && via_helper "$cap" "$log"; } \
    && ok "inherited-override E2 through helper (early-stop after E2)" || no "inherited-override path (rc=$r)"

fi

if [ "$SLICE" = 2 ]; then
# --- known NEG ---
printf '#!/bin/sh\necho seed-gen-red >&2\nexit 3\n' > "$S/red-gen"; chmod +x "$S/red-gen"
# helper direct rc (分列)
hr=$(env -u SEED_GEN_CC SEED_GEN=1 SEED_GEN_BIN="$S/red-gen" SEED_GEN_DIR="$S/hcache-red" \
    sh exec/pp/gen-delta.sh "$S/h-red.json" >"$S/h-red.out" 2>"$S/h-red.err"; echo $?)
[ "$hr" -eq 3 ] && ok "NEG-red helper_rc=3" || no "NEG-red helper_rc=$hr (want 3)"
hr=$(env -u SEED_GEN_CC SEED_GEN=1 SEED_GEN_BIN=/nonexistent/seed-gen SEED_GEN_DIR="$S/hcache-miss" \
    sh exec/pp/gen-delta.sh "$S/h-miss.json" >"$S/h-miss.out" 2>"$S/h-miss.err"; echo $?)
[ "$hr" -eq 2 ] && ok "NEG-missing helper_rc=2" || no "NEG-missing helper_rc=$hr (want 2)"
hr=$(env -u SEED_GEN_BIN -u SEED_GEN_CC SEED_GEN=yes SEED_GEN_DIR="$S/hcache-bad" \
    sh exec/pp/gen-delta.sh "$S/h-bad.json" >"$S/h-bad.out" 2>"$S/h-bad.err"; echo $?)
[ "$hr" -eq 2 ] && ok "NEG-badsg helper_rc=2" || no "NEG-badsg helper_rc=$hr (want 2)"

for case_ in "red:SEED_GEN=1 SEED_GEN_BIN=$S/red-gen" "missing:SEED_GEN=1 SEED_GEN_BIN=/nonexistent/seed-gen" "badsg:SEED_GEN=yes"; do
    n=${case_%%:*}; envs=${case_#*:}
    cap=$S/neg-$n; log=$S/neglog-$n
    r=$(run_chain "$cap" "$log" $envs)
    # chain normalizes to 1; no lex; no python pp fallback; err snapshot nonempty
    { [ "$r" -eq 1 ] && ! hasclass "$log" pass-lex && ! hasclass "$log" pass-gen-pp && ! hasclass "$log" helper-parse2 \
        && [ ! -f "$cap/e1.json" ] && [ -s "$cap/e2-gen.err" ] \
        && grep -q 'E2 gen failed' "$log.out"; } \
        && ok "NEG-$n chain rc=$r, no lex, parse2 helper never started, err captured, no Python fallback" \
        || no "NEG-$n rc=$r lex=$(hasclass "$log" pass-lex; echo $?) err=$([ -s "$cap/e2-gen.err" ] && echo y || echo n) $(tail -2 "$log.out" | tr '\n' ' ')"
    evw "NEG-$n chain_rc=$r"
    [ -n "$EV" ] && cp -f "$cap/e2-gen.err" "$EV/e2-gen.err-$n" 2>/dev/null || true
done

# K5-1i parse2-only red / missing BIN: pp green, parse2 fails, nothing after it
for case_ in "red:3:$S/red-gen" "missing:2:/nonexistent/seed-gen"; do
    n=${case_%%:*}; rest=${case_#*:}; want=${rest%%:*}; bin=${rest#*:}
    cap=$S/np2-$n; log=$S/np2log-$n
    r=$(run_chain "$cap" "$log" NEG_PARSE2_BIN="$bin")
    hs=$(helpers "$log")
    aft=$(awk -F '\t' 'f { n++ } $1 == "helper-parse2-rc" { f = 1 } END { print n + 0 }' "$log")
    { [ "$r" -eq 1 ] && grep -q 'E3 gen failed' "$log.out" && [ "$hs" = "pp parse2 " ] \
        && haslog "$log" helper-pp-rc 0 && haslog "$log" helper-parse2-rc "$want" \
        && [ -f "$cap/e2.json" ] && [ -f "$cap/e1.json" ] && [ ! -f "$cap/e3.json" ] && [ "$aft" -eq 0 ] \
        && ! hasclass "$log" pass-parse2 && ! hasclass "$log" short-parse2 && ! hasclass "$log" pass-gen-pp; } \
        && ok "parse2-only $n: pp rc 0 (e2, e1 made), parse2 rc $want, chain rc 1 E3 gen failed, no e3, no call after parse2, no Python" \
        || no "parse2-only $n (chain rc=$r helpers '$hs' calls_after=$aft) $(tail -2 "$log.out" | tr '\n' ' ')"
    evw "NEG-parse2-$n chain_rc=$r helpers=$hs calls_after_parse2=$aft"
done
# MUTANT-parse2: the old direct gen.py parse2 line (Python parse2 short-circuited: proves the wiring only; stop at tbl)
M2=exec/c/chain-oldparse2.sh
python3 - "$CHAIN" "$M2" <<'PY' || { echo "seedppch: parse2 mutation failed"; exit 2; }
import sys
s = open(sys.argv[1]).read()
needle = 'b 60 env SEED_GEN_DIR="$T/.seed-gen-cache" sh "$R/exec/parse2gen/gen-delta.sh" "$T/e3.json" >"$T/e3-gen.out" 2>"$T/e3-gen.err" || { echo "chain: E3 gen failed"; exit 1; }\n'
old = 'b 60 python3 exec/build/gen.py parse2 "$T/e3.json" >/dev/null 2>&1 || { echo "chain: E3 gen failed"; exit 1; }\n'
if s.count(needle) != 1:
    raise SystemExit("new parse2 line not found exactly once in harness chain")
open(sys.argv[2], "w").write(s.replace(needle, old))
PY
cap=$S/mut2; log=$S/mut2log
cp -f "$CHAIN" "$S/chain-harness2.bak"; cp -f "$M2" "$CHAIN"
r=$(run_chain "$cap" "$log" SEEDPPCH_STOP_AFTER_E3=1)
cp -f "$S/chain-harness2.bak" "$CHAIN"
hs=$(helpers "$log")
{ hasclass "$log" short-parse2 && [ "$hs" = "pp " ]; } \
    && ok "mutant (old direct gen.py parse2) is caught: Python parse2 logged, no parse2 helper" || no "parse2 mutant not caught (rc=$r helpers '$hs')"
evw "MUTANT-parse2 chain_rc=$r helpers=$hs"

# MUTANT: old direct gen.py pp line
M=exec/c/chain-old.sh
python3 - "$CHAIN" "$M" <<'PY' || { echo "seedppch: mutation failed"; exit 2; }
import sys
s = open(sys.argv[1]).read()
# match the production gen-delta invocation (comments may precede)
needle = 'b 60 env SEED_GEN_DIR="$T/.seed-gen-cache" sh "$R/exec/pp/gen-delta.sh" "$T/e2.json" >"$T/e2-gen.out" 2>"$T/e2-gen.err" || { echo "chain: E2 gen failed"; exit 1; }\n'
old = 'b 60 python3 exec/build/gen.py pp "$T/e2.json" >/dev/null 2>&1 || { echo "chain: E2 gen failed"; exit 1; }\n'
if s.count(needle) != 1:
    raise SystemExit("new pp line not found exactly once in harness chain")
open(sys.argv[2], "w").write(s.replace(needle, old))
PY
cap=$S/mut; log=$S/mutlog
# run mutant: temporarily point CHAIN at M by copying over harness chain path used by run_chain
# run_chain always uses $CHAIN; swap file
cp -f "$CHAIN" "$S/chain-harness.bak"
cp -f "$M" "$CHAIN"
r=$(run_chain "$cap" "$log" SEEDPPCH_STOP_AFTER_E2=1)
cp -f "$S/chain-harness.bak" "$CHAIN"
# mutant uses gen.py pp; early-stop after E2; must NOT look like via_helper
{ ! via_helper "$cap" "$log" && hasclass "$log" pass-gen-pp && [ -f "$cap/e2.json" ]; } \
    && ok "mutant (old direct gen.py pp) is caught: not through the helper" \
    || no "mutant not caught (rc=$r via=$(via_helper "$cap" "$log"; echo $?) pp=$(hasclass "$log" pass-gen-pp; echo $?))"
evw "MUTANT chain_rc=$r"
fi

if [ "$SLICE" = 3 ]; then
# the C side in this slice: both helpers, default route, one private cache in the scratch (pp builds cold)
mkdir -p "$S/c3"
env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC SEED_GEN_DIR="$S/c3/cache" "$REALSH" "$W/exec/pp/gen-delta.sh" "$S/c3/e2.json" 2>"$S/c3-pp.err"; c2=$?
env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC SEED_GEN_DIR="$S/c3/cache" "$REALSH" "$W/exec/parse2gen/gen-delta.sh" "$S/c3/e3.json" 2>"$S/c3-parse2.err"; c3=$?
evw "C3 pp_rc=$c2 parse2_rc=$c3"
cp -f "$S/c3/e2.json" "$S/e2.json" 2>/dev/null || :; cp -f "$S/c3/e3.json" "$S/e3.json" 2>/dev/null || :
evw "C_E2_SHA $(shaf "$S/e2.json")"; evw "C_E3_SHA $(shaf "$S/e3.json")"
{ [ "$c2" = 0 ] && [ "$c3" = 0 ]; } && ok "slice-3 C references: pp and parse2 helpers rc 0" || no "slice-3 C references (pp rc=$c2 parse2 rc=$c3)"

# SEED_GEN=0 named reference
cap=$S/cap-py; log=$S/log-py
r=$(run_chain "$cap" "$log" SEED_GEN=0 PARSE2_PY=pass SEEDPPCH_STOP_AFTER_E3=1)
cp -f "$cap/e2.json" "$S/e2-sg0.json" 2>/dev/null || :; cp -f "$cap/e3.json" "$S/e3-py.json" 2>/dev/null || :
{ hasclass "$log" pass-gen-pp && [ -f "$S/e2-sg0.json" ] && [ -f "$S/e2.json" ] && cmp -s "$S/e2-sg0.json" "$S/e2.json" \
    && grep -q 'tbl e2 failed' "$log.out"; } \
    && ok "SEED_GEN=0 named reference: pp helper -> gen.py pp, byte-equal to C e2 (stop at tbl)" \
    || no "SEED_GEN=0 pp route (rc=$r)"
{ [ "$(helpers "$log")" = "pp parse2 " ] && haslog "$log" pass-parse2 "python3 exec/build/gen.py parse2 $(awk -F '\t' '$1 == "helper-parse2" { n = split($2, a, " "); print a[n]; exit }' "$log")" \
    && ! hasclass "$log" pass-gen-c && [ -f "$S/e3-py.json" ] && [ -f "$S/e3.json" ] && cmp -s "$S/e3-py.json" "$S/e3.json" \
    && [ "$(cat "$cap.snap/after-parse2" 2>/dev/null)" = ABSENT ] && [ "$(cat "$cap.snap/after-parse2.rc" 2>/dev/null)" = 0 ]; } \
    && ok "SEED_GEN=0 named reference: parse2 helper -> gen.py parse2 (really run), no compile, no cache, byte-equal to C e3" \
    || no "SEED_GEN=0 parse2 route (rc=$r helpers '$(helpers "$log")')"
evw "SEEDGEN0 chain_rc=$r helpers=$(helpers "$log")"
fi

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$CHAIN") || exit 2
after_gd=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' exec/pp/gen-delta.sh) || exit 2
[ "$before" = "$after" ] && ok "checkout $CHAIN untouched" || no "checkout $CHAIN changed"
[ "$before_gd" = "$after_gd" ] && ok "checkout exec/pp/gen-delta.sh untouched" || no "checkout gen-delta.sh changed"
after_p2=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' exec/parse2gen/gen-delta.sh) || exit 2
[ "$before_p2" = "$after_p2" ] && ok "checkout exec/parse2gen/gen-delta.sh untouched" || no "checkout parse2gen/gen-delta.sh changed"
if [ -n "$EV" ]; then
    case $SLICE in
        1) files="shimneg log-pos log-inh"; dirs="cap-pos.snap";;
        2) files="neglog-red neglog-missing neglog-badsg np2log-red np2log-missing mut2log mutlog"; dirs="np2-red.snap np2-missing.snap";;
        3) files="log-py c3-pp.err c3-parse2.err"; dirs="cap-py.snap";;
    esac
    for f in $files; do
        cp "$S/$f" "$EV/$f" 2>/dev/null || { evfail=1; echo "seedppch: EVIDENCE copy of $f failed" >&2; }
        cp "$S/$f.out" "$EV/$f.out" 2>/dev/null || true
    done
    for d in $dirs; do
        cp -R "$S/$d" "$EV/$d" || { evfail=1; echo "seedppch: EVIDENCE copy of $d failed" >&2; }
    done
    printf 'before_chain %s\nafter_chain %s\nbefore_gendeelta %s\nafter_gendeelta %s\ntree %s\n' \
        "$before" "$after" "$before_gd" "$after_gd" "$H" >> "$EV/identity.txt" || evfail=1
fi
echo "seedppch  slice $SLICE  same $same  bad $bad"
evw "seedppch  slice $SLICE  same $same  bad $bad  tree $H"
[ "$bad" = 0 ] && [ "$same" -gt 0 ] && [ "$evfail" = 0 ]
