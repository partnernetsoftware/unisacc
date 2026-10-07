#!/usr/bin/env python3
"""R20-1 B: byte-compare C99 and Python constructors on product δ graphs.

Each invocation owns its scratch tree and has a 55 s total budget.  Groups are
separate gate jobs so slow target generators never hide behind another target.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
TARGETS = ("lnx/arm64", "lnx/x86_64", "osx/arm64", "osx/x86_64", "win/arm64", "win/x86_64")
SHARED = (
    ("e2", "exec/build/gen.py", ("pp", "--shared-predefines")),
    ("e1", "exec/build/gen.py", ("lex", "--typed")),
    ("e3", "exec/build/gen.py", ("parse2",)),
    ("e4", "exec/build/gen.py", ("opt", "--o2")),
    ("o1", "exec/build/gen.py", ("opt",)),
    ("prune", "exec/build/gen.py", ("prune",)),
    ("nativeabi", "exec/build/gen.py", ("nativeabi",)),
)
FEATURES = (
    ("tokenpp", "exec/build/gen.py", ("pp", "--shared-predefines", "--no-autoinc")),
    ("tokenlex", "exec/build/gen.py", ("lex",)),
    ("warnlex", "exec/build/gen.py", ("lex", "--locations")),
    ("warnparse", "exec/build/gen.py", ("parse2", "--warnings", "--errors")),
    ("warnunits", "exec/build/gen.py", ("parse2/units", "--locations")),
    ("errorparse", "exec/build/gen.py", ("parse2", "--errors")),
    ("warnpp-shared", "exec/build/gen.py", ("pp", "--locations", "--shared-predefines")),
)

FEATURE_SHARDS = {"features-1": ("warnparse", "tokenpp", "tokenlex"),
                  "features-2": ("errorparse", "warnlex"),
                  "features-3": ("warnunits", "warnpp-shared")}
assert sorted(n for v in FEATURE_SHARDS.values() for n in v) == sorted(x[0] for x in FEATURES)

def specs(group):
    if group == "shared":
        return SHARED
    if group == "features":
        return FEATURES
    # Three parse2-sized models (~30-35 s cold each) cannot share one 55 s job.
    if group in FEATURE_SHARDS:
        return tuple(x for x in FEATURES if x[0] in FEATURE_SHARDS[group])
    if group == "object":
        return tuple((f"object-{arch}-{stage}", script, args)
                     for arch in ("arm64", "x86_64")
                     for stage, script, args in (
                         ("lower", "exec/build/gen.py", ("lower", "--full", "--object") + (("--arm64",) if arch == "arm64" else ())),
                         ("elf", "exec/build/gen.py", ("enc/arm" if arch == "arm64" else "enc", "--object")),
                     ))
    if group not in TARGETS:
        raise SystemExit("group must be shared, features, object, or one of " + ", ".join(TARGETS))
    osname, arch = group.split("/")
    flags = (("--osx",) if osname == "osx" else ("--win",) if osname == "win" else ())
    aflag = ("--arm64",) if arch == "arm64" else ()
    image = {"lnx": "elf", "osx": "macho", "win": "pe"}[osname]
    return (("lower", "exec/build/gen.py", ("lower", "--full",) + flags + aflag),
            ("elf", "exec/build/gen.py", ("enc/arm" if arch == "arm64" else "enc", "--" + image)))


def run(deadline, *args, env=None):
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise RuntimeError("seed matrix exceeded 55 s")
    p = subprocess.run(args, cwd=ROOT, capture_output=True, env=env,
                       timeout=min(50, remaining))
    if p.returncode:
        raise RuntimeError(f"{args}: rc={p.returncode}: {p.stderr[-1000:]!r}")
    return p


def main(group):
    deadline = time.monotonic() + 55
    with tempfile.TemporaryDirectory(prefix="seed-matrix-") as tmp:
        d = Path(tmp)
        cnet = d / "seed-net"
        # 0.0.25 B1: the seed constructor is built by unisacc.com (the installed previous release),
        # not by the system cc -- the self-hosting road uses our own compiler wherever it can
        run(deadline, "sh", str(ROOT / "unisacc.com"), "-fno-trim-libc", "seed/net.c", "-o", str(cnet))
        ctbl = d / "seed-tbl"                    # 0.0.25 B2: delta JSON -> flat table in C as well
        run(deadline, "sh", str(ROOT / "unisacc.com"), "-fno-trim-libc", "seed/tbl.c", "-o", str(ctbl))
        done = []
        for name, generator, flags in specs(group):
            graph, table = d / (name + ".json"), d / (name + ".tbl")
            py, c = d / (name + ".py.net"), d / (name + ".c.net")
            run(deadline, sys.executable, generator, str(graph), *flags)
            run(deadline, sys.executable, "exec/c/tbl.py", str(graph), str(table))
            ctable = d / (name + ".c.tbl")
            run(deadline, str(ctbl), str(graph), str(ctable))
            if table.read_bytes() != ctable.read_bytes():
                raise AssertionError(f"{group}/{name}: seed/tbl.c table differs from exec/c/tbl.py")
            run(deadline, sys.executable, "exec/c/net.py", str(table), str(py))
            run(deadline, str(cnet), str(table), str(c))
            if py.read_bytes() != c.read_bytes():
                raise AssertionError(f"{group}/{name}: first differing network byte")
            done.append(f"{name}={c.stat().st_size}")
        print(f"seed matrix {group}: {len(done)} byte-identical tables and networks (seed/tbl.c, seed/net.c); " + " ".join(done))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: seedconstructmatrix.py GROUP")
    main(sys.argv[1])
