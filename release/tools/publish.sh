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
required=$(python3 - "$ACC" "$WANT" "$TAG" <<'PY'
import json,re,sys
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
six=c.get('six-native-cells-final-bytes',{})
if six.get('cells_public_sha256')!=want: no('six-native court does not name the signed bytes')
if six.get('cells')!=6: no('six-native court did not cover six cells')
de=c.get('windows-defender-final-bytes',{})
if de.get('final_sha256')!=want: no('defender court does not name the signed bytes')
if not de.get('cells'): no('defender court names no scanned cells')
if not (c.get('owner-promotion') or {}).get('authority'): no('owner-promotion has no named authority')
open_items=[x.get('item') for x in d.get('pending',[]) if x.get('state')!='RESOLVED']
if open_items: no('acceptance receipt has pending items: %s' % ', '.join(map(str,open_items)))
assets=(d.get('published') or {}).get('public_assets')
if not isinstance(assets,list) or not assets: no('acceptance receipt has no asset set')
if any(not isinstance(a,str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*',a) for a in assets): no('acceptance receipt has an invalid asset name')
if len(set(assets))!=len(assets): no('acceptance receipt repeats an asset')
if 'unisacc.com' not in assets: no('acceptance asset set omits unisacc.com')
print(' '.join(assets))
PY
) || { printf "%s\n" "$required"; exit 1; }
have=$(gh release view "$TAG" --json assets -q '[.assets[].name]|join(" ")') || exit 1
for a in $required; do
  case " $have " in *" $a "*) ;; *) echo "refused: no receipt asset $a on $TAG yet"; exit 1;; esac
done
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
# a draft's asset needs the authenticated client (0.0.20: plain curl of the asset URL failed)
mkdir "$T/d" && gh release download "$TAG" --pattern unisacc.com -D "$T/d" >/dev/null || { echo "draft download failed"; exit 1; }
mv "$T/d/unisacc.com" "$T/draft-unisacc.com"
draft_hash=$(shasum -a 256 "$T/draft-unisacc.com" | cut -d' ' -f1)
[ "$draft_hash" = "$WANT" ] || { echo "REFUSED DRAFT BYTES: $draft_hash != $WANT"; exit 1; }
for a in $(gh release view "$TAG" --json assets -q '.assets[].name'); do
  case " $required " in *" $a "*) ;; *) gh release delete-asset "$TAG" "$a" -y >/dev/null || exit 1; echo "removed $a";; esac
done
gh release edit "$TAG" --draft=false --latest >/dev/null || exit 1
curl -sfL -o "$T/unisacc.com" "https://github.com/$(gh repo view --json nameWithOwner -q .nameWithOwner)/releases/download/$TAG/unisacc.com" || { echo "download failed"; exit 1; }
got=$(shasum -a 256 "$T/unisacc.com" | cut -d' ' -f1)
echo "public $TAG: $(gh release view "$TAG" --json assets -q '[.assets[].name]|join(", ")')"
[ "$got" = "$WANT" ] && echo "public unisacc.com sha256 $got = signed" || { echo "PUBLIC BYTES DIFFER: $got != $WANT"; exit 1; }
