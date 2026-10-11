#!/bin/sh
# Run form. unisacc runs the file table directly (run mode, no -o) with the arguments after it:
#   unisacc.com -include ... tui.c render.c ... cols.cx home.cx json.cx ... [args]
# tui.c is the entry (main); no textual #include of units. Only global headers use -include.
# Run mode is the unisacc default and the csih requirement. -include paths are absolute.
# -include csih_message_io.h is NOT needed (csih_message_io.c includes it) and breaks the forward file.
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 1
UNI=${UNISACC:-/Users/wjc/repos/unisacc/unisacc.com}
cd "$ROOT" || exit 1
if [ $# -eq 0 ]; then
    set -- agent
fi
exec /bin/sh "$UNI" -include "$ROOT/csih_cols.h" -include "$ROOT/csih_home.h" \
    -include "$ROOT/json.h" \
    tui.c render.c term.c chat.c clock.c tools.c cols.cx home.cx \
    file.c shell.c edit.c gate.c json.cx session.c \
    agent.c plugin.c net.c \
    reload_state.c reload_session_decode.c reload_session_encode.c \
    reload_io.c reload_load.c reload_consume.c journal_checkpoint.c \
    reload_owner.c csih_message.c csih_message_io.c context_index.c \
    "$@"
