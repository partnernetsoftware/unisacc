#!/bin/bash
# precommithookcheck (0.0.40-prep): in a scratch clone with the hook installed, a shipping-path commit outside
# commitgate is refused and leaves no commit; commitgate's marker, the logged emergency skip and non-shipping
# paths pass; an unclassifiable state fails closed.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-hook.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "precommithook: $*"; exit 1; }
cd "$T" && git init -q . && mkdir -p tests release/tools/hooks && cp "$R/tests/freezecheck.py" tests/ && cp "$R/release/tools/hooks/pre-commit" release/tools/hooks/ && cp "$R/release/tools/install_hooks.sh" release/tools/
export GIT_AUTHOR_NAME=t GIT_AUTHOR_EMAIL=t@t GIT_COMMITTER_NAME=t GIT_COMMITTER_EMAIL=t@t
git add -A && git commit -q -m base && release/tools/install_hooks.sh > /dev/null || fail "install failed"
release/tools/install_hooks.sh > /dev/null || fail "reinstall of the same hook refused"
base=$(git rev-parse HEAD)
mkdir -p src && echo x > src/a.c && git add src/a.c
git commit -q -m x 2> /dev/null; [ $? -ne 0 ] && [ "$(git rev-parse HEAD)" = "$base" ] || fail "shipping path committed outside commitgate"
UNISACC_COMMITGATE_SKIP=1 git commit -q -m skip 2> err; [ $? -eq 0 ] && grep -q WITHOUT err || fail "emergency skip not honoured/logged"
echo y > src/a.c && git add src/a.c && UNISACC_COMMITGATE_RUN=1 git commit -q -m gated || fail "commitgate marker refused"
mkdir -p research && echo n > research/n.md && git add research/n.md && git commit -q -m doc || fail "non-shipping commit refused"
echo z > src/a.c && git add src/a.c && mv tests/freezecheck.py tests/fc.bak
before=$(git rev-parse HEAD); git commit -q -m x 2> /dev/null; [ $? -ne 0 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "unclassifiable state did not fail closed"
mv tests/fc.bak tests/freezecheck.py; echo other > .git/hooks/pre-commit
release/tools/install_hooks.sh > /dev/null 2>&1 && fail "a different existing hook was overwritten"
echo "precommithook  shipping path outside commitgate refused, no commit; commitgate marker and logged skip pass; non-shipping passes; unclassifiable fails closed; installer never overwrites a different hook"
