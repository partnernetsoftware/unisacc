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
# the real chain: commitgate's own git commit passes through the installed hook
printf '#!/bin/sh\nexit 0\n' > green; chmod +x green; echo w > src/a.c && git add src/a.c; before=$(git rev-parse HEAD)
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m chain --suite difftest_o-1 -- src/a.c > /dev/null 2>&1 && [ "$(git rev-parse HEAD)" != "$before" ] || fail "commitgate commit was blocked by the hook"
rm -f green
mkdir -p research && echo n > research/n.md && git add research/n.md && git commit -q -m doc || fail "non-shipping commit refused"
echo z > src/a.c && git add src/a.c && # rename of a shipping file out of the closure is still seen (the deleted source end)
git reset -q src/a.c; git checkout -q -- src/a.c; mkdir -p research; git mv src/a.c research/a.md
before=$(git rev-parse HEAD); git commit -q -m mv 2> /dev/null; [ $? -ne 0 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "renaming a shipping file out was not caught"
git mv research/a.md src/a.c; echo z > src/a.c && git add src/a.c
# an upstream git failure inside the hook pipeline fails closed (pipefail)
mkdir -p shim && printf '#!/bin/sh\n[ "$1" = diff ] && exit 7\nexec /usr/bin/env -u PATH %s "$@"\n' "$(command -v git)" > shim/git && chmod +x shim/git
before=$(git rev-parse HEAD); PATH=$PWD/shim:$PATH .git/hooks/pre-commit 2> /dev/null; [ $? -ne 0 ] || fail "a failing git diff in the hook was not fail-closed"
rm -rf shim
mv tests/freezecheck.py tests/fc.bak
before=$(git rev-parse HEAD); git commit -q -m x 2> /dev/null; [ $? -ne 0 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "unclassifiable state did not fail closed"
mv tests/fc.bak tests/freezecheck.py; echo other > .git/hooks/pre-commit
release/tools/install_hooks.sh > /dev/null 2>&1 && fail "a different existing hook was overwritten"
rm .git/hooks/pre-commit; (cd / && "$T/release/tools/install_hooks.sh" > /dev/null) && [ -x "$T/.git/hooks/pre-commit" ] || fail "install from another cwd did not land in this clone"
rm .git/hooks/pre-commit; ln -s /dev/null .git/hooks/pre-commit; release/tools/install_hooks.sh > /dev/null 2>&1 && fail "a symlinked hook was replaced"
rm .git/hooks/pre-commit; git config core.hooksPath elsewhere; release/tools/install_hooks.sh > /dev/null 2>&1 && fail "install reported success under a custom core.hooksPath"
git config --unset core.hooksPath
echo "precommithook  shipping path outside commitgate refused, no commit; commitgate marker, real commitgate chain and logged skip pass; non-shipping passes; unclassifiable fails closed; rename out caught; failing git diff fails closed; installer: other cwd lands here, never overwrites a different or symlinked hook, refuses custom core.hooksPath"
