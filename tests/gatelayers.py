#!/usr/bin/env python3
"""Semantic breadth of gate suites; elapsed time remains gatequeue's own estimate.

These layers guide diagnosis.  They never remove jobs from an unfiltered release
queue.  An unknown suite is an error so a new gate must be assigned deliberately.
"""
import argparse
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
LAYERS = ("contract", "stage", "pipeline", "platform")

# Single declaration, format, inventory, and runner contracts.  They need no
# whole-program behavior to establish their claim.
CONTRACT = set("""
tools finite-template decision-ledger dsl-ops modelbenchcheck bound qprefix
pptruth package-footer ape-version proc-enum seedpy ledgercheck freezecheck
revivedscan facts-export docs tapebin-roundtrip tapebin-shape tape-reader
script-inventory subtract-safety subtract-safety-selftest c99-ledger
publish-order front-bounds gate-infra gate-layers tsv-build-account pipeline-cache
lib-bindings-registry
""".split())
CONTRACT_PREFIX = ("manifest-entries-", "fresh-order-")

# Target execution, native ABI, or a cross-target image is part of the claim.
PLATFORM = set("""
c99 apps-real realprog hosthdr syscall6 winposix ccinterop elfobj
nativeboot windows-resolver-host
""".split())
PLATFORM_PREFIX = ("bigclosure-", "nativeboot-", "fat-", "ffi-", "exec-mach", "exec-win",
                   "exec-pe", "exec-arm", "exec-bind", "exec-container",
                   "seed-matrix-lnx-", "seed-matrix-osx-", "seed-matrix-win-")

# A complete compiler route, probe corpus, or shipped product is exercised.
PIPELINE = set("""
declshape volatile-comma malloc asmtext combo forward forward-multi
linkunits staticinit tagforward parserbounds formatonce staticunits cli
ccparity run multi hostile source target kernel source-layout
""".split())
PIPELINE_PREFIX = ("stages-", "corpus-", "difftest-", "difftest_o-", "csmithdiff-",
                   "ccrun-", "opt-run-", "closure-", "declmatrix-", "com-",
                   "strconvert-", "exec-chain-", "exec-e3self", "exec-e4self",
                   "exec-self", "exec-winx86self", "exec-formats")

# A single delta, builder stage, or library subsystem is checked in isolation.
STAGE = set("""
strconvert-host libneed apps-structure seed-construct seedfacts seedparse2-1 seedparse2-2 seed-matrix-shared
seed-matrix-object exec-bridge-linux warn diag target-package
""".split())
STAGE_PREFIX = ("exec-", "lib-", "rowcov-", "seed-construct-", "seed-matrix-features-",
                "warn-", "diag-", "referee-", "shared-e2-")


def layer(name):
    if name in CONTRACT or name.startswith(CONTRACT_PREFIX):
        return "contract"
    if name in PLATFORM or name.startswith(PLATFORM_PREFIX):
        return "platform"
    if name in PIPELINE or name.startswith(PIPELINE_PREFIX):
        return "pipeline"
    if name in STAGE or name.startswith(STAGE_PREFIX):
        return "stage"
    raise ValueError("unclassified gate suite: " + name)


def select(jobs, exact=None, through=None):
    """Preserve gate order while selecting one layer or an increasing prefix."""
    if exact is not None and through is not None:
        raise ValueError("choose one layer selector")
    if exact is None and through is None:
        return jobs
    chosen = {exact} if exact is not None else set(LAYERS[:LAYERS.index(through) + 1])
    return {name: command for name, command in jobs.items() if layer(name) in chosen}


def gate_names(com=True):
    env = dict(os.environ, TERM_SH="0")
    args = ["sh", "tests/gate.sh", "--list"] + (["--com"] if com else [])
    out = subprocess.check_output(args, cwd=ROOT, env=env, timeout=10, text=True)
    names = out.splitlines()
    if not names or len(names) != len(set(names)):
        raise ValueError("empty or duplicate gate suite list")
    return names


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--check", action="store_true", help="audit every base and product suite")
    ap.add_argument("--list", action="store_true", help="print name and layer")
    args = ap.parse_args()
    if not args.check and not args.list:
        ap.error("choose --check or --list")
    all_names = gate_names()
    base = gate_names(False)
    if all_names[:len(base)] != base:
        raise ValueError("--com gate list must extend the base list")
    labels = [(name, layer(name)) for name in all_names]
    if args.check:
        inventory = dict.fromkeys(all_names)
        assert list(select(inventory, through="platform")) == all_names
        assert set().union(*(set(select(inventory, exact=k)) for k in LAYERS)) == set(all_names)
        assert sum(len(select(inventory, exact=k)) for k in LAYERS) == len(all_names)
        assert list(select(dict.fromkeys(base), through="platform")) == base
        try:
            layer("new-suite-without-reviewed-layer")
        except ValueError:
            pass
        else:
            raise AssertionError("new gate suites must require classification")
    if args.list:
        for name, kind in labels:
            print(kind + "\t" + name)
    if args.check:
        counts = {kind: sum(label == kind for _, label in labels) for kind in LAYERS}
        print("gate layers: %d base, %d product-inclusive; %s" %
              (len(base), len(all_names), ", ".join("%s %d" % (k, v) for k, v in counts.items())))


if __name__ == "__main__":
    main()
