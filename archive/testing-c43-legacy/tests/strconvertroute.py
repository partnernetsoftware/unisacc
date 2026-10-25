#!/usr/bin/env python3
"""Check the conversion boundary probe through one actual compiler route.

Run seed, classic and model as separate queue jobs. Classic uses a private
host build of the committed/generated single source; never build_ref.sh.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from strconvertcheck import probe, expected

ROOT = Path(__file__).resolve().parents[1]


def run(command, cwd=ROOT, limit=25):
    result = subprocess.run([sys.executable, str(ROOT/'tests/bound.py'), str(limit),
                             *map(str, command)], cwd=cwd, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'command exited {result.returncode}: {command}\n'
                           + result.stderr.decode(errors='replace'))
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('route', choices=['seed', 'classic', 'model'])
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='unisacc-str-route-') as tmp:
        tmp = Path(tmp)
        source = tmp/'convert.c'
        source.write_text(probe())
        if args.route == 'seed':
            command = [sys.executable, '-m', 'unisa', 'run', source, '--drive', 'built']
        elif args.route == 'classic':
            compiler_source = tmp/'reference.c'
            from sourceflat import export_source
            flat=export_source(tmp/'unisacc-flat.c')
            compiler_source.write_bytes((ROOT/'tests/refshim.h').read_bytes()+flat.read_bytes()+
                                        (ROOT/'tests/reffoot.h').read_bytes())
            run([os.environ.get('CC', 'cc'), '-std=c99', '-w', '-O2',
                 compiler_source, '-o', tmp/'classic'])
            command = [tmp/'classic', '-run', source]
        else:
            compiler = Path(os.environ.get('MODEL_COM', ROOT/'unisacc.com')).resolve()
            prefix = ['/bin/sh', compiler] if compiler.read_bytes()[:2] == b'MZ' else [compiler]
            command = [*prefix, '-run', source]
        output = run(command)
        if output != expected().encode():
            (tmp/'actual.txt').write_bytes(output)
            raise RuntimeError('conversion route differs from independent expected output')
        print(f'strconvert: {args.route} 371 cases and atol/null-end passed')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print('strconvert: FAIL:', error, file=sys.stderr)
        raise SystemExit(1)
