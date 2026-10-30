#!/usr/bin/env python3
"""rulingwait.py [LEDGER] [--now ISO] [--blockers FILE] [--prev-pending FILE]
(0.0.39 WF4 + 0.0.40 idle-cut): read release/ruling-requests.tsv and report per-request
request->decision, decision->applied, and applied->begin wall-clock waits; PENDING rows
report their age.  Exits 1 on a malformed row (APPLIED without applied_at/ref, times out
of order) so the ledger cannot fake progress.  Waiting is wall clock, never CPU waste
or progress.

0.0.40 gate (等裁空转裁剪):
  --blockers FILE   one id per line claimed as currently blocking the active scope.
                    Any id whose ledger state is APPLIED is STALE_BLOCK (rc 2): already
                    consumed rulings must not keep blocking.  PENDING/DECIDED may remain.
  --prev-pending FILE  previous pending-id snapshot (whitespace-separated).  If the
                    current pending set equals that snapshot and no applied_at/begun_at
                    is newer than the oldest pending request in that set, exit rc 3
                    IDLE_NO_PROGRESS (no-action loop must not be recorded as progress).

Column 9 begun_at is optional (8-column ledgers still parse); empty/UNKNOWN begin does
not invent seconds.  Fixture-level enablement != real Stop-hook first observation."""
import datetime, pathlib, sys
ROOT = pathlib.Path(__file__).resolve().parents[2]
COLS = ('id', 'requested_at', 'decided_at', 'applied_at', 'state', 'decided_by', 'ref', 'subject', 'begun_at')


def parse(path):
    rows = []
    for line in pathlib.Path(path).read_text().splitlines():
        if not line.strip() or line.startswith('#'): continue
        f = line.split('\t')
        if len(f) not in (8, 9):
            raise SystemExit('rulingwait: MALFORMED line (want 8 or 9 tab-separated columns): %r' % line)
        if len(f) == 8: f.append('')
        rows.append(dict(zip(COLS, f)))
    ids = [r['id'] for r in rows]
    if len(ids) != len(set(ids)): raise SystemExit('rulingwait: MALFORMED duplicate id')
    for r in rows:
        for k in ('requested_at', 'decided_at', 'applied_at', 'begun_at'):
            v = r[k]
            if v and v != 'UNKNOWN' and datetime.datetime.fromisoformat(v).tzinfo is None:
                raise SystemExit('rulingwait: MALFORMED %s %s has no timezone' % (r['id'], k))
    return rows


def t(s): return datetime.datetime.fromisoformat(s) if s and s != 'UNKNOWN' else None


def check(rows):
    bad = []
    for r in rows:
        req, dec, app, beg = t(r['requested_at']), t(r['decided_at']), t(r['applied_at']), t(r['begun_at'])
        if r['state'] not in ('PENDING', 'DECIDED', 'APPLIED') or not req: bad.append((r['id'], 'state/requested_at'))
        if r['state'] == 'PENDING' and (dec or app or beg): bad.append((r['id'], 'PENDING with decision/applied/begin time'))
        if r['state'] in ('DECIDED', 'APPLIED') and not ((dec or r['decided_at'] == 'UNKNOWN') and r['decided_by']): bad.append((r['id'], 'decision without time/decider'))
        if r['state'] == 'APPLIED' and not (app and r['ref']): bad.append((r['id'], 'APPLIED without applied_at/ref'))
        if r['state'] != 'APPLIED' and beg: bad.append((r['id'], 'begin without APPLIED'))
        if (dec and req and dec < req) or (app and dec and app < dec) or (app and req and app < req): bad.append((r['id'], 'times out of order'))
        if beg and app and beg < app: bad.append((r['id'], 'begin before applied'))
        if beg and req and beg < req: bad.append((r['id'], 'begin before requested'))
    return bad


def load_ids(path):
    text = pathlib.Path(path).read_text()
    return [x for x in text.replace(',', ' ').split() if x and not x.startswith('#')]


