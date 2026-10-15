#!/bin/sh
# RI snapshot (standalone import, 0.0.44): sealed repo fixtures become snapshots; each step is bounded by tests/bound.
#   1. middleflow e3 and stagecontract-e1-e3: overlay hit reproduces the sealed output; a dropped PRESENT key falls through and the
#      consumer capture records its actual reply (presence 0) where the overlay recorded presence 1 (visible, not inferred)
#   2. different inputs: the snapshot's upstream input (refuse) and the consumer's input (accept) are separate identities; the run
#      succeeds and reproduces the accept output; the headers carry each side's own hash
#   3. real E3 -> lower chain: the lower model runs on the E3 output (the sealed lower input); the lower capture header carries the
#      E3 output's hash; the snapshot header carries the E3 input's hash
# Built from exec/c/run.c and tests/ri-snapshot-fromledger.c; the work directory is a fresh temporary one.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
B() { "$R/tests/bound" 55 "$@"; }
W=$(mktemp -d "${TMPDIR:-/tmp}/risnapshotcheck.XXXXXX"); trap 'rm -rf "$W"' EXIT HUP INT TERM
B cc -O2 -w exec/c/run.c -o "$W/run" || { echo "risnapshotcheck: run build failed"; exit 1; }
B cc -O2 -w -I exec/c tests/ri-snapshot-fromledger.c -o "$W/snapfrom" || { echo "risnapshotcheck: helper build failed"; exit 1; }
M=tests/seals/middleflow-step0/e3
for pair in "middleflow-step0/e3|$M/accept|$M/model.net" "stagecontract-e1-e3|tests/seals/stagecontract-e1-e3/accept|tests/seals/stagecontract-e1-e3/e3.net"; do
  name=${pair%%|*}; rest=${pair#*|}; A=${rest%%|*}; MD=${rest#*|}
  U=e3:lnx/x86_64/tape/O0; IN=$A/input.bytes; t=$(echo "$name" | tr '/' '_')
  B "$W/snapfrom" "$A" "$W/full-$t" "$U" "$IN" -1 > /dev/null
  B "$W/run" --ri-snapshot "$W/full-$t" --upstream-stage "$U" --stage "$U" --capture-out "$W/cf-$t" "$MD" "$IN" > "$W/of.out" 2>/dev/null
  cmp -s "$W/of.out" "$A/output.bytes" || { echo "risnapshotcheck FAIL $name: overlay output differs from sealed output"; exit 1; }
  B "$W/snapfrom" "$A" "$W/drop-$t" "$U" "$IN" 1 > /dev/null
  B "$W/run" --ri-snapshot "$W/drop-$t" --upstream-stage "$U" --stage "$U" --capture-out "$W/cd-$t" "$MD" "$IN" > "$W/dr.out" 2>/dev/null
  cmp -s "$W/dr.out" "$A/output.bytes" || { echo "risnapshotcheck FAIL $name: fall-through output differs"; exit 1; }
  F=$(awk -F'\t' '$1=="00636c692f6572726f722d6c696d6974"{print $2}' "$W/cf-$t/resources.tsv")
  D=$(awk -F'\t' '$1=="00636c692f6572726f722d6c696d6974"{print $2}' "$W/cd-$t/resources.tsv")
  [ "$F" = 1 ] && [ "$D" = 0 ] || { echo "risnapshotcheck FAIL $name: capture answers not distinguished (full=$F dropped=$D)"; exit 1; }
  echo "ok   $name: overlay == sealed output; dropped key falls through (capture presence $D vs overlay $F)"
done
# 2. different inputs: upstream refuse input in the snapshot, consumer accept input at run time
E=tests/seals/middleflow-step0/e3
B "$W/snapfrom" "$E/refuse" "$W/snap-up" "e3:lnx/x86_64/tape/O0" "$E/refuse/input.bytes" -1 > /dev/null
B "$W/run" --ri-snapshot "$W/snap-up" --upstream-stage e3:lnx/x86_64/tape/O0 --stage e3:lnx/x86_64/tape/O0 --capture-out "$W/cap-diff" "$E/model.net" "$E/accept/input.bytes" > "$W/diff.out" 2>/dev/null || { echo "risnapshotcheck FAIL different-input: run rc!=0"; exit 1; }
cmp -s "$W/diff.out" "$E/accept/output.bytes" || { echo "risnapshotcheck FAIL different-input: output differs from accept output"; exit 1; }
UH=$(sha256sum "$E/refuse/input.bytes" | cut -d' ' -f1); CH=$(sha256sum "$E/accept/input.bytes" | cut -d' ' -f1)
grep -q "input_sha256=$UH" "$W/snap-up/resources.tsv" || { echo "risnapshotcheck FAIL different-input: snapshot header is not the upstream input"; exit 1; }
grep -q "input_sha256=$CH" "$W/cap-diff/resources.tsv" || { echo "risnapshotcheck FAIL different-input: capture header is not the consumer input"; exit 1; }
echo "ok   different-input: upstream (refuse) and consumer (accept) identities separate; output == accept sealed output"
# 3. real chain E3 -> lower: the lower model on the E3 output (equal to the sealed lower input)
L=tests/seals/middleflow-step0/lower
[ "$(cmp -s "$E/accept/output.bytes" "$L/accept/input.bytes" && echo y)" = y ] || { echo "risnapshotcheck FAIL chain: E3 output is not the lower input"; exit 1; }
B "$W/snapfrom" "$L/accept" "$W/snap-chain" "e3:lnx/x86_64/tape/O0" "$E/accept/input.bytes" -1 > /dev/null
B "$W/run" --ri-snapshot "$W/snap-chain" --upstream-stage e3:lnx/x86_64/tape/O0 --stage lower:lnx/x86_64/run/O0 --capture-out "$W/cap-chain" "$L/model.net" "$E/accept/output.bytes" > "$W/chain.out" 2>/dev/null || { echo "risnapshotcheck FAIL chain: lower run rc!=0"; exit 1; }
cmp -s "$W/chain.out" "$L/accept/output.bytes" || { echo "risnapshotcheck FAIL chain: lower output differs from sealed lower output"; exit 1; }
CH2=$(sha256sum "$E/accept/output.bytes" | cut -d' ' -f1)
grep -q "stage=lower:lnx/x86_64/run/O0	input_sha256=$CH2" "$W/cap-chain/resources.tsv" || { echo "risnapshotcheck FAIL chain: lower capture header is not the E3 output"; exit 1; }
echo "ok   chain E3 -> lower: lower output == sealed lower output; lower capture header binds the E3 output"
echo "risnapshotcheck PASS: overlay (2 fixtures, fall-through visible), different inputs, real E3 -> lower chain"
