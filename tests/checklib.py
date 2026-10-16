"""Shared helpers for the check scripts (0.0.15 R15-2, third slice).

Before this module 74 scripts carried their own `run()`, 25 their own `call()`
and 14 their own `sha()`; the byte-identical copies now import from here.
    run(args, timeout=60, **kw)  -> CompletedProcess, output captured, args str()-ed
    bounded(*cmd, secs=20)       -> the command under tests/bound, check=True
    sha(path)                    -> sha256 hex of the file's bytes
Import from anywhere in the repository with
    sys.path.insert(0, <repo>/tests); from checklib import run
"""
import hashlib
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]


def run(args, timeout=60, **kw):
    return subprocess.run(list(map(str, args)), capture_output=True, timeout=timeout, **kw)


def bounded(*cmd, secs=20):
    subprocess.run([str(ROOT / 'tests/bound'), str(secs), *map(str, cmd)], check=True)


def sha(p):
    return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()


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
