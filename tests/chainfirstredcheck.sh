#!/bin/sh
# K5-1i 首红后零启动（0902 裁定 (A) 续；夹具）。Controlled check -- not exec-chain acceptance.
#  * scratch tree = git archive of the fixed tip (default 7b07b723), the worktree must match it under the archived
#    paths; nothing in the worktree is touched.
#  * PATH shims for sh / python3 / cc log every call (class, argv, rc) to SHIMLOG, tab-separated: ns, class, argv.
#    cc -o X exec/c/run.c leaves a wrapper at X that logs probe-run / check-net before exec'ing the real binary.
#    UA is a logging wrapper around a reference binary built once in the scratch (tests/build_ref.sh).
#  * POS: E3 really runs (helper rc 0); the chain reaches tbl, probe run and UA.  Proves the logging sees them.
#  * NEG: the sh shim makes the E3 helper (chain.sh:55) return rc 3 without running it.  The first non-zero child rc
#    is that 3; after it there must be ZERO calls of tbl / net / check-net / probe-run / ua-ref / gen (python) and
#    no other call at all; chain rc 1 with "chain: E3 gen failed"; no e3.json.
#  * A NEG pass is a predicted negative: it shows the command layer did not start later work.  It does not show
#    process-group or tree clearing, TERM, UA behaviour beyond the wrapper, or the exec-chain budget.
# usage: tests/chainfirstredcheck.sh [pos|neg]      (no argument: pos then neg)
# env:   CHAINFR_TIP=<commit> (default 7b07b723)
#        FIRSTRED_EVIDENCE=<new or empty dir outside the scratch>: keeps shimlogs, chain stdout/stderr, rc, e3-gen.err,
#        sha256 sums and a summary, written before the scratch is removed.  A failing run keeps the scratch.
set -u
R=$(cd "$(dirname "$0")/.." && pwd) || exit 2; cd "$R" || exit 2
TIP=${CHAINFR_TIP:-7b07b72343907d06f797340ae26585f14174a377}
PATHS="exec seed unisa include kernel src unisacc.c weights Makefile tests/bound tests/bound.c tests/bound.py tests/lib.sh tests/knownfail.py tests/build_ref.sh tests/export_ref.sh tests/sourceflat.py tests/refshim.h tests/reffoot.h tests/c/a_char.c"
git rev-parse --verify -q "$TIP^{commit}" >/dev/null || { echo "chainfirstred: no commit $TIP"; exit 2; }
git diff --quiet "$TIP" -- $PATHS || { echo "chainfirstred: worktree differs from $TIP under the archived paths"; exit 2; }
[ -z "$(git status --porcelain --untracked-files=normal -- $PATHS)" ] || { echo "chainfirstred: untracked files under the archived paths"; exit 2; }
want=${1:-both}
case $want in pos|neg|both) ;; *) echo "usage: $0 [pos|neg]"; exit 2;; esac
REALSH=$(command -v sh) && REALPY=$(command -v python3) && REALCC=$(command -v cc) \
    || { echo "chainfirstred: sh, python3 or cc missing"; exit 2; }
S=$(mktemp -d "${TMPDIR:-/tmp}/chainfr.XXXXXX") || exit 2
EV=${FIRSTRED_EVIDENCE:-}
keep=0
if [ -n "$EV" ]; then
    case "$EV" in "$S"|"$S"/*) echo "chainfirstred: FIRSTRED_EVIDENCE inside the scratch"; exit 2;; esac
    mkdir -p "$EV" && [ -z "$(ls -A "$EV")" ] || { echo "chainfirstred: FIRSTRED_EVIDENCE must be a new or empty directory: $EV"; exit 2; }
fi
finish() {
    st=$1
    if [ "$keep" -ne 0 ] || [ "$st" -ne 0 ]; then echo "chainfirstred: scratch kept at $S" >&2; else rm -rf "$S"; fi
    exit "$st"
}
trap 'finish $?' EXIT
trap 'exit 130' INT TERM HUP
W=$S/tree; mkdir "$W" "$S/shim" "$S/ua" "$S/case" || exit 2
H=$(git rev-parse --verify "$TIP^{commit}") || exit 2
git archive -o "$S/tree.tar" "$H" $PATHS || { echo "chainfirstred: git archive failed"; exit 2; }
tar -x -C "$W" -f "$S/tree.tar" || { echo "chainfirstred: scratch tree failed"; exit 2; }
BOUND_CACHE=$S/boundcache; export BOUND_CACHE

# ---- shims (literal heredocs; runtime values come from the environment) ----
cat > "$S/shim/sh" <<'SHIM'
#!/bin/sh
log() { printf '%s\t%s\t%s\n' "$(date +%s%N)" "$1" "$2" >> "$SHIMLOG"; }
case "${1:-}" in
*/exec/parse2gen/gen-delta.sh)
    log helper-parse2 "$*"
    if [ "${FR_MODE:-pos}" = neg ]; then
        echo "fr-shim: injected E3 helper failure rc 3 (helper not run)" >&2; rc=3
    else
        "$FR_REALSH" "$@"; rc=$?
    fi
    if [ -f "$(dirname "$2")/e3.json" ]; then e=yes; else e=no; fi
    log e3json-present "$e"
    cp -f "$(dirname "$2")/e3-gen.err" "$SHIMLOG.e3-gen.err" 2>/dev/null || :
    log helper-parse2-rc "$rc"
    exit "$rc";;
