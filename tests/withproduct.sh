#!/bin/sh
# Run a command in a tracked export that includes the installed product pair.
# Copies made by the command can therefore carry the compiler used by cxbin.
# Keep the private export under out/ as evidence; nothing is deleted here.
set -eu
unset GIT_DIR GIT_WORK_TREE GIT_COMMON_DIR GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES
[ "$#" -gt 0 ] || { echo 'withproduct: command required' >&2; exit 2; }
R=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
B=$("$R/tests/bound" --helper) || exit 2
for f in unisacc.com unisacc.com.build.json; do
    [ -s "$R/$f" ] || { echo "withproduct: missing $f" >&2; exit 2; }
done
mkdir -p "$R/out"
private=$(mktemp -d "$R/out/withproduct.XXXXXX")
mkdir "$private/tree"
"$B" 15 git -C "$R" -c tar.umask=022 archive --format=tar --output="$private/source.tar" HEAD
"$B" 15 tar -xf "$private/source.tar" -C "$private/tree"
cp -p "$R/unisacc.com" "$R/unisacc.com.build.json" "$private/tree/"
"$B" 20 sh "$R/unisacc.com" -run "$private/tree/seed/ident.c" -- "$private/tree" check "$private/tree/unisacc.com" > "$private/identity.log"
"$B" 10 git -C "$private/tree" init -q
"$B" 15 git -C "$private/tree" add -A
"$B" 10 git -C "$private/tree" add -f -- unisacc.com unisacc.com.build.json
"$B" 15 git -C "$private/tree" -c user.name=fixture -c user.email=fixture@localhost commit -q -m 'tracked product fixture'
echo "withproduct: private export $private/tree" >&2
cd "$private/tree"
exec "$@"
