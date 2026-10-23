#!/bin/sh
# Copy a manifest of existing repository files into a private fixture.
set -eu
[ "$#" = 3 ] || exit 2
source_root=${1:?source root}
target_root=${2:?fixture root}
manifest=${3:?manifest}
relative() {
    case $1 in ''|/*|..|../*|*/../*|*/..) echo 'fixturecopy: unsafe relative file' >&2; exit 2;; esac
}
[ -d "$source_root" ] && [ -d "$target_root" ] || exit 2
relative "$manifest"
[ -s "$source_root/$manifest" ] || exit 2
count=0
while IFS= read -r file || [ -n "$file" ]; do
    relative "$file"
    [ -f "$source_root/$file" ] || { echo "fixturecopy: missing source $file" >&2; exit 2; }
    case $file in */*) mkdir -p "$target_root/${file%/*}";; esac
    cp "$source_root/$file" "$target_root/$file"
    count=$((count + 1))
done < "$source_root/$manifest"
[ "$count" -gt 0 ]
