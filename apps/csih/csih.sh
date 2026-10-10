#!/bin/sh
# Dev release form. One unisacc BUILD (-o) of the file table, then run the binary:
#   unisacc.com -include ... tui.c render.c ... cols.cx home.cx json.cx ... -o .build/csih
#   .build/csih [agent|run|selftest|...]
# tui.c is the entry (main); no textual #include of units. Only global headers use -include.
# -include csih_message_io.h is NOT needed (csih_message_io.c includes it) and breaks the forward file.
# WHY build, not run: in run mode (no -o) a .cx input's definitions are dropped and
# the callers fall back to "no host function" (reject: not covered, 2026-10-10).
# Build mode treats .c and .cx alike. The other release form is csih.com. Not this file.
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 1
UNI=${UNISACC:-/Users/wjc/repos/unisacc/unisacc.com}
cd "$ROOT" || exit 1
if [ $# -eq 0 ]; then
    set -- agent
fi
mkdir -p "$ROOT/.build" || exit 1
OUT="$ROOT/.build/csih"
/bin/sh "$UNI" -include "$ROOT/csih_cols.h" -include "$ROOT/csih_home.h" \
    -include "$ROOT/json.h" \
    tui.c render.c term.c chat.c clock.c tools.c cols.cx home.cx \
    file.c shell.c edit.c gate.c json.cx session.c \
    agent.c plugin.c net.c \
    reload_state.c reload_session_decode.c reload_session_encode.c \
    reload_io.c reload_load.c reload_consume.c journal_checkpoint.c \
    reload_owner.c csih_message.c csih_message_io.c context_index.c \
    -o "$OUT" || exit $?
exec "$OUT" "$@"
