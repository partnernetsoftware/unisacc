#!/bin/bash
# install_hooks.sh -- link release/tools/hooks/pre-commit into THIS clone's hooks directory (local only; no
# remote protection or CI is changed).  Refuses to overwrite a different existing hook.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); H=$(git -C "$R" rev-parse --git-common-dir)/hooks; mkdir -p "$H"
src=$R/release/tools/hooks/pre-commit; dst=$H/pre-commit
if [ -e "$dst" ] && ! cmp -s "$dst" "$src"; then echo "install_hooks: $dst exists and differs; not overwritten" >&2; exit 1; fi
cp "$src" "$dst"; chmod +x "$dst"; echo "install_hooks: $dst installed (from $src)"
