#!/bin/sh
# exec/c/comboot.sh -- exec/c/comboot.py without Python (0.0.32 B5).  Same commands, files and output:
#   comboot.sh stage N FILE    record FILE as stage N in out/comboot/stages.json (sha256, bytes, and
#                              built_by_sha256 = stage N-1's sha256 when N > 1)
#   comboot.sh cmp A B         the fixed point: A and B byte-equal
#   comboot.sh shard NAME      one com-comboot job: seed | stage2 | stage3 | fixedpoint; each does only
#                              its own step, checks its prerequisites, says `skipped:` (exit 0; STRICT=1
#                              fails) when they are absent, and exits 75 at a step boundary once
#                              COMBOOT_BUDGET seconds (default 40) have gone -- re-run the same shard
#   comboot.sh report          the three sha256 values
# Step markers hold "<source identity> <sha256 of the step's proof file>", the identity being seed/ident.c's
# (= exec/c/provenance.py source_digest, what build.json records), so markers from either tool agree.
# Every make is bounded by the native tests/bound 55.  See comboot.py for the design notes (N22).
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$ROOT/out/comboot; REC=$OUT/stages.json
SEED_DIR=${SEED_DIR:-/tmp/unisacc-seed-com}; SEED=$SEED_DIR/unisacc-seed.com
STAGE2=$ROOT/unisacc.com; STAGE3=$SEED_DIR/stage3/unisacc.com; COMB_BUILD=$SEED_DIR/comb-build
HELLO=$ROOT/examples/hello.c; SEED_PROBE='hello from C99'
BUDGET=${COMBOOT_BUDGET:-40}; T0=$(date +%s)
STEP_LIST="shared lnx/arm64 lnx/x86_64 osx/arm64 osx/x86_64 win/arm64 win/x86_64 pack-prep-1 pack-prep-2 pack-prep-3 pack-models pack-driver"
STRICT=${STRICT:-0}
B=$ROOT/tests/bound
die() { printf '%s\n' "$*" >&2; exit 1; }
sha() { shasum -a 256 "$1" | cut -d' ' -f1; }
size() { wc -c < "$1" | tr -d ' '; }
shown() { case $1 in "$ROOT"/*) printf '%s' "${1#"$ROOT"/}";; *) printf '%s' "$1";; esac; }

# stages.json: json.dumps(indent=2, sort_keys=True) of {stageN: {built_by_sha256?, bytes, path, sha256}}
get() {   # get stageN field
    [ -f "$REC" ] || return 0
    awk -v k="\"$1\": {" -v f="\"$2\": " 'index($0, k) { inb = 1; next } inb && /^  }/ { inb = 0 }
        inb && index($0, f) { v = substr($0, index($0, f) + length(f)); sub(/,$/, "", v); gsub(/"/, "", v); print v; exit }' "$REC"
}
jstr() { printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'; }
save() {   # save: rewrite REC from the variables s1_* s2_* s3_*
    mkdir -p "$OUT"; tmp=$REC.tmp.$$; first=1
    { printf '{'
      for _i in 1 2 3; do
        eval "_p=\${s${_i}_path:-}; _h=\${s${_i}_sha:-}; _z=\${s${_i}_bytes:-}; _b=\${s${_i}_by:-}"
        [ -n "$_h" ] || continue
        [ $first = 1 ] && printf '\n' || printf ',\n'; first=0
        printf '  "stage%s": {\n' "$_i"
        [ -n "$_b" ] && printf '    "built_by_sha256": "%s",\n' "$_b"
        printf '    "bytes": %s,\n    "path": "%s",\n    "sha256": "%s"\n  }' "$_z" "$(jstr "$_p")" "$_h"
      done
      [ $first = 1 ] && printf '}\n' || printf '\n}\n'
    } > "$tmp" && mv -f "$tmp" "$REC"
}
load() { for _i in 1 2 3; do eval "s${_i}_path=\$(get stage$_i path); s${_i}_sha=\$(get stage$_i sha256); s${_i}_bytes=\$(get stage$_i bytes); s${_i}_by=\$(get stage$_i built_by_sha256)"; done; }

stage() {   # stage N PATH
    n=$1; p=$2
    [ -s "$p" ] || die "comboot: stage $n artifact missing: $p"
    load
    eval "s${n}_path=\$p; s${n}_sha=\$(sha \"\$p\"); s${n}_bytes=\$(size \"\$p\"); s${n}_by="
    if [ "$n" -gt 1 ]; then
        eval "prev=\${s$((n-1))_sha:-}"
        [ -n "$prev" ] || die "comboot: stage $n recorded before stage $((n-1))"
        eval "s${n}_by=\$prev"
    fi
    save
    eval "echo \"comboot stage $n  \$p  sha256 \$s${n}_sha\""
}
cmp2() {
    for p in "$1" "$2"; do [ -e "$p" ] || die "comboot: missing $p"; done
    if ! cmp -s "$1" "$2"; then
        printf 'comboot: NOT a fixed point\n  %s  %s\n  %s  %s\n' "$1" "$(sha "$1")" "$2" "$(sha "$2")" >&2; exit 1
    fi
    echo "comboot fixed point holds: $1 == $2 (sha256 $(sha "$1"))"
}
skipped() { echo "skipped: $1 not built ($2)"; [ "$STRICT" = 1 ] && exit 1; exit 0; }

IDENT=
source_digest() {
    if [ -z "$IDENT" ]; then
        mkdir -p "$OUT"; IDENT=$OUT/ident
        "$B" 50 cc -std=c99 -D_POSIX_C_SOURCE=200809L -O2 -w "$ROOT/seed/ident.c" -o "$IDENT.$$" && mv -f "$IDENT.$$" "$IDENT" || die "comboot: seed/ident.c does not build"
    fi
    "$B" 50 "$IDENT" "$ROOT"
}
step_manifest() {   # model_dir step
    case $2 in
        pack|pack-driver) echo "$1/unisacc-next.com.build.json";;
        pack-prep-*) echo "$1/$2.done";;
        pack-models) echo "$1/compiler.pkg";;
        *) echo "$1/$(echo "$2" | tr / -)/manifest.json";;
    esac
}
marker() { echo "$1/step-$(echo "$2" | tr / _)"; }
step_done() {
    m=$(step_manifest "$1" "$2"); k=$(marker "$1" "$2")
    [ -e "$m" ] && [ -e "$k" ] || return 1
    [ "$(cat "$k")" = "$(source_digest) $(sha "$m")" ]
}
mark_done() {
    m=$(step_manifest "$1" "$2")
    [ -e "$m" ] || die "comboot: step $2 reported success but wrote no $m; refusing to record it as done"
    mkdir -p "$1"; printf '%s %s\n' "$(source_digest)" "$(sha "$m")" > "$(marker "$1" "$2")"
}
build_step() {   # the one place make is called, one bounded step
    step_done "$1" "$2" && return 0
    log=$1/.comboot-step.log; mkdir -p "$1"
    (cd "$ROOT" && "$B" 55 make model-com "MODEL_DIR=$1" "MODEL_STEP=$2") > "$log" 2>&1; rc=$?
    [ $rc = 0 ] || { echo "comboot: model step $2 failed (rc=$rc); last output:" >&2; tail -20 "$log" >&2; exit 1; }
    mark_done "$1" "$2"
}
build_model() {
    [ -e "$1/unisacc-next.com" ] && step_done "$1" pack-driver && return 0
    for st in $STEP_LIST; do
        if ! step_done "$1" "$st" && [ $(( $(date +%s) - T0 )) -gt "${BUDGET%.*}" ]; then
            echo "comboot: $(( $(date +%s) - T0 )) s used, step $st pending; re-run the same shard (exit 75)"; exit 75
        fi
        build_step "$1" "$st"
    done
}
install_stage() {   # model_dir dest: artifact + build.json, tmp + rename
    src=$1/unisacc-next.com; [ -e "$src" ] || die "comboot: no $src after the build"
    mkdir -p "$(dirname "$2")"
    cp "$src" "$2.tmp.$$" && chmod 755 "$2.tmp.$$" && mv -f "$2.tmp.$$" "$2"
    cp "$src.build.json" "$2.build.json.tmp.$$" && chmod 644 "$2.build.json.tmp.$$" && mv -f "$2.build.json.tmp.$$" "$2.build.json"
}
LOCK=
lock() {   # mkdir lock with the owner's pid; a dead owner's lock is taken over
    LOCK=$1.lock; mkdir -p "$(dirname "$LOCK")"
    if mkdir "$LOCK" 2>/dev/null; then echo $$ > "$LOCK/pid"; return 0; fi
    owner=$(cat "$LOCK/pid" 2>/dev/null || :)
    if [ -z "$owner" ] || ! kill -0 "$owner" 2>/dev/null; then rm -rf "$LOCK"; lock "$1"; return 0; fi
    die "comboot: $2 is being built by another run ($LOCK); invoke this shard again once it is done"
}
unlock() { [ -n "$LOCK" ] && rm -rf "$LOCK"; LOCK=; }
seed_ready() {
    side=$SEED.build.json
    [ -e "$side" ] || die "comboot: $SEED has no build.json sidecar; rebuild the seed with \`make seed-com SEED_DIR=$SEED_DIR\`"
    want=$(sed -n 's/.*"artifact_sha256": *"\([0-9a-f]*\)".*/\1/p' "$side"); got=$(sha "$SEED")
    [ "$want" = "$got" ] || die "comboot: seed sha256 $got does not match its sidecar $side ($want); the seed is not the artifact it claims to be"
    text=$("$B" 55 /bin/sh "$SEED" -run "$HELLO" 2>&1); rc=$?
    case "$rc:$text" in 0:*"$SEED_PROBE"*) ;; *) die "comboot: the seed did not run hello.c (rc=$rc)
