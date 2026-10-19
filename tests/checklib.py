"""Shared helpers for the check scripts (0.0.15 R15-2, third slice).

Before this module 74 scripts carried their own `run()`, 25 their own `call()`
and 14 their own `sha()`; the byte-identical copies now import from here.
    run(args, timeout=60, **kw)  -> CompletedProcess, output captured, args str()-ed
    notcovered(r)                -> r is the standard `not covered` rejection (predicate)
    bounded(*cmd, secs=20)       -> the command under tests/bound, check=True
    sha(path)                    -> sha256 hex of the file's bytes
Import from anywhere in the repository with
    sys.path.insert(0, <repo>/tests); from checklib import run
"""
import hashlib
import pathlib
import sys
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]


def run(args, timeout=60, **kw):
    return subprocess.run(list(map(str, args)), capture_output=True, timeout=timeout, **kw)


def notcovered(r):
    """The declared-input-boundary reply: exit 1, empty stdout, 'not covered' on stderr."""
    return r.returncode == 1 and not r.stdout and b'not covered' in r.stderr


def bounded(*cmd, secs=20):
    subprocess.run([str(ROOT / 'tests/bound'), str(secs), *map(str, cmd)], check=True)


def bound_cmd(secs, *argv):
    """argv for tests/bound.py: [python, bound.py, secs, *argv] (no -B). Shape shared by csmithdiff, declmatrix, declshape, gateaudit, gatelayers."""
    return [sys.executable, str(ROOT / 'tests/bound.py'), str(secs), *argv]   # argv passed through unchanged: element types as at the call site


def sha(p):
    return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()


def verify_cached_file(path, expected_sha256, *, executable=False):
    """Shared require-cached artifact invariant; callers supply their named refusal."""
    import os
    import re
    import stat
    path = pathlib.Path(path)
    info = path.lstat()
    if not stat.S_ISREG(info.st_mode) or info.st_size == 0:
        raise ValueError('cached artifact is not a nonempty regular file: ' + str(path))
    if not isinstance(expected_sha256, str) or not re.fullmatch(r'[0-9a-f]{64}', expected_sha256):
        raise ValueError('cached artifact has invalid digest: ' + str(path))
    if executable and (not info.st_mode & 0o111 or not os.access(path, os.X_OK)):
        raise ValueError('cached artifact is not executable: ' + str(path))
    if sha(path) != expected_sha256:
        raise ValueError('cached artifact SHA mismatch: ' + str(path))
    return path


def parse_cc_make_dependencies(text):
    """Parse the supported cc -M prerequisite spelling; reject ambiguous syntax."""
    names = []
    word = []
    i = 0
    while i < len(text):
        c = text[i]
        if c in "\"'":
            raise ValueError('unsupported quote in cc Make dependencies')
        if c == '\\':
            if i + 1 == len(text):
                raise ValueError('unpaired escape in cc Make dependencies')
            following = text[i + 1]
            if following == '\n':
                # A Make continuation is whitespace, not token concatenation.
                if word:
                    names.append(''.join(word)); word = []
            elif following in (' ', '\t', '#'):
                word.append(following)
            else:
                raise ValueError('unsupported escape in cc Make dependencies')
            i += 2
            continue
        if c == '$':
            if i + 1 >= len(text) or text[i + 1] != '$':
                raise ValueError('unsupported dollar expression in cc Make dependencies')
            word.append('$'); i += 2; continue
        if c == '#' or c == ':' or c == '\r' or c == '\x00':
            raise ValueError('unsupported syntax in cc Make dependencies')
        if c.isspace():
            if word:
                names.append(''.join(word)); word = []
        else:
            word.append(c)
        i += 1
    if word:
        names.append(''.join(word))
    return names


def cc_read_set_identity(compiler, sources, flags=(), *, cwd=None, env=None, timeout=55):
    """Ask cc for each translation unit's full (including system) header read set.

    Dependency-only preprocessing builds no object or executable. Preserve the
    caller's actual compiler flags/environment; return paths plus content SHA.
    Cache users must recompute this before accepting a prepared artifact.
    """
    base = pathlib.Path(cwd or pathlib.Path.cwd()).resolve()
    compiler = str(pathlib.Path(compiler).resolve(strict=True))
    files = {}
    commands = []
    for source in sources:
        command = [compiler, *map(str, flags), '-M', '-MT', '__checklib_readset__', str(source)]
        result = subprocess.run(command, cwd=base, env=env, capture_output=True,
                                text=True, timeout=timeout)
        commands.append(command)
        if result.returncode:
            raise ValueError('cc dependency scan failed rc=' + str(result.returncode) + ': ' + result.stderr[-1000:])
        text = result.stdout
        prefix = '__checklib_readset__:'
        if not text.startswith(prefix):
            raise ValueError('cc dependency scan has no expected target')
        names = parse_cc_make_dependencies(text[len(prefix):])
        if not names:
            raise ValueError('cc dependency scan returned an empty read set')
        resolved_source = (base / source).resolve(strict=True)
        seen = set()
        for name in names:
            path = (base / name).absolute()
            real = path.resolve(strict=True)
            info = real.stat()
            if not real.is_file():
                raise ValueError('cc dependency is not a regular file: ' + str(real))
            files[str(path)] = {'resolved': str(real), 'sha256': sha(real)}
            seen.add(real)
        if resolved_source not in seen:
            raise ValueError('cc dependency scan omitted its source: ' + str(source))
    if not files:
        raise ValueError('cc dependency scan has no translation units')
    return {'schema': 'cc-header-readset-v1', 'compiler': compiler,
            'compiler_sha256': sha(compiler), 'commands': commands,
            'files': dict(sorted(files.items()))}


def copy_runtime(root, destination):
    """Copy runtime sources and their local quoted-include closure, preserving paths.

    Seed all runtime translation units/headers (including asm), then follow
    quoted includes. System headers remain the host compiler's responsibility.
    Return the exact copied input hashes for fixture provenance.
    """
    import re
    import shutil
    root = pathlib.Path(root).resolve(strict=True)
    destination = pathlib.Path(destination)
    pending = sorted(p for p in (root / 'exec/c').rglob('*')
                     if p.is_file() and p.suffix in ('.c', '.h', '.S'))
    copied = {}
    while pending:
        source = pending.pop()
        relative = source.relative_to(root).as_posix()
        if relative in copied:
            continue
        data = source.read_bytes()
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        copied[relative] = hashlib.sha256(data).hexdigest()
        for name in re.findall(rb'^\s*#\s*include\s*"([^"\n]+)"', data, re.M):
            include = (source.parent / name.decode()).resolve()
            include.relative_to(root)  # No input may escape the source root.
            try:
                include.stat()
            except FileNotFoundError:
                raise FileNotFoundError('runtime include missing: ' + str(include))
            pending.append(include)
    return dict(sorted(copied.items()))
