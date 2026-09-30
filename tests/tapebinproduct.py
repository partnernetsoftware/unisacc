"""Model-driver tapebin entry: same model, same tape, same six images."""

import os
import pathlib
import subprocess
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from unisa.tapebin import TARGETS, encode, target_id


def run(argv):
    p = subprocess.run([str(x) for x in argv], capture_output=True, timeout=20)
    if p.returncode:
        raise AssertionError((argv, p.returncode, p.stderr[:500]))
    return p.stdout


def main():
    product = pathlib.Path(os.environ.get("MODEL_COM", "unisacc.com")).resolve()
    if not product.is_file():
        raise AssertionError("missing model compiler")
    with tempfile.TemporaryDirectory(prefix="unisacc-tapebin-product-") as tmp:
        base = pathlib.Path(tmp)
        driver = base / "driver"
        run(["cc", "-O2", "-std=c99", "exec/c/compiler.c", "-o", driver])
        for target in TARGETS:
            tape = run(["sh", product, "examples/hello.c", "-t", target])
            binary = base / "hello.tapebin"
            binary.write_bytes(encode(tape.decode("latin-1"), origin_target=target_id(target)))
            output = base / "image"
            run([driver, "--models", product, binary, "-b", target, "-o", output])
            want = run(["sh", product, "examples/hello.c", "-b", target, "-o", "-"])
            if output.read_bytes() != want:
                raise AssertionError("model image differs for " + target)
        host = "osx/arm64" if sys.platform == "darwin" else "lnx/x86_64"
        tape = run(["sh", product, "examples/hello.c", "-t", host])
        binary.write_bytes(encode(tape.decode("latin-1"), origin_target=target_id(host)))
        got = run([driver, "--models", product, "-run", binary])
        want = run(["sh", product, "-run", "examples/hello.c"])
        if got != want:
            raise AssertionError("model run differs")
        wrong = "win/arm64" if host != "win/arm64" else "lnx/x86_64"
        rejected = subprocess.run(
            [str(driver), "--models", str(product), str(binary), "-b", wrong, "-o", str(output)],
            capture_output=True,
            timeout=20,
        )
        if rejected.returncode == 0 or b"origin" not in rejected.stderr.lower():
            raise AssertionError("origin mismatch was not rejected")
        run([driver, "--models", product, "--force-origin", binary, "-b", wrong, "-o", output])
    print("tapebin product: six images, host run, origin guard")


if __name__ == "__main__":
    main()
