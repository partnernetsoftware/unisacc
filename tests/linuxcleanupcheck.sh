#!/bin/sh
# Exercise the real Linux wrapper's guarded guest cleanup with a shell Lima mock.
# This is deletion-scope evidence only; it does not claim VM compatibility.
set -eu
R=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
B=$("$R/tests/bound" --helper) || exit 2
base=$(mktemp -d "${TMPDIR:-/tmp}/linux-cleanup.XXXXXX") || exit 2
cleanup() {
    rc=$?
    trap - EXIT HUP INT TERM
    if ! sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$base"; then
        echo "linuxcleanupcheck: guarded fixture cleanup failed: $base" >&2
        [ "$rc" -ne 0 ] || rc=1
    fi
    exit "$rc"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
fail() { echo "linuxcleanupcheck: $* (fixture $base)" >&2; exit 1; }
fixture=$base/repo
guest=$base/guest
tools=$base/bin
S=$guest/unisa-scratch
mkdir -p "$fixture/tests" "$fixture/include" "$fixture/corpus/c-testsuite/tests/single-exec" "$tools" "$S/linux.stale" "$S/tmp.stale"
sh "$R/tests/fixturecopy.sh" "$R" "$fixture" tests/linuxcleanup-files.tsv
cat > "$fixture/tests/probe.sh" <<'PROBE'
#!/bin/sh
echo latest-failure
exit 3
PROBE
chmod 755 "$fixture/tests/probe.sh"
cat > "$tools/limactl" <<'LIMA'
#!/bin/sh
set -eu
case ${1:-} in
    list)
        case "$*" in
            *'.Status'*) echo Running;;
            *) arch=$(uname -m); [ "$arch" != arm64 ] || arch=aarch64; printf '%s\n' "$arch";;
        esac;;
    shell)
        case "$*" in
            *'/proc/meminfo'*) echo 8000000;;
            *'bash'*)
                for command do :; done
                HOME=${LINUX_TEST_HOME:?} export HOME
                exec /bin/bash -c "$command";;
            *) echo yes;;
        esac;;
    *) echo 'mock limactl: unexpected command' >&2; exit 2;;
esac
LIMA
chmod 755 "$tools/limactl"
printf 'old\n' > "$S/linux.stale/marker"
printf 'old\n' > "$S/tmp.stale/marker"
printf 'keep\n' > "$guest/unrelated"
for name in unisa-linux unisa-tmp; do
    mkdir "$guest/$name"
    printf 'legacy keep\n' > "$guest/$name/marker"
done
PATH=$tools:$PATH
LINUX_TEST_HOME=$guest
STRICT=1
export PATH LINUX_TEST_HOME STRICT
assert_scope() {
    [ "$(cat "$guest/unrelated")" = keep ] || fail 'unrelated guest file changed'
    for name in unisa-linux unisa-tmp; do
        [ "$(cat "$guest/$name/marker")" = 'legacy keep' ] || fail "legacy $name changed"
    done
    for path in "$S"/linux.* "$S"/tmp.* "$S/lock" "$S/current.log"; do
        if [ -e "$path" ] || [ -L "$path" ]; then fail "guest scratch survived: $path"; fi
    done
}
rc=0
"$B" 10 /bin/bash "$fixture/tests/linux.sh" probe > "$base/failure.stdout" 2> "$base/failure.stderr" || rc=$?
if [ "$rc" -ne 3 ]; then
    cat "$base/failure.stderr" "$base/failure.stdout" >&2
    fail "failed probe rc=$rc, expected 3"
fi
printf 'latest-failure\n' > "$base/expected-failure.log"
cmp -s "$base/expected-failure.log" "$guest/unisa-linux-last-failure.log" || fail 'failure log missing or changed'
assert_scope
cat > "$fixture/tests/probe.sh" <<'PROBE'
#!/bin/sh
echo success
PROBE
rc=0
"$B" 10 /bin/bash "$fixture/tests/linux.sh" probe > "$base/success.stdout" 2> "$base/success.stderr" || rc=$?
if [ "$rc" -ne 0 ]; then
    cat "$base/success.stderr" "$base/success.stdout" >&2
    fail "successful probe rc=$rc, expected 0"
fi
cmp -s "$base/expected-failure.log" "$guest/unisa-linux-last-failure.log" || fail 'success overwrote the latest failure log'
assert_scope
echo 'linux: old scratch cleared; failure tree removed; latest failure log retained; unrelated and legacy paths preserved (mock guest)'
