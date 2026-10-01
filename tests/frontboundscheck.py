#!/usr/bin/env python3
"""Keep anonymous aggregate tags inside the front end's token bounds."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="unisacc-front-bounds-") as d:
    work = Path(d)
    source = work / "ref.c"
    compiler = work / "ref"
    probe = work / "anon.c"
    tape = work / "anon.tape"
    probe.write_text("struct { int x; } a; int main(void) { return a.x; }\n")
    env = dict(os.environ, CFLAGS="-O1 -fsanitize=bounds")
    build = subprocess.run([str(ROOT / "tests/build_ref.sh"), str(source), str(compiler)],
                           cwd=ROOT, env=env, capture_output=True, text=True, timeout=30)
    assert build.returncode == 0, build.stderr
    env["UBSAN_OPTIONS"] = "halt_on_error=1"
    run = subprocess.run([str(compiler), "-S", "-o", str(tape), str(probe)],
                         cwd=ROOT, env=env, capture_output=True, text=True, timeout=10)
    assert run.returncode == 0, (run.returncode, run.stderr)
    assert "runtime error:" not in run.stderr, run.stderr
    assert tape.stat().st_size > 0
print("front bounds: anonymous struct compiles without token underflow")
