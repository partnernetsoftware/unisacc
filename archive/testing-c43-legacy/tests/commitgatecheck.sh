#!/bin/bash
# commitgatecheck: in a scratch repository, a red gate whose log ends in a green-looking line leaves no commit,
# a green gate commits exactly the given path, missing paths/message are refused, and the gate override is
# refused without the self-test flag.
set -u
# T becomes a fixture git repository.  The one deletion guard (tests/safedel.cx, commissar ruling 2026-10-23) refuses any
# directory that is or holds a .git by rule, so nothing here deletes it: it stays under $TMPDIR (unisacc-commitgate.*)
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-commitgate.XXXXXX")
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
# 0.0.43 step 4: the derived-consistency step (release/tools/derivedcheck.py) in this scratch repository.  Its map and
# its suite are the scratch repository's own: src/front_pp.c is derived into exec/facts/front.tsv (a sha of the source).
mkdir -p release/tools tests exec/facts
cp "$R/release/tools/derivedcheck.py" release/tools/
printf 'src/front_pp.c\tfacts-export\n' > release/derived-map.tsv
cat > tests/gatedeps.json <<'JSON'
{"suites": {"facts-export": {"command": ["python3", "-c", "import hashlib,pathlib,sys; h=hashlib.sha256(pathlib.Path('src/front_pp.c').read_bytes()).hexdigest(); sys.exit(0 if pathlib.Path('exec/facts/front.tsv').read_text().strip()==h else 1)"]}}}
JSON
echo 'int v = 1;' > src/front_pp.c; sha256sum src/front_pp.c | cut -d' ' -f1 > exec/facts/front.tsv
git add release tests exec src && git -c user.name=t -c user.email=t@t commit -q -m 'derived-consistency fixture'
echo 'int v = 2;' > src/front_pp.c; git add src/front_pp.c
before=$(git rev-parse HEAD)
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m x --suite difftest_o-1 -- src/front_pp.c > /dev/null 2>&1
[ $? -eq 5 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "a changed derived source without its regenerated facts was committed"
sha256sum src/front_pp.c | cut -d' ' -f1 > exec/facts/front.tsv; git add exec/facts/front.tsv
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m ok --suite difftest_o-1 --suite exec-driver-core-modes -- src/front_pp.c exec/facts/front.tsv > /dev/null 2>&1 || fail "a changed derived source with its regenerated facts was refused"
[ "$(git rev-parse HEAD)" != "$before" ] || fail "the consistent pair did not commit"
# a research-only commit adds no derived check: the facts are still stale here (not regenerated), yet it commits
echo note > research-note.md; git add research-note.md
echo 'int v = 3;' > src/front_pp.c; git add src/front_pp.c
before=$(git rev-parse HEAD)
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m doc -- research-note.md > /dev/null 2>&1 || fail "a research-only commit was refused by the derived step"
[ "$(git rev-parse HEAD)" != "$before" ] || fail "the research-only commit did not commit"
git checkout -q -- src/front_pp.c 2>/dev/null || true
# 0.0.43 step 5: the derived step bounds each suite on its own.  Four suites whose sum exceeds the bound (scaled to
# 3 s here, DERIVEDCHECK_BOUND) pass when each finishes inside it; one suite over its own bound is refused.
cat > tests/gatedeps.json <<'JSON'
{"suites": {"facts-export": {"command": ["python3", "-c", "import hashlib,pathlib,sys; h=hashlib.sha256(pathlib.Path('src/front_pp.c').read_bytes()).hexdigest(); sys.exit(0 if pathlib.Path('exec/facts/front.tsv').read_text().strip()==h else 1)"]},
  "slow1": {"command": ["python3", "-c", "import time; time.sleep(2)"]}, "slow2": {"command": ["python3", "-c", "import time; time.sleep(2)"]},
  "slow3": {"command": ["python3", "-c", "import time; time.sleep(2)"]}, "slow4": {"command": ["python3", "-c", "import time; time.sleep(2)"]},
  "slowbad": {"command": ["python3", "-c", "import time; time.sleep(5)"]}}}
JSON
printf 'src/front_pp.c\tfacts-export\nsrc/multi.c\tslow1\nsrc/multi.c\tslow2\nsrc/multi.c\tslow3\nsrc/multi.c\tslow4\nsrc/bad.c\tslowbad\n' > release/derived-map.tsv
git add tests/gatedeps.json release/derived-map.tsv && git -c user.name=t -c user.email=t@t commit -q -m 'derived bounds fixture'
echo 'int m = 1;' > src/multi.c; git add src/multi.c
before=$(git rev-parse HEAD)
DERIVEDCHECK_BOUND=3 COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m multi --suite difftest_o-1 -- src/multi.c > /dev/null 2>&1 || fail "four suites whose sum exceeds the bound, each inside it, were refused"
[ "$(git rev-parse HEAD)" != "$before" ] || fail "the multi-suite commit did not commit"
echo 'int bad = 1;' > src/bad.c; git add src/bad.c
before=$(git rev-parse HEAD)
DERIVEDCHECK_BOUND=3 COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$R/release/tools/commitgate.sh" -m bad --suite difftest_o-1 -- src/bad.c > /dev/null 2>&1
[ $? -eq 5 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "a suite over its own bound was committed"
# 0.0.43 landing (research/c43-workflow-lessons.md section 2): --index commits exactly the index, checked in an export of
# the index tree, never another lander's uncommitted or indexed lines; one landing lock (tests/landlock) serialises
# commitgate runs, is re-entrant under tests/landlock, and stays out of the gate.  T2 is a second fixture repository
# (kept under $TMPDIR like T); lander A and lander B share shared.txt with disjoint hunks.
CG=$R/release/tools/commitgate.sh; LL=$R/tests/landlock
T2=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-commitgate-index.XXXXXX"); G=$T2/.git
cd "$T2" && git init -q . && seq 1 40 > shared.txt && echo own > own.txt && git add . && git commit -q -m base || fail "index fixture"
mkpatch() { cat "$1" > "$G/save" && git show ":$1" > "$1" && sed -i "$2" "$1" && git diff -- "$1" > "$3"; cat "$G/save" > "$1"; }   # FILE SED OUT
upto() { n=0; until [ -s "$1" ] || [ $n -ge 200 ]; do sleep 0.05; n=$((n + 1)); done; }   # wait (10 s at most) for a file
mkpatch shared.txt 's/^2$/A2/' "$G/a2.patch"; sed -i 's/^30$/B30/' shared.txt   # A's hunk made against HEAD; B's uncommitted hunk
git apply --cached "$G/a2.patch" && git apply "$G/a2.patch" || fail "--index: A's hunk did not apply beside B's"
printf '#!/bin/sh\ngrep -qx A2 shared.txt && grep -qx 30 shared.txt && ! grep -q B30 shared.txt && [ ! -e /dev/fd/9 ]\n' > "$G/look"; chmod +x "$G/look"
base=$(git rev-parse HEAD)
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$G/look "$CG" --index -m A2 -- shared.txt > /dev/null 2>&1 || fail "--index: A's hunk was not committed (or its gate saw B's line or held the lock)"
[ "$(git rev-parse HEAD^)" = "$base" ] && git show HEAD:shared.txt | grep -qx A2 && git show HEAD:shared.txt | grep -qx 30 || fail "--index: the commit is not exactly A's hunk"
grep -qx A2 shared.txt && grep -qx B30 shared.txt && [ "$(git diff --name-only)" = shared.txt ] && git diff --cached --quiet \
  && [ "$(git diff | grep -c '^[-+][0-9AB]')" = 2 ] || fail "--index: B's uncommitted hunk was touched or committed"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$CG" --index -m x -- shared.txt > /dev/null 2>&1; [ $? -eq 2 ] || fail "--index with an empty index was not refused"
mkpatch shared.txt 's/^3$/A3/' "$G/a3.patch"; git apply --cached "$G/a3.patch" && git apply "$G/a3.patch" || fail "--index: A's second hunk did not apply"
echo b > own.txt && git add own.txt   # B's indexed work, outside A's declared paths
before=$(git rev-parse HEAD); idx=$(git write-tree)
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$CG" --index -m x -- shared.txt > "$G/out" 2>&1; rc=$?
[ $rc -eq 2 ] && [ "$(git rev-parse HEAD)" = "$before" ] && [ "$(git write-tree)" = "$idx" ] && grep -qx '  own.txt' "$G/out" \
  || fail "--index: an indexed path outside the declared list was not refused by name (or HEAD/the index moved)"
git reset -q -- own.txt
printf '#!/bin/sh\ngit -C "%s" add own.txt\n' "$T2" > "$G/sneak"; chmod +x "$G/sneak"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$G/sneak "$CG" --index -m x -- shared.txt > /dev/null 2>&1
[ $? -eq 4 ] && [ "$(git rev-parse HEAD)" = "$before" ] || fail "--index: an index change during the gate was committed"
git reset -q -- own.txt
COMMITGATE_LOCK_WAIT=0 "$LL" env COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$CG" --index -m A3 -- shared.txt > /dev/null 2>&1 \
  || fail "commitgate --index under tests/landlock did not commit (it waited for its own lock?)"
git show HEAD:shared.txt | grep -qx A3 && ! git show HEAD:shared.txt | grep -q B30 && grep -qx B30 shared.txt || fail "--index under tests/landlock: wrong commit"
X=$("$LL" --export) && [ "$(git -C "$X" rev-parse HEAD^{tree})" = "$(git write-tree)" ] && grep -qx 30 "$X/shared.txt" || fail "--export is not the index tree"
sed -i 's/^5$/D5/' "$X/shared.txt" && "$LL" --add shared.txt || fail "--add refused"
git show :shared.txt | grep -qx D5 && ! git show :shared.txt | grep -q B30 && grep -qx D5 shared.txt && grep -qx B30 shared.txt \
  || fail "--add: the index is not the export's bytes, or the working-tree merge lost B's line"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$CG" --index -m D5 -- shared.txt > /dev/null 2>&1 && git show HEAD:shared.txt | grep -qx D5 || fail "--index after --add did not commit"
# the lock: a holder makes COMMITGATE_LOCK_WAIT=0 give up (rc 6, holder named), and a default waiter run only after it
printf '#!/bin/sh\necho gate >> "%s/order"\n' "$G" > "$G/mark"; printf '#!/bin/sh\necho start >> "%s/order"; sleep 0.5; echo end >> "%s/order"\n' "$G" "$G" > "$G/slow"; chmod +x "$G/mark" "$G/slow"
: > "$G/order"; : > "$G/held"; echo x >> own.txt; before=$(git rev-parse HEAD)
"$LL" sh -c 'echo held > "$1"; sleep 2; echo held-end >> "$2"' sh "$G/held" "$G/order" & holder=$!
upto "$G/held"
COMMITGATE_LOCK_WAIT=0 COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$T/green "$CG" -m x -- own.txt > "$G/out" 2>&1; rc=$?
[ $rc -eq 6 ] && [ "$(git rev-parse HEAD)" = "$before" ] && grep -q 'held by: pid' "$G/out" || fail "a held landing lock did not stop COMMITGATE_LOCK_WAIT=0 (rc $rc)"
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$G/mark "$CG" -m own -- own.txt > "$G/out" 2>&1 || fail "the waiter did not commit after the holder"
wait $holder
[ "$(tr '\n' ' ' < "$G/order")" = "held-end gate " ] && grep -q 'waiting for the landing lock' "$G/out" || fail "the waiter's gate ran while the lock was held"
: > "$G/order"; echo 1 > f1; echo 2 > f2; git add f1 f2
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$G/slow "$CG" -m f1 -- f1 > /dev/null 2>&1 & p1=$!
COMMITGATE_SELFTEST=1 COMMITGATE_GATE=$G/slow "$CG" -m f2 -- f2 > /dev/null 2>&1 & p2=$!
wait $p1; r1=$?; wait $p2; r2=$?
[ $r1 -eq 0 ] && [ $r2 -eq 0 ] && [ "$(git log --format=%s -2 | sort | tr "\n" " ")" = "f1 f2 " ] || fail "two concurrent commitgate runs did not both commit (rc $r1 $r2)"
[ "$(tr '\n' ' ' < "$G/order")" = "start end start end " ] || fail "two concurrent commitgate runs overlapped (no landing lock)"
echo "commitgate  red gate with a green-looking tail leaves no commit; green commits only the given paths; override needs the self-test flag; killed gate leaves no commit; another repository refused; commit failure rc 3; missing paths refused; freezecheck product closure or unisacc.c needs a non-contract suite; weights and shipping seed guarded; seed README excluded; named suite must declare every shipping path (uncovered, unclassified in either naming order, directory refused); inputs changed during the gate (content, mode, index, member added/deleted) rc 4, no commit; --index commits exactly the index (another lander's uncommitted hunk in the same file untouched, an indexed path outside the list refused by name, empty index refused, index moved rc 4), checked in the index export; tests/landlock --export/--add; one landing lock: re-entrant under tests/landlock, out of the gate, rc 6 when not free in time, concurrent runs serialised"
