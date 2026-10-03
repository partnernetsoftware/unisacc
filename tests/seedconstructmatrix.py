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
    ("e2", "exec/pp/gen.py", ("--shared-predefines",)),
    ("e1", "exec/lex/gen.py", ("--typed",)),
    ("e3", "exec/parse2/gen2.py", ()),
    ("e4", "exec/build/gen.py", ("opt", "--o2")),
    ("o1", "exec/build/gen.py", ("opt",)),
    ("prune", "exec/build/gen.py", ("prune",)),
    ("nativeabi", "exec/nativeabi/gen.py", ()),
)
FEATURES = (
    ("tokenpp", "exec/pp/gen.py", ("lnx/x86_64", "--shared-predefines")),
    ("tokenlex", "exec/lex/gen.py", ()),
    ("warnlex", "exec/lex/gen.py", ("--locations",)),
    ("warnparse", "exec/parse2/gen2.py", ("--warnings", "--errors")),
    ("warnunits", "exec/parse2/units.py", ("--locations",)),
    ("errorparse", "exec/parse2/gen2.py", ("--errors",)),
    ("warnpp-shared", "exec/pp/gen.py", ("--locations", "--shared-predefines")),
)


def specs(group):
    if group == "shared":
        return SHARED
    if group == "features":
        return FEATURES
    if group == "object":
        return tuple((f"object-{arch}-{stage}", script, args)
                     for arch in ("arm64", "x86_64")
                     for stage, script, args in (
                         ("lower", "exec/build/gen.py", ("lower", "--full", "--object") + (("--arm64",) if arch == "arm64" else ())),
                         ("elf", "exec/enc/arm.py" if arch == "arm64" else "exec/enc/gen.py", ("--object",)),
                     ))
    if group not in TARGETS:
        raise SystemExit("group must be shared, features, object, or one of " + ", ".join(TARGETS))
    osname, arch = group.split("/")
    flags = (("--osx",) if osname == "osx" else ("--win",) if osname == "win" else ())
    aflag = ("--arm64",) if arch == "arm64" else ()
    image = {"lnx": "elf", "osx": "macho", "win": "pe"}[osname]
    return (("lower", "exec/build/gen.py", ("lower", "--full",) + flags + aflag),
            ("elf", "exec/enc/arm.py" if arch == "arm64" else "exec/enc/gen.py", ("--" + image,)))


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
        run(deadline, "cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
            "-o", str(cnet), "seed/net.c")
        done = []
        for name, generator, flags in specs(group):
            graph, table = d / (name + ".json"), d / (name + ".tbl")
            py, c = d / (name + ".py.net"), d / (name + ".c.net")
            env = dict(os.environ, E2_AUTOINC="0") if name == "tokenpp" else None
            run(deadline, sys.executable, generator, str(graph), *flags, env=env)
            run(deadline, sys.executable, "exec/c/tbl.py", str(graph), str(table))
            run(deadline, sys.executable, "exec/c/net.py", str(table), str(py))
            run(deadline, str(cnet), str(table), str(c))
            if py.read_bytes() != c.read_bytes():
                raise AssertionError(f"{group}/{name}: first differing network byte")
            done.append(f"{name}={c.stat().st_size}")
        print(f"seed matrix {group}: {len(done)} byte-identical δ; " + " ".join(done))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: seedconstructmatrix.py GROUP")
    main(sys.argv[1])
