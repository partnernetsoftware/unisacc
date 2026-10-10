#!/bin/bash
# commitgatecheck: in a scratch repository, a red gate whose log ends in a green-looking line leaves no commit,
# a green gate commits exactly the given path, missing paths/message are refused, and the gate override is
# refused without the self-test flag.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-commitgate.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "commitgate: $*"; exit 1; }
cd "$T" && git init -q . && git -c user.name=t -c user.email=t@t commit -q --allow-empty -m base
export GIT_AUTHOR_NAME=t GIT_AUTHOR_EMAIL=t@t GIT_COMMITTER_NAME=t GIT_COMMITTER_EMAIL=t@t
printf '#!/bin/sh\necho "gate  suites 3   failed 0   unverified 0"\nexit 1\n' > red; printf '#!/bin/sh\nexit 0\n' > green; chmod +x red green
echo a > a; echo b > b; git add a b
base=$(git rev-parse HEAD)
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/red "$R/release/tools/commitgate.sh" -m x -- a > /dev/null 2>&1 && fail "red gate committed"
[ "$(git rev-parse HEAD)" = "$base" ] || fail "a commit appeared after a red gate"
COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x -- a > /dev/null 2>&1; [ $? -eq 2 ] || fail "override honoured without self-test flag"
"$R/release/tools/commitgate.sh" -m x > /dev/null 2>&1; [ $? -eq 2 ] || fail "no paths was not refused"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m ok -- a > /dev/null 2>&1 || fail "green gate did not commit"
[ "$(git show --name-only --format= HEAD)" = a ] && git diff --cached --quiet -- a && ! git diff --cached --quiet -- b || fail "commit did not contain exactly the given path"
echo "commitgate  red gate with a green-looking tail leaves no commit; green commits only the given paths; override needs the self-test flag; missing paths refused"
