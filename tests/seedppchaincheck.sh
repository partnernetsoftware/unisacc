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
# usage: tests/seedppchaincheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
S=$(mktemp -d "${TMPDIR:-/tmp}/seedppch.XXXXXX") || exit 2
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
    for f in e2.json e2-py.json e2-c-ref.json; do
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
BOUND_CACHE=$S/boundcache; export BOUND_CACHE
REALPY=$(command -v python3) && REALCC=$(command -v cc) || { echo "seedppch: python3 or cc missing"; exit 2; }
mkdir "$S/shim"
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
            parse2) log pass-parse2 "python3 \$*"; exec "$REALPY" "\$@";;
            *) log pass "python3 \$*"; exec "$REALPY" "\$@";;
        esac;;
    exec/c/tbl.py|exec/c/net.py) log pass "python3 \$*"; exec "$REALPY" "\$@";;
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
if [ -n "\$known" ] && [ -z "\$run" ] && [ "\$srcs" -eq 1 ]; then log pass "cc \$*"; exec "$REALCC" "\$@"; fi
log UNKNOWN "cc \$*"; exit 97
SHIM
chmod +x "$S/shim/python3" "$S/shim/cc"
same=0; bad=0
ok() { echo "SAME $1"; evw "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; evw "DIFF $1"; bad=$((bad + 1)); }
# shim negatives
SHIMLOG=$S/shimneg; export SHIMLOG; : > "$SHIMLOG"
"$S/shim/python3" exec/c/other.py >/dev/null 2>&1; r=$?
[ "$r" -eq 97 ] && ok "python3 shim refuses an unknown script" || no "python3 shim did not refuse (rc=$r)"
"$S/shim/cc" -o x >/dev/null 2>&1; r=$?
[ "$r" -eq 97 ] && ok "cc shim refuses unknown argv" || no "cc shim did not refuse (rc=$r)"
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

haslog() { awk -F '\t' -v c="$2" -v a="$3" '$1 == c && $2 == a { f = 1 } END { exit !f }' "$1"; }
hasclass() { awk -F '\t' -v c="$2" '$1 == c { f = 1 } END { exit !f }' "$1"; }
via_helper() {   # via_helper CAP LOG: keyed seed-gen in captured cache, no direct gen.py pp
    nb=$(ls "$1/cache" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}$')
    ns=$(ls "$1/cache" 2>/dev/null | grep -c '^seed-gen-[0-9a-f]\{16\}\.sha256$')
    [ "$nb" -eq 1 ] && [ "$ns" -eq 1 ] && ! hasclass "$2" pass-gen-pp && haslog "$2" pass "python3 - cc -std=c99 -O2 -w"
}
run_chain() {   # run_chain CAP LOG [ENV...]: run scratch chain; prints rc; CAP gets snapshots
    cap=$1; log=$2; shift 2
    rm -rf "$cap"; mkdir -p "$cap"; : > "$log"
    env -u SEED_GEN -u SEED_GEN_BIN -u SEED_GEN_CC -u SEED_GEN_DIR \
        PATH="$S/shim:$PATH" SHIMLOG="$log" CHAINCAP="$cap" UA="$S/ua" \
        CHAINSHARD=1/1 NETWORK=0 "$@" \
        sh "$CHAIN" tests/c/a_char.c > "$log.out" 2>&1
    echo $?
}

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
# inherited SEED_GEN_DIR overridden (sentinel untouched)
mkdir -p "$S/sentinel-dir" && echo sentinel > "$S/sentinel-dir/sentinel"
cap=$S/cap-inh; log=$S/log-inh
r=$(run_chain "$cap" "$log" SEED_GEN_DIR="$S/sentinel-dir" SEEDPPCH_STOP_AFTER_E2=1)
[ "$(ls -A "$S/sentinel-dir")" = sentinel ] && [ "$(cat "$S/sentinel-dir/sentinel")" = sentinel ] \
    && ok "inherited SEED_GEN_DIR overridden (sentinel unchanged)" || no "inherited SEED_GEN_DIR was used"
# early-stop: E2 ok then lex shim exits 1 -> chain "E1 gen failed" (rc 1); wiring still via helper
{ [ "$r" -eq 1 ] && grep -q 'E1 gen failed' "$log.out" && via_helper "$cap" "$log"; } \
    && ok "inherited-override E2 through helper (early-stop after E2)" || no "inherited-override path (rc=$r)"

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
    { [ "$r" -eq 1 ] && ! hasclass "$log" pass-lex && ! hasclass "$log" pass-gen-pp \
        && [ ! -f "$cap/e1.json" ] && [ -s "$cap/e2-gen.err" ] \
        && grep -q 'E2 gen failed' "$log.out"; } \
        && ok "NEG-$n chain rc=$r, no lex, err captured, no Python fallback" \
        || no "NEG-$n rc=$r lex=$(hasclass "$log" pass-lex; echo $?) err=$([ -s "$cap/e2-gen.err" ] && echo y || echo n) $(tail -2 "$log.out" | tr '\n' ' ')"
    evw "NEG-$n chain_rc=$r"
    [ -n "$EV" ] && cp -f "$cap/e2-gen.err" "$EV/e2-gen.err-$n" 2>/dev/null || true
done

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

# SEED_GEN=0 named reference
cap=$S/cap-py; log=$S/log-py
r=$(run_chain "$cap" "$log" SEED_GEN=0 SEEDPPCH_STOP_AFTER_E2=1)
{ hasclass "$log" pass-gen-pp && [ -f "$cap/e2.json" ] && [ -f "$S/e2.json" ] && cmp -s "$cap/e2.json" "$S/e2.json" \
    && grep -q 'E1 gen failed' "$log.out"; } \
    && ok "SEED_GEN=0 named reference: helper -> gen.py pp, byte-equal to C e2 (early-stop)" \
    || no "SEED_GEN=0 route (rc=$r)"
evw "SEEDGEN0 chain_rc=$r"

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$CHAIN") || exit 2
after_gd=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' exec/pp/gen-delta.sh) || exit 2
[ "$before" = "$after" ] && ok "checkout $CHAIN untouched" || no "checkout $CHAIN changed"
[ "$before_gd" = "$after_gd" ] && ok "checkout exec/pp/gen-delta.sh untouched" || no "checkout gen-delta.sh changed"
if [ -n "$EV" ]; then
    for f in shimneg log-pos log-inh neglog-red neglog-missing neglog-badsg mutlog log-py; do
        cp "$S/$f" "$EV/$f" 2>/dev/null || { evfail=1; echo "seedppch: EVIDENCE copy of $f failed" >&2; }
        cp "$S/$f.out" "$EV/$f.out" 2>/dev/null || true
    done
    printf 'before_chain %s\nafter_chain %s\nbefore_gendeelta %s\nafter_gendeelta %s\ntree %s\n' \
        "$before" "$after" "$before_gd" "$after_gd" "$H" >> "$EV/identity.txt" || evfail=1
fi
echo "seedppch  same $same  bad $bad"
evw "seedppch  same $same  bad $bad  tree $H"
[ "$bad" = 0 ] && [ "$same" -gt 0 ] && [ "$evfail" = 0 ]
