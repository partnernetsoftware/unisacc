#!/usr/bin/env python3
"""External referee for the binsel table: the host C compiler decides which C binary
operators depend on signedness on 64-bit operands (C99 6.5.5-6.5.14).

For every (operator, signedness) key of unisa.gold's binsel stage, a C program built
with the host cc evaluates the operator on operand values chosen so that the signed
and unsigned interpretations differ whenever the standard says they can. The table
must name an unsigned flavour exactly for the keys where the host's unsigned result
differs from its signed result. 32 keys, all decided by the host, none by this script.
"""
import pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from unisa.gold import BINSEL_OPS, binsel_label, _BIN_U

def main():
    ops = list(BINSEL_OPS)
    # operands: a negative-when-signed left operand and a small positive right operand
    prog = ['#include <stdio.h>', 'int main(void){', '  long long a=-7, b=3; unsigned long long ua=(unsigned long long)-7, ub=3;']
    for i, op in enumerate(ops):
        sh = 'b' if op not in ('<<', '>>') else '3'
        prog.append(f'  printf("%d %lld %llu\\n", {i}, (long long)(a {op} {sh if op in ("<<",">>") else "b"}), (unsigned long long)(ua {op} {sh if op in ("<<",">>") else "ub"}));')
    prog += ['  return 0;', '}']
    with tempfile.TemporaryDirectory(prefix='binsel-audit-') as td:
        t = pathlib.Path(td); (t / 'p.c').write_text('\n'.join(prog) + '\n')
        subprocess.run(['cc', '-std=c99', '-w', '-o', str(t / 'p'), str(t / 'p.c')], check=True, timeout=30)
        out = subprocess.run([str(ROOT / 'tests/bound'), '10', str(t / 'p')], capture_output=True, text=True, check=True, timeout=15).stdout
    host = {}
    for line in out.splitlines():
        i, s, u = line.split(); host[ops[int(i)]] = (int(s), int(u))
    bad = []; checked = 0
    for op in ops:
        s, u = host[op]
        # signed value reinterpreted as unsigned 64-bit for a fair comparison
        differs = (s % (1 << 64)) != u
        for sign in ('s', 'u'):
            flav = binsel_label(op, sign); checked += 1
            table_unsigned = flav in _BIN_U.values()
            if sign == 's' and table_unsigned: bad.append((op, sign, flav, 'signed key mapped to an unsigned flavour'))
            if sign == 'u' and table_unsigned != differs: bad.append((op, sign, flav, f'host says signedness {"matters" if differs else "does not matter"}'))
    print(f'binsel referee: host cc decided {len(ops)} operators, {checked} keys checked, '
          f'{sum(1 for op in ops if (host[op][0] % (1<<64)) != host[op][1])} operators where signedness matters, disagreements {len(bad)}')
    for b in bad: print('  DISAGREE', b)
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())
