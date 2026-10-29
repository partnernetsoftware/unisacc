#!/bin/sh
# csih CX entry. Upper modules: pragma only. Lower (net/term/file/shell/…): may #include.
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 1
REPO=$(CDPATH= cd -- "$ROOT/../.." && pwd) || exit 1
UNI=${UNISACC:-$REPO/unisacc.com}
cd "$ROOT" || exit 1
if [ $# -eq 0 ]; then
    set -- agent
fi
if [ -f "$ROOT/csih.cx" ]; then
    exec /bin/sh "$UNI" -run "$ROOT/csih.cx" -- "$@"
fi
if [ -f "$ROOT/tui.cx" ]; then
    exec /bin/sh "$UNI" -run "$ROOT/tui.cx" -- "$@"
fi
echo "csih: missing tui.cx / csih.cx" >&2
exit 1
