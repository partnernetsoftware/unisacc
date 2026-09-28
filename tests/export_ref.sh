#!/bin/sh
# Canonical include source -> independent C file. No Python, no writes to
# the root source. unisacc.c is the sole ordered assembly manifest.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd)
[ "$#" -eq 1 ] || { echo 'usage: tests/export_ref.sh OUTPUT' >&2; exit 2; }
case "$1" in /*) out=$1;; *) out="$PWD/$1";; esac
[ "$out" != "$R/unisacc.c" ] || { echo 'export_ref: refusing root source overwrite' >&2; exit 2; }
tmp="$out.$$"
trap 'rm -f "$tmp"' EXIT HUP INT TERM
cd "$R"
awk '
function emit(file,    line, child, base, rc) {
    if (seen[file]++) return
    base = file; sub(/[^\/]*$/, "", base)
    while ((rc = getline line < file) > 0) {
        if (line ~ /^[ \t]*#include[ \t]+"[^\"]+"[ \t]*$/) {
            child = line
            sub(/^[ \t]*#include[ \t]+"/, "", child)
            sub(/"[ \t]*$/, "", child)
            emit(base child)
        } else print line
    }
    close(file)
    if (rc < 0) { print "export_ref: cannot read " file > "/dev/stderr"; exit 1 }
}
BEGIN { emit("unisacc.c") }
' > "$tmp"
[ -s "$tmp" ] || { echo 'export_ref: empty source' >&2; exit 1; }
mv -f "$tmp" "$out"
