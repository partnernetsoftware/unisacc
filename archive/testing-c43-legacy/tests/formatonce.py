#!/usr/bin/env python3
"""Format diagnostics must not re-run the expression parser or change code."""
import os
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ua = os.environ["UA"]
command = ["sh", ua] if Path(ua).read_bytes().startswith(b"MZ") else [ua]
cases = {
    "labels": 'int main(void){volatile int a=2; printf("%d\\n",a>1?a*2:a+1);printf("marker\\n");return 0;}',
    "temporary": 'struct S{int x;long y;};struct S make(void){struct S s={3,4};return s;}int use(struct S s){return s.x+s.y;}int main(void){printf("%d\\n",use(make()));return 0;}',
    "compound": 'int main(void){printf("%d %ld\\n",((int[]){3,4})[1],sizeof(struct {int x;}));return 0;}',
    "nested": 'int main(void){printf("%d\\n",printf("inner\\n"));return 0;}',
    "enum": 'int main(void){printf("%ld %d\\n",sizeof(enum {VALUE=3}),VALUE);return 0;}',
}

def run(args):
    p = subprocess.run(args, capture_output=True, timeout=15)
    assert p.returncode == 0, (args, p.returncode, p.stderr[:1000])
    return p

def check(item):
    name, body = item
    with tempfile.TemporaryDirectory(prefix="unisacc-formatonce-") as directory:
        root = Path(directory)
        src = root / "input.c"
        src.write_text('#include <stdio.h>\n' + body + '\n')
        run(["cc", "-std=c99", str(src), "-o", str(root / "host")])
        expected = run([str(root / "host")]).stdout
        for level in range(3):
            args = command + [f"-O{level}", str(src)]
            quiet = run(args + ["-S", "-o", "-"])
            loud = run(args + ["-Wall", "-S", "-o", "-"])
            assert quiet.stdout == loud.stdout, (name, level, "-Wall changed tape")
            got = run(args + ["-Wall", "-run"])
            assert got.stdout == expected, (name, level, expected, got.stdout)
        print(f"PASS {name}: three levels, tape invariant and host output", flush=True)

with ThreadPoolExecutor(max_workers=2) as pool:
    list(pool.map(check, cases.items()))
print(f"formatonce: {len(cases)} cases")
