"""Persistent enum descriptors: explicit E3 NET and private typed-token oracle.
Usage: enumcheck.py E3_NET EXECUTOR TYPED_DUMP LOWER_NET ENC_NET EMPTY_OUTPUT
macOS arm64 native part; no model/UA replacement or implicit shared cache.
"""
import os
import platform
import subprocess
import sys
from pathlib import Path
R = Path(__file__).resolve().parents[2]
net, executor, dump, lower, enc, out = map(lambda s: Path(s).resolve(), sys.argv[1:])
assert (platform.system(), platform.machine()) == ("Darwin", "arm64")
out.mkdir(parents=True, exist_ok=False)

def run(args, **kw):
    return subprocess.run(["perl", str(R / "tests/bound.pl"), "10", *map(str, args)], capture_output=True, **kw)

def tokens(source):
    p = run([dump, "-dump-tokens", source], env=dict(os.environ, UA_TYPESPELL="1"))
    assert p.returncode == 0, p.stderr
    return p.stdout

def parse(name, text):
    src = out / (name + ".c")
    src.write_text(text)
    tok = out / (name + ".tokens")
    tok.write_bytes(tokens(src))
    return src, run([executor, net, tok, src])

rejects = {
    "object": "enum E; enum E x;",
    "local": "enum E; int f(void){enum E x;return 0;}",
    "array": "enum E; enum E a[2];",
    "member": "enum E; struct S{enum E x;};",
    "bitfield": "enum E; struct S{enum E x:2;};",
    "sizeof": "enum E; enum E *p; int f(void){return sizeof(*p);}",
    "parameter": "enum E; int f(enum E x){return 0;}",
    "return": "enum E; enum E f(void){return 0;}",
    "call": "enum E; int f(enum E); int g(void){return f(0);}",
    "kind1": "struct E; enum E *p;",
    "kind2": "enum E; struct E *p;",
    "kind3": "enum E; struct E {int x;};",
    "redefinition": "enum E{A}; enum E{B};",
    "shadow": "enum E{A}; int f(void){enum E;enum E *p;return sizeof(*p);}",
}
for name, text in rejects.items():
    source, p = parse(name, text + "int main(void){return 0;}\n")
    assert p.returncode == 1, (name, p.stderr)
    host = run(["cc", "-std=c11", "-fsyntax-only", source])
    assert host.returncode != 0, (name, "host accepted rejection probe")
for count in (59, 60):
    text = "".join("enum E%d;\n" % i for i in range(count)) + "int main(void){return 0;}"
    _, p = parse("capacity%d" % count, text)
    assert p.returncode == (0 if count == 59 else 1), (count, p.stderr)
    if count == 60: assert b"enum descriptor pool exhausted" in p.stderr

src, p = parse("legal", (R / "exec/parse2/probes/enum_forward.c").read_text())
assert p.returncode == 0, p.stderr
inp = out / "legal.tape"
inp.write_bytes(p.stdout)
p = run([executor, "--chain", inp, src, R / "include", lower, enc])
assert p.returncode == 0, p.stderr
exe = out / "native"
exe.write_bytes(p.stdout)
exe.chmod(0o755)
for args in (["codesign", "-s", "-", "-f", exe], ["cc", src, "-o", out / "host"]):
    p = run(args)
    assert p.returncode == 0, p.stderr
actual, expected = run([exe]), run([out / "host"])
assert actual.returncode == expected.returncode == 0
assert actual.stdout == expected.stdout, (actual.stdout, expected.stdout)
(out / "stdout").write_bytes(actual.stdout)
print("enum descriptors: NET/native = host; 14 invalid types rejected; capacity 59 accepts / 60 rejects")
