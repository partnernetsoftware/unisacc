#!/usr/bin/env python3
"""嵌套 bound 的真父子测试：bound 调 bound，C 与 Python 入口混合。

只靠真实的进程嵌套：外层 bound 启动 sh -c，sh 里再调用内层 bound。内层通过继承环境拿到外层的硬截止。
不注入任何截止值。每条用例打印 父入口、子入口、rc、耗时。

用法：python3 -I tests/boundnested.py REPO_ROOT [--gate] [--only 1,2,...]
"""
import os
import re
import shlex
import subprocess
import sys
import tempfile
import time

REPO = sys.argv[1]
GATE = '--gate' in sys.argv[2:]
if sys.platform != 'linux':
    print('SKIP %s: needs Linux /proc (subreaper, ledger identity, dmesg admission); host is %s'
          % (os.path.basename(__file__), sys.platform), file=sys.stderr)
    sys.exit(77)
ARGS = sys.argv[2:]
ONLY = set(int(x) for x in ARGS[ARGS.index('--only') + 1].split(',')) if '--only' in ARGS else None
CASE_NO = [0]
# gate: parent N 5 for cases 1-4, two cases 2 x bound_max(5) = 14.5 s < 16 s; hand runs keep 8
PN = 5 if GATE else 8


def wanted(no):
    return ONLY is None or no in ONLY


sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from admission_r7p import admit
# Hard deadline = N + TOTAL_EXTRA_SECS; outer-wall bound = hard + CLEANUP_EPSILON_SECS.  Both read from tests/bound.c.
_BOUND_SRC = open(os.path.join(REPO, 'tests', 'bound.c')).read()
TOTAL_EXTRA = float(re.search(r'#define TOTAL_EXTRA_SECS ([0-9.]+)', _BOUND_SRC).group(1))
EPS = float(re.search(r'#define CLEANUP_EPSILON_SECS ([0-9.]+)', _BOUND_SRC).group(1))
def hard(n):
    return n + TOTAL_EXTRA
def bound_max(n):
    return hard(n) + EPS
ENTRY = {
    'c': [os.path.join(REPO, 'tests', 'bound')],
    'py': ['python3', '-I', os.path.join(REPO, 'tests', 'bound.py')],
}
os.environ['BOUND_CACHE'] = os.path.join(tempfile.mkdtemp(prefix='bound-nested-'), 'cache')
results = []


def inner(entry, seconds, command):
    """内层调用，作为外层 sh -c 的一段字符串。"""
    return shlex.join(ENTRY[entry] + [str(seconds), 'sh', '-c', command])


def case(label, parent, parent_n, child, child_n, child_cmd, want_rc, max_secs, min_secs=0.0):
    CASE_NO[0] += 1
    if not wanted(CASE_NO[0]):
        return None
    script = inner(child, child_n, child_cmd)
    cmd = ENTRY[parent] + [str(parent_n), 'sh', '-c', script]
    if not GATE:
        admit(label)
    t0 = time.monotonic()
    p = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    dt = time.monotonic() - t0
    why = []
    if p.returncode != want_rc:
        why.append('rc=%d want %d' % (p.returncode, want_rc))
    if dt > max_secs:
        why.append('took %.2fs > %.2f' % (dt, max_secs))
    if dt < min_secs:
        why.append('took %.2fs < %.2f' % (dt, min_secs))
    ok = not why
    results.append((label, ok))
    print(('PASS ' if ok else 'FAIL ') + '%s parent=%s(N=%d) child=%s(N=%d) rc=%d %.2fs' %
          (label, parent, parent_n, child, child_n, p.returncode, dt) + (' | ' + '; '.join(why) if why else ''))
    if not ok and p.stderr:
        print('      stderr: ' + p.stderr.strip().splitlines()[-1])
    return p


# 1-4. 子比父长：父 N=8 钳住子 N=30，约 8 秒返回，rc 142
case('C->C longer child', 'c', PN, 'c', 30, 'sleep 30', 142, bound_max(PN))
case('C->py longer child', 'c', PN, 'py', 30, 'sleep 30', 142, bound_max(PN))
case('py->C longer child', 'py', PN, 'c', 30, 'sleep 30', 142, bound_max(PN))
case('py->py longer child', 'py', PN, 'py', 30, 'sleep 30', 142, bound_max(PN))

# 5. 子比父短：父 N=8，子 N=2 跑 1 秒，rc 0
case('py->C shorter child', 'py', 8, 'c', 2, 'sleep 1', 0, bound_max(8), min_secs=0.9)

# 6. 父比子短：父 N=2，子 N=30 → 约 2 秒 142
case('C->C shorter parent', 'c', 2, 'c', 30, 'sleep 30', 142, bound_max(2))

# 8. 截止确实逐层下传：父导出的值，子 sh 里读到的值必须与父相同或更早
if wanted(8):
    if not GATE:
        admit('nested-deadline-propagates')
    t_parent = subprocess.run(
        ENTRY['c'] + ['8', 'sh', '-c',
                      'echo P=$UNISACC_BOUND_DEADLINE_MONO; ' + inner('py', 5, 'echo C=$UNISACC_BOUND_DEADLINE_MONO')],
        capture_output=True, text=True, timeout=60)
    lines = dict(x.split('=', 1) for x in t_parent.stdout.split() if '=' in x)
    p_val = float(lines.get('P', 'nan'))
    c_val = float(lines.get('C', 'nan'))
    ok = t_parent.returncode == 0 and p_val == p_val and c_val == c_val and c_val <= p_val + 1e-6
    results.append(('deadline propagates down (C parent -> py child)', ok))
    print(('PASS ' if ok else 'FAIL ') + 'deadline propagates: parent=%s child=%s rc=%d' %
          (lines.get('P'), lines.get('C'), t_parent.returncode))

# 7. 真孙进程的清理：外层 N=3，子 sh 里起一个后台孙进程，清理后该孙进程必须不在
marker = '%d' % os.getpid()   # sleep 需要合法数字：97.<pid> 能唯一标识孙进程
case('C->C grandchild reaped at hard deadline', 'c', 3, 'c', 10,
     'sh -c "sleep 97.%s & sleep 98.%s"' % (marker, marker), 142, bound_max(3))
if wanted(7):
    left = subprocess.run(['pgrep', '-f', 'sleep 97.%s' % marker], capture_output=True, text=True).stdout.split()
    ok = not left
    results.append(('no grandchild left after hard stop', ok))
    print(('PASS ' if ok else 'FAIL ') + 'grandchild left: %s' % (left or 'none'))

failed = [label for label, ok in results if not ok]
print('SUMMARY nested: %d/%d pass' % (len(results) - len(failed), len(results)))
sys.exit(1 if failed else 0)
