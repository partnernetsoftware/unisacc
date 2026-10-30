#!/bin/bash
# queuestart.sh Q BACKUP CANDIDATE_DIR [MODE] -- 0.0.40 start mode; the logic is release/tools/queuestart.py
# (fail-closed fresh/resume, receipt, refusal log).  Kept as the entry queue.sh and older notes name.
exec python3 "$(dirname "$0")/queuestart.py" "$1" "$2" "$3" "${4:-fresh}"
