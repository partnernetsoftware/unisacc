# What every suite needs, once.  Sourced, never executed.
#
#   . "$(dirname "$0")/lib.sh"
#
# Four things repeat across thirty suites, and each of them has been got
# wrong at least once:
#
#   * the reference build.  Twelve suites spell `UA=${UA:-/tmp/ua_ref}` and
#     then build it if absent.  Two sessions sharing /tmp/ua_ref once made a
#     270-image "divergence" that was a stale binary from another tree, so
#     ua_ready also says where the binary came from when UA was inherited.
#   * the watchdog.  macOS has no timeout(1); the idiom is
#     `perl -e 'alarm N; exec @ARGV'` and it appears thirty times.  A step
#     without one hung this session for twenty minutes.
#   * this host's target.  Seven suites carry the same `uname` case, and a
#     suite that gets it wrong silently tests a target it cannot run.
#   * a scratch directory that is removed on exit, including on failure.
#
# It deliberately does NOT abstract the counting or the summary line: a
# failure has to read as a sentence about this suite, and a shared reporter
# would flatten them all into the same one.
: "${UA:=/tmp/ua_ref}"
UA_RUN=${UA_RUN:-$UA}
_LIB_R=$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")/.." && pwd)

# ua_ready -- the self-hosted compiler exists, or build it.
ua_ready() {
    [ -x "$UA" ] && return 0
    "$_LIB_R/tests/build_ref.sh" "$UA.c" "$UA" >/dev/null || {
        echo "  FAIL could not build the reference compiler ($UA)"; exit 1; }
}

# bound N cmd... -- run with a hard limit.  macOS has no timeout(1).
bound() { local n=$1; shift; perl -e 'alarm shift; exec @ARGV' "$n" "$@"; }

# host_target -- echoes this machine's target, or nothing when it is not one
# of the six.  A caller that gets "" must SKIP, not guess.
host_target() {
    case "$(uname -s)/$(uname -m)" in
        Darwin/arm64)  echo osx/arm64;;
        Darwin/x86_64) echo osx/x86_64;;
        Linux/x86_64)  echo lnx/x86_64;;
        Linux/aarch64) echo lnx/arm64;;
        *)             echo "";;
    esac
}

# scratch -- a directory removed when the suite exits, however it exits.
#
# The trap is registered HERE, when the file is sourced, not inside
# scratch().  A trap set inside `$(...)` belongs to the substitution's own
# subshell and fires the moment it returns: the first version of this
# deleted the directory before the caller could write a file into it, and
# five diag cases failed with "cannot open input".
_LIB_TMP=$(mktemp -d)
trap 'rm -rf "$_LIB_TMP"' EXIT INT TERM
_LIB_N=0
scratch() {
    _LIB_N=$((_LIB_N + 1))
    mkdir -p "$_LIB_TMP/$_LIB_N"
    echo "$_LIB_TMP/$_LIB_N"
}

# ratchet <baseline-file> <count> <passing-list> -- 0 when the count held.
#
# Three suites (ccrun, corpus, tools) carried a byte-identical copy of this,
# and selfgap a fourth, more general one.  They had already drifted: only
# two of the three sorted the list before comparing, and only two printed
# the lost entries unbounded.  A ratchet whose regression report depends on
# which file you are reading is not a ratchet.
#
# The caller keeps its own summary line: "unisacc-compiled 90 wrong 0" and
# "corpus 220 pass 214" are different sentences about different things, and
# flattening them would cost more in legibility than it saves in lines.
ratchet() {
    local base=$1 count=$2 list=$3 prev sorted
    sorted=$(mktemp); sort "$list" > "$sorted"
    if [ ! -f "$base" ]; then
        echo "$count" > "$base"; cp "$sorted" "$base.list"
        echo "  baseline recorded: $count"
        rm -f "$sorted"; return 0
    fi
    prev=$(cat "$base")
    if [ "$count" -lt "$prev" ]; then
        echo "  REGRESSION: $count < baseline $prev"
        comm -13 "$sorted" "$base.list" 2>/dev/null | head -20 | sed 's/^/    lost /'
        rm -f "$sorted"; return 1
    fi
    if [ "$count" -gt "$prev" ]; then
        echo "  baseline $prev -> $count (run with RATCHET=1 to record)"
        if [ "${RATCHET:-0}" = "1" ]; then
            echo "$count" > "$base"; cp "$sorted" "$base.list"; echo "  recorded."
        fi
    fi
    rm -f "$sorted"; return 0
}
