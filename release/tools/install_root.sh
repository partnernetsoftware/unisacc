#!/bin/bash
# install_root.sh CAND_DIR -- put a release candidate back at the repo root as the local "current
# product" (RELEASE-PIPELINE §8 and §17): the unsigned unisacc-next.com pair and the seed pair, then
# provenance, subtract-safety and the version-equality check.  0.0.30 published without the seed step
# and the root seed stayed at 0.0.29; this folds the four manual copies into one tool.
set -u
C=${1:?candidate dir}; cd "$(dirname "$0")/../.." || exit 1
for f in "$C/unisacc-next.com" "$C/unisacc-next.com.build.json" "$C/seed/unisacc-seed.com" "$C/seed/unisacc-seed.com.build.json"; do
    [ -s "$f" ] || { echo "install_root: missing $f"; exit 1; }
done
cp "$C/unisacc-next.com" unisacc.com && cp "$C/unisacc-next.com.build.json" unisacc.com.build.json || exit 1
cp "$C/seed/unisacc-seed.com" unisacc-seed.com && cp "$C/seed/unisacc-seed.com.build.json" unisacc-seed.com.build.json || exit 1
chmod +x unisacc.com unisacc-seed.com
if [ -d "$C/model-audit" ]; then rm -rf "${PWD:?}/model-audit" && cp -R "$C/model-audit" model-audit || exit 1; fi
python3 tests/bound.py 30 python3 exec/c/provenance.py check unisacc.com || { echo "install_root: provenance failed"; exit 1; }
python3 tests/bound.py 30 python3 tests/subtractsafety.py | tail -1 || exit 1
a=$(./unisacc.com --version); b=$(./unisacc-seed.com --version)
[ "$a" = "$b" ] || { echo "install_root: VERSION MISMATCH com [$a] seed [$b]"; exit 1; }
echo "install_root: $a (com $(shasum -a 256 unisacc.com | cut -c1-12), seed $(shasum -a 256 unisacc-seed.com | cut -c1-12))"
