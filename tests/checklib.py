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
