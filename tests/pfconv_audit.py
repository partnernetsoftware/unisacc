#!/usr/bin/env python3
"""External referee for the pfconv table: the host printf decides what each conversion
letter prints, and the table's routine class must agree with that observable behaviour.

For every letter the table knows, a host-compiled program prints a probe value; the
class the table assigns (int / u32 / hex / HEX / oct / chr / str) is checked against
the host output's shape: decimal with sign, unsigned decimal of -1, lowercase or
uppercase hex, octal, a character, a string. The letters and the expected routine
names come from unisa.gold; nothing here restates the table.
"""
import pathlib, re, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from unisa import gold

def main():
    import inspect
    src = inspect.getsource(gold)
    m = re.search(r'_PF = \{([^}]*)\}', src); pairs = dict(re.findall(r'"(\w)": "(\w+)"', m.group(1)))
    probes = {'d': '-42', 'i': '-42', 'u': '(unsigned)-1', 'x': '255', 'X': '255', 'o': '8', 'p': '(void*)0', 'c': "'Q'", 's': '"str"'}
    prog = ['#include <stdio.h>', 'int main(void){']
    for c in pairs:
        prog.append(f'  printf("{c} [%{c}]\\n", {probes[c]});')
    prog += ['  return 0;', '}']
    with tempfile.TemporaryDirectory(prefix='pfconv-audit-') as td:
        t = pathlib.Path(td); (t / 'p.c').write_text('\n'.join(prog) + '\n')
        subprocess.run(['cc', '-std=c99', '-w', '-o', str(t / 'p'), str(t / 'p.c')], check=True, timeout=30)
        out = subprocess.run([str(ROOT / 'tests/bound'), '10', str(t / 'p')], capture_output=True, text=True, check=True, timeout=15).stdout
    host = dict(re.findall(r'^(\w) \[(.*)\]$', out, re.M))
    shape = {'int': lambda v: v == '-42', 'u32': lambda v: v == '4294967295', 'hex': lambda v: v in ('ff', '0x0', '0', '(nil)') or re.fullmatch(r'0x[0-9a-f]+', v) is not None,
             'HEX': lambda v: v == 'FF', 'oct': lambda v: v == '10', 'chr': lambda v: v == 'Q', 'str': lambda v: v == 'str'}
    bad = [(c, r, host[c]) for c, r in pairs.items() if not shape[r](host[c])]
    print(f'pfconv referee: host printf decided {len(pairs)} conversions, disagreements {len(bad)}')
    for b in bad: print('  DISAGREE', b)
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())
