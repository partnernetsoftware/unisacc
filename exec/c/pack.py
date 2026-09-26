#!/usr/bin/env python3
"""Construction-time package writer; the C runtime reads it without Python.
Manifest rows: route TAB stage TAB input-format TAB output-format TAB model.
Model paths are relative to their manifest. Equal network bytes are stored once.
"""
import argparse
import pathlib
import re

NAME = re.compile(r'[A-Za-z0-9_./-]+\Z')


def build(manifests):
    stages, models, index, seen, last = [], [], {}, set(), {}
    for manifest in map(pathlib.Path, manifests):
        for line, text in enumerate(manifest.read_text().splitlines(), 1):
            if not text or text.startswith('#'):
                continue
            cols = text.split('\t')
            if len(cols) != 5 or not all(NAME.fullmatch(x) for x in cols[:4]):
                raise ValueError(f'{manifest}:{line}: expected five columns and valid names')
            route, stage, inp, out, path = cols
            if (route, stage) in seen:
                raise ValueError(f'{manifest}:{line}: duplicate stage {route}/{stage}')
            if route in last and last[route] != inp:
                raise ValueError(f'{manifest}:{line}: format mismatch {last[route]} -> {inp}')
            data = (manifest.parent / path).read_bytes()
            if not data.startswith(b'N ') or not data.endswith(b'\n'):
                raise ValueError(f'{manifest}:{line}: expected canonical network text')
            if data not in index:
                index[data] = len(models)
                models.append(data)
            stages.append((route, stage, inp, out, index[data]))
            seen.add((route, stage)); last[route] = out
    if not stages:
        raise ValueError('empty package')
    head = f'P 1 {len(models)} {len(stages)}\n'
    head += ''.join('D ' + ' '.join(map(str, row)) + '\n' for row in stages)
    result = head.encode('ascii') + b''.join(f'M {len(b)}\n'.encode() + b for b in models)
    if len(result) >= 2**31:
        raise ValueError('package exceeds runtime byte extent')
    return result


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('-o', '--output', required=True, type=pathlib.Path)
    ap.add_argument('manifests', nargs='+', type=pathlib.Path)
    args = ap.parse_args()
    try:
        data = build(args.manifests)
        args.output.write_bytes(data)
    except (OSError, ValueError) as exc:
        ap.exit(1, f'pack: {exc}\n')
    print(f'package: {len(data)} bytes -> {args.output}')
