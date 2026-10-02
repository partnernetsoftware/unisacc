#!/bin/bash
# rc_tag.sh TAG -- bind a release to its source commit (0.0.22 P1-B 2, after minicon's candidate.yml).
# Run after the candidate.json commit is pushed.  Tags HEAD as rc/TAG and pushes the tag; signing and
# publishing then follow rc/TAG, so main may keep moving.  Refuses when the seal's sealed_from_commit
# is not an ancestor of HEAD on origin (0.0.21: a rebase after sealing left 40d3d9d off origin).
set -eu; TAG=${1:?tag, e.g. v0.0.22}; R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
git fetch -q origin main
SHA=$(git rev-parse HEAD)
git merge-base --is-ancestor "$SHA" origin/main || { echo "HEAD $SHA is not on origin/main: push first" >&2; exit 2; }
from=$(python3 -c "import json;print(json.load(open('release/candidate.json'))['sealed_from_commit'])")
ver=$(python3 -c "import json;print(json.load(open('release/candidate.json'))['version'])")
[ "v$ver" = "$TAG" ] || { echo "candidate.json is version $ver, not $TAG" >&2; exit 2; }
git merge-base --is-ancestor "$from" origin/main || { echo "sealed_from_commit $from is not on origin/main (rebased after sealing): reseal" >&2; exit 2; }
git tag -a "rc/$TAG" -m "release candidate $TAG (candidate.json sealed from $from)" "$SHA"
git push -q origin "refs/tags/rc/$TAG"
[ "$(git ls-remote origin "refs/tags/rc/$TAG^{}" | cut -c1-40)" = "$SHA" ] && echo "rc/$TAG -> $SHA" || { echo "rc/$TAG not visible on origin" >&2; exit 1; }
