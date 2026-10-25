#!/usr/bin/env python3
"""Fresh, stage-parallel TSV construction; compare both complete shipped files.

Uses the product reader, constructor, exhaustive verifier and serializers.
No model answers or previous construction cache are used as build input.
Only temporary per-stage files are written; tests/bound owns each child tree.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from unisa import intnet, tsvgold, uns2
from unisa.gold import ALL


def build_stage(name, output):
    stage = tsvgold.load_stage(ROOT / "weights/gold" / (name + ".tsv"))
    if stage.name != name:
        raise ValueError("TSV stage name differs: " + name)
    started = time.monotonic()
    net = intnet.build(name, verify=True, stages={name: stage})
    intnet.save_all({name: net}, str(output))
    print("stage %s keys %d verified units %d %.2fs" %
          (name, stage.rows(), net.H, time.monotonic() - started), flush=True)


def check(jobs):
    with tempfile.TemporaryDirectory(prefix="unisacc-tsvbuild-") as td:
        directory = Path(td)
        helper = subprocess.run([str(ROOT / "tests/bound"), "--helper"],
                                capture_output=True, text=True, timeout=10, check=True).stdout.strip()
        if not helper:
            raise ValueError("empty bound helper")

        def worker(name):
            return name, subprocess.run(
                [helper, "52", sys.executable, str(Path(__file__).resolve()),
                 "--stage", name, "--out", str(directory / (name + ".json"))],
                cwd=ROOT, capture_output=True, text=True)

        with ThreadPoolExecutor(max_workers=jobs) as pool:
            outcomes = list(pool.map(worker, ALL))
        failed = False
        for name, result in outcomes:
            print(result.stdout, end="")
            if result.stderr:
                print(result.stderr, end="", file=sys.stderr)
            if result.returncode != 0:
                print("FAIL stage %s exited %d" % (name, result.returncode), file=sys.stderr)
                failed = True
        if failed:
            return 1
        reg = tsvgold.load_all(ROOT / "weights/gold", ALL)
        nets = {}
        for name in ALL:
            part = intnet.load_all(str(directory / (name + ".json")), stages=reg)
            if set(part) != {name}:
                raise ValueError("wrong per-stage output: " + name)
            nets.update(part)
        if set(nets) != set(ALL) or len(nets) != len(ALL):
            raise ValueError("missing/duplicate constructed stage")
        raw_uns2 = uns2.dump(nets, reg)
        full_json = directory / "built.json"
        intnet.save_all(nets, str(full_json))
        ok = True
        for kind, got in (("uns2", raw_uns2), ("json", full_json.read_bytes())):
            want = (ROOT / "weights" / ("built." + kind)).read_bytes()
            same = got == want
            print("from-tsv  %s  stages %d  %d B  shipped %d B  raw bytes %s" %
                  (kind, len(nets), len(got), len(want), "IDENTICAL" if same else "DIFFERENT"))
            ok = ok and same
        return 0 if ok else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jobs", type=int, choices=range(1, 5), default=2)
    parser.add_argument("--stage", choices=ALL)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    if args.stage:
        if args.out is None:
            parser.error("--stage requires --out")
        build_stage(args.stage, args.out)
        return 0
    if args.out is not None:
        parser.error("--out requires --stage")
    return check(args.jobs)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.SubprocessError, AssertionError) as exc:
        print("tsvbuild FAIL: %s" % exc, file=sys.stderr)
        raise SystemExit(1)
