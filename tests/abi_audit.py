#!/usr/bin/env python3
"""The syscall numbers in the abi table, against this machine's own headers.
[A-51] [S-15 A2]

The catalog's syscall numbers were typed in by hand, three columns of
nineteen.  Enumeration proves the net reproduces them; nothing checked them
against an operating system.  A machine can only vouch for its OWN
(os, arch), so this audits exactly that one, from the system's headers:
this Mac checks the osx column, the Linux VM checks lnx/arm64, GitHub's
Linux runner checks lnx/x86_64.  Between them every column is covered.

A number is right when it IS the syscall the op names.  Where an op has no
syscall of its own on a platform, the catalog must say "none" (the lowering
then refuses it) rather than borrow a number: its first run found macOS
`nanosleep` mapped to 240, which is SYS_listxattr.
"""
import os
import platform
import re
import subprocess
import sys
import tempfile

R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, R)

# the header symbol each op means, per OS.  An op the OS has no syscall for
# maps to None: the catalog must say "none" for it.
NAME = {
    "lnx": {"open": ["SYS_open", "SYS_openat"]},
    "osx": {"clock_gettime": None, "nanosleep": None,
            "futex": ["SYS_ulock_wait"], "clone": ["SYS_bsdthread_create"]},
}


def host():
    s, m = platform.system(), platform.machine()
    os_ = {"Darwin": "osx", "Linux": "lnx"}.get(s)
    arch = {"arm64": "arm64", "aarch64": "arm64", "x86_64": "x86_64",
            "AMD64": "x86_64"}.get(m)
    return os_, arch


def main():
    from unisa import catalog
    os_, arch = host()
    if not os_ or not arch:
        print("  skip (not a platform this compiler targets)")
        return 0
    header = "sys/syscall.h"
    ops = list(catalog.SYSOPS)
    want = {}
    lines = ["#include <stdio.h>", "#include <%s>" % header,
             "int main(void) {"]
    probes = []
    for op in ops:
        names = NAME.get(os_, {}).get(op, ["SYS_" + op])
        if names is None:
            want[op] = None
            continue
        # the first spelling the headers define: `open` is SYS_openat on
        # arm64 Linux, which has no SYS_open at all
        expr = "-1"
        for nm in reversed(names):
            expr = "\n#ifdef %s\n %s\n#else\n %s\n#endif\n" % (nm, nm, expr)
        lines.append('    printf("%%s %%ld\\n", "%s", (long)(%s));' % (op, expr))
        probes.append(op)
    lines += ["    return 0;", "}"]
    tmp = tempfile.mkdtemp()
    src, exe = os.path.join(tmp, "a.c"), os.path.join(tmp, "a")
    open(src, "w").write("\n".join(lines) + "\n")
    p = subprocess.run(["cc", "-w", "-o", exe, src], capture_output=True, text=True)
    if p.returncode != 0:
        print("  skip (cannot build against <%s>: %s)" % (header, p.stderr[:120]))
        return 0
    for ln in subprocess.run([exe], capture_output=True, text=True).stdout.split("\n"):
        if ln.strip():
            op, n = ln.split()
            want[op] = int(n)

    ok = bad = 0
    for op in ops:
        have = catalog.sysno(op, os_, arch)
        w = want.get(op)
        if w is None:
            if have == "none":
                ok += 1
            else:
                bad += 1
                print("  FAIL %-14s the catalog says %s, but %s/%s has no such "
                      "syscall -- it must say none" % (op, have, os_, arch))
            continue
        if w < 0:
            bad += 1
            print("  FAIL %-14s the headers here define no syscall for it" % op)
            continue
        # osx puts the BSD class in the high bits on every arch
        expect = hex(catalog.OSX_CLASS_BIT | w) if os_ == "osx" else str(w)
        if have == expect:
            ok += 1
        else:
            bad += 1
            print("  FAIL %-14s the catalog says %s, <%s> says %s"
                  % (op, have, header, expect))
    print()
    print("abi_audit  %s/%s  syscalls agree %d   wrong %d   (against this "
          "machine's <%s>)" % (os_, arch, ok, bad, header))
    return 1 if bad or not ok else 0


if __name__ == "__main__":
    sys.exit(main())
