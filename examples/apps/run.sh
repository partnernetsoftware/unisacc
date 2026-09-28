#!/bin/sh
# Real host data -> ordinary C analyser. Collectors are explicit system tools.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=${APP_ROOT:-$(CDPATH= cd -- "$here/../.." && pwd)}
if [ "${1-}" != --bounded ]; then
    exec perl "$root/tests/bound.pl" 55 sh "$0" --bounded "$@"
fi
shift
app=${1-}; [ $# -eq 0 ] || shift
case "$app" in procview|memmap|winlayout|exeinfo) ;; *) echo 'usage: run.sh procview|memmap|winlayout|exeinfo [FILE|- ...]' >&2; exit 2;; esac
compiler=${APP_COM:-$root/unisacc.com}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-live.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
run() {
    if [ "$(head -c 2 "$compiler")" = MZ ]; then
        /bin/sh "$compiler" -run "$here/$app.c" "$@"
    else
        "$compiler" -run "$here/$app.c" "$@"
    fi
}
if [ $# -gt 0 ]; then run "$@"; exit $?; fi
case "$app" in
    procview)
        ps -axo pid=,ppid=,rss=,comm= > "$tmp/input"
        run "$tmp/input" ;;
    memmap)
        case "$(uname -s)" in
            Linux) run ;;
            Darwin)
                cc -std=c99 -Wall -Wextra "$here/tools/selfmaps.c" -o "$tmp/collector"
                "$tmp/collector" > "$tmp/input"
                echo 'memmap: analysing the live macOS collector process, not the analyser' >&2
                run "$tmp/input" ;;
            *) echo 'memmap: provide a real maps file on this host' >&2; exit 1;;
        esac ;;
    winlayout)
        case "$(uname -s)" in
            Darwin)
                cc -std=c99 -Wall -Wextra "$here/tools/wingeom.c" -framework CoreGraphics -framework CoreFoundation -o "$tmp/collector"
                "$tmp/collector" > "$tmp/input"
                run "$tmp/input" ;;
            *) echo 'winlayout: provide real bottom-to-top geometry (screen W H, then X Y W H title)' >&2; exit 1;;
        esac ;;
    exeinfo) run "$compiler" ;;
esac
