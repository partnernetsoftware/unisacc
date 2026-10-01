#!/bin/bash
# publish.sh TAG SIGNED_COM_SHA256 -- make a draft release public the unisacc way (R18-11 ③):
# delete every asset except the signed unisacc.com and the notarized dmg, publish as latest,
# then download the public unisacc.com and compare it with the signed hash from the receipt.
# Prints the verdict; nothing here signs or edits the repository.
set -u
TAG=${1:?tag}; WANT=${2:?signed sha256}
cd "$(dirname "$0")/../.." || exit 1          # gh needs the repository (0.0.19 published once with uploads failing outside it)
have=$(gh release view "$TAG" --json assets -q '[.assets[].name]|join(" ")') || exit 1
case " $have " in *" unisacc.com "*) ;; *) echo "refused: no signed unisacc.com on $TAG yet"; exit 1;; esac
case " $have " in *" unisacc-macos-universal.dmg "*) ;; *) echo "refused: no unisacc-macos-universal.dmg on $TAG yet"; exit 1;; esac
for a in $(gh release view "$TAG" --json assets -q '.assets[].name'); do
  case "$a" in unisacc.com|unisacc-macos-universal.dmg) ;; *) gh release delete-asset "$TAG" "$a" -y >/dev/null || exit 1; echo "removed $a";; esac
done
gh release edit "$TAG" --draft=false --latest >/dev/null || exit 1
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
url=$(gh release view "$TAG" --json assets -q '.assets[]|select(.name=="unisacc.com").url')
curl -sfL -o "$T/unisacc.com" "$url" || { echo "download failed"; exit 1; }
got=$(shasum -a 256 "$T/unisacc.com" | cut -d' ' -f1)
echo "public $TAG: $(gh release view "$TAG" --json assets -q '[.assets[].name]|join(", ")')"
[ "$got" = "$WANT" ] && echo "public unisacc.com sha256 $got = signed" || { echo "PUBLIC BYTES DIFFER: $got != $WANT"; exit 1; }
