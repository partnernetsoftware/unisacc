#!/usr/bin/env python3
"""Compile-reject runner: each tests/reject/*.c must be refused by the compiler under test.

A case passes when the compiler exits non-zero, prints a diagnostic on stderr, and produces no
output file.  The system C compiler is run on the same file as the reference: it must also refuse
it, so a case cannot pass because the reference itself accepts the program.  Usage:
    python3 tests/rejectcheck.py --package ./unisacc.com [--case NAME ...]
"""
import argparse, glob, os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
CASES = os.path.join(HERE, 'reject')


def refused_by_compiler(pkg, src, out):
    # the package is an APE image: the repo's own tests run it as `sh PACKAGE`, not by exec
    # an APE image runs through the shell (the repo's own tests use `sh PACKAGE`); a source-built ELF runs directly
    with open(pkg, 'rb') as fh:
        elf = fh.read(4) == b'\x7fELF'
    cmd = [pkg] if elf else ['sh', pkg]
    p = subprocess.run(cmd + [src, '-o', out], capture_output=True, timeout=30)
    # a refusal is a normal exit with a positive status: a signal death (negative returncode) is not a refusal
    return p.returncode > 0 and bool(p.stderr) and not os.path.exists(out)


def refused_by_reference(src, out):
    p = subprocess.run(['cc', '-std=c99', '-pedantic-errors', '-o', out, src], capture_output=True, timeout=30)
    return p.returncode != 0


def selftest():
    """controls: a compiler that kills itself after writing a diagnostic must not count as a refusal"""
    with tempfile.TemporaryDirectory() as td:
        fake = os.path.join(td, 'fake.sh')
        with open(fake, 'w') as fh:
            fh.write('#!/bin/sh\necho "fake: refused" >&2\nkill -TERM $$\n')
        os.chmod(fake, 0o755)
        src = os.path.join(td, 'x.c')
        open(src, 'w').write('int main(void){return 0;}\n')
        out = os.path.join(td, 'x.bin')
        if refused_by_compiler(fake, src, out):
            print('selftest FAIL: signal death counted as refusal'); return 1
    print('selftest ok: signal death is not a refusal')
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--package')
    ap.add_argument('--case', action='append', default=[])
    ap.add_argument('--selftest', action='store_true')
    a = ap.parse_args()
    if a.selftest:
        return selftest()
    if not a.package:
        ap.error('--package is required unless --selftest')
    files = sorted(glob.glob(os.path.join(CASES, '*.c')))
    if a.case:
        files = [f for f in files if os.path.basename(f)[:-2] in a.case]
    bad = []
    with tempfile.TemporaryDirectory() as td:
        for f in files:
            name = os.path.basename(f)[:-2]
            out = os.path.join(td, name + '.bin')
            ok = refused_by_compiler(a.package, f, out) and refused_by_reference(f, out + '.ref')
            print('%s %s' % (name, 'REFUSED' if ok else 'NOT_REFUSED'))
            if not ok:
                bad.append(name)
    if not files:
        print('rejectcheck: no cases'); return 1
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
