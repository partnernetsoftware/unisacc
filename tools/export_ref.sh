#!/bin/sh
# Compatibility loader for the native source exporter; no export algorithm here.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
exec sh "$here/../unisacc.com" "$here/sourceflat.cx" -- "$@"
