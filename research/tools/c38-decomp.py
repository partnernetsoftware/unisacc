# Job-seconds by attempt role from a frozen release-queue.log. Not wall-clock.
import re, sys, collections
att = collections.defaultdict(list)
for l in open(sys.argv[1]):
    m = re.match(r'(DONE|DEFER) (\S+) rc=(\S+) ([\d.]+)s', l)
    if m: att[m.group(2)].append((m.group(1), m.group(3), float(m.group(4))))
b = collections.Counter(); n = collections.Counter()
for s, seq in att.items():
    last = max(i for i, a in enumerate(seq) if a[0] == 'DONE') if any(a[0] == 'DONE' for a in seq) else -1
    for i, (ev, rc, t) in enumerate(seq):
        if ev == 'DEFER': k = 'deferral'
        elif i == last: k = 'useful (final pass)' if rc == '0' else 'final red/not-executed'
        elif rc in ('0',): k = 'rework (pass later redone)'
        else: k = 'retry (red later redone)'
        b[k] += t; n[k] += 1
tot = sum(b.values())
for k in sorted(b): print('%-28s %5d %8.1f %5.1f%%' % (k, n[k], b[k], 100*b[k]/tot))
print('%-28s %5d %8.1f' % ('total', sum(n.values()), tot))
