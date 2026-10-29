#!/bin/sh
# csih CX entry. Upper modules: pragma only. Lower (net/term/file/shell/…): may #include.
# Daily use: `./csih.sh prepare` once (rebuilds out/csih-e2e-bin/csih only when the
# source digest changed), then `./csih.sh` opens the TUI at once.
# CSIH_BIN=native ELF overrides; CSIH_COLD=1 forces cold -run (~47s/module graph).
CALLER=$(pwd)
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 1
REPO=$(CDPATH= cd -- "$ROOT/../.." && pwd) || exit 1
UNI=${UNISACC:-$REPO/unisacc.com}
OUT=$REPO/out/csih-e2e-bin
BIN=$OUT/csih
cd "$ROOT" || exit 1
src_digest() {
    ls *.cx *.h 2>/dev/null | LC_ALL=C sort | xargs cat | sha256sum | cut -d' ' -f1
}
if [ $# -eq 0 ]; then
    set -- agent
fi
if [ "$1" = prepare ]; then
    d=$(src_digest)
    if [ -x "$BIN" ] && [ "$(cat "$BIN.src" 2>/dev/null)" = "$d" ]; then
        echo "csih prepare: up to date $BIN"
        exit 0
    fi
    mkdir -p "$OUT" || exit 1
    echo "csih prepare: building $BIN (source digest changed, ~1min)"
    timeout 300 sh "$UNI" "$ROOT/csih.cx" -o "$BIN.new" >"$OUT/build.out" 2>"$OUT/build.err" || {
        echo "csih prepare: build failed, see $OUT/build.err" >&2; exit 1; }
    mv "$BIN.new" "$BIN" && echo "$d" >"$BIN.src" || exit 1
    echo "csih prepare: built $BIN"
    exit 0
fi
if [ -z "$CSIH_BIN" ] && [ -z "$CSIH_COLD" ] && [ -x "$BIN" ]; then
    if [ "$(cat "$BIN.src" 2>/dev/null)" = "$(src_digest)" ]; then
        CSIH_BIN=$BIN
    else
        echo "csih: $BIN is stale or unstamped; run: $0 prepare (or CSIH_COLD=1 for ~47s cold -run)" >&2
        exit 1
    fi
fi
if [ -n "$CSIH_BIN" ] && [ -x "$CSIH_BIN" ]; then
    # agent/run work in the caller's directory, not in apps/csih.
    case "$1" in agent|run) cd "$CALLER" || exit 1 ;; esac
    # unisacc's signal() never reaches the kernel, so csih cannot restore the
    # tty itself when killed. Run it as a child and restore here on a signal exit.
    if [ -t 0 ] && TTY_SAVED=$(stty -g 2>/dev/null); then
        "$CSIH_BIN" "$@"
        rc=$?
        if [ $rc -gt 128 ]; then
            stty "$TTY_SAVED" 2>/dev/null
            printf '\033[?1000l\033[?1006l\033[?2004l\033[?25h\033[0m\ncsih: 被信号 %d 结束，已恢复终端\n' $((rc - 128)) >&2
        fi
        exit $rc
    fi
    exec "$CSIH_BIN" "$@"
fi
if [ -z "$CSIH_COLD" ]; then
    echo "csih: missing $BIN; run: $0 prepare (or CSIH_COLD=1 for ~47s cold -run)" >&2
    exit 1
fi
if [ -f "$ROOT/csih.cx" ]; then
    exec /bin/sh "$UNI" -run "$ROOT/csih.cx" -- "$@"
fi
echo "csih: missing csih.cx" >&2
exit 1