def stale_blocks(rows, blocker_ids):
    by = {r['id']: r for r in rows}
    stale = []
    for i in blocker_ids:
        r = by.get(i)
        if r and r['state'] == 'APPLIED':
            stale.append(i)
        elif r is None:
            stale.append('%s(UNKNOWN_ID)' % i)
    return stale


def idle_no_progress(rows, prev_pending):
    pend = [r['id'] for r in rows if r['state'] == 'PENDING']
    if sorted(pend) != sorted(prev_pending):
        return False
    if not prev_pending:
        return False  # empty==empty is not an observed wait loop
    # any applied/begun newer than the earliest still-pending request means forward motion
    pend_rows = [r for r in rows if r['id'] in prev_pending]
    earliest = min(t(r['requested_at']) for r in pend_rows)
    for r in rows:
        for k in ('applied_at', 'begun_at'):
            ts = t(r[k])
            if ts and ts > earliest:
                return False
    return True


def main(argv=None):
    a = list(argv if argv is not None else sys.argv[1:])
    now = t(a[a.index('--now') + 1]) if '--now' in a else datetime.datetime.now().astimezone()
    blockers_path = a[a.index('--blockers') + 1] if '--blockers' in a else None
    prev_path = a[a.index('--prev-pending') + 1] if '--prev-pending' in a else None
    skip = set()
    for flag in ('--now', '--blockers', '--prev-pending'):
        if flag in a:
            i = a.index(flag); skip.add(i); skip.add(i + 1)
    path = next((x for i, x in enumerate(a) if i not in skip and not x.startswith('--')), ROOT / 'release/ruling-requests.tsv')
    rows = parse(path); bad = check(rows)
    for i, why in bad: print('rulingwait: MALFORMED %s: %s' % (i, why))
    m = lambda d: '%6.1f min' % (d.total_seconds() / 60)
    for r in rows:
        req, dec, app, beg = t(r['requested_at']), t(r['decided_at']), t(r['applied_at']), t(r['begun_at'])
        if r['state'] == 'PENDING':
            print('  %-3s PENDING  waiting %s  %s' % (r['id'], m(now - req), r['subject']))
        elif dec and req:
            extra = ''
            if app: extra += ', applied after ' + m(app - dec)
            if beg and app: extra += ', begun after ' + m(beg - app)
            elif beg and not app: extra += ', begun ' + r['begun_at']
            print('  %-3s %-8s decided after %s%s  by %s  %s' % (r['id'], r['state'], m(dec - req), extra, r['decided_by'], r['subject']))
        elif r['decided_at'] == 'UNKNOWN':
            extra = ''
            if app: extra += ', applied ' + r['applied_at']
            if beg: extra += ', begun ' + r['begun_at']
            print('  %-3s %-8s decision time UNKNOWN%s  by %s  %s' % (r['id'], r['state'], extra, r['decided_by'], r['subject']))
        else:
            print('  %-3s %-8s MALFORMED  %s' % (r['id'], r['state'], r['subject']))
    pend = [r['id'] for r in rows if r['state'] == 'PENDING']
    print('rulingwait  %d requests  pending %d (%s)' % (len(rows), len(pend), ' '.join(pend) or '-'))
    if bad:
        return 1
    if blockers_path is not None:
        stale = stale_blocks(rows, load_ids(blockers_path))
        if stale:
            print('rulingwait: STALE_BLOCK already-APPLIED must not block current scope: %s' % ' '.join(stale))
            return 2
        print('rulingwait  blockers ok (no APPLIED id in current scope blockers)')
    if prev_path is not None:
        prev = load_ids(prev_path)
        if idle_no_progress(rows, prev):
            print('rulingwait: IDLE_NO_PROGRESS pending unchanged and no newer applied/begin (not progress)')
            return 3
        print('rulingwait  watch ok (pending moved or applied/begin advanced)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
