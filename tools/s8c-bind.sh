#!/bin/sh
# tools/s8c-bind.sh ROOT SESSION CANDIDATE MAP
# init (SESSION must be absent) then `all-waves material` for every MAP name.
set -e
R=${1:?ROOT}; S=${2:?SESSION}; C=${3:?CANDIDATE}; M=${4:?MAP}
cd "$R"
test ! -e "$S" || { echo "s8c-bind: session-must-be-absent $S" >&2; exit 2; }
tools/bound 55 sh ./unisacc.com tools/all-waves.cx -- init "$R" "$S" "$C" "$M"
awk -F'\t' 'NF>=2 && $1 !~ /^#/{print $1}' "$M" | while read n; do
  tools/bound 55 sh ./unisacc.com tools/all-waves.cx -- material "$R" "$S" "$n"
done
echo BIND_DONE