*/exec/pp/gen-delta.sh)
    log helper-pp "$*"; "$FR_REALSH" "$@"; rc=$?
    log helper-pp-rc "$rc"; exit "$rc";;
*) exec "$FR_REALSH" "$@";;
esac
SHIM
cat > "$S/shim/python3" <<'SHIM'
#!/bin/sh
log() { printf '%s\t%s\t%s\n' "$(date +%s%N)" "$1" "$2" >> "$SHIMLOG"; }
cls=py-other
case "$*" in
*exec/c/tbl.py*) cls=tbl;;
*exec/c/net.py*) cls=net;;
*exec/build/gen.py*) cls=gen;;
esac
log "$cls" "$*"
"$FR_REALPY" "$@"; rc=$?
log "$cls-rc" "$rc"
exit "$rc"
SHIM
cat > "$S/shim/cc" <<'SHIM'
#!/bin/sh
log() { printf '%s\t%s\t%s\n' "$(date +%s%N)" "$1" "$2" >> "$SHIMLOG"; }
out=""; prev=""
for a in "$@"; do [ "$prev" = -o ] && out=$a; prev=$a; done
case "$*" in
*" exec/c/run.c"*)
    log cc-run "$*"
    "$FR_REALCC" "$@"; rc=$?
    log cc-run-rc "$rc"
    [ "$rc" -eq 0 ] && [ -n "$out" ] || exit "$rc"
    mv -f "$out" "$out.real" || exit 1
    cat > "$out" <<'WRAP'
#!/bin/sh
c=probe-run; [ "${1:-}" = --check-net ] && c=check-net
printf '%s\t%s\t%s\n' "$(date +%s%N)" "$c" "$*" >> "$SHIMLOG"
exec "$0.real" "$@"
WRAP
    chmod +x "$out" || exit 1
    exit 0;;
*)
    log cc-other "$*"
    exec "$FR_REALCC" "$@";;
esac
SHIM
cat > "$S/shim/ua_ref" <<'SHIM'
#!/bin/sh
printf '%s\t%s\t%s\n' "$(date +%s%N)" ua-ref "$*" >> "$SHIMLOG"
exec "$FR_UA_BIN" "$@"
SHIM
chmod +x "$S/shim/sh" "$S/shim/python3" "$S/shim/cc" "$S/shim/ua_ref" || exit 2

# ---- reference binary for UA, built once in the scratch (not via the shim) ----
( cd "$W" && "$W/tests/bound" 40 "$REALSH" tests/build_ref.sh "$S/ua/ua_ref.c" "$S/ua/ua_ref.bin" >"$S/ua/build.out" 2>&1 )
ua_rc=$?
rm -f "$S/ua/ua_ref.c"
[ "$ua_rc" -eq 0 ] && [ -x "$S/ua/ua_ref.bin" ] || { echo "chainfirstred: UA reference build failed rc $ua_rc (see $S/ua/build.out)"; keep=1; exit 2; }
printf 'tests/c/a_char.c\n' > "$S/keep.txt"

# ---- one case: run chain in the scratch with the shims, then assert from SHIMLOG ----
run_case() {   # run_case NAME MODE
    name=$1; mode=$2
    cd "$W" || return 2
    SL=$S/case/$name.shimlog; : > "$SL"
    env PATH="$S/shim:$PATH" SHIMLOG="$SL" FR_MODE="$mode" FR_REALSH="$REALSH" FR_REALPY="$REALPY" \
        FR_REALCC="$REALCC" FR_UA_BIN="$S/ua/ua_ref.bin" UA="$S/shim/ua_ref" \
        CHAINSHARD=1/1 NETWORK="${CHAINFR_NETWORK:-1}" CHAINKEEP="$S/keep.txt" \
        "$W/tests/bound" 58 "$REALSH" exec/c/chain.sh tests/c/a_char.c \
        >"$S/case/$name.out" 2>"$S/case/$name.err"
    rc=$?
    echo "$rc" > "$S/case/$name.rc"
    cd "$R" || return 2
    return 0
}

