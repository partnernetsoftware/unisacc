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

# Exercise the real cleanup helper's natural-exit race and shared deadline without spawning work.
class CleanupProbe:
    def __init__(self, result): self.returncode=result; self.signals=[]
    def poll(self): return self.returncode
    def terminate(self): self.signals.append('TERM')
    def wait(self, timeout=None): return self.returncode
    def kill(self): self.signals.append('KILL')
for code in (0,1,77,142):
    proc=CleanupProbe(code); active={'p':(proc,None,0,12)}; got=[]
    def settled(name, rc, cleanup=None):
        got.append((name,rc,cleanup)); active.pop(name)
    gq.cleanup_active(active,settled)
    check(got==[('p',code,None)] and not proc.signals and not active,
          'cleanup preserves natural pre-TERM rc %s without signaling'%code)

# A callback failure must not prevent another live process from being reaped first.
class LiveCleanupProbe(CleanupProbe):
    def __init__(self): super().__init__(None); self.reaped=False
    def wait(self, timeout=None): self.reaped=True; self.returncode=143; return 143
done=CleanupProbe(0); live=LiveCleanupProbe(); active={'done':(done,None,0,12),'live':(live,None,0,12)}
seen=[]; failure=OSError('injected settlement ENOSPC')
def failed_settlement(name, rc, cleanup=None):
    check(live.reaped and live.signals==['TERM'], 'all live jobs reaped before any settlement callback')
    seen.append((name,rc)); active.pop(name)
    if name=='done': raise failure
try:
    gq.cleanup_active(active,failed_settlement)
    check(False,'settlement OSError must propagate')
except OSError as error:
    check(error is failure,'first settlement exception object preserved')
check(seen==[('done',0),('live',143)] and not active,'remaining callbacks execute after first callback error')
print('gatequeue timeout settlement: PASS' if fails == 0 else 'gatequeue timeout settlement: FAIL %d' % fails)
sys.exit(1 if fails else 0)
