#!/bin/sh
# stagerun.sh STAGE... -- run the gate suites of compiler stages (tests/stagemap.c) on a frozen snapshot
# of HEAD, so commits landing on the shared main during the run cannot void it ("inputs changed").
# The snapshot is a shared clone (objects borrowed, not a worktree) under $TMPDIR, reused per commit.
# Needs UA, UNISACC_FFI_X86_PROVIDER; MODEL_COM defaults to the snapshot's unisacc.com; JOBS default 4.
# Run it directly (not under term.sh): each window is its own term.sh hand-off.
# Prints the queue line per window; exit 0 all green, 1 a failure, 2 usage.
plan_only=0
if [ "${1:-}" = --plan-only ]; then plan_only=1; shift; fi
[ $# -ge 1 ] || { echo 'usage: tests/stagerun.sh [--plan-only] STAGE...' >&2; exit 2; }
# Preserve the user's stage arguments before the original mapper replaces argv.
stage_args=$(mktemp "${TMPDIR:-/tmp}/stagerun-stages.XXXXXX") || exit 2
trap 'rm -f "$stage_args"' EXIT
printf '%s\n' "$@" > "$stage_args"

R=$(cd "$(dirname "$0")/.." && pwd); head=$(git -C "$R" rev-parse HEAD) || exit 2
snap=${TMPDIR:-/tmp}/unisacc-stage-$(echo "$head" | cut -c1-12)
if [ ! -d "$snap/.git" ]; then
    rm -rf "$snap.part"; git clone -q --shared --no-checkout "$R" "$snap.part" && git -C "$snap.part" checkout -q "$head" || exit 2
    for f in unisacc.com unisacc.com.build.json unisacc-seed.com unisacc-seed.com.build.json; do
        [ -f "$R/$f" ] && cp -p "$R/$f" "$snap.part/"
    done
    # the corpus is gitignored, so a clone has none: link it per entry as release/tools/queue.sh does
    # (without it diag saw 0 damaged-corpus programs and warn read nothing from cc)
    for c in "$R"/corpus/*; do [ -e "$c" ] && { mkdir -p "$snap.part/corpus"; ln -s "$c" "$snap.part/corpus/${c##*/}"; }; done
    mv "$snap.part" "$snap"
fi
cd "$snap" || exit 2
com=${MODEL_COM:-$snap/unisacc.com}
set --  $(UNISACC_FFI_X86_PROVIDER=LIST ./tests/gate.sh --list --com | "$com" -run tests/stagemap.c -- --stage "$@") || exit 2
[ $# -gt 0 ] || { echo 'stagerun: no suites selected' >&2; exit 2; }
# Delegate recommendations to the same planner, but keep the legacy mapper's
# actual execution set. A subshell preserves that original argv exactly.
(
  for n in "$@"; do set -- "$@" --actual-suite "$n"; done
  # Drop the leading legacy names; retain only their explicit comparison options.
  while [ "$#" -gt 0 ] && [ "$1" != --actual-suite ]; do shift; done
  while IFS= read -r stage; do set -- "$@" --stage "$stage"; done < "$stage_args"
  env UNISACC_TIME_SCALE=1 python3 "$R/tests/bound.py" 5 python3 "$R/tests/gateselect.py" --root "$snap" \
    --com --tier dev --base "$head" --head "$head" --shadow-report --comparison-label legacy-stage "$@"
) || echo 'stagerun: gateselect shadow UNAVAILABLE (5s budget or planning refusal); legacy selection retained' >&2
printf 'stagerun legacy: suites=%s names=%s\n' "$#" "$*"
[ "$plan_only" -eq 0 ] || exit 0
sel=""; for n in "$@"; do sel="$sel --suite $n"; done
state=$snap/.stagestate-$(echo "$sel" | cksum | cut -d' ' -f1)
log=$state.log; : > "$log"; n=0
while :; do
    # one bounded Terminal hand-off per window (tests/term.sh: XProtect-exempt, 60 s ceiling)
    ./tests/term.sh env REALPROG_CACHE="$R/corpus" UA="$UA" MODEL_COM="$com" UNISACC_FFI_X86_PROVIDER="$UNISACC_FFI_X86_PROVIDER" \
        python3 tests/gatequeue.py --state "$state" --com --jobs "${JOBS:-4}" $sel > "$log.w" 2>&1; rc=$?
    cat "$log.w" >> "$log"; grep -E '^(queue:|DONE .* rc=[1-9]|inputs changed)' "$log.w"
    n=$((n+1)); [ $rc = 75 ] || [ $rc = 142 ] || break
done
echo "stagerun $(echo "$head" | cut -c1-12) suites $# windows $n rc=$rc log $log"; [ $rc = 0 ]
