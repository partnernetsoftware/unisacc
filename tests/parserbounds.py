#!/usr/bin/env python3
"""Boundary safety: explicit capacity rejection or host-correct execution."""
import os
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ua = os.environ["UA"]  # Mandatory: never fall back to a shared compiler.
command = ["sh", ua] if Path(ua).read_bytes().startswith(b"MZ") else [ua]

def check(case):
    name, source, flags, diagnostic = case
    with tempfile.TemporaryDirectory(prefix="unisacc-parserbounds-") as directory:
        root = Path(directory)
        src = root / "input.c"
        src.write_text(source)
        result = subprocess.run([*command, *flags, "-S", str(src), "-o", str(root / "out")],
                                capture_output=True, timeout=15)
        if result.returncode == 1 and diagnostic:
            if diagnostic.encode() not in result.stderr:
                raise RuntimeError(f"{name}: unexpected rejection {result.stderr[:500]!r}")
            print(f"PASS {name} (bounded rejection)", flush=True)
            return
        if result.returncode != 0 or not (root / "out").stat().st_size:
            raise RuntimeError(f"{name}: exit {result.returncode}, {result.stderr[:500]!r}")
        # Capacity is implementation-specific. Acceptance must prove correct
        # execution, including beyond the classic front end's fixed arrays.
        host = root / "host"
        built = subprocess.run(["cc", "-O0", str(src), "-o", str(host)],
                               capture_output=True, timeout=15)
        if built.returncode != 0:
            raise RuntimeError(f"{name}: host build {built.returncode}, {built.stderr[:500]!r}")
        want = subprocess.run([str(host)], capture_output=True, timeout=10)
        got = subprocess.run([*command, *flags, "-O0", "-run", str(src)],
                             capture_output=True, timeout=15)
        if want.returncode != 0 or got.returncode != 0 or got.stdout != want.stdout:
            raise RuntimeError(f"{name}: runtime exits {got.returncode}/{want.returncode}, "
                               f"stdout {got.stdout[:100]!r}/{want.stdout[:100]!r}, {got.stderr[:500]!r}")
        print(f"PASS {name} (executed, same as host)", flush=True)

cases = []
for count in (256, 257, 1000):
    source = '#include <stdio.h>\nint choose(int x){switch(x){\n' + "".join(
        f"case {i}: return {i * 3};\n" for i in range(count)) + (
        '}return -1;}\nint main(void){int i,total=0;for(i=0;i<' + str(count) +
        ';i++) total+=choose(i);printf("%d %d %d\\n",total,choose(-1),choose(' +
        str(count) + '));return 0;}\n')
    cases.append((f"cases-{count}", source, [],
                  "switch case capacity exceeded" if count > 256 else None))
for count in (4096, 4097, 9000):
    source = '#include <stdio.h>\nint main(void){printf("' + "a" * count + '");return 0;}\n'
    cases.append((f"format-{count}", source, ["-Wall"],
                  "decoded string exceeds buffer capacity" if count > 4096 else None))
for count in (4096, 4097):
    source = '#include <stdio.h>\nint main(void){char s[]="' + "a" * count + ('";int i,sum=0;for(i=0;i<' + str(count) + ';i++) sum+=s[i];printf("%d %d %d\\n",(int)sizeof(s),sum,(int)s[' + str(count) + ']);return 0;}\n')
    cases.append((f"initializer-{count}", source, [],
                  "decoded string exceeds buffer capacity" if count > 4096 else None))
with ThreadPoolExecutor(max_workers=2) as pool:
    list(pool.map(check, cases))
print(f"parserbounds: {len(cases)} passed")
