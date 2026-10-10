#!/usr/bin/env python3
"""rulingwait.py [LEDGER] [--now ISO] (0.0.39 WF4): read release/ruling-requests.tsv and report, per request,
request->decision and decision->applied wall-clock waits; PENDING rows report their age.  Exits 1 on a
malformed row (APPLIED without applied_at/ref, times out of order) so the ledger cannot fake progress.
Waiting is wall clock, never counted as CPU waste or as progress."""
import datetime, pathlib, sys
ROOT = pathlib.Path(__file__).resolve().parents[2]


def parse(path):
    rows = []
    for line in pathlib.Path(path).read_text().splitlines():
        if not line.strip() or line.startswith('#'): continue
        f = (line.split('\t') + [''] * 8)[:8]
        rows.append(dict(zip(('id', 'requested_at', 'decided_at', 'applied_at', 'state', 'decided_by', 'ref', 'subject'), f)))
    return rows


def t(s): return datetime.datetime.fromisoformat(s) if s and s != 'UNKNOWN' else None


def check(rows):
    bad = []
    for r in rows:
        req, dec, app = t(r['requested_at']), t(r['decided_at']), t(r['applied_at'])
        if r['state'] not in ('PENDING', 'DECIDED', 'APPLIED') or not req: bad.append((r['id'], 'state/requested_at'))
        if r['state'] == 'PENDING' and (dec or app): bad.append((r['id'], 'PENDING with decision/applied time'))
        if r['state'] in ('DECIDED', 'APPLIED') and not ((dec or r['decided_at'] == 'UNKNOWN') and r['decided_by']): bad.append((r['id'], 'decision without time/decider'))
        if r['state'] == 'APPLIED' and not (app and r['ref']): bad.append((r['id'], 'APPLIED without applied_at/ref'))
        if dec and req and dec < req or app and dec and app < dec: bad.append((r['id'], 'times out of order'))
    return bad


def main(argv=None):
    a = argv if argv is not None else sys.argv[1:]
    now = t(a[a.index('--now') + 1]) if '--now' in a else datetime.datetime.now().astimezone()
    path = next((x for x in a if not x.startswith('--') and (a.index(x) == 0 or a[a.index(x) - 1] != '--now')), ROOT / 'release/ruling-requests.tsv')
    rows = parse(path); bad = check(rows)
    for i, why in bad: print('rulingwait: MALFORMED %s: %s' % (i, why))
    m = lambda d: '%6.1f min' % (d.total_seconds() / 60)
    for r in rows:
        req, dec, app = t(r['requested_at']), t(r['decided_at']), t(r['applied_at'])
        if r['state'] == 'PENDING': print('  %-3s PENDING  waiting %s  %s' % (r['id'], m(now - req), r['subject']))
        elif dec and req: print('  %-3s %-8s decided after %s%s  by %s  %s' % (r['id'], r['state'], m(dec - req), ', applied after ' + m(app - dec) if app else '', r['decided_by'], r['subject']))
        elif r['decided_at'] == 'UNKNOWN': print('  %-3s %-8s decision time UNKNOWN%s  by %s  %s' % (r['id'], r['state'], ', applied ' + r['applied_at'] if app else '', r['decided_by'], r['subject']))
        else: print('  %-3s %-8s MALFORMED  %s' % (r['id'], r['state'], r['subject']))
    pend = [r['id'] for r in rows if r['state'] == 'PENDING']
    print('rulingwait  %d requests  pending %d (%s)' % (len(rows), len(pend), ' '.join(pend) or '-'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
