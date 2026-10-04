#!/usr/bin/env python3
"""V1: observable memory trace, not merely agreement of two tapes.

The defined global object must be read six times: scalar and pointer comma-left, bare
scalar and pointer statements, and for initialisation and step. Address-of and sizeof must not read it. The independent system
compiler's optimised assembly must contain a read of that object as well.
UA must identify a freshly built reference; --com uses MODEL_COM additionally.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from unisa.tape import parse
from unisa.vm import VM


def run(argv):
    p = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=20)
    if p.returncode:
        raise AssertionError((argv, p.returncode, p.stderr.decode(errors="replace")))
    return p.stdout


class TraceVM(VM):
    def __init__(self, tape):
        super().__init__(tape, max_steps=10000)
        self.observed = tape.syms["g_observed"]
        self.reads = []
        self.code_reads = []
        self.designator = tape.labels.get("designator")

    def ld(self, addr, width, signed=True):
        if self.designator is not None and addr == self.designator:
            self.code_reads.append((addr, width))
        if addr < self.observed + 4 and self.observed < addr + width:
            self.reads.append((addr - self.observed, width))
        return super().ld(addr, width, signed)


def main():
    source = ROOT / "tests/c/a_volatile_comma.c"
    cc = os.environ.get("CC", "cc")
    with tempfile.TemporaryDirectory(prefix="v1-reads-") as d:
        asm = Path(d) / "system.s"
        run([cc, "-w", "-O2", "-S", str(source), "-o", str(asm)])
        # AArch64 uses a page-relative load, x86 uses a RIP-relative mov.
        loads = [line for line in asm.read_text().splitlines()
                 if "observed" in line and re.search(r"\b(?:ldr\w*|ldur\w*|mov\w*)\s", line)]
        assert loads, "system compiler produced no observed-object load; unsupported assembly syntax"
    compilers = [os.environ["UA"]]
    if "--com" in sys.argv:
        compilers.append(os.environ["MODEL_COM"])
    count = 0
    for compiler in compilers:
        for level in (0, 1, 2):
            prefix = ["sh", compiler] if compiler.endswith(".com") and os.name != "nt" else [compiler]
            text = run(prefix + [str(source), "-S", "-o", "-", "-O" + str(level)])
            tape = parse(text.decode("latin1"))
            vm = TraceVM(tape)
            result = vm.run()
            assert vm.reads == [(0, 4)] * 6 and not vm.code_reads and result[1] == 0 and not result[0], (compiler, level, vm.reads, result)
            count += 1
    if "--e3-tape" in sys.argv:
        path = Path(sys.argv[sys.argv.index("--e3-tape") + 1])
        vm = TraceVM(parse(path.read_text()))
        vm.run()
        assert vm.reads == [(0, 4)] * 6, (str(path), vm.reads)
        count += 1
    print("volatile comma: system-cc read; %d traces have exactly six reads (O0/O1/O2)" % count)


if __name__ == "__main__":
    main()
