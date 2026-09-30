"""Structural and byte-stability checks for the v1 Python codec."""

import pathlib
import subprocess
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

from unisa.tape import parse
from unisa.tapebin import decode, encode
from unisa.tapebin import TARGETS, target_id


def check(text):
    original = parse(text)
    first = encode(original)
    if first != encode(original):
        raise AssertionError("nondeterministic encoding")
    restored = decode(first)
    if original.records != restored.records:
        raise AssertionError("record order or value changed")
    if encode(restored) != first:
        raise AssertionError("binary roundtrip changed")
    if parse(restored.to_canonical_text()).records != original.records:
        raise AssertionError("canonical print changed records")
    for damaged in (first[:-1], first[:64] + bytes([first[64] ^ 1]) + first[65:]):
        try:
            decode(damaged)
        except ValueError:
            pass
        else:
            raise AssertionError("damaged package accepted")


def main():
    check('L:\n  nop\n.bss z 24\n.str a "x"\nL:\n.str a "y"\n  call L\n')
    files = sorted(pathlib.Path("examples").glob("*.c"))[:10]
    if not files:
        raise AssertionError("no C inputs")
    tapes = []
    for path in files:
        tape = subprocess.check_output([sys.executable, "-m", "unisa", "tape", str(path)])
        check(tape.decode("latin-1"))
        tapes.append((path, tape))
    with tempfile.TemporaryDirectory(prefix="unisacc-tapebin-") as tmp:
        base = pathlib.Path(tmp)
        ref = base / "ref"
        subprocess.run(["./tests/build_ref.sh", str(base / "ref.c"), str(ref)],
                       check=True, timeout=30)
        for path, text in tapes:
            plain = base / "probe.tape"
            binary = base / "probe.tapebin"
            plain.write_bytes(text)
            binary.write_bytes(encode(text.decode("latin-1")))
            c_binary = base / "probe-c.tapebin"
            subprocess.run([str(ref), str(plain), "--tapebin", "-o", str(c_binary)],
                           check=True, timeout=20)
            if c_binary.read_bytes() != binary.read_bytes():
                raise AssertionError("C/Python encoder bytes differ for " + str(path))
            images = []
            for source in (plain, binary):
                image = base / ("probe-image-%d" % len(images))
                subprocess.run([str(ref), str(source), "-b", "lnx/x86_64", "-o", str(image)],
                               check=True, timeout=20)
                images.append(image.read_bytes())
            if images[0] != images[1]:
                raise AssertionError("C reference image differs for " + str(path))
        for target in TARGETS:
            text = subprocess.check_output([sys.executable, "-m", "unisa", "tape",
                                            "examples/hello.c", "--target", target], timeout=20)
            plain = base / "hello.tape"
            binary = base / "hello.tapebin"
            plain.write_bytes(text)
            binary.write_bytes(encode(text.decode("latin-1"), origin_target=target_id(target)))
            images = []
            for source in (plain, binary):
                image = base / ("image-%d" % len(images))
                subprocess.run([str(ref), str(source), "-b", target, "-o", str(image)],
                               check=True, timeout=20)
                images.append(image.read_bytes())
            if images[0] != images[1]:
                raise AssertionError("C reference image differs for " + target)
        rejected = subprocess.run([str(ref), str(binary), "-b", "lnx/x86_64",
                                   "-o", str(base / "wrong-origin")],
                                  capture_output=True, timeout=20)
        if rejected.returncode == 0 or (base / "wrong-origin").exists():
            raise AssertionError("origin mismatch was accepted")
        host = "osx/arm64" if sys.platform == "darwin" else "lnx/x86_64"
        plain.write_bytes(subprocess.check_output([sys.executable, "-m", "unisa", "tape",
                                                   "examples/hello.c", "--target", host], timeout=20))
        binary.write_bytes(encode(plain.read_text("latin-1"), origin_target=target_id(host)))
        runs = [subprocess.run([str(ref), "-run", str(source)], capture_output=True, timeout=20)
                for source in (plain, binary)]
        if [(r.returncode, r.stdout) for r in runs][0] != [(r.returncode, r.stdout) for r in runs][1]:
            raise AssertionError("C reference run differs")
    print("tapebin: %d C/Python byte-identical tapes, 6 target images, C run" % len(files))


if __name__ == "__main__":
    main()
