"""Shipped model product reads tapebin bytes itself on all six targets.

0.0.43 T-1: the reference compiler is prepared once by com-tapebin-ref (tests/tapebinref.py)
and only located here, by content key; a shard never builds it.  Parts:
  TAPEBIN_PART=six    calls, six-target images, host run and origin guard (fixed cost)
  TAPEBIN_SHARD=k/6   files[k-1::6] of examples/ and tests/c/, each verdict recorded
  TAPEBIN_PART=cover  after all shards: slices partition the file set, and the known
                      upstream tape differences are observed in total, not per shard
"""

import hashlib
import json
import os
import pathlib
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, str(HERE))
from unisa.tapebin import TARGETS, decode, encode, target_id
from knownfail import read as knownfail_read
import tapebinref

SHARDS = 6


def run(argv):
    p = subprocess.run([str(x) for x in argv], capture_output=True, timeout=20)
    if p.returncode:
        raise AssertionError((argv, p.returncode, p.stderr[:500]))
    return p.stdout


def shard():
    """TAPEBIN_SHARD=k/6: probe files k-1, k-1+6, ...  Six slices of ~43 files, each ~24 s
    on a quiet host, against the 42 s queue window (0.0.43 T-1)."""
    v = os.environ.get("TAPEBIN_SHARD", "1/%d" % SHARDS)
    k, n = (int(x) for x in v.split("/"))
    if not (1 <= k <= n) or n != SHARDS:
        raise AssertionError("bad TAPEBIN_SHARD " + v + " (need k/%d)" % SHARDS)
    return k, n


def six_part(product, ref, host, base):
    """Calls, six-target images, host run and origin guard: the fixed cost, run once."""
    calls = base / "calls.tapebin"
    calls.write_bytes(encode("entry:\n  callr r5\n  callm [r5+0]\n  ret\n"))
    decoded = base / "calls.tape"
    run(["sh", product, calls, "-t", "lnx/arm64", "-o", decoded])
    if decoded.read_bytes() != decode(calls.read_bytes()).to_canonical_text().encode("latin-1"):
        raise AssertionError("model tapebin reader confuses callr and callm")
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
    binary = base / "hello.tapebin"; output = base / "image"
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


def known_names():
    known = set()
    for name, reason in knownfail_read("exec/c/chain.knownfail").items():
        if not reason.startswith("R14-8 P1 "):
            raise AssertionError("bad tapebin knownfail line: " + name)
        known.add(name)
    return known


def all_files():
    files = sorted(pathlib.Path("examples").glob("*.c")) + sorted(pathlib.Path("tests/c").glob("*.c"))
    if not files:
        raise AssertionError("empty tape input set")
    names = [f.name for f in files]
    if len(set(names)) != len(names):
        raise AssertionError("duplicate probe names: verdicts are keyed by name")
    return files


def record(state, name, data):
    (state / name).write_text(json.dumps(data, indent=1, sort_keys=True) + "\n")


def cover(product, state_base):
    """All six shards and the six-target part, for this product: the slices must partition the
    file set, and the known upstream tape differences must be observed exactly as listed, once."""
    files = all_files()
    names = [f.name for f in files]
    state = state_base
    seen = []; verdicts = {}; observed_known = set()
    for k in range(1, SHARDS + 1):
        path = state / ("shard-%d-of-%d.json" % (k, SHARDS))
        if not path.is_file():
            raise AssertionError("cover: missing shard record " + path.name)
        data = json.loads(path.read_text())
        if data["n"] != SHARDS or data["k"] != k:
            raise AssertionError("cover: shard record does not match its slice: " + path.name)
        seen += data["files"]
        verdicts.update(data["verdicts"])
        observed_known |= set(data["known"])
    if not (state / "six.json").is_file():
        raise AssertionError("cover: six-target part not recorded")
    if sorted(seen) != sorted(names) or len(seen) != len(names):
        raise AssertionError("cover: slices do not partition the file set (%d recorded, %d files)" % (len(seen), len(names)))
    known = known_names()
    if not known <= set(names):
        raise AssertionError("cover: known upstream tape diff names outside the file set: %s" % sorted(known - set(names)))
    if observed_known != known:
        raise AssertionError(("cover: upstream tape difference set changed", sorted(observed_known), sorted(known)))
    counts = {v: sum(1 for x in verdicts.values() if x == v) for v in ("matched", "refused", "known")}
    print("tapebin product cover: %d files partitioned into %d slices; %s; known upstream tape diffs %d (global)"
          % (len(seen), SHARDS, counts, len(known)))


def main():
    part = os.environ.get("TAPEBIN_PART", "shard")
    product = pathlib.Path(os.environ.get("MODEL_COM", "unisacc.com")).resolve()
    if not product.is_file():
        raise AssertionError("missing model compiler")
    host = "osx/arm64" if sys.platform == "darwin" else "lnx/x86_64"
    state = tapebinref.state_dir(hashlib.sha256(product.read_bytes()).hexdigest())
    if part == "cover":
        return cover(product, state)
    ref = tapebinref.locate()
    with tempfile.TemporaryDirectory(prefix="unisacc-tapebin-product-") as tmp:
        base = pathlib.Path(tmp)
        if part == "six":
            six_part(product, ref, host, base)
            record(state, "six.json", {"part": "six", "targets": list(TARGETS), "host": host})
            print("tapebin product six: %d targets, host run, origin guard" % len(TARGETS))
            return
        if part != "shard":
            raise AssertionError("bad TAPEBIN_PART " + part)
        k, n = shard()
        refused_ok = set(knownfail_read("tests/product-refusals.knownfail"))
        known_all = known_names()
        files = all_files()
        mine = files[k - 1::n]
        names = {f.name for f in mine}
        known_tape_diffs = known_all & names
        observed_tape_diffs = set()
        verdicts = {}
        matched = 0
        for path in mine:
            classic = base / "classic.tapebin"
            product_bin = base / "product.tapebin"
            for f in (classic, product_bin):
                f.unlink(missing_ok=True)
            a = subprocess.run([str(ref), str(path), "-t", host, "--tapebin", "-o", str(classic)],
                               capture_output=True, timeout=20)
            b = subprocess.run(["sh", str(product), str(path), "-t", host, "--tapebin", "-o", str(product_bin)],
                               capture_output=True, timeout=20)
            if path.name in refused_ok:
                if a.returncode or not b.returncode:
                    raise AssertionError((str(path), "listed in tests/product-refusals.knownfail but not (reference accepts, product refuses): delete or fix its line"))
                verdicts[path.name] = "refused"
                continue
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
                verdicts[path.name] = "known"
            elif classic.read_bytes() != product_bin.read_bytes():
                raise AssertionError("product and reference tapebin differ for " + str(path))
            else:
                matched += 1
                verdicts[path.name] = "matched"
        if observed_tape_diffs != known_tape_diffs:
            raise AssertionError(("upstream tape difference set changed", sorted(observed_tape_diffs)))
        record(state, "shard-%d-of-%d.json" % (k, n),
               {"k": k, "n": n, "files": sorted(names), "verdicts": verdicts, "known": sorted(observed_tape_diffs)})
        print("tapebin product shard %d/%d: %d files, %d equal-source packages, %d known upstream tape diffs"
              % (k, n, len(mine), matched, len(known_tape_diffs)))


if __name__ == "__main__":
    main()
