#!/usr/bin/env python3
"""Merge comdemo.py records from several machines into one matrix and judge it.

usage: comdemo_matrix.py --out MATRIX.json RESULT.json...
Judgement: every record passed on its own machine; every deterministic program printed
identical bytes on every machine (stdout sha256); all records ran the same candidate bytes.
Prints a table (program x machine) and exits 1 on any disagreement.
"""
import argparse, json, pathlib, sys

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', type=pathlib.Path, required=True)
    ap.add_argument('results', nargs='+', type=pathlib.Path)
    a = ap.parse_args()
    recs = [json.loads(p.read_text()) for p in a.results]
    labels = [r['label'] or f"{r['host']['system']}/{r['host']['machine']}" for r in recs]
    problems = []
    shas = {r['com_sha256'] for r in recs}
    if len(shas) != 1: problems.append(f'different candidate bytes across machines: {sorted(shas)}')
    for lab, r in zip(labels, recs):
        if r['status'] != 'passed': problems.append(f'{lab}: failed {r["failures"]}')
    programs = sorted({n for r in recs for n in r['programs']})
    table = {}
    for n in programs:
        cells = {lab: r['programs'].get(n) for lab, r in zip(labels, recs)}
        table[n] = {lab: (None if c is None else {'rc': c['rc'], 'sha': c['stdout_sha256'][:12], 'ok': c['ok'], 'seconds': c['seconds'], 'reference': c.get('reference'), 'unsupported': c.get('unsupported')}) for lab, c in cells.items()}
        det = [c for c in cells.values() if c and c['kind'] == 'deterministic']
        if det and len({c['stdout_sha256'] for c in det}) != 1:
            problems.append(f'{n}: deterministic output differs across machines')
    matrix = {'schema': 1, 'machines': labels, 'candidate_sha256': sorted(shas), 'programs': table, 'problems': problems,
              'status': 'passed' if not problems else 'failed'}
    a.out.write_text(json.dumps(matrix, indent=1))
    w = max(len(n) for n in programs) if programs else 8
    print('program'.ljust(w), *[l[:14].ljust(14) for l in labels])
    for n in programs:
        print(n.ljust(w), *[('-' if c is None else ('n/a ' if c.get('unsupported') else ('ok ' if c['ok'] else 'FAIL')) + ' ' + c['sha'][:6] + (' *' if c['reference'] == 'match' else '  ')).ljust(14) for c in table[n].values()])
    print(matrix['status'], *problems, sep='\n' if problems else ' ')
    return 0 if not problems else 1

if __name__ == '__main__':
    sys.exit(main())
