#!/usr/bin/env python3
"""Private compiler doubles: timings cannot turn functional failures green."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

BENCH = Path(__file__).with_name('modelbench.py')


def main():
    with tempfile.TemporaryDirectory(prefix='unisacc-modelbench-check-') as directory:
        root = Path(directory)
        source = root / 'probe.c'
        source.write_text('int main(void) { return 0; }\n')
        prelude = '#!/bin/sh\nset -eu\nwhile [ "$1" != "-o" ]; do shift; done\nshift\n'
        bodies = {
            'normal': 'printf image > "$1"\n',
            'nonzero': 'printf image > "$1"\necho success\nexit 2\n',
            'missing': 'echo success\n',
            'empty': ': > "$1"\n',
            'unequal': 'printf other > "$1"\n',
            'slow': 'sleep 30 &\nwait\n',
        }
        for name, body in bodies.items():
            path = root / name
            path.write_text(prelude + body)
            path.chmod(0o700)
        environment = {k: v for k, v in os.environ.items() if k not in ('UNISA_MAXSTEPS', 'UNISA_CONTAINER', 'UNISA_KERNEL')}
        cases = [(name, 'normal', [], name == 'normal') for name in bodies]
        cases += [('normal', 'normal', ['--samples', '2'], True),
                  ('normal', 'normal', ['--max-seconds', '0.000001'], False),
                  ('normal', 'nonzero', [], False),
                  ('normal', 'slow', [], False)]
        for index, (name, referee, extra, want) in enumerate(cases):
            output = root / f'result-{index}.json'
            command = [sys.executable, str(BENCH), '--compiler', str(root / name),
                       '--reference', str(root / referee), '--source', str(source),
                       '--target', 'lnx/x86_64', '--output', str(output),
                       '--timeout', '0.15' if 'slow' in (name, referee) else '3'] + extra
            run = subprocess.run(command, capture_output=True, timeout=5, env=environment)
            report = json.loads(output.read_text())
            assert (run.returncode == 0) == want, (name, run.stderr)
            assert report['ok'] == want, (name, report)
            assert report['inputs']['source']['sha256'], report
            if name == 'nonzero':
                assert report['samples'][0]['rc'] == 2, report
            if name == 'slow':
                assert report['samples'][0]['timeout'], report
            if want:
                assert all(s['equal_reference'] and s['output_bytes'] > 0
                           for s in report['samples']), report
            print(f'modelbenchcheck candidate={name} reference={referee}: passed', flush=True)
        package = root / 'external.pkg'; package.write_bytes(b'package')
        kernel = root / 'external.core'; kernel.write_bytes(b'kernel')
        external_env = dict(environment, UNISA_CONTAINER=str(package),
                            UNISA_KERNEL=str(kernel), UNISA_MAXSTEPS='123')
        command = [sys.executable, str(BENCH), '--compiler', str(root / 'normal'),
                   '--reference', str(root / 'normal'), '--source', str(source),
                   '--target', 'lnx/x86_64', '--output', str(root / 'external.json')]
        run = subprocess.run(command, capture_output=True, timeout=5, env=external_env)
        report = json.loads((root / 'external.json').read_text())
        assert run.returncode == 0 and report['ok']
        assert report['environment']['UNISA_MAXSTEPS'] == '123'
        for key in ('UNISA_CONTAINER', 'UNISA_KERNEL'):
            assert report['inputs'][key]['sha256'] and report['inputs'][key]['path'] == str(Path(external_env[key]).resolve())
        mutator = root / 'mutator'
        mutator.write_text(prelude + 'printf changed > "$UNISA_CONTAINER"\nprintf image > "$1"\n')
        mutator.chmod(0o700)
        mutated = command.copy(); mutated[mutated.index('--compiler') + 1] = str(mutator)
        run = subprocess.run(mutated, capture_output=True, timeout=5, env=external_env)
        assert run.returncode != 0 and 'input changed' in run.stderr.decode()
        external_env['UNISA_KERNEL'] = str(root / 'absent-kernel')
        run = subprocess.run(command, capture_output=True, timeout=5, env=external_env)
        assert run.returncode != 0
        print('modelbenchcheck external identity/change/missing: passed', flush=True)
        # A missing input must not permit the JSON report to overwrite another input.
        before = source.read_bytes()
        run = subprocess.run([sys.executable, str(BENCH), '--compiler', str(root / 'normal'),
                              '--reference', str(root / 'normal'), '--source', str(root / 'absent'),
                              '--target', 'lnx/x86_64', '--output', str(root / 'normal')],
                             capture_output=True, timeout=5)
        assert run.returncode != 0 and (root / 'normal').read_text() == prelude + bodies['normal']
        assert source.read_bytes() == before
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
