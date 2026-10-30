#!/usr/bin/env python3
"""Pinned private static libffi provider (Darwin arm64/x86_64 only).
Supply the official 3.5.2 release tarball explicitly; no download or global
install occurs. Reinvoke the same command three times: each invocation runs
only the next configure/make/install step, with a 50-second process-group
watchdog. A timed-out/failed step remains pending. --step selects one step.
The final manifest describes inputs and output hashes, not ABI qualification
or bit-for-bit reproducibility across different toolchains/output paths.
"""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import shlex
import shutil
import signal
import subprocess
import tarfile
import time

VERSION = '3.5.2'
TAR_SHA = 'f3a3082a23b37c293a4fcd1053147b371f2ff91fa7ea1b2a52e335676bac82dc'
PROFILES = {'osx/arm64': 'aarch64-apple-darwin',
            'osx/x86_64': 'x86_64-apple-darwin'}
STEPS = ('configure', 'make', 'install')
ARTIFACTS = ('lib/libffi.a', 'include/ffi.h', 'include/ffitarget.h',
             'include/ffi/ffi.h', 'include/ffi/ffitarget.h', 'LICENSE')


def sha(p):
    with p.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def write_json(p, value):
    temporary = p.with_suffix(p.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
    os.replace(temporary, p)


def file_hashes(root):
    return {str(p.relative_to(root)): sha(p) for p in sorted(root.rglob('*'))
            if p.is_file()}


def tool(command):
    args = shlex.split(command)
    if not args:
        raise ValueError('empty tool command')
    binary = shutil.which(args[0])
    if not binary:
        raise ValueError('tool not found: ' + args[0])
    return [str(Path(binary).resolve()), *args[1:]]


def command(args, cwd, env, seconds=50):
    started = time.monotonic()
    proc = subprocess.Popen(args, cwd=cwd, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, start_new_session=True)
    timed_out = False
    try:
        stdout, stderr = proc.communicate(timeout=seconds)
    except subprocess.TimeoutExpired:
        timed_out = True
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        stdout, stderr = proc.communicate()
    return {'argv': args, 'cwd': str(cwd), 'environment': env,
            'limit_seconds': seconds, 'seconds': time.monotonic() - started,
            'rc': 124 if timed_out else proc.returncode,
            'timed_out': timed_out,
            'stdout': stdout.decode(errors='replace'),
            'stderr': stderr.decode(errors='replace')}


def prepare_source(tarball, source):
    # The digest is checked before any archive member is read/extracted.
    if sha(tarball) != TAR_SHA:
        raise ValueError('official libffi 3.5.2 tarball SHA256 mismatch')
    with tarfile.open(tarball, 'r:gz') as archive:
        members = archive.getmembers()
        for m in members:
            path = PurePosixPath(m.name)
            if (path.is_absolute() or '..' in path.parts or not path.parts
                    or path.parts[0] != 'libffi-' + VERSION
                    or not (m.isfile() or m.isdir())):
                raise ValueError('unexpected release archive member: ' + m.name)
        archive.extractall(source.parent, members=members, filter='data')


def build(a):
    if platform.system() != 'Darwin' or a.target not in PROFILES:
        raise ValueError('implemented profiles require Darwin: osx/arm64 or osx/x86_64')
    host_arch = {'arm64': 'arm64', 'aarch64': 'arm64', 'x86_64': 'x86_64'}.get(platform.machine())
    if host_arch is None:
        raise ValueError('unsupported Darwin host architecture')
    tarball = a.tarball.resolve(strict=True)
    if sha(tarball) != TAR_SHA:
        raise ValueError('official libffi 3.5.2 tarball SHA256 mismatch')
    output = a.output.resolve()
    repo = Path(__file__).resolve().parents[2]
    forbidden = (repo, Path('/usr'), Path('/opt'), Path('/System'),
                 Path('/Library'), Path('/bin'), Path('/sbin'))
    if output == Path('/') or any(output == p or p in output.parents for p in forbidden):
        raise ValueError('output must be a private directory outside repository/system prefixes')
    cc, make = tool(a.cc), tool(a.make)
    config = {'target': a.target, 'version': VERSION, 'source_tar_sha256': TAR_SHA,
              'compiler': cc, 'make': make, 'jobs': a.jobs,
              'host_triplet': PROFILES['osx/' + host_arch]}
    try:
        output.stat()
    except FileNotFoundError:
        output.mkdir(parents=True)
    state_path = output / 'build-state.json'
    try:
        state_path.stat()
    except FileNotFoundError:
        if list(output.iterdir()):
            raise ValueError('new output must be empty')
    with (output / '.build.lock').open('a') as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise ValueError('provider output is already being built')
        source = output / 'source' / ('libffi-' + VERSION)
        builddir = output / 'build'
        env = {k: os.environ[k] for k in ('PATH', 'HOME', 'TMPDIR') if k in os.environ}
        arch = a.target.split('/')[1]
        env.update({'LC_ALL': 'C', 'LANG': 'C', 'CC': shlex.join(cc + ['-arch', arch]),
                    'CFLAGS': '-O2 -fPIC -arch ' + arch, 'LDFLAGS': '-arch ' + arch,
                    'ZERO_AR_DATE': '1', 'SOURCE_DATE_EPOCH': '1754092800',
                    'CONFIG_SHELL': '/bin/sh'})
        try:
            state = json.loads(state_path.read_text())
        except FileNotFoundError:
            source.parent.mkdir()
            prepare_source(tarball, source)
            builddir.mkdir()
            state = {'schema': 1, 'config': config, 'tarball_path': str(tarball),
                     'source_files_sha256': file_hashes(source),
                     'builder_sha256': sha(Path(__file__)), 'commands': [], 'completed': []}
            write_json(state_path, state)
        if state['config'] != config or state['builder_sha256'] != sha(Path(__file__)):
            raise ValueError('resume configuration or builder changed; use a fresh output')
        if state['source_files_sha256'] != file_hashes(source):
            raise ValueError('verified source tree changed; use a fresh output')
        if 'install' in state['completed']:
            manifest = json.loads((output / 'manifest.json').read_text())
            for name, facts in manifest['artifacts'].items():
                if sha(output / name) != facts['sha256']:
                    raise ValueError('completed provider artifact changed: ' + name)
            return manifest
        pending = next(s for s in STEPS if s not in state['completed'])
        step = pending if a.step == 'next' else a.step
        if step != pending:
            raise ValueError('next required step is ' + pending)
        args = {'configure': [str(source / 'configure'), '--host=' + PROFILES[a.target],
                              '--build=' + config['host_triplet'], '--disable-shared',
                              '--enable-static', '--disable-docs', '--disable-multi-os-directory',
                              '--prefix=' + str(output)],
                'make': make + ['-j' + str(a.jobs)],
                'install': make + ['install']}[step]
        event = command(args, builddir, env)
        event['stage'] = step
        state['commands'].append(event)
        if event['rc']:
            write_json(state_path, state)
            raise RuntimeError(step + ' failed; rc=' + str(event['rc']) + '; see ' + str(state_path))
        if step == 'install':
            mirror = output / 'include' / 'ffi'
            mirror.mkdir(exist_ok=True)
            for header in ('ffi.h', 'ffitarget.h'):
                shutil.copyfile(output / 'include' / header, mirror / header)
            shutil.copyfile(source / 'LICENSE', output / 'LICENSE')
            shutil.copyfile(source / 'LICENSE-BUILDTOOLS', output / 'LICENSE-BUILDTOOLS.libffi')
            artifacts = {name: {'sha256': sha(output / name), 'bytes': (output / name).stat().st_size}
                         for name in ARTIFACTS}
            manifest = {'schema': 1, 'target': a.target, 'version': VERSION,
                        'source_tar_sha256': TAR_SHA,
                        'source_url': 'https://github.com/libffi/libffi/releases/download/v3.5.2/libffi-3.5.2.tar.gz',
                        'license': {'name': 'MIT', 'path': 'LICENSE',
                                    'sha256': sha(output / 'LICENSE'),
                                    'buildtools_path': 'LICENSE-BUILDTOOLS.libffi',
                                    'buildtools_sha256': sha(output / 'LICENSE-BUILDTOOLS.libffi')},
                        'artifacts': artifacts, 'commands': state['commands'],
                        'source_files_sha256': state['source_files_sha256'],
                        'builder_sha256': state['builder_sha256'],
                        'scope': 'private static Darwin provider; ABI qualification is separate; Windows unsupported'}
            write_json(output / 'manifest.json', manifest)
        state['completed'].append(step)
        write_json(state_path, state)
        return {'schema': 1, 'target': a.target, 'completed': state['completed'],
                'next_step': next((s for s in STEPS if s not in state['completed']), None),
                'manifest': str(output / 'manifest.json') if step == 'install' else None}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--tarball', required=True, type=Path)
    ap.add_argument('--output', required=True, type=Path)
    ap.add_argument('--target', required=True, choices=sorted(PROFILES))
    ap.add_argument('--step', choices=('next', *STEPS), default='next')
    ap.add_argument('--cc', default='cc')
    ap.add_argument('--make', default='make')
    ap.add_argument('--jobs', type=int, choices=range(1, 5), default=2)
    a = ap.parse_args()
    try:
        print(json.dumps(build(a), sort_keys=True))
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError, tarfile.TarError) as e:
        ap.exit(1, 'buildffiprovider: ' + str(e) + '\n')


if __name__ == '__main__':
    main()
