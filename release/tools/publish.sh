#!/bin/bash
# publish.sh TAG SIGNED_COM_SHA256 ACCEPTANCE_JSON -- make a draft release public the unisacc way (R18-11 ③):
# verify the draft's signed unisacc.com before removing other assets or publishing;
# then download the public asset and compare it with the signed hash again.
# Prints the verdict; nothing here signs or edits the repository.
# 0.0.39 WF3: ACCEPTANCE (the release acceptance receipt) must say release_eligible true for exactly
# these signed bytes, or nothing is touched (0.0.38 published while the signing receipt said false).
set -u
TAG=${1:?tag}; WANT=${2:?signed sha256}; ACC=${3:?release acceptance receipt (research/r<N>-release-acceptance.json)}
cd "$(dirname "$0")/../.." || exit 1          # gh needs the repository (0.0.19 published once with uploads failing outside it)
python3 - "$ACC" "$WANT" "$TAG" <<'PY' || exit 1
import json,sys
acc,want,tag=sys.argv[1:4]
def no(why): print('refused:',why); sys.exit(1)
try: d=json.load(open(acc))
except Exception as e: no('no readable acceptance receipt: %s' % e)
if d.get('schema')!=1: no('acceptance receipt schema is not 1')
if 'v'+str(d.get('version'))!=tag: no('acceptance receipt is for v%s, not %s' % (d.get('version'),tag))
if d.get('release_eligible') is not True: no('acceptance receipt is not release_eligible')
if d.get('windows',{}).get('after_sha256')!=want: no('acceptance receipt names other signed bytes')
c=d.get('courts') or {}
for court in ('six-native-cells-final-bytes','windows-defender-final-bytes'):
    r=c.get(court) or {}
    if r.get('conclusion')!='success' or not r.get('run_id'): no('court %s has no successful run' % court)
s=c.get('six-native-cells-final-bytes',{}).get('cells_public_sha256')
if s is not None and s!=want: no('six-native court tested other bytes')
if not (c.get('owner-promotion') or {}).get('authority'): no('owner-promotion has no named authority')
open_items=[x.get('item') for x in d.get('pending',[]) if x.get('state')!='RESOLVED']
if open_items: no('acceptance receipt has pending items: %s' % ', '.join(map(str,open_items)))
PY
have=$(gh release view "$TAG" --json assets -q '[.assets[].name]|join(" ")') || exit 1
case " $have " in *" unisacc.com "*) ;; *) echo "refused: no signed unisacc.com on $TAG yet"; exit 1;; esac
case " $have " in *" unisacc-macos-universal.dmg "*) ;; *) echo "refused: no unisacc-macos-universal.dmg on $TAG yet"; exit 1;; esac
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
# a draft's asset needs the authenticated client (0.0.20: plain curl of the asset URL failed)
mkdir "$T/d" && gh release download "$TAG" --pattern unisacc.com -D "$T/d" >/dev/null || { echo "draft download failed"; exit 1; }
mv "$T/d/unisacc.com" "$T/draft-unisacc.com"
draft_hash=$(shasum -a 256 "$T/draft-unisacc.com" | cut -d' ' -f1)
[ "$draft_hash" = "$WANT" ] || { echo "REFUSED DRAFT BYTES: $draft_hash != $WANT"; exit 1; }
for a in $(gh release view "$TAG" --json assets -q '.assets[].name'); do
  case "$a" in unisacc.com|unisacc-macos-universal.dmg) ;; *) gh release delete-asset "$TAG" "$a" -y >/dev/null || exit 1; echo "removed $a";; esac
done
gh release edit "$TAG" --draft=false --latest >/dev/null || exit 1
curl -sfL -o "$T/unisacc.com" "https://github.com/$(gh repo view --json nameWithOwner -q .nameWithOwner)/releases/download/$TAG/unisacc.com" || { echo "download failed"; exit 1; }
got=$(shasum -a 256 "$T/unisacc.com" | cut -d' ' -f1)
echo "public $TAG: $(gh release view "$TAG" --json assets -q '[.assets[].name]|join(", ")')"
[ "$got" = "$WANT" ] && echo "public unisacc.com sha256 $got = signed" || { echo "PUBLIC BYTES DIFFER: $got != $WANT"; exit 1; }
