#!/bin/bash
# The tape interpreter on hand-written tapes. [TP-4]
#
# Everything else reaches the VM through the compiler; these fixtures exercise
# it directly, so a VM regression cannot hide behind a front-end change.
set -u
pass=0; fail=0
chk() { if [ "$2" = "$3" ]; then pass=$((pass+1)); printf "  ok   %-10s %s\n" "$1" "$3"
        else fail=$((fail+1)); printf "  FAIL %-10s got '%s' want '%s'\n" "$1" "$3" "$2"; fi; }
chk hello "hello from C99" "$(python3 -m unisa vm tests/tapes/hello.tape)"
chk fact  "120"            "$(python3 -m unisa vm tests/tapes/fact.tape)"
chk call  "55"             "$(python3 -m unisa vm tests/tapes/call.tape)"
echo; echo "vm fixtures $pass   wrong $fail"
[ "$fail" -eq 0 ]
