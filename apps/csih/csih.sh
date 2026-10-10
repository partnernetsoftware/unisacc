#!/bin/sh
# Dev release form. One unisacc invocation, file table on the command line:
#   unisacc.com csih.c render.c term.c ... net.c [agent|run|selftest|...]
# csih.c includes tui.c, so tui.c is not its own slot.
# The other release form is csih.com, one cross-arch executable. Not this file.
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 1
UNI=${UNISACC:-/Users/wjc/repos/unisacc/unisacc.com}
cd "$ROOT" || exit 1
if [ $# -eq 0 ]; then
    set -- agent
fi
exec /bin/sh "$UNI" \
    csih.c \
    render.c term.c chat.c clock.c tools.c cols.c \
    file.c shell.c edit.c gate.c json.c session.c \
    agent.c plugin.c net.c \
    "$@"