$text";; esac
    echo "comboot seed ok  sha256 $got   runs hello.c"
}
shard() {
    case $1 in
    seed)
        [ -e "$SEED" ] || skipped seed "make seed-com SEED_DIR=$SEED_DIR"
        seed_ready
        [ "$(get stage1 sha256)" = "$(sha "$SEED")" ] || stage 1 "$SEED";;
    stage2)
        [ -e "$SEED" ] || skipped stage2 "the seed it is built from: make seed-com SEED_DIR=$SEED_DIR"
        d=$COMB_BUILD/stage2; lock "$d" "stage 2"; trap unlock EXIT
        UA=$SEED; export UA
        build_model "$d"; install_stage "$d" "$STAGE2"; unlock
        [ -e "$STAGE2" ] || die "comboot-shard stage2: no $STAGE2 after the build"
        stage 2 "$STAGE2"
        [ "$(get stage2 built_by_sha256)" = "$(get stage1 sha256)" ] || die "comboot-shard stage2: sidecar seed sha256 does not match stage 1"
        echo "comboot-shard stage2: sidecar seed sha256 matches stage 1";;
    stage3)
        [ -e "$SEED" ] || skipped stage3 "the seed it is built from: make seed-com SEED_DIR=$SEED_DIR"
        [ -e "$STAGE2" ] || skipped stage3 "stage 2 ($(shown "$STAGE2")): run the stage2 shard"
        [ "$(get stage2 built_by_sha256)" = "$(get stage1 sha256)" ] || skipped stage3 "a stage 2 built by this seed ($(shown "$STAGE2") is not; run the stage2 shard)"
        d=$COMB_BUILD/stage3; lock "$d" "stage 3"; trap unlock EXIT
        UA=$STAGE2; export UA
        build_model "$d"; install_stage "$d" "$STAGE3"; unlock
        [ -e "$STAGE3" ] || die "comboot-shard stage3: no $STAGE3 after the build"
        stage 3 "$STAGE3";;
    fixedpoint)
        [ -e "$STAGE2" ] && [ -e "$STAGE3" ] || skipped fixedpoint "stage 2 and stage 3 ($(shown "$STAGE2"), $(shown "$STAGE3"))"
        cmp2 "$STAGE2" "$STAGE3";;
    *) die "comboot: unknown shard '$1'";;
    esac
}
case ${1:-} in
    stage) [ $# = 3 ] || die "usage: comboot.sh stage N FILE"; stage "$2" "$3";;
    cmp) [ $# = 3 ] || die "usage: comboot.sh cmp A B"; cmp2 "$2" "$3";;
    shard) [ $# = 2 ] || die "usage: comboot.sh shard NAME"; shard "$2";;
    report) for n in 1 2 3; do v=$(get stage$n sha256); printf '%-7s %s\n' stage$n "${v:-(unrecorded)}"; done;;
    *) sed -n '2,13p' "$0" >&2; exit 2;;
esac
