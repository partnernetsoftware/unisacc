# What every suite needs, once.  Sourced, never executed.
#
#   . "$(dirname "$0")/lib.sh"
#
# Four things repeat across thirty suites, and each of them has been got
# wrong at least once:
#
#   * the reference build.  Suites spell `UA=${UA:-/tmp/ua_ref}` and then build
#     it if absent.  Two sessions sharing /tmp/ua_ref once made a 270-image
#     "divergence" that was a stale binary from another tree; ua_ready now
#     stamps the binary with its source hash AND serialises builds behind a lock
#     with atomic replacement (2026-09-29), because a stamp detects staleness but
#     cannot stop several suites from building onto the same path at once.
#   * the watchdog.  macOS has no timeout(1); the idiom is
#     `tests/bound N CMD...`, which also cleans descendants.  A step
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
# callers set R (the repo root) first; BASH_SOURCE is a bashism dash rejects
_LIB_R=${R:-$(cd "$(dirname "$0")/.." && pwd)}

# ua_ready -- the self-hosted compiler exists, or build it.
ua_rebuild_to() {   # ua_rebuild_to <final-path> <stamp-value-or-empty>
    # Build beside the destination and rename, never onto it: POSIX rename is
    # atomic within a filesystem, so a process executing $UA keeps its inode and
    # a concurrent reader sees either the old binary or the new one, never half
    # of either.  Writing in place is what produced the 270-image divergence.
    local tmp="$1.tmp.$$"
    bound 30 "$_LIB_R/tests/build_ref.sh" "$tmp.c" "$tmp" >/dev/null || {
        rm -f "$tmp" "$tmp.c"
        echo "  FAIL could not build the reference compiler ($1)"; exit 1; }
    mv -f "$tmp" "$1" || { rm -f "$tmp"; echo "  FAIL could not install $1"; exit 1; }
    [ -n "$2" ] && printf '%s\n' "$2" > "$1.stamp.tmp.$$" && mv -f "$1.stamp.tmp.$$" "$1.stamp"
    return 0
}
ua_ready() {
    # A binary that exists is not a binary that is current: a stale
    # /tmp/ua_ref once passed the old compiler off as the new one, on this
    # host and inside the Linux VM.  The default build is stamped with the
    # hash of the sources it came from and rebuilt when they differ; a UA
    # the caller chose is used as given.
    # Two sessions sharing /tmp/ua_ref once made a 270-image "divergence" that
    # was a stale binary from another tree; the stamp catches staleness, and a
    # build lock plus atomic replacement (2026-09-29) catch concurrency, which
    # the stamp alone cannot: without them every parallel suite that finds the
    # stamp stale builds onto the same path at the same time.
    local want
    if [ "$UA" != /tmp/ua_ref ]; then [ -x "$UA" ] && return 0; ua_rebuild_to "$UA" ""; return 0; fi
    want=$(cat "$_LIB_R"/kernel/*.inc "$_LIB_R"/kernel/*.c "$_LIB_R"/src/*.c "$_LIB_R"/src/*.h \
           "$_LIB_R"/unisacc.c "$_LIB_R"/tests/export_ref.sh \
           "$_LIB_R"/tests/build_ref.sh "$_LIB_R"/tests/refshim.h "$_LIB_R"/tests/reffoot.h | cksum)
    [ -x "$UA" ] && [ "$(cat "$UA.stamp" 2>/dev/null)" = "$want" ] && return 0
    # Someone may already be building exactly this.  Wait for the stamp, but
    # never wait forever: the fallback builds privately and replaces atomically,
    # so losing the race costs a duplicate build and cannot corrupt anything.
    local lock="$UA.lock.d" held=0 i=0
    if mkdir "$lock" 2>/dev/null; then
        held=1; printf '%s\n' "$$" > "$lock/pid"
    else
        local owner; owner=$(cat "$lock/pid" 2>/dev/null || :)
        if [ -n "$owner" ] && ! kill -0 "$owner" 2>/dev/null; then
            rm -rf "$lock"                     # holder died; take it over
            mkdir "$lock" 2>/dev/null && { held=1; printf '%s\n' "$$" > "$lock/pid"; }
        fi
    fi
    if [ "$held" = 0 ]; then
        while [ $i -lt 50 ]; do
            [ "$(cat "$UA.stamp" 2>/dev/null)" = "$want" ] && return 0
            sleep 1; i=$((i+1))
        done
    fi
    ua_rebuild_to "$UA" "$want"
    [ "$held" = 1 ] && rm -rf "$lock"
    return 0
}

# bound N cmd... -- run with a hard limit.  macOS has no timeout(1).
_BOUND_HELPER=$("$_LIB_R/tests/bound" --helper) || exit 2
bound() { local n=$1; shift; "$_BOUND_HELPER" "$n" "$@"; }

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
