#!/usr/bin/env python3
"""Fresh-process model compile timings, accepted only for exact output images.

The candidate is an APE launched by /bin/sh; the reference is a compiler
with the same CLI (native or APE). No generated program is executed.
Each invocation has a shared 55-second budget; repeat invocations for
long self-compiles instead of requesting many samples. No baseline is inferred.
"""
import argparse
import hashlib
import json
import math
import os
import platform
from pathlib import Path
import signal
import stat
import statistics
import subprocess
import sys
import tempfile
import time


def fingerprint(path):
    if not stat.S_ISREG(path.stat().st_mode):
        raise ValueError(f'expected regular file: {path}')
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def compile_once(launcher, source, target, output, deadline):
    command = launcher + ['-O2', str(source), '-b', target, '-o', str(output)]
    result = {'command': command, 'rc': None, 'elapsed_seconds': 0.0,
              'timeout': False, 'ok': False}
    started = time.monotonic()
    remaining = deadline - started
    if remaining <= 0:
        result['error'] = 'invocation deadline exhausted'
        result['timeout'] = True
        return result
    with tempfile.TemporaryFile() as log:
        proc = None
        try:
            proc = subprocess.Popen(command, stdout=log, stderr=log,
                                    start_new_session=True)
            try:
                result['rc'] = proc.wait(timeout=remaining)
            except subprocess.TimeoutExpired:
                result['timeout'] = True
                result['error'] = 'compile timed out'
        except OSError as error:
            result['error'] = str(error)
        finally:
            # Also remove children left behind by a compiler that already exited.
            if proc is not None:
                try:
                    os.killpg(proc.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                result['rc'] = proc.wait(timeout=1)
            result['elapsed_seconds'] = time.monotonic() - started
        log.seek(0, os.SEEK_END)
        length = log.tell()
        log.seek(max(0, length - 4096))
        result['log_tail'] = log.read().decode('utf-8', errors='replace')
    if result['timeout'] or result['rc'] != 0:
        result.setdefault('error', 'compiler returned nonzero')
        return result
    try:
        size = output.stat().st_size
        if not stat.S_ISREG(output.stat().st_mode) or size == 0:
            raise ValueError('output image is empty or not a regular file')
        result['output_bytes'] = size
        result['output_sha256'] = fingerprint(output)
        result['ok'] = True
    except (OSError, ValueError) as error:
        result['error'] = str(error)
    return result


def main(argv=None):
    started = time.monotonic()
    ap = argparse.ArgumentParser(description=__doc__)
    for flag in ('compiler', 'reference', 'source', 'output'):
        ap.add_argument('--' + flag, type=Path, required=True)
    ap.add_argument('--target', required=True)
    ap.add_argument('--samples', type=int, default=1)
    ap.add_argument('--timeout', type=float, default=55,
                    help='shared reference + sample budget in seconds (0 < n <= 55)')
    ap.add_argument('--max-seconds', type=float,
                    help='explicit per-candidate-sample regression limit')
    args = ap.parse_args(argv)
    if not 1 <= args.samples <= 100:
        ap.error('samples must be 1..100; all samples share the timeout')
    if not math.isfinite(args.timeout) or not 0 < args.timeout <= 55:
        ap.error('timeout must be finite and in (0, 55]')
    if args.max_seconds is not None and (not math.isfinite(args.max_seconds)
                                         or args.max_seconds <= 0):
        ap.error('max-seconds must be finite and positive')
    # Reject output/input aliasing before any missing-input failure can write JSON.
    output = args.output.resolve()
    if output in {getattr(args, name).resolve() for name in ('source', 'compiler', 'reference')}:
        ap.error('JSON output must not overwrite an input')
    report = {'ok': False, 'target': args.target, 'optimization': '-O2',
              'samples_requested': args.samples, 'samples': [],
              'timeout_seconds': args.timeout, 'max_seconds': args.max_seconds,
              'host': platform.platform(),
              'environment': {'UNISA_MAXSTEPS': os.environ.get('UNISA_MAXSTEPS')}}
    try:
        paths = {name: getattr(args, name).resolve(strict=True)
                 for name in ('source', 'compiler', 'reference')}
        output = args.output.resolve()
        if output in paths.values():
            ap.error('JSON output must not overwrite an input')
        report['inputs'] = {name: {'path': str(path), 'sha256': fingerprint(path)}
                            for name, path in paths.items()}
        reference = paths['reference']
        with reference.open('rb') as f:
            ape = f.read(2) == b'MZ'
        ref_launcher = ['/bin/sh', str(reference)] if ape else [str(reference)]
        deadline = started + args.timeout
        with tempfile.TemporaryDirectory(prefix='unisacc-modelbench-') as temp:
            temp = Path(temp)
            ref_output = temp / 'reference.bin'
            ref = compile_once(ref_launcher, paths['source'], args.target,
                               ref_output, deadline)
            report['reference'] = ref
            if not ref['ok']:
                raise ValueError('reference compile failed')
            expected = ref_output.read_bytes()
            for index in range(args.samples):
                image = temp / f'sample-{index + 1}.bin'
                sample = compile_once(['/bin/sh', str(paths['compiler'])],
                                      paths['source'], args.target, image, deadline)
                sample['sample'] = index + 1
                report['samples'].append(sample)
                if not sample['ok']:
                    raise ValueError(f'sample {index + 1}: compile failed')
                sample['equal_reference'] = image.read_bytes() == expected
                if not sample['equal_reference']:
                    sample['ok'] = False
                    sample['error'] = 'output differs from reference'
                    raise ValueError(f'sample {index + 1}: output differs')
                if args.max_seconds is not None and sample['elapsed_seconds'] > args.max_seconds:
                    sample['ok'] = False
                    sample['error'] = 'explicit performance limit exceeded'
                    raise ValueError(f'sample {index + 1}: performance regression')
        for name, path in paths.items():
            if fingerprint(path) != report['inputs'][name]['sha256']:
                raise ValueError(f'input changed during measurement: {name}')
        report['median_seconds'] = statistics.median(s['elapsed_seconds'] for s in report['samples'])
        report['ok'] = True
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        report['error'] = str(error)
    report['total_elapsed_seconds'] = time.monotonic() - started
    try:
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    except OSError as error:
        print(f'modelbench: cannot write report: {error}', file=sys.stderr)
        return 1
    if not report['ok']:
        print('modelbench: ' + report['error'], file=sys.stderr)
    return 0 if report['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
