#!/bin/sh
# Apply the seven parked reference patches in their verified order (window only).
# --check: dry run on the current tree; no files are changed.
set -e
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
mode=; [ "$1" = --check ] && mode=--check
for p in research/g5-typedef-leak/typedef-reset.patch research/g5-unsized-rows/unsized-rows.patch \
         research/g5-str3d-init/str3d-refuse.patch research/g5-bkscr/bkscr-bound.patch \
         research/g5-header-splice-line/header-splice-line.patch research/g5-line-refusal-rc/err-atu-counts.patch \
         research/g5-window/l1prime-multiunit.patch; do
  if [ -n "$mode" ]; then t=$(mktemp -d); git archive HEAD src include | tar -x -C "$t"; break; fi
  git apply "$p"; echo "applied $p"
done
if [ -n "$mode" ]; then
  for p in research/g5-typedef-leak/typedef-reset.patch research/g5-unsized-rows/unsized-rows.patch \
           research/g5-str3d-init/str3d-refuse.patch research/g5-bkscr/bkscr-bound.patch \
           research/g5-header-splice-line/header-splice-line.patch research/g5-line-refusal-rc/err-atu-counts.patch \
           research/g5-window/l1prime-multiunit.patch; do
    (cd "$t" && git apply "$OLDPWD/$p") && echo "ok $p"
  done
  [ -n "$t" ] && sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$t"   # the one guard (commissar ruling 2026-10-23)
fi
echo "next: export --check, re-export, diff facts, gates"
