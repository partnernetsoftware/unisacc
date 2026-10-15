#!/bin/sh
# Offline Mach-O seed tools. The container carries ISA-neutral PIC blobs;
# its Mach-O header is not a runtime dependency on the build host OS.
set -eu
R=$(cd "$(dirname "$0")/../../.." && pwd)
arch=${1:?architecture}; mode=${2:?compile|link|nm}; shift 2
case $arch in arm64|x86_64) ;; *) echo 'unsupported kernel ISA' >&2; exit 2;; esac
b="$R/tests/bound"
# bound 20: a timeout (alarm, rc 142) is named on stderr; any other failure keeps its own status.
bounded() { rc=0; "$b" 20 "$@" || rc=$?; [ "$rc" -ne 142 ] || echo "machocc: $mode timed out after 20 s (bound 20): $1" >&2; exit "$rc"; }
case $(uname -s) in
    Darwin)
        case $mode in
            compile) bounded cc -arch "$arch" "$@";;
            link) bounded cc -arch "$arch" -nostdlib -Wl,-static -Wl,-e,_kernel_entry -Wl,-no_uuid -Wl,-no_fixup_chains "$@";;
            nm) bounded nm "$@";;
        esac;;
    Linux)
        case $mode in
            compile) bounded clang --target="$arch-apple-macos11" "$@";;
            link)
                ld=$(command -v ld64.lld || :)
                if [ -z "$ld" ]; then
                    for v in 24 23 22 21 20 19 18 17 16 15 14; do
                        ld=$(command -v "ld64.lld-$v" || :)
                        [ -z "$ld" ] || break
                    done
                fi
                [ -n "$ld" ] || { echo 'kernel seed linker missing: install LLVM lld' >&2; exit 2; }
                bounded "$ld" -arch "$arch" -platform_version macos 11.0 11.0 -e _kernel_entry -no_uuid -no_fixup_chains "$@";;
            nm) bounded llvm-nm "$@";;
        esac;;
    *) echo 'unsupported kernel seed host: require Darwin or Linux' >&2; exit 2;;
esac
echo 'unknown kernel seed operation' >&2
exit 2
