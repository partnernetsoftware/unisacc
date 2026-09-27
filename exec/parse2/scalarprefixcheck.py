"""Check explicit E2/E1/E3/lower/ARM Mach-O nets against host cc.
Usage: python3 exec/parse2/scalarprefixcheck.py NET_DIR EXECUTOR PRIVATE_OUTPUT
NET_DIR contains e2.net/e1.net/e3.net/lower.net/enc.net; no cached model substitution.
"""
import platform
import subprocess
import sys
from pathlib import Path

R = Path(__file__).resolve().parents[2]
models, executor, out = map(lambda x: Path(x).resolve(), sys.argv[1:])
if (platform.system(), platform.machine()) != ("Darwin", "arm64"):
    sys.exit("this native check requires macOS arm64; not tested")
out.mkdir(parents=True, exist_ok=False)

def run(args):
    return subprocess.run(["perl", str(R / "tests/bound.pl"), "10", *map(str, args)],
                          capture_output=True)

def chain(source, stages):
    return run([executor, "--chain", source, source, R / "include",
                *(models / (stage + ".net") for stage in stages)])

src = R / "exec/parse2/probes/scalar_prefix.c"
p = chain(src, ("e2", "e1", "e3", "lower", "enc"))
assert p.returncode == 0, p.stderr.decode()
exe = out / "model"
exe.write_bytes(p.stdout)
exe.chmod(0o755)
for command in (["codesign", "-s", "-", "-f", exe], ["cc", src, "-o", out / "host"]):
    p = run(command)
    assert p.returncode == 0, p.stderr.decode()
model, host = run([exe]), run([out / "host"])
assert model.returncode == host.returncode == 0, (model, host)
assert model.stdout == host.stdout, (model.stdout, host.stdout)
(out / "stdout").write_bytes(model.stdout)
for name, body in (
    ("pointer", "int main(void){int x; return +&x;}"),
    ("aggregate", "struct A{int x;}; int main(void){struct A a; +a; return 0;}"),
    ("void", "void f(void){} int main(void){+f();return 0;}"),
):
    source = out / (name + ".c")
    source.write_text(body)
    p = chain(source, ("e2", "e1", "e3"))
    assert p.returncode == 1 and b"unary + requires arithmetic operand" in p.stderr, (name, p)
print("scalar prefix: actual NET/native matches host; pointer/aggregate/void rejected")
