#!/bin/sh
# csih CX entry. Upper modules: pragma only. Lower (net/term/file/shell/…): may #include.
# CSIH_BIN=native ELF (from `unisacc.com csih.cx -o …`) skips cold -run (~47s/module graph).
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 1
REPO=$(CDPATH= cd -- "$ROOT/../.." && pwd) || exit 1
UNI=${UNISACC:-$REPO/unisacc.com}
cd "$ROOT" || exit 1
if [ $# -eq 0 ]; then
    set -- agent
fi
if [ -n "$CSIH_BIN" ] && [ -x "$CSIH_BIN" ]; then
    exec "$CSIH_BIN" "$@"
fi
if [ -f "$ROOT/csih.cx" ]; then
    exec /bin/sh "$UNI" -run "$ROOT/csih.cx" -- "$@"
fi
if [ -f "$ROOT/tui.cx" ]; then
    exec /bin/sh "$UNI" -run "$ROOT/tui.cx" -- "$@"
fi
echo "csih: missing tui.cx / csih.cx" >&2
exit 1
