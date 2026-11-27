#!/bin/sh
# tests/fixtures/win-m53-s1/judge.sh -- Independent judge for win-m53-s1-parity.
# Never executes the candidate APE. Independently extracts tape/lnx-x86_64/m53.tapeslice
# from the trusted candidate APE (or P4 carrier), binds every vector (id, len, expect_ret,
# input_sha) to tests/fixtures/win-m53-s1/vectors.tsv, rejects duplicates, and verifies
# the v3 receipt + binding_sha.
set -eu

: "${CAND_APE:?missing CAND_APE}"
: "${CAND_SHA:?missing CAND_SHA}"
: "${DISP_TS:?missing DISP_TS}"
: "${NOW_TS:?missing NOW_TS}"
: "${RUN_ID:?missing RUN_ID}"
: "${SOURCE_COMMIT:?missing SOURCE_COMMIT}"
: "${EXPECT_TARGET:?missing EXPECT_TARGET}"
: "${RCPT:?missing RCPT}"
: "${OUT:?missing OUT}"
: "${SLICE:?missing SLICE}"
: "${LOC:?missing LOC}"
VECTORS="${VECTORS:-tests/fixtures/win-m53-s1/vectors.tsv}"
JOB="${JOB:-tests/fixtures/win-m53-s1/job.tsv}"
BASELINE="${BASELINE:-cx/runtime/stream_attributes.cx}"

test -s "$CAND_APE" && test -s "$RCPT" && test -s "$OUT" && test -s "$SLICE" && test -s "$LOC" && test -s "$VECTORS"
test "$(sha256sum "$CAND_APE" | awk '{print $1}')" = "$CAND_SHA"

TMPD="$(mktemp -d "${TMPDIR:-/tmp}/win-m53-judge.XXXXXX")"
trap 'rm -rf "$TMPD"' EXIT

# Extract P4 container bytes from trusted APE (UNIPKG1 footer) or raw P4 carrier
P4_FILE="$TMPD/trusted.p4"
HEAD4="$(head -c 4 "$CAND_APE")"
if [ "$HEAD4" = "P 4 " ]; then
  cp "$CAND_APE" "$P4_FILE"
else
  APE_SZ="$(stat -c %s "$CAND_APE")"
  test "$APE_SZ" -gt 32
  MAGIC="$(tail -c 16 "$CAND_APE" | head -c 8)"
  test "$MAGIC" = "$(printf 'UNIPKG1\n')"
  PKG_LEN="$(od -An -tu8 -N8 -j $((APE_SZ - 8)) "$CAND_APE" | tr -d ' \n')"
  PKG_OFF=$((APE_SZ - 16 - PKG_LEN))
  test "$PKG_LEN" -gt 10 && test "$PKG_OFF" -ge 0
  tail -c +"$((PKG_OFF + 1))" "$CAND_APE" | head -c "$PKG_LEN" > "$P4_FILE"
fi

