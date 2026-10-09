#!/bin/sh
# installpaircheck: release/tools/installpair.sh (0.0.38).  Fixture pairs only: a stage pair with the
# candidate's bytes and receipt origin is installed, and installing again changes no digest
# (idempotent); other bytes, another source or a damaged receipt fall back to the candidate pair.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-installpair.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "installpair: $*"; exit 1; }
I="$R/release/tools/installpair.sh"
mk() {  # dir name bytes commit sources
  mkdir -p "$1"; printf '%s' "$3" > "$1/$2"
  a=$(printf '%s' "$3" | sha256sum | cut -d' ' -f1)
  printf '{"artifact_sha256":"%s","sources_sha256":"%s","commit":"%s"}\n' "$a" "$5" "$4" > "$1/$2.build.json"; }
digest() { cat "$1/unisacc.com" "$1/unisacc.com.build.json" | sha256sum | cut -d' ' -f1; }
mk "$T/c" unisacc-next.com PRODUCT c1 SRC
mk "$T/s/stage3" unisacc.com PRODUCT c2 SRC
mkdir -p "$T/w"; "$I" "$T/c" "$T/s" "$T/w" || fail "install failed"
grep -q '"c2"' "$T/w/unisacc.com.build.json" || fail "same-origin stage pair not chosen"
d1=$(digest "$T/w"); "$I" "$T/c" "$T/s" "$T/w"; [ "$d1" = "$(digest "$T/w")" ] || fail "second install changed the root (not idempotent)"
cp -p "$T/s/stage3/unisacc.com" "$T/s/stage3/unisacc.com.build.json" "$T/w2" 2>/dev/null || { mkdir -p "$T/w2"; cp -p "$T/s/stage3/"* "$T/w2/"; }
[ "$d1" = "$(digest "$T/w2")" ] || fail "root differs from what comboot reinstalls"
mk "$T/s2/stage3" unisacc.com PRODUCT c2 OTHERSRC; mkdir -p "$T/w3"; "$I" "$T/c" "$T/s2" "$T/w3"
grep -q '"c1"' "$T/w3/unisacc.com.build.json" || fail "a stage receipt from another source was accepted"
mk "$T/s3/stage3" unisacc.com OTHERBYTES c2 SRC; mkdir -p "$T/w4"; "$I" "$T/c" "$T/s3" "$T/w4"
grep -q '"c1"' "$T/w4/unisacc.com.build.json" || fail "a stage pair with other bytes was accepted"
mk "$T/s4/stage3" unisacc.com PRODUCT c2 SRC; echo 'not json' > "$T/s4/stage3/unisacc.com.build.json"; mkdir -p "$T/w5"; "$I" "$T/c" "$T/s4" "$T/w5"
grep -q '"c1"' "$T/w5/unisacc.com.build.json" || fail "a damaged stage receipt was accepted"
echo "installpair  same-origin stage pair installed, reinstall idempotent and equal to comboot's, other source/bytes/damaged receipt fall back to the candidate pair"
