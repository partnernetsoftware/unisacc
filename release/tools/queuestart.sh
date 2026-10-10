#!/bin/bash
# queuestart.sh Q B D [MODE] -- apply the queue start mode before any window runs (0.0.40 gate).
# MODE is fresh (default) or resume.  fresh never copies a QUEUE_BACKUP into Q; resume requires an
# explicit mode, a backup build.json whose artifact_sha256 matches the candidate, and a complete
# state generation (results.json).  Wrong/incomplete/mismatched sources fail closed (rc 2).
# Writes Q/start-receipt.txt (mode/restored/source/artifact/backup).  Does not delete historical
# backup trees.  Called from queue.sh; also the surface for tests/queuestartcheck.sh.
set -u
Q=${1:?queue state dir}; B=${2:?backup dir}; D=${3:?candidate dir}; MODE=${4:-fresh}

fail() { echo "queue: $*"; exit 2; }

[ -f "$D/unisacc-next.com" ] || fail "candidate missing $D/unisacc-next.com"
art=$(shasum -a 256 "$D/unisacc-next.com" | cut -d' ' -f1)

receipt() {  # mode restored source
  mkdir -p "$Q"
  printf 'mode=%s\nrestored=%s\nsource=%s\nartifact_sha256=%s\nbackup=%s\n' \
    "$1" "$2" "$3" "$art" "$B" > "$Q/start-receipt.txt"
}

case "$MODE" in
  fresh)
    # Default: do not read B.  A live Q (mid-run restart) is kept; a missing Q starts empty.
    if [ ! -d "$Q" ]; then
      for g in state state.old; do
        [ -d "$B/$g" ] && echo "queue: fresh start; backup $B/$g left untouched (not restored)"
      done
      mkdir -p "$Q"
      receipt fresh no none
    else
      [ -f "$Q/start-receipt.txt" ] || receipt fresh no none
      echo "queue: fresh start; using existing state at $Q (backup not read)"
    fi
    ;;
  resume)
    [ -f "$B/unisacc-next.com.build.json" ] || fail "resume refused: backup has no build.json at $B"
    want=$(python3 -c "import json,sys; print(json.load(open(sys.argv[1]))['artifact_sha256'])" \
      "$B/unisacc-next.com.build.json" 2>/dev/null) || want=
    [ -n "$want" ] || fail "resume refused: backup build.json unreadable"
    [ "$want" = "$art" ] || fail "resume refused: backup artifact ${want:0:12} != candidate ${art:0:12}"
    src=
    for g in state state.old; do
      if [ -d "$B/$g" ] && [ -f "$B/$g/results.json" ]; then src=$g; break; fi
    done
    [ -n "$src" ] || fail "resume refused: backup has no complete state (need $B/state/results.json)"
    if [ -f "$Q/results.json" ]; then
      fail "resume refused: live state already at $Q; will not mix with backup restore"
    fi
    mkdir -p "$Q" && cp -pR "$B/$src/." "$Q/" || fail "resume refused: copy from $B/$src failed"
    receipt resume yes "$B/$src"
    echo "queue: resumed state from $B/$src artifact=${art:0:12} (stamps re-checked by gatequeue)"
    ;;
  *)
    fail "QUEUE_START must be fresh or resume (got: $MODE)"
    ;;
esac
