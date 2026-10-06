#!/bin/sh
# build_seed.sh DIR -- unisacc-seed.com without Python (0.0.30 S1).
#   host cc builds the exported compiler source (tests/build_ref.sh) -> ref;
#   ref -b writes the compiler for win/x86_64 and the four Unix targets;
#   seed/ape.c (host cc) packs them into DIR/unisacc-seed.com + .build.json.
# The seed is the classic table compiler (weights inline), not the packaged
# product; it is what comboot stage 1 needs: it compiles, -run's and emits tapes.
# Every step is bounded by tests/bound (native).  Nothing here runs python3.
set -eu
D=${1:?seed dir}; R=$(cd "$(dirname "$0")/../.." && pwd); mkdir -p "$D"; D=$(cd "$D" && pwd)
B="$R/tests/bound"; W="$D/seed-work"; mkdir -p "$W"
"$B" 58 "$R/tests/build_ref.sh" "$W/ref.c" "$W/ref"
"$R/tests/export_ref.sh" "$W/flat.c"
"$B" 30 cc -std=c99 -O2 -w -o "$W/ape" "$R/seed/ape.c"
pids=
for t in win/x86_64 lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64; do
    "$B" 55 "$W/ref" -O2 -b "$t" "$W/flat.c" -o "$W/slice-$(echo $t | tr / -)" & pids="$pids $!"
done
for p in $pids; do wait "$p"; done
"$B" 30 "$W/ape" "$W/unisacc-seed.com" "$W/slice-win-x86_64" "$W/slice-lnx-x86_64" \
    "$W/slice-lnx-arm64" "$W/slice-osx-x86_64" "$W/slice-osx-arm64"
chmod +x "$W/unisacc-seed.com"
out=$("$B" 20 sh "$W/unisacc-seed.com" -run "$R/examples/hello.c")
case "$out" in *"hello from C99"*) ;; *) echo "build_seed: the seed did not run hello.c: $out" >&2; exit 1;; esac
sha=$(shasum -a 256 "$W/unisacc-seed.com" | cut -d' ' -f1)
src=$(shasum -a 256 "$W/flat.c" | cut -d' ' -f1)
n=$(wc -c < "$W/unisacc-seed.com" | tr -d ' ')
commit=$(git -C "$R" rev-parse HEAD)
mv -f "$W/unisacc-seed.com" "$D/unisacc-seed.com"
printf '{\n  "artifact_sha256": "%s",\n  "built_by": "release/tools/build_seed.sh (no Python)",\n  "bytes": %s,\n  "commit": "%s",\n  "exported_source_sha256": "%s",\n  "schema": 1\n}\n' \
    "$sha" "$n" "$commit" "$src" > "$D/unisacc-seed.com.build.json"
echo "seed: $D/unisacc-seed.com  sha256 $sha  $n B"
