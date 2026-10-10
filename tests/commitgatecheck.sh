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
printf '#!/bin/sh\nkill -9 $$\n' > killed; chmod +x killed
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/killed "$R/release/tools/commitgate.sh" -m x -- a > /dev/null 2>&1 && fail "killed gate committed"
[ "$(git rev-parse HEAD)" = "$base" ] || fail "a commit appeared after a killed gate"
(cd "$T" && "$R/release/tools/commitgate.sh" -m x -- a > /dev/null 2>&1); [ $? -eq 2 ] || fail "a commit outside the checked repository was not refused"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x -- no-such-path > /dev/null 2>&1; [ $? -eq 3 ] || fail "git commit failure was not reported as 3"
"$R/release/tools/commitgate.sh" -m x > /dev/null 2>&1; [ $? -eq 2 ] || fail "no paths was not refused"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m ok -- a > /dev/null 2>&1 || fail "green gate did not commit"
[ "$(git show --name-only --format= HEAD)" = a ] && git diff --cached --quiet -- a && ! git diff --cached --quiet -- b || fail "commit did not contain exactly the given path"
mkdir -p src && echo x > src/a.c && git add src/a.c
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x -- src/a.c > /dev/null 2>&1; [ $? -eq 2 ] || fail "a src/ change with the default (infrastructure) set was not refused"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x --suite gate-layers -- src/a.c > /dev/null 2>&1; [ $? -eq 2 ] || fail "a src/ change with only contract suites was not refused"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x --suite difftest_o-1 -- src/a.c > /dev/null 2>&1 || fail "a src/ change with a product suite was refused"
for tree in exec kernel include unisa weights; do
  mkdir -p "$tree"; echo x > "$tree/input.c"; git add "$tree/input.c"
  before=$(git rev-parse HEAD)
  COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x -- "$tree/input.c" > /dev/null 2>&1
  [ $? -eq 2 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "$tree default set accepted or committed"
  COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x --suite gate-layers -- "./$tree" > /dev/null 2>&1
  [ $? -eq 2 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "$tree contract-only directory accepted or committed"
  suite=difftest_o-1; [ "$tree" = exec ] && suite=exec-driver-core-modes   # difftest_o-1 does not declare exec/
  COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m ok --suite $suite -- "$tree/input.c" > /dev/null 2>&1 || fail "$tree covering product suite refused"
done
mkdir -p seed
for name in gen.c compilerpack.c; do
 echo x > "seed/$name"; git add "seed/$name"
 before=$(git rev-parse HEAD)
 COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x --suite gate-layers -- "seed/$name" > /dev/null 2>&1
 [ $? -eq 2 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "seed/$name contract-only accepted"
done
echo doc > seed/README; git add seed/README
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m doc --suite gate-layers -- seed/README > /dev/null 2>&1 || fail "seed README counted as product"
echo x > weights/x.tsv; git add weights/x.tsv
before=$(git rev-parse HEAD)
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x --suite gate-layers -- weights/x.tsv > /dev/null 2>&1
[ $? -eq 2 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "weights TSV contract-only accepted"
# 0.0.40-prep (机房主任 18:28) (1): the named product suite must declare every shipping path it is offered for
echo y > weights/gold.tsv; git add weights/gold.tsv; before=$(git rev-parse HEAD)
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x --suite exec-elfobject -- weights/gold.tsv > /dev/null 2>&1
[ $? -eq 2 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "a product suite that does not declare weights/ was accepted"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x --suite exec-elfobject --suite tools -- weights/gold.tsv > /dev/null 2>&1
[ $? -eq 2 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "an unclassified suite was not refused (any() short-circuit)"
for order in "--suite tools --suite difftest_o-1" "--suite difftest_o-1 --suite tools"; do   # 机房主任 18:33: naming order must not skip an unclassified suite
  COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x $order -- weights/gold.tsv > /dev/null 2>&1
  [ $? -eq 2 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "unclassified suite skipped with order: $order"
done
mkdir -p exec/d && echo z > exec/d/new.c && git add exec/d/new.c
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x --suite difftest_o-1 -- ./exec > /dev/null 2>&1
[ $? -eq 2 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "a directory pathspec with an uncovered shipping file was accepted"
# (2): inputs changed while the gate ran -> no commit, rc 4 (content, mode, index), even if the gate looks green
printf '#!/bin/sh\necho changed >> weights/gold.tsv\necho "gate  suites 1   failed 0"\nexit 0\n' > edit; chmod +x edit
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/edit "$R/release/tools/commitgate.sh" -m x --suite difftest_o-1 -- weights/gold.tsv > /dev/null 2>&1
[ $? -eq 4 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "a working-tree change during the gate was committed"
git add weights/gold.tsv
printf '#!/bin/sh\nchmod +x weights/gold.tsv\nexit 0\n' > mode; chmod +x mode
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/mode "$R/release/tools/commitgate.sh" -m x --suite difftest_o-1 -- weights/gold.tsv > /dev/null 2>&1
[ $? -eq 4 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "a mode change during the gate was committed"
chmod -x weights/gold.tsv
printf '#!/bin/sh\necho staged > weights/gold.tsv; git add weights/gold.tsv; git checkout -q -- weights/gold.tsv 2>/dev/null; exit 0\n' > stage; chmod +x stage
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/stage "$R/release/tools/commitgate.sh" -m x --suite difftest_o-1 -- weights/gold.tsv > /dev/null 2>&1
[ $? -eq 4 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "an index change during the gate was committed"
mkdir -p weights/m && echo m > weights/m/one.tsv && git add weights/m/one.tsv
printf '#!/bin/sh\necho n > weights/m/two.tsv\nexit 0\n' > addm; chmod +x addm
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/addm "$R/release/tools/commitgate.sh" -m x --suite difftest_o-1 -- weights/m > /dev/null 2>&1
[ $? -eq 4 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "a member added under the pathspec during the gate was committed"
rm -f weights/m/two.tsv
printf '#!/bin/sh\nrm -f weights/m/one.tsv\nexit 0\n' > delm; chmod +x delm
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/delm "$R/release/tools/commitgate.sh" -m x --suite difftest_o-1 -- weights/m > /dev/null 2>&1
[ $? -eq 4 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "a member deleted under the pathspec during the gate was committed"
git checkout -q -- weights/m/one.tsv
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m ok --suite difftest_o-1 -- weights/gold.tsv > /dev/null 2>&1 || fail "stable covered input was not committed"
echo "commitgate  red gate with a green-looking tail leaves no commit; green commits only the given paths; override needs the self-test flag; killed gate leaves no commit; another repository refused; commit failure rc 3; missing paths refused; freezecheck product closure or unisacc.c needs a non-contract suite; weights and shipping seed guarded; seed README excluded; named suite must declare every shipping path (uncovered, unclassified in either naming order, directory refused); inputs changed during the gate (content, mode, index, member added/deleted) rc 4, no commit"