# Binary-safe P4 F-record extraction for tape/lnx-x86_64/m53.tapeslice
read_line_at() {
  tail -c +"$(($1 + 1))" "$P4_FILE" | head -n 1
}
POS=0
HDR="$(read_line_at "$POS")"
POS=$((POS + ${#HDR} + 1))
set -- $HDR
test "$1" = "P" && test "$2" = "4"
PM="$3"; PS="$4"; NR="$5"
I=0
while [ "$I" -lt "$PS" ]; do
  L="$(read_line_at "$POS")"
  POS=$((POS + ${#L} + 1))
  I=$((I + 1))
done
I=0
while [ "$I" -lt "$PM" ]; do
  L="$(read_line_at "$POS")"
  POS=$((POS + ${#L} + 1))
  set -- $L
  POS=$((POS + $2))
  I=$((I + 1))
done
FOUND_SL=0
I=0
while [ "$I" -lt "$NR" ]; do
  L="$(read_line_at "$POS")"
  POS=$((POS + ${#L} + 1))
  set -- $L
  test "$1" = "F"
  KL="$2"; DL="$3"; RL="$4"; CO="$5"
  KEY="$(tail -c +"$((POS + 1))" "$P4_FILE" | head -c "$KL")"
  if [ "$KEY" = "tape/lnx-x86_64/m53.tapeslice" ]; then
    test "$FOUND_SL" -eq 0 && test "$CO" = "0" && test "$DL" = "$RL" && test "$DL" -ge 128
    tail -c +"$((POS + KL + 1))" "$P4_FILE" | head -c "$DL" > "$TMPD/trusted_m53.tapeslice"
    FOUND_SL=1
  fi
  POS=$((POS + KL + DL))
  I=$((I + 1))
done
if [ "$FOUND_SL" -ne 1 ]; then
  echo "REFUSE missing-slice-key" >&2
  exit 1
fi

TRUSTED_SLICE_SHA="$(sha256sum "$TMPD/trusted_m53.tapeslice" | awk '{print $1}')"
SLICE_SHA="$(sha256sum "$SLICE" | awk '{print $1}')"
if [ "$SLICE_SHA" != "$TRUSTED_SLICE_SHA" ]; then
  echo "REFUSE slice-mismatch" >&2
  exit 1
fi

OUT_SHA="$(sha256sum "$OUT" | awk '{print $1}')"
JOB_SHA="$(sha256sum "$JOB" | awk '{print $1}')"
BASE_SHA="$(sha256sum "$BASELINE" | awk '{print $1}')"
VEC_SHA="$(sha256sum "$VECTORS" | awk '{print $1}')"
IN_SHA="$(printf 'job\t%s\nbaseline\t%s\nslice\t%s\nvectors\t%s\n' "$JOB_SHA" "$BASE_SHA" "$TRUSTED_SLICE_SHA" "$VEC_SHA" | sha256sum | awk '{print $1}')"

grep -Fx "$(printf 'target\t%s' "$EXPECT_TARGET")" "$LOC" >/dev/null || { echo "REFUSE bad-target-locator" >&2; exit 1; }
grep -Fx "$(printf 'candidate_sha\t%s' "$CAND_SHA")" "$LOC" >/dev/null || { echo "REFUSE bad-candidate-locator" >&2; exit 1; }
grep -Fx "$(printf 'slice_sha\t%s' "$TRUSTED_SLICE_SHA")" "$LOC" >/dev/null || { echo "REFUSE bad-slice-locator" >&2; exit 1; }
grep -Fx "$(printf 'vectors_sha\t%s' "$VEC_SHA")" "$LOC" >/dev/null || { echo "REFUSE bad-vectors-sha" >&2; exit 1; }
grep -Fx "$(printf 'slice_source\tdelivered_ape:tape/lnx-x86_64/m53.tapeslice')" "$LOC" >/dev/null || { echo "REFUSE bad-slice-source" >&2; exit 1; }

# Bind every vector ID, input len, expected return value, and input_sha to repo vectors.tsv; reject duplicates
awk -v tgt="$EXPECT_TARGET" '
FNR == NR {
  if (FNR == 1) {
    if ($0 != "id\tlen\tptr_nonnull\tkind\texpect_ret\tinput_sha") { print "REFUSE bad-vectors-header" > "/dev/stderr"; bad = 1; exit 1 }
    next;
  }
  if (NF != 6) { print "REFUSE bad-vectors-row" > "/dev/stderr"; bad = 1; exit 1 }
  vid = $1;
  if (vid in exp_len) { print "REFUSE duplicate-table-vector" > "/dev/stderr"; bad = 1; exit 1 }
  if (length($6) != 64 || $6 !~ /^[0-9a-f]+$/) { print "REFUSE bad-table-input-sha" > "/dev/stderr"; bad = 1; exit 1 }
  exp_len[vid] = $2;
  exp_ret[vid] = $5;
  exp_isha[vid] = $6;
  want_n++;
  if ($5 == "1") want_pos++; else want_neg++;
  next;
}
/\r$/ { print "REFUSE cr-in-output" > "/dev/stderr"; bad = 1; exit 1 }
/^VEC\t/ {
  n = split($0, f, "\t");
  if (n != 8) { print "REFUSE bad-vec-fields" > "/dev/stderr"; bad = 1; exit 1 }
  vid = f[2];
  if (!(vid in exp_len)) { print "REFUSE unknown-vector" > "/dev/stderr"; bad = 1; exit 1 }
  if (vid in seen_v) { print "REFUSE duplicate-vector" > "/dev/stderr"; bad = 1; exit 1 }
  if (f[3] != ("len=" exp_len[vid])) { print "REFUSE bad-vector-len" > "/dev/stderr"; bad = 1; exit 1 }
  if (f[4] != ("native=" exp_ret[vid])) { print "REFUSE bad-vector-native" > "/dev/stderr"; bad = 1; exit 1 }
  if (f[5] != ("portable=" exp_ret[vid])) { print "REFUSE bad-vector-portable" > "/dev/stderr"; bad = 1; exit 1 }
  if (f[6] != ("s1_def=" exp_ret[vid])) { print "REFUSE bad-vector-s1def" > "/dev/stderr"; bad = 1; exit 1 }
  st = substr(f[7], 7) + 0;
  if (st <= 0) { print "REFUSE bad-vector-steps" > "/dev/stderr"; bad = 1; exit 1 }
  if (f[8] != ("in_sha=" exp_isha[vid])) { print "REFUSE bad-vector-in-sha" > "/dev/stderr"; bad = 1; exit 1 }
  seen_v[vid] = 1;
  vecs++;
  next;
}
/^SUMMARY\t/ {
  want_s = "\ttarget=" tgt "\tvectors=" want_n "\tpos=" want_pos "\tneg=" want_neg "\t";
  if (index($0, want_s) == 0 || $0 !~ /\tverdict=PASS$/) { print "REFUSE bad-summary" > "/dev/stderr"; bad = 1; exit 1 }
  sum++;
  next;
}
{ print "REFUSE unexpected-output-line" > "/dev/stderr"; bad = 1; exit 1 }
END {
  if (bad || want_n != 12 || vecs != want_n || sum != 1) exit 1;
  for (vid in exp_len) if (!(vid in seen_v)) { print "REFUSE missing-vector" > "/dev/stderr"; exit 1 }
}' "$VECTORS" "$OUT"

awk -v run_id="$RUN_ID" -v rev="$SOURCE_COMMIT" -v target="$EXPECT_TARGET" \
    -v csha="$CAND_SHA" -v isha="$IN_SHA" -v osha="$OUT_SHA" \
    -v now_ts="$NOW_TS" -v min_ts="$DISP_TS" '
BEGIN { n = 0; bad = 0 }
function is_hex64_nz(s) { return length(s) == 64 && s ~ /^[0-9a-f]+$/ && s !~ /^0+$/; }
/\r$/ { bad = 1; exit 1 }
{
  if (split($0, f, "\t") != 2) { bad = 1; exit 1 }
  if (f[1] in seen) { bad = 1; exit 1 }
  seen[f[1]] = f[2];
  n++;
}
END {
  if (bad || n != 14) exit 1;
  if (seen["version"] != "3") exit 1;
  if (seen["run_id"] != run_id || seen["rev"] != rev || seen["unit"] != "m53") exit 1;
  if (seen["target"] != target || seen["producer"] != "win-m53-s1-parity" || seen["purpose"] != "s1-parity") exit 1;
  if (seen["candidate_sha"] != csha || seen["inputs_sha"] != isha || seen["output_sha"] != osha) exit 1;
  if (seen["rc"] != "0" || seen["verdict"] != "PASS") exit 1;
  ts = seen["ts"] + 0;
  if (ts <= 0 || ts < min_ts - 60 || ts < now_ts - 86400 || ts > now_ts + 60) exit 1;
  if (!is_hex64_nz(seen["binding_sha"])) exit 1;
}' "$RCPT"

GOT_BIND="$(awk -F'\t' '$1=="binding_sha"{print $2}' "$RCPT")"
EXP_BIND="$(grep -v '^binding_sha	' "$RCPT" | sha256sum | awk '{print $1}')"
test "$GOT_BIND" = "$EXP_BIND"
printf 'JUDGE_OK\ttarget=%s\tslice_sha=%s\tvectors_sha=%s\n' "$EXPECT_TARGET" "$TRUSTED_SLICE_SHA" "$VEC_SHA"
