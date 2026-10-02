#!/bin/bash
# winrun.sh EXE...  -- run Windows executables in the UTM VM (guest agent) and
# print each one's output after a "=== name" line.  The VM must be started
# (tests/vms.sh up, or utmctl start); exits 2 when it does not answer.
set -u
UTM=/Applications/UTM.app/Contents/MacOS/utmctl; vm=${WINVM:-minicon-win-arm-64}
[ -n "$("$UTM" ip-address "$vm" 2>/dev/null | head -1)" ] || { echo "winrun: $vm agent not answering"; exit 2; }
t=$(date +%s)$RANDOM; Z='C:\u\z'"$t"'.txt'; D='C:\u\d'"$t"'.txt'; G='C:\u\g'"$t"'.bat'
{ printf '@echo off\r\n'; n=0
  for f in "$@"; do n=$((n+1)); X='C:\u\w'"$t"'_'"$n"'.exe'
    "$UTM" file push "$vm" "$X" < "$f" 2>/dev/null
    printf 'echo === %s >> %s\r\n' "$(basename "$f")" "$Z"; printf '%s >> %s 2>&1\r\n' "$X" "$Z"
    printf 'echo rc=%%ERRORLEVEL%% >> %s\r\n' "$Z"; done
  printf 'echo done > %s\r\n' "$D"; } | "$UTM" file push "$vm" "$G" 2>/dev/null
"$UTM" exec "$vm" --cmd "cmd.exe" -- /c "$G" >/dev/null 2>&1 || { echo "winrun: guest exec refused"; exit 1; }
i=0; while [ $i -lt 60 ]; do case "$("$UTM" file pull "$vm" "$D" 2>&1)" in *done*) break;; esac; i=$((i+1)); sleep 1; done
"$UTM" file pull "$vm" "$Z" 2>&1 | tr -d '\r' | sed -e 's/[ \t]*$//'
