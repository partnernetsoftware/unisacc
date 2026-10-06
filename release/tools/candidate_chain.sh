#!/bin/zsh
# candidate_chain.sh VERSION STATE_DIR NOTE [WAIT_FILE] -- RELEASE-PIPELINE §9 steps 1-5 in one bounded chain:
# worktree at HEAD (+ gitignored root/seed pairs) -> same-source UA -> build_candidate -> comboot N22 -> seal ->
# commit candidate.json -> gatedeps unchanged -> push (ls-remote verified) -> rc tag -> queue tree -> queue loop
# (via release/tools/queue.sh).  Prints "queue tree PATH" so a signing chain can start alongside.  (0.0.30 retro)
set -u
setopt pipefail   # `cmd | tail -1 || exit 1` must see cmd failing (cdx: false|tail -1 returns 0)
V=$1; B=$2; NOTE=$3; OLD=${4:-}; R=$(git rev-parse --show-toplevel)
[ -z "$OLD" ] || until grep -q "queue done" $OLD 2>/dev/null; do sleep 20; done
H=$(git -C $R rev-parse --short HEAD); W=/private/tmp/unisacc-cand-$H
[ -d $W ] || git -C $R worktree add --detach $W $H >/dev/null 2>&1 || exit 1
(cd $R && cp unisacc.com unisacc.com.build.json unisacc-seed.com unisacc-seed.com.build.json $W/) || exit 1
cd $W
python3 tests/bound.py 58 ./tests/build_ref.sh $B/ua.c $B/ua | tail -1 || exit 1
release/tools/build_candidate.sh $B/cand $B/ua > $B/build.log 2>&1 || { tail -3 $B/build.log; exit 1; }
D=$B/cand/seed
python3 exec/c/comboot.py stage 1 $D/unisacc-seed.com > $B/cb.log 2>&1 || exit 1
for s in stage2 stage3 fixedpoint; do until SEED_DIR=$D python3 tests/bound.py 58 python3 exec/c/comboot.py shard $s >> $B/cb.log 2>&1; r=$?; [ $r -ne 75 ]; do :; done; echo "$s rc=$r"; [ $r -eq 0 ] || exit 1; done
python3 tests/bound.py 58 release/tools/seal_candidate.sh $B/cand $V "$NOTE" | tail -1 || exit 1
cp $W/release/candidate.json $R/release/candidate.json
cd $R && git commit -q -m "$V: seal candidate ($NOTE)

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>" -- release/candidate.json || exit 1
python3 tests/bound.py 58 make gatedeps | tail -1 | cut -c1-120 || exit 1
git diff --quiet tests/gatedeps.json || git commit -q -m "$V: refresh gatedeps at seal" -- tests/gatedeps.json   # docs/test commits since the last refresh move guard stamps; not product closure
git push -q origin main; [ "$(git ls-remote origin refs/heads/main | cut -c1-40)" = "$(git rev-parse HEAD)" ] || { echo NOT-PUSHED; exit 1; }
git tag -d rc/v$V >/dev/null 2>&1; git push -q origin :refs/tags/rc/v$V 2>/dev/null
release/tools/rc_tag.sh v$V | tail -1 || exit 1
H=$(git rev-parse --short HEAD); echo "queue tree /tmp/unisacc-queue-$H"
# the queue loop is release/tools/queue.sh (worktree, corpus links, REALPROG_CACHE, load pause, lock wait);
# launch this chain with the longest background timeout -- the 0.0.30 loop was killed at a 30-minute default
exec release/tools/queue.sh $B/cand $B/ua $B/cand/seed
