#!/bin/bash
# Apple: bundle -> sign (private keychain) -> notarize app zip -> staple app -> dmg -> notarize dmg -> staple -> assess.
# usage: apple-sign.sh PAYLOAD.com SHA256 WORKDIR   (secrets read from ~/.private_keys, never echoed)
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); PAYLOAD=$1; SHA=$2; W=$3
ID="Developer ID Application: PARTNERNET SOFTWARE PTY LTD (L2N7M5M544)"
KC="$W/unisacc-signing.keychain-db"; KCPW=$(python3 -c 'import secrets;print(secrets.token_hex(16))')
P12=${APPLE_P12:-~/.private_keys/minicon_devid.p12}; P12PW=$(cat "${APPLE_P12_PASSWORD_FILE:-$HOME/.private_keys/minicon_devid_p12_password.txt}")   # paths only; values never printed
PROFILE=${NOTARY_PROFILE:-minicon-notary}
b() { python3 $R/tests/bound.py 55 "$@"; }
saved=$(security list-keychains -d user | tr -d '" ')
cleanup() { security list-keychains -d user -s $saved || true; security delete-keychain "$KC" 2>/dev/null || true; }
trap cleanup EXIT
mkdir -p "$W"
b python3 $R/release/macosbundle.py build --work "$W/q" --payload "$PAYLOAD" --sha256 "$SHA"
security create-keychain -p "$KCPW" "$KC"; security set-keychain-settings -lut 21600 "$KC"; security unlock-keychain -p "$KCPW" "$KC"
security import "$P12" -k "$KC" -P "$P12PW" -T /usr/bin/codesign -T /usr/bin/security >/dev/null
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$KCPW" "$KC" >/dev/null
security list-keychains -d user -s "$KC" $saved
b python3 $R/release/macosbundle.py sign --work "$W/q" --identity "$ID" --keychain "$KC"
APP="$W/q/Unisacc.app"; ZIP="$W/Unisacc.app.zip"
b ditto -c -k --keepParent "$APP" "$ZIP"
xcrun notarytool submit "$ZIP" --keychain-profile "$PROFILE" --wait --timeout 20m | tee "$W/notary-app.txt"
grep -q "status: Accepted" "$W/notary-app.txt"
b xcrun stapler staple "$APP"; b xcrun stapler validate "$APP"
b python3 $R/release/macosbundle.py dmg --work "$W/q" --identity "$ID" --keychain "$KC"
DMG="$W/q/unisacc-private-macos-universal.dmg"
xcrun notarytool submit "$DMG" --keychain-profile "$PROFILE" --wait --timeout 20m | tee "$W/notary-dmg.txt"
grep -q "status: Accepted" "$W/notary-dmg.txt"
b xcrun stapler staple "$DMG"; b xcrun stapler validate "$DMG"
b python3 $R/release/macosbundle.py assess --work "$W/q"
shasum -a 256 "$ZIP" "$DMG" | tee "$W/apple-assets.sha256"
