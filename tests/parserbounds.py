#!/usr/bin/env python3
"""Compile-only boundary checks; each compiler invocation is bounded."""
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
        expected = 1 if diagnostic else 0
        if result.returncode != expected or (diagnostic and diagnostic.encode() not in result.stderr):
            raise RuntimeError(f"{name}: exit {result.returncode}, {result.stderr[:500]!r}")
        print(f"PASS {name}", flush=True)

cases = []
for count in (256, 257, 1000):
    source = "int main(void){switch(0){\n" + "".join(
        f"case {i}: return {i};\n" for i in range(count)) + "}return 0;}\n"
    cases.append((f"cases-{count}", source, [],
                  "switch case capacity exceeded" if count > 256 else None))
for count in (4096, 4097, 9000):
    source = '#include <stdio.h>\nint main(void){printf("' + "a" * count + '");return 0;}\n'
    cases.append((f"format-{count}", source, ["-Wall"],
                  "decoded string exceeds buffer capacity" if count > 4096 else None))
for count in (4096, 4097):
    source = 'int main(void){char s[]="' + "a" * count + '";return s[0];}\n'
    cases.append((f"initializer-{count}", source, [],
                  "decoded string exceeds buffer capacity" if count > 4096 else None))
with ThreadPoolExecutor(max_workers=2) as pool:
    list(pool.map(check, cases))
print(f"parserbounds: {len(cases)} passed")