# class count after the first non-zero E3 rc (line red); line 0 means there is no red
count_after() {   # count_after LOG RED CLASS
    awk -F'\t' -v r="$2" -v c="$3" 'NR>r && $2==c {n++} END{print n+0}' "$1"
}
fail=0
report() { echo "chainfirstred: $*"; }
check() { if [ "$1" = ok ]; then report "ok   $2"; else report "FAIL $2"; fail=1; fi; }
verdict() {   # verdict PASSFLAG TEXT
    [ "$1" = 1 ] && check ok "$2" || check no "$2"
}

if [ "$want" = pos ] || [ "$want" = both ]; then
    run_case pos pos
    L=$S/case/pos.shimlog
    rc=$(cat "$S/case/pos.rc")
    red=$(awk -F'\t' '$2=="helper-parse2-rc" && $3!=0 {print NR; exit}' "$L")
    red=${red:-0}
    p=0
    [ "$rc" = 0 ] && grep -Eq "^chain (tbl|net) +files 1 +equal 1 " "$S/case/pos.out" && p=1; verdict "$p" "POS chain rc 0 and one equal probe (rc=$rc)"
    p=0; [ "$(awk -F'\t' '$2=="helper-parse2-rc"{print $3}' "$L")" = 0 ] && p=1; verdict "$p" "POS E3 helper ran and returned 0"
    p=0; [ "$(awk -F'\t' '$2=="e3json-present"{print $3}' "$L")" = yes ] && p=1; verdict "$p" "POS e3.json was produced"
    tb=$(count_after "$L" 0 tbl); pr=$(count_after "$L" 0 probe-run); ua=$(count_after "$L" 0 ua-ref)
    nt=$(count_after "$L" 0 net); cn=$(count_after "$L" 0 check-net)
    p=0; [ "$tb" -gt 0 ] && [ "$pr" -gt 0 ] && [ "$ua" -gt 0 ] && p=1
    verdict "$p" "POS the later steps really ran (tbl=$tb probe-run=$pr ua-ref=$ua) -- the logger sees them"
    p=0; [ "$nt" -gt 0 ] && [ "$cn" -gt 0 ] && p=1
    verdict "$p" "POS net and check-net really ran (net=$nt check-net=$cn), so the NEG zeros are not trivial"
    report "POS first red line: $red (0 = none); lines $(wc -l < "$L")"
fi

if [ "$want" = neg ] || [ "$want" = both ]; then
    run_case neg neg
    L=$S/case/neg.shimlog
    rc=$(cat "$S/case/neg.rc")
    red=$(awk -F'\t' '$2=="helper-parse2-rc" && $3!=0 {print NR; exit}' "$L")
    red=${red:-0}
    first=$(awk -F'\t' '$2 ~ /-rc$/ && $3!=0 {print $2"="$3; exit}' "$L")
    p=0; [ "$rc" = 1 ] && grep -q '^chain: E3 gen failed$' "$S/case/neg.out" && p=1
    verdict "$p" "NEG chain rc 1 and 'chain: E3 gen failed' (rc=$rc)"
    p=0; [ "$first" = "helper-parse2-rc=3" ] && p=1; verdict "$p" "NEG first non-zero child rc is the E3 helper (first=$first)"
    p=0; [ "$red" -gt 0 ] && [ "$(awk -F'\t' '$2=="e3json-present"{print $3}' "$L")" = no ] && p=1
    verdict "$p" "NEG E3 red recorded and no e3.json"
    for c in tbl net check-net probe-run ua-ref gen; do
        n=$(count_after "$L" "$red" "$c")
        p=0; [ "$n" -eq 0 ] && p=1; verdict "$p" "NEG after first red: $c = $n"
    done
    all=$(awk -F'\t' -v r="$red" 'NR>r {n++} END{print n+0}' "$L")
    p=0; [ "$all" -eq 0 ] && p=1; verdict "$p" "NEG after first red: all shim events = $all"
    report "NEG first red line $red of $(wc -l < "$L"); the scratch shimlog is the evidence"
fi

if [ -n "$EV" ]; then
    for n in pos neg; do
        [ -f "$S/case/$n.shimlog" ] || continue
        cp "$S/case/$n.shimlog" "$EV/$n.shimlog"; cp "$S/case/$n.out" "$EV/$n.chain.out"; cp "$S/case/$n.err" "$EV/$n.chain.err"
        cp "$S/case/$n.rc" "$EV/$n.rc"
        [ -f "$S/case/$n.shimlog.e3-gen.err" ] && cp "$S/case/$n.shimlog.e3-gen.err" "$EV/$n.e3-gen.err"
    done
    cp "$S/ua/build.out" "$EV/ua-build.out" 2>/dev/null || :
    ( cd "$EV" && sha256sum * > SHA256SUMS ) || { echo "chainfirstred: EVIDENCE sha256 failed"; fail=1; }
    echo "tip $H" > "$EV/summary.txt"
    echo "fail $fail" >> "$EV/summary.txt"
fi
[ "$fail" -eq 0 ] || keep=1
exit "$fail"
