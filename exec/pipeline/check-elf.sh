#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
# Fixed end-to-end set; a missing source or any rejecting stage fails.
[ -s exec/pipeline/keep-elf.txt ] || { echo 'empty ELF keep list'; exit 1; }
# ELF_SHARD=K/N keeps every Nth path starting at the Kth, so the gate can run
# the list as several bounded jobs; unset keeps all of it.
case ${ELF_SHARD:-} in
  '') set -- $(cat exec/pipeline/keep-elf.txt) ;;
  */*) set -- $(awk -v k="${ELF_SHARD%/*}" -v n="${ELF_SHARD#*/}" 'NF && (NR - 1) % n == k - 1' exec/pipeline/keep-elf.txt) ;;
  *) echo "ELF_SHARD wants K/N"; exit 2 ;;
esac
[ $# -gt 0 ] || { echo "ELF_SHARD=$ELF_SHARD selected nothing"; exit 1; }
b ./exec/pipeline/elf.sh "$T" "$@" > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/pipeline/check-elf.py "$UA" "$T" "$@"
