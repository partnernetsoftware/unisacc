#!/bin/sh
# Linux readback only: no release creation, edits, tagging, signing, or publication.
# Run under tools/bound 55. ROOT TAG NEW_ABSOLUTE_OUT OLD_C46_2_APE.
# READBACK_EXPECT_DRAFT=true is only for a private-candidate smoke; default requires public stable metadata.
set -eu
ROOT=${1:?repository root}; TAG=${2:?release tag}; OUT=${3:?new absolute output directory}; OLD=${4:?old c46-2 APE}
DRAFT=${READBACK_EXPECT_DRAFT:-false}
case "$DRAFT" in true|false) ;; *) exit 2;; esac
cd "$ROOT"
mkdir "$OUT"
gh release view "$TAG" --repo partnernetsoftware/unisacc --json tagName,isDraft,isPrerelease,targetCommitish --jq "if .isDraft == $DRAFT and .isPrerelease == false and .targetCommitish == \"44182b4059d51191cb021241c666b521bf34b2ad\" then . else error(\"release metadata mismatch\") end"
if [ "$DRAFT" = false ]; then
  test "$(gh api "repos/partnernetsoftware/unisacc/git/ref/tags/$TAG" --jq .object.sha)" = 44182b4059d51191cb021241c666b521bf34b2ad
fi
gh release download "$TAG" --repo partnernetsoftware/unisacc --dir "$OUT"
(cd "$OUT" && printf '%s\n' \
 '884adfef1815e87750c86c7036d3074552a66ce18dfa855588ab279c2aa99fcf  unisacc.com' \
 '29e13aeb8c62a09c0323a9d86dcd51ab8b35684696264b3fc66571d79654ef6c  pkg.bin' \
 'bd16566df04a3d3caf248146c03419a62328db3f72f4af7459ddccad8f381b10  slice-win-arm64' \
 '56fa8f14317f6150d0e6f182e0a51bf09ffe4168f36166216561477bdf2086e5  unisacc.com.build.json' | sha256sum --check -)
test "$(sh "$OUT/unisacc.com" --version)" = 'unisacc 0.0.46'
sh ./unisacc.com tools/chain-step.cx -- smoke "$OUT/unisacc.com"
sh ./unisacc.com tools/check-run-comm.cx -- "$ROOT" "$OUT/unisacc.com" "$OLD"
printf 'C46_3_READBACK\tPASS\texpected_draft=%s\ttag=%s\n' "$DRAFT" "$TAG"
