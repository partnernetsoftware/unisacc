"""Shipped model product reads tapebin bytes itself on all six targets."""

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
        ref = base / "ref"
        run(["./tests/build_ref.sh", base / "ref.c", ref])
        for target in TARGETS:
            binary = base / "hello.tapebin"
            classic = base / "classic.tapebin"
            run(["sh", product, "examples/hello.c", "-t", target, "--tapebin", "-o", binary])
            run([ref, "examples/hello.c", "-t", target, "--tapebin", "-o", classic])
            if binary.read_bytes() != classic.read_bytes():
                raise AssertionError("product and reference tapebin differ for " + target)
            output = base / "image"
            run(["sh", product, binary, "-b", target, "-o", output])
            want = run(["sh", product, "examples/hello.c", "-b", target, "-o", "-"])
            if output.read_bytes() != want:
                raise AssertionError("model image differs for " + target)
        host = "osx/arm64" if sys.platform == "darwin" else "lnx/x86_64"
        matched = 0
        known_tape_diffs = set()
        for line in pathlib.Path("tests/tapebin.knownfail").read_text().splitlines():
            if not line or line.startswith("#"):
                continue
            cols = line.split(maxsplit=2)
            if len(cols) != 3 or cols[1] != "R14-8" or cols[0] in known_tape_diffs:
                raise AssertionError("bad tapebin knownfail line: " + line)
            known_tape_diffs.add(cols[0])
        observed_tape_diffs = set()
        files = sorted(pathlib.Path("examples").glob("*.c")) + sorted(pathlib.Path("tests/c").glob("*.c"))
        if not files:
            raise AssertionError("empty tape input set")
        for path in files:
            classic = base / "classic.tapebin"
            product_bin = base / "product.tapebin"
            for f in (classic, product_bin):
                f.unlink(missing_ok=True)
            a = subprocess.run([str(ref), str(path), "-t", host, "--tapebin", "-o", str(classic)],
                               capture_output=True, timeout=20)
            b = subprocess.run(["sh", str(product), str(path), "-t", host, "--tapebin", "-o", str(product_bin)],
                               capture_output=True, timeout=20)
            if bool(a.returncode) != bool(b.returncode):
                raise AssertionError((str(path), "reference/product acceptance differs", a.stderr[:160], b.stderr[:160]))
            if a.returncode:
                raise AssertionError("previously compiling probe now refused: " + str(path))
            classic_text = run([ref, path, "-t", host])
            product_text = run(["sh", product, path, "-t", host])
            origin = target_id(host)
            if classic.read_bytes() != encode(classic_text.decode("latin-1"), origin_target=origin):
                raise AssertionError("reference codec differs from Python for " + str(path))
            if product_bin.read_bytes() != encode(product_text.decode("latin-1"), origin_target=origin):
                raise AssertionError("product codec differs from Python for " + str(path))
            shared_tape = base / "shared.tape"
            shared_tape.write_bytes(product_text)
            run([ref, shared_tape, "-t", host, "--tapebin", "-o", classic])
            run(["sh", product, shared_tape, "-t", host, "--tapebin", "-o", product_bin])
            same_input = encode(product_text.decode("latin-1"), origin_target=0)
            if classic.read_bytes() != same_input or product_bin.read_bytes() != same_input:
                raise AssertionError("same-input three-codec comparison differs for " + str(path))
            if classic_text != product_text:
                observed_tape_diffs.add(path.name)
            elif classic.read_bytes() != product_bin.read_bytes():
                raise AssertionError("product and reference tapebin differ for " + str(path))
            else:
                matched += 1
        if observed_tape_diffs != known_tape_diffs:
            raise AssertionError(("upstream tape difference set changed", sorted(observed_tape_diffs)))
        run(["sh", product, "examples/hello.c", "-t", host, "--tapebin", "-o", binary])
        got = run(["sh", product, "-run", binary])
        want = run(["sh", product, "-run", "examples/hello.c"])
        if got != want:
            raise AssertionError("model run differs")
        wrong = "win/arm64" if host != "win/arm64" else "lnx/x86_64"
        rejected = subprocess.run(
            ["sh", str(product), str(binary), "-b", wrong, "-o", str(output)],
            capture_output=True,
            timeout=20,
        )
        if rejected.returncode == 0 or b"origin" not in rejected.stderr.lower():
            raise AssertionError("origin mismatch was not rejected")
        run(["sh", product, "--force-origin", binary, "-b", wrong, "-o", output])
    print("tapebin product: %d equal-source packages, %d known upstream tape diffs, six images, host run, origin guard" % (matched, len(known_tape_diffs)))


if __name__ == "__main__":
    main()
