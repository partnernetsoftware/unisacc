#!/bin/bash
# installpair.sh CAND_DIR SEED_DIR DEST -- put the product pair the gates read into DEST (0.0.38, from queue.sh).
# com-comboot-stage2 later reinstalls the seed's stage pair into the same root; when that pair has the
# candidate's exact bytes AND its receipt names the candidate's artifact and source, install it now, so
# the reinstall changes no input (full038c: 124 results voided by a differing build.json).  Otherwise
# -- other bytes, other source, a damaged receipt -- the candidate's own pair, as before.
set -u
D=${1:?candidate dir}; SEED=${2:?seed dir}; W=${3:?destination}
SP="$SEED/stage3"
# same bytes are not enough: the stage receipt must name the candidate's artifact and source (cdx2)
same_origin() { python3 - "$SP/unisacc.com.build.json" "$D/unisacc-next.com.build.json" 2>/dev/null <<'PY'
import json,sys
a,b=(json.load(open(p)) for p in sys.argv[1:3])
sys.exit(0 if a.get('artifact_sha256')==b.get('artifact_sha256') and a.get('sources_sha256')==b.get('sources_sha256') and a.get('artifact_sha256') else 1)
PY
}
if [ -s "$SP/unisacc.com" ] && [ -s "$SP/unisacc.com.build.json" ] && cmp -s "$SP/unisacc.com" "$D/unisacc-next.com" && same_origin; then
  cp -p "$SP/unisacc.com" "$W/unisacc.com" && cp -p "$SP/unisacc.com.build.json" "$W/unisacc.com.build.json"
else
  cp -p "$D/unisacc-next.com" "$W/unisacc.com" && cp -p "$D/unisacc-next.com.build.json" "$W/unisacc.com.build.json"
fi

