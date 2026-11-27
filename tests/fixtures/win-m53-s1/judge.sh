#!/bin/sh
# Thin entry: verify pinned independent trusted launcher digest (previous published
# release v0.1.6 unisacc.com, never the candidate) and exec tests/win-m53-s1-judge.cx.
set -eu
TRUSTED_LAUNCHER_SHA="ed50199fd085be4da2fa692db8010057be6674f8bfc609b101e8d18daf23583f"
LAUNCHER="${JUDGE_LAUNCHER:-./unisacc.com}"
test -s "$LAUNCHER"
GOT_SHA="$(sha256sum "$LAUNCHER" | awk '{print $1}')"
if [ "$GOT_SHA" != "$TRUSTED_LAUNCHER_SHA" ]; then
  echo "REFUSE untrusted-launcher-sha" >&2
  exit 1
fi
if [ -n "${CAND_SHA:-}" ] && [ "$GOT_SHA" = "$CAND_SHA" ]; then
  echo "REFUSE launcher-equals-candidate" >&2
  exit 1
fi
exec sh "$LAUNCHER" tests/win-m53-s1-judge.cx --
