#!/bin/bash
# install_hooks.sh -- install release/tools/hooks/pre-commit for THIS clone (local only; no remote protection or
# CI is changed).  Resolved from the script's repository, not the caller's cwd.  The hooks directory is the
# clone's common one, so every worktree of this clone gets it.  Refuses: a custom core.hooksPath (the hook would
# never run), a symlinked or different existing hook.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd)
C=$(git -C "$R" rev-parse --path-format=absolute --git-common-dir); H=$C/hooks
hp=$(git -C "$R" config --get core.hooksPath || true)
if [ -n "$hp" ]; then echo "install_hooks: core.hooksPath=$hp is set; the hook would not run from $H (refused)" >&2; exit 1; fi
mkdir -p "$H"; src=$R/release/tools/hooks/pre-commit; dst=$H/pre-commit
if [ -L "$dst" ]; then echo "install_hooks: $dst is a symlink; not touched" >&2; exit 1; fi
if [ -e "$dst" ] && ! cmp -s "$dst" "$src"; then echo "install_hooks: $dst exists and differs; not overwritten" >&2; exit 1; fi
cp "$src" "$dst"; chmod +x "$dst"; echo "install_hooks: $dst installed (from $src)"
