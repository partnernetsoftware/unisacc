#!/usr/bin/env python3
"""APE build cache: `unisa ape exec/c/compiler.c --via UA -O2 --payload PAYLOAD -o OUT`, keyed by what it read.

usage: apecache.py OUT UA PAYLOAD

The read set comes from strace -f (the APE spawns the UA binary per target, and a Python audit hook cannot see
those children; compilerpack's readtrace would mark the set incomplete).  The job key covers the Python version,
the source, the UA binary's digest and the payload's digest; the index keeps the read-set names (the same index
and stamp scheme as compilerpack.py: _stamp re-reads each name), and a lookup hits only when every stored name
re-stamps equal, with the artifact matching its own manifest.  Paths written during the build (APE temporaries,
the output) are not inputs.  A miss builds, and then stores the artifact under its read-set key.  Nothing is reused
across different bytes: any changed input changes a stamp or the job key.
"""
import fcntl, hashlib, json, os, pathlib, re, shutil, subprocess, sys, tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'exec/c'))
import compilerpack as cp   # _stamp, _digest: the one stamp scheme for read-set keys

OUT, UA, PAYLOAD = sys.argv[1:4]
BASE = pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE', tempfile.gettempdir() + '/unisacc-model-cache'))
OPEN = re.compile(r'^(\d+\s+)?openat\((?:AT_FDCWD|-?\d+),\s*"([^"]+)",\s*([^)]*)\)\s*=\s*(-?\d+)')
JOB = hashlib.sha256(json.dumps(['ape-readset-v1', sys.version, 'exec/c/compiler.c', '-O2',
                                 cp._digest(pathlib.Path(UA)), cp._digest(pathlib.Path(PAYLOAD))]).encode()).hexdigest()


def readkey(stamp):
    return hashlib.sha256(json.dumps([JOB, stamp], sort_keys=True).encode()).hexdigest()


def cache_valid(cache):
    try:
        m = json.loads((cache / 'manifest.json').read_text())
        return set(m) == {'driver.com'} and cp._digest(cache / 'driver.com') == m['driver.com']
    except (FileNotFoundError, ValueError, KeyError):
        return False


def measured_names(log, work):
    """Read-only regular-file opens of the build, absolute; minus anything the build itself wrote."""
    reads, writes = set(), set()
    for line in open(log, errors='replace'):
        m = OPEN.match(line)
        if not m:
            continue
        path, flags, fd = m.group(2), m.group(3), int(m.group(4))
        if fd < 0:
            continue
        p = os.path.normpath(path if os.path.isabs(path) else str(ROOT / path))
        if any(f in flags for f in ('O_WRONLY', 'O_RDWR', 'O_CREAT', 'O_TRUNC')):
            writes.add(p)
        else:
            reads.add(p)
    keep = set()
    for p in reads - writes:
        if p.startswith(('/proc/', '/sys/', '/dev/')) or p == os.path.normpath(PAYLOAD):
            continue
        if p.startswith(str(work)) or p.startswith(str(pathlib.Path(OUT).resolve().parent)):
            continue
        if '/__pycache__/' in p or p.endswith('.pyc'):   # bytecode caches are not inputs (readset.py's rule)
            continue
        keep.add('f:' + p)
    return sorted(keep)


def main():
    BASE.mkdir(parents=True, exist_ok=True)
    index = BASE / ('ape-index-' + JOB + '.json')
    with (BASE / ('ape-index-' + JOB + '.lock')).open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        try:
            seen = json.loads(index.read_text())
        except FileNotFoundError:
            seen = []
        for names in seen:
            stamp = cp._stamp(ROOT, set(names))
            cache = BASE / ('ape-' + readkey(stamp))
            if cache_valid(cache):
                shutil.copyfile(cache / 'driver.com', OUT)
                print('ape-cache: hit %s' % readkey(stamp)[:16])
                return 0
        work = pathlib.Path(tempfile.mkdtemp(prefix='ape-' + JOB[:12] + '.build-', dir=BASE))
        try:
            log = work / 'strace.log'
            cmd = ['strace', '-f', '-qq', '-e', 'trace=openat', '-o', str(log),
                   sys.executable, '-m', 'unisa', 'ape', 'exec/c/compiler.c', '--via', UA, '-O2',
                   '--payload', PAYLOAD, '-o', str(work / 'driver.com')]
            r = subprocess.run(cmd, cwd=str(ROOT), capture_output=True, timeout=55)
            if r.returncode:
                sys.stderr.write(r.stdout.decode(errors='replace')[-400:] + r.stderr.decode(errors='replace')[-400:])
                return r.returncode
            names = measured_names(log, work)
            (work / 'strace.log').unlink()
            stamp = cp._stamp(ROOT, set(names))
            cache = BASE / ('ape-' + readkey(stamp))
            (work / 'manifest.json').write_text(json.dumps({'driver.com': cp._digest(work / 'driver.com')}, sort_keys=True))
            if cache.exists():
                shutil.rmtree(cache)
            work.rename(cache)
            if names not in seen:
                seen.append(names)
                tmp = index.with_suffix('.tmp'); tmp.write_text(json.dumps(seen)); tmp.replace(index)
            shutil.copyfile(cache / 'driver.com', OUT)
            print('ape-cache: miss, built and stored %s (%d read names)' % (readkey(stamp)[:16], len(names)))
            return 0
        finally:
            if work.exists():
                shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
