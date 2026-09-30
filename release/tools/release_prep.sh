#!/bin/bash
# release_prep.sh CAND_DIR VERSION  -- stage the release inputs from a verified candidate: unsigned zip + receipt skeleton + Apple bundle build.
# Nothing here signs or publishes; each phase is a separate, reviewable step.  All work under /tmp/r<minor>-release.
set -eu; C=$1; V=${2:?version}; W=/tmp/r${V##*.}-release; mkdir -p $W/windows $W/apple
cp "$C/unisacc-next.com" $W/unisacc.com; cp "$C/unisacc-next.com.build.json" $W/unisacc.com.build.json
( cd $W && rm -f windows/unisacc-unsigned.zip && zip -X -j windows/unisacc-unsigned.zip unisacc.com >/dev/null )
SHA=$(shasum -a 256 $W/unisacc.com | cut -c1-64); ZSHA=$(shasum -a 256 $W/windows/unisacc-unsigned.zip | cut -c1-64); BYTES=$(wc -c < $W/unisacc.com | tr -d ' ')
echo "unisacc.com sha256=$SHA bytes=$BYTES"; echo "zip sha256=$ZSHA"
echo "next: fill upstream run id/attempt into $W/windows/unsigned-receipt.json (schema in release/README.md), create the draft release with gh, upload zip+receipt, dispatch windows-signing.yml qualification then company; Apple: release/macosbundle.py build|sign|dmg|assess --work $W/apple --payload $W/unisacc.com --sha256 $SHA --identity 'Developer ID Application: PARTNERNET SOFTWARE PTY LTD (L2N7M5M544)', notarytool submit --key ~/.private_keys/AuthKey_9Q7ZVW88L4.p8 --key-id 9Q7ZVW88L4 --issuer 9ec986b2-8342-4d4f-8314-c54b940203ab --wait, stapler staple the .app and .dmg"
