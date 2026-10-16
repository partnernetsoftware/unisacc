#!/usr/bin/env python3
"""gatequeue rc 142 settlement: a truncated attempt is pending, a full-budget attempt is terminal, and a retry needs a full window.

The window length is span (seconds).  A full attempt has left >= span-3 and limit >= span-4 (gatequeue's own rule)."""
import pathlib, sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gatequeue as gq

fails = 0
def check(cond, msg):
    global fails
    print(('ok   ' if cond else 'FAIL ') + msg)
    fails += 0 if cond else 1

SPAN = 55
# truncated retry (the second, solo attempt given only the remainder): not terminal
check(gq.rc142_settles('solo', 14, SPAN) is False, 'truncated solo retry (limit 14 of 55) is pending, not a terminal 142')
check(gq.rc142_settles('solo', SPAN - 5, SPAN) is False, 'solo attempt just below full budget (limit span-5) is pending')
# full-budget attempts: terminal
check(gq.rc142_settles('solo', SPAN - 4, SPAN) is True, 'full-budget solo retry (limit span-4) is a terminal 142')
check(gq.rc142_settles('full', SPAN - 4, SPAN) is True, 'full-budget first attempt (limit span-4) is a terminal 142')
check(gq.rc142_settles('full', SPAN - 3, SPAN) is True, 'full attempt with limit span-3 is terminal')
check(gq.rc142_settles('full', SPAN - 5, SPAN) is False, 'full-kind attempt with limit span-5 (below the full budget) is pending')
# tail fills are never terminal
check(gq.rc142_settles('tail', 50, SPAN) is False, 'tail fill is never a terminal 142, even with a large limit')
# retry admission: only with a full window
check(gq.retry_admissible(14, SPAN) is False, 'retry is not admitted with 14 s left (window remainder)')
check(gq.retry_admissible(SPAN - 4, SPAN) is False, 'retry is not admitted when left < span-3')
check(gq.retry_admissible(SPAN - 3, SPAN) is True, 'retry is admitted with a full window (left >= span-3)')
check(gq.retry_admissible(SPAN, SPAN) is True, 'retry is admitted with the whole window')

# state reuse: a stored truncated 142 (state4's closure-c1: solo, limit 14) is pending on load; full-budget and rc 0 are kept
jobs = {'closure-c1': 1, 'full-ok': 1, 'legacy': 1, 'ok': 1}
stored = {'closure-c1': {'rc': 142, 'kind': 'solo', 'limit': 14}, 'full-ok': {'rc': 142, 'kind': 'full', 'limit': SPAN - 3},
          'legacy': {'rc': 142, 'limit': SPAN - 3}, 'ok': {'rc': 0, 'kind': 'full', 'limit': 14}}
check(gq.stored_142_pending(stored, jobs, SPAN) == ['closure-c1'], 'stored truncated 142 (solo, limit 14) is pending on load')
check(gq.stored_142_pending(stored, {'full-ok': 1}, SPAN) == [], 'stored full-budget 142 stays terminal on load')
print('gatequeue timeout settlement: PASS' if fails == 0 else 'gatequeue timeout settlement: FAIL %d' % fails)
sys.exit(1 if fails else 0)
