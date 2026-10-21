#!/usr/bin/env python3
"""Semantic breadth of gate suites; elapsed time remains gatequeue's own estimate.

These layers guide diagnosis.  They never remove jobs from an unfiltered release
queue.  An unknown suite is an error so a new gate must be assigned deliberately.
"""
import argparse
import os
from pathlib import Path
from checklib import bound_cmd
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[1]
LAYERS = ("contract", "stage", "pipeline", "platform")

# Single declaration, format, inventory, and runner contracts.  They need no
# whole-program behavior to establish their claim.
CONTRACT = set("""
gate-select
identity-product-negative identity-include-negative identity-include-consumer-negative identity-prep-negative
reviewed-trees finite-template decision-ledger dsl-ops modelbenchcheck bound qprefix
pptruth package-footer ape-version proc-enum seedpy ledgercheck freezecheck
revivedscan facts-export docs tapebin-roundtrip tapebin-shape tape-reader envpin staticimage staticimage-selftest abi-manifest abi-manifest-selftest early-stage-seal-selftest pp-leaf-seal pp-leaf-seal-negative pp-leaf-capture-selftest lex-e1-seal opt-e4-seal middle-flow-e3 middle-flow-lower-accept middle-flow-ledger
script-inventory script-inventory-self inventory-derived subtract-safety subtract-safety-selftest c99-ledger
publish-order front-bounds gate-infra gate-layers tsv-build-account pipeline-cache
lib-bindings-registry
hostcheck seedmemory stagelog exittable h1source warmup gate-infra-38 installpair attemptchain c99precheck rulingwait namedrefuse checkrun commitgate whyrerun draftfetch draftguard precommithook k2b queuestart queuestart-strict queuestart-launch queuetimeout gatesummary observation gqalive queuetimer docedit
watchdog-orphan watchdog-eof watchdog-success
boundreason boundstall-r7g boundnested-1 boundnested-2 boundnested-3 boundnested-4 boundnested-5
stage-contract-e1e3
latebind-p6
resourceflow
include-key
gatequeue-symlink
""".split())   # 0.0.38 P1-P7 pipeline contracts
CONTRACT_PREFIX = ("manifest-entries-", "fresh-order-", "tapebin-roundtrip-", "tools-")

# Target execution, native ABI, or a cross-target image is part of the claim.
PLATFORM = set("""
c99 apps-real realprog hosthdr syscall6 winposix ccinterop elfobj minicon
nativeboot windows-resolver-host lifecycle luatests sqlspeed
""".split())
PLATFORM_PREFIX = ("bigclosure-", "host-triple-", "nativeboot-", "fat-", "ffi-", "exec-mach", "exec-win",
                   "exec-pe", "exec-arm", "exec-bind", "exec-container",
                   "seed-matrix-lnx-", "seed-matrix-osx-", "seed-matrix-win-")

# A complete compiler route, probe corpus, or shipped product is exercised.
PIPELINE = set("""
closure-c1a closure-c1b closure-c1c
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
exec-librarycallback-wiring-prepare
exec-librarycallsite-reader-model exec-librarycallsitelower-model-1 exec-librarycallsitelower-model-2 exec-librarycallsitelower-model-3 exec-librarycallsitelower-model-4 exec-librarytypedlower-model-1 exec-librarytypedlower-model-2 exec-librarytypedlower-model-3 exec-librarytypedlower-model-4 exec-libraryfn-model exec-librarydata-model exec-libraryfunctionimports-model exec-librarytypedreader-model exec-librarypartition-model strconvert-host libneed apps-structure seed-construct seedfacts seedparse2-1 seedparse2-2 seed-matrix-shared
seed-matrix-shared-prepare seed-matrix-shared-e2 seed-matrix-shared-e1 seed-matrix-shared-e3 seed-matrix-shared-e4 seed-matrix-shared-o1 seed-matrix-shared-prune seed-matrix-shared-nativeabi
seed-matrix-object seedgen seedgen-let-exit copy-contract ri-snapshot-overlay ri-snapshot-core seedape seedpack exec-bridge-linux warn diag target-package prevheaders
unisa-bits unisa-hostabi unisa-peversion unisa-padded unisa-zdata unisa-linalg unisa-libneed unisa-uns1
unisa-catalog unisa-fp unisa-tape unisa-tapebin unisa-tapebin-shape unisa-image-fat unisa-image-elf
unisa-image-macho unisa-image-pe unisa-ape unisa-vm unisa-gold-prec unisa-emit-arm-nwmem
unisa-gold-lex-label unisa-gold-type-label unisa-gold-binsel-label unisa-gold-opinfo-label
unisa-gold-peep-label unisa-gold-scope-label unisa-gold-pp-label unisa-gold-reloc-label
unisa-gold-irsel-label unisa-gold-parse-label
""".split())   # c42: the 0.0.41 K6 C-reference vs Python point-sample checks: one subsystem each, not a route or target
STAGE_PREFIX = ("tapecheck-model-", "exec-", "lib-", "rowcov-", "seed-construct-", "seed-matrix-features-",
                "warn-", "diag-", "referee-", "shared-e2-", "seedgen-")


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


def parse_stage_rows(raw, names, vocabulary):
    """Validate the sole C authority's stage/also wire, preserving gate order."""
    rows = raw.splitlines()
    if len(rows) != len(names):
        raise ValueError("stage-map incomplete gate coverage")
    result = {}
    for expected, row in zip(names, rows):
        fields = row.split("\t")
        if len(fields) != 3:
            raise ValueError("stage-map invalid column count")
        primary, name, wire = fields
        if name != expected or name in result or primary not in vocabulary:
            raise ValueError("stage-map invalid suite or primary: " + name)
        tags = [] if wire == "-" else wire.split(",")
        if (not wire or tags != sorted(set(tags)) or
                any(not tag or tag not in vocabulary or tag == primary for tag in tags)):
            raise ValueError("stage-map invalid also: " + name)
        result[name] = (primary, tuple(tags))
    return result


def stage_labels(names):
    """Compile and query stagemap.c; never duplicate its stage assignments."""
    with tempfile.TemporaryDirectory(prefix="stagemap-audit-") as tmp:
        binary = str(Path(tmp) / "stagemap")
        subprocess.run(bound_cmd(55,
                        "cc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                        str(ROOT / "tests/stagemap.c"), "-o", binary),
                       check=True, timeout=58)
        vocabulary = set(subprocess.check_output([binary, "--stages"], text=True, timeout=5).splitlines())
        raw = subprocess.check_output([binary, "--list"], input="\n".join(names) + "\n",
                                      text=True, timeout=5)
    return parse_stage_rows(raw, names, vocabulary)


def check_stage_wire():
    vocabulary = {"E3", "build", "meta"}
    parse_stage_rows("E3\tx\tbuild,meta\n", ["x"], vocabulary)
    for wire in ("", "build,build", "meta,build", "build,", "build, meta", "E3", "unknown"):
        try:
            parse_stage_rows("E3\tx\t" + wire + "\n", ["x"], vocabulary)
        except ValueError:
            continue
        raise AssertionError("invalid also wire accepted: " + repr(wire))


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
        check_stage_wire()
        stage_labels(all_names)
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
