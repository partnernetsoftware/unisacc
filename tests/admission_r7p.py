"""Per-case admission (r7p): load1 must be below 6 and the last VM fork (dmesg) more than 30 s ago, checked before
each case.  Hand-run measurement only: waits and re-checks.  Gate runs do not call admit.  Each admitted case appends one receipt line:
label, load1, fork gap, and how long it waited.  The receipt path comes from UNISACC_ADMISSION_RECEIPT.
"""
import os
import re
import subprocess
import time

LOAD_MAX = 6.0
FORK_GAP_MIN = 30.0
RECEIPT = os.environ.get('UNISACC_ADMISSION_RECEIPT', os.path.join(os.getcwd(), 'admission-receipt.txt'))


def load1():
    return float(open('/proc/loadavg').read().split()[0])


def uptime():
    return float(open('/proc/uptime').read().split()[0])


def last_vm_fork_uptime():
    """Returns (uptime of the last VM fork, or None if the log has none).  An unreadable dmesg raises: an unknown
    fork history is never treated as 'no fork'."""
    r = subprocess.run(['dmesg'], capture_output=True, text=True, timeout=10)
    if r.returncode != 0:
        raise RuntimeError('dmesg failed rc=%d: admission cannot be proven' % r.returncode)
    last = None
    for line in r.stdout.splitlines():
        if 'virtual machine fork' in line:
            m = re.match(r'\[\s*([0-9.]+)\]', line)
            if m:
                last = float(m.group(1))
    return last


def check(label):
    """One check.  Returns (ok, load1, fork gap or None, why).  An unreadable dmesg or a log without a VM fork line is
    unproven, never admitted."""
    load = load1()
    try:
        last = last_vm_fork_uptime()
    except RuntimeError as e:
        return False, load, None, str(e)
    if last is None:
        return False, load, None, 'no VM fork line in dmesg'
    gap = uptime() - last
    if load >= LOAD_MAX:
        return False, load, gap, 'load1=%.2f >= %.1f' % (load, LOAD_MAX)
    if gap <= FORK_GAP_MIN:
        return False, load, gap, 'fork_gap=%.1f s <= %.1f s' % (gap, FORK_GAP_MIN)
    return True, load, gap, ''


def admit(label, max_wait=1800.0):
    """Hand-run measurement only: wait up to max_wait, re-checking every 10 s, so timings are comparable.  Gate runs
    never call this: the bound contract (wall within hard + epsilon) is asserted directly, whatever the load."""
    t0 = time.monotonic()
    while True:
        ok, load, gap, why = check(label)
        if ok:
            break
        if time.monotonic() - t0 > max_wait:
            raise SystemExit('admission not met for %s within %.0f s (%s)' % (label, max_wait, why))
        time.sleep(10)
    waited = time.monotonic() - t0
    with open(RECEIPT, 'a') as f:
        f.write('%s load1=%.2f fork_gap_s=%s waited_s=%.0f\n'
                % (label, load, 'none' if gap is None else '%.1f' % gap, waited))
    return load, gap
