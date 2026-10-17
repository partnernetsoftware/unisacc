#!/bin/sh
# ps stand-in (r7g static draft): recorded through the ledger helper, then never answers.
exec sh "$(dirname "$0")/boundstall_ledger.sh" ps-stall sleep 1000."$BOUNDSTALL_PID"
