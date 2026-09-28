#!/usr/bin/env python3
"""Check the classic source/data boundary; product P3 is deliberately separate."""
import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path


def check(root):
    sys.path.insert(0, str(root))
    from unisa import ckernel, gold, intnet
    nets = intnet.load_all(str(root / 'weights/built.json'))
    expected, meta = ckernel._blob(nets)
    aggregate = (root / 'kernel/unisa_model.inc').read_text()
    canonical = (root / 'unisacc.c').read_text()
    if re.search(r'char\s*\*\s*(MODEL|DENSE)\s*=', canonical):
        raise ValueError('root source contains weights instead of includes')
    actual = {}
    for kind, symbol in [('weight', 'MODEL'), ('dense', 'DENSE')]:
        region = re.search(r'char \*' + symbol + r' =\n(.*?);', aggregate, re.S)
        if not region:
            raise ValueError('missing aggregate ' + symbol)
        names = re.findall(r'^#include "([^"]+)"$', region[1], re.M)
        want = [kind + '.' + name + '.inc' for name in gold.ALL]
        if names != want:
            raise ValueError(symbol + ' fragment order/inventory differs')
        chunks = []
        for name in names:
            text = (root / 'kernel' / name).read_text()
            literals = re.findall(r'^  "([^"\n]*)"$', text, re.M)
            if not literals or any(re.sub(r'\\x[0-9a-f]{2}', '', s) for s in literals):
                raise ValueError('invalid literal fragment ' + name)
            chunks.append(bytes.fromhex(''.join(literals).replace('\\x', '')))
        actual[symbol] = b''.join(chunks)
        on_disk = sorted(p.name for p in (root / 'kernel').glob(kind + '.*.inc'))
        if on_disk != sorted(want):
            raise ValueError('orphan or missing ' + kind + ' fragment')
    if actual['MODEL'] != expected:
        raise ValueError('MODEL bytes differ from constructed networks')
    for i, row in enumerate(meta):
        match = re.search(r'STAGE_OFF\[%d\] = (\d+);' % i, aggregate)
        if not match or int(match[1]) != row[4]:
            raise ValueError('MODEL offset changed for ' + row[0])
    dense = bytearray()
    for i, name in enumerate(gold.ALL):
        match = re.search(r'STAGE_DOFF\[%d\] = (\d+);' % i, aggregate)
        if not match or int(match[1]) != len(dense):
            raise ValueError('DENSE offset changed for ' + name)
        net = nets[name]
        indices = {head: {label: i for i, label in enumerate(labels)}
                   for head, labels in net.heads}
        for key in gold.STAGES[name].keys():
            answers = net.predict(key)
            dense.extend(indices[head][answers[head]] for head, _ in net.heads)
    if actual['DENSE'] != dense:
        raise ValueError('DENSE bytes differ from network evaluation')
    before = (root / 'unisacc.c').read_bytes()
    with tempfile.TemporaryDirectory(prefix='unisacc-source-layout-') as folder:
        output = Path(folder) / 'flat.c'
        subprocess.run([str(root / 'tests/bound'), '10',
                        str(root / 'tests/export_ref.sh'), str(output)],
                       check=True, timeout=12)
        flat = output.read_text()
        if re.search(r'^\s*#include\s+"', flat, re.M):
            raise ValueError('independent export still has local includes')
        for name in ('MODEL', 'DENSE'):
            if len(re.findall(r'char \*' + name + r' =', flat)) != 1:
                raise ValueError('independent export duplicates ' + name)
    if (root / 'unisacc.c').read_bytes() != before:
        raise ValueError('export modified canonical source')
    print('source layout: 18 weight + 18 dense fragments; MODEL/DENSE exact; flat export independent')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parent.parent)
    args = parser.parse_args()
    try:
        check(args.root.resolve())
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print('source layout: FAIL ' + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
