"""Structural and byte-stability checks for the v1 Python codec."""

import pathlib
import subprocess
import sys
import tempfile
import time
import os

T0 = time.time()


def stage(name):
    # 0.0.34 queue: tapebin-roundtrip hit 49 s with an empty log; name each stage's elapsed time
    print("tapebin: %-28s %5.1fs" % (name, time.time() - T0), file=sys.stderr, flush=True)


def reference(base):
    """The same-source UA the gate passes, else a private build (as before)."""
    ua = os.environ.get("UA")
    if ua and os.access(ua, os.X_OK):
        return pathlib.Path(ua)
    ref = base / "ref"
    subprocess.run(["./tests/build_ref.sh", str(base / "ref.c"), str(ref)], check=True, timeout=30)
    stage("private reference built")
    return ref

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

from unisa.tape import parse
from unisa.tapebin import decode, encode
from unisa.tapebin import TARGETS, target_id
from knownfail import read as knownfail_read


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


def main(part):
    check('.global exported\n.extern imported\nexported:\n  ret\n')
    unit_text = '.unit 2\n.global g_a\n.bss g_a 4\n.gdef g_a\n.extern g_b\n'
    check(unit_text)
    for invalid in ('.gdef g_a\n', '.unit 1\n', '.global g_a\n.unit 2\n',
                    '.unit 2\n.unit 2\n'):
        try:
            encode(invalid)
        except ValueError:
            pass
        else:
            raise AssertionError('invalid unit tape accepted: ' + invalid)
    check('L:\n  nop\n.bss z 24\n.str a "x"\nL:\n.str a "y"\n  call L\n')
    known = list(knownfail_read("exec/c/chain.knownfail"))
    files = sorted(pathlib.Path("examples").glob("*.c")) + [pathlib.Path("tests/c") / name for name in known]
    if not files:
        raise AssertionError("no C inputs")
    tapes = []
    if part in ("codec", "all"):
        for path in files:
            tape = subprocess.check_output([sys.executable, "-m", "unisa", "tape", str(path)], timeout=20)
            check(tape.decode("latin-1"))
            tapes.append((path, tape))
        stage("python tapes (%d)" % len(files))
    with tempfile.TemporaryDirectory(prefix="unisacc-tapebin-") as tmp:
        base = pathlib.Path(tmp)
        ref = reference(base)
        unit_plain = base / 'unit.tape'
        unit_binary = base / 'unit.tapebin'
        unit_plain.write_text(unit_text)
        subprocess.run([str(ref), str(unit_plain), '--tapebin', '-o', str(unit_binary)],
                       check=True, timeout=20)
        if unit_binary.read_bytes() != encode(unit_text):
            raise AssertionError('C/Python unit record encoding differs')
        all_sources = sorted(pathlib.Path("examples").glob("*.c")) + sorted(pathlib.Path("tests/c").glob("*.c"))
        if not all_sources:
            raise AssertionError("empty full probe set")
        if part.startswith("structure") or part == "all":
            if "/" in part:   # structure-K/N: every N-th probe from K
                k, n = (int(x) for x in part.split("-", 1)[1].split("/"))
                all_sources = all_sources[k - 1::n]
            for path in all_sources:
                text = subprocess.check_output([str(ref), str(path), "-t", "osx/arm64"], timeout=20)
                check(text.decode("latin-1"))
            stage("structural roundtrips (%d)" % len(all_sources))
        if part.startswith("structure"):
            print("tapebin structure: %d structural roundtrips" % len(all_sources))
            return
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
        stage("C/Python encoder + images")
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
        stage("targets + run")
    if part == "codec":
        print("tapebin codec: %d C/Python byte-identical tapes, 6 target images, C run" % len(files))
        return
    print("tapebin: %d structural roundtrips, %d C/Python byte-identical tapes, 6 target images, C run" % (len(all_sources), len(files)))


if __name__ == "__main__":
    # structure[-K/N] | codec (the two gate shards, 0.0.34) | all (default, both in one run)
    main(sys.argv[1] if len(sys.argv) > 1 else "all")
