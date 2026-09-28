#!/usr/bin/env python3
"""R9 conversions: private host oracle and independent LP64 expectations.
--emit-probe writes a normal subset C probe; --check-output checks route output.
"""
import argparse
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# text, base, signed result, unsigned result, end offset, overflow flags
CASES = [
    ("", 10, 0, 0, 0, 0, 0), ("  +", 10, 0, 0, 0, 0, 0),
    ("  -z", 36, -35, (1 << 64) - 35, 4, 0, 0),
    ("0x", 0, 0, 0, 1, 0, 0), ("-0Xq", 16, 0, 0, 2, 0, 0),
    ("0x1f!", 0, 31, 31, 4, 0, 0), ("0779", 0, 63, 63, 3, 0, 0),
    ("\t -42tail", 10, -42, (1 << 64) - 42, 5, 0, 0),
    ("9223372036854775807", 10, (1 << 63) - 1, (1 << 63) - 1, 19, 0, 0),
    ("9223372036854775808!", 10, (1 << 63) - 1, 1 << 63, 19, 1, 0),
    ("-9223372036854775808", 10, -(1 << 63), 1 << 63, 20, 0, 0),
    ("-9223372036854775809!", 10, -(1 << 63), (1 << 63) - 1, 20, 1, 0),
    ("18446744073709551615", 10, (1 << 63) - 1, (1 << 64) - 1, 20, 1, 0),
    ("18446744073709551616x", 10, (1 << 63) - 1, (1 << 64) - 1, 20, 1, 1),
    ("-18446744073709551615", 10, -(1 << 63), 1, 21, 1, 0),
    ("-18446744073709551616", 10, -(1 << 63), (1 << 64) - 1, 21, 1, 1),
    ("ffffffffffffffff!", 16, (1 << 63) - 1, (1 << 64) - 1, 16, 1, 0),
    ("10000000000000000", 16, (1 << 63) - 1, (1 << 64) - 1, 17, 1, 1),
    ("999999999999999999999999999999999999999999999999!", 10,
     (1 << 63) - 1, (1 << 64) - 1, 48, 1, 1),
    ("z!", 36, 35, 35, 1, 0, 0), ("0b10", 0, 0, 0, 1, 0, 0),
]


# Exercise carry/cutoff in every supported base, with independent integer math.
def digits(n, base):
    text = ""
    while n:
        text = "0123456789abcdefghijklmnopqrstuvwxyz"[n % base] + text
        n //= base
    return text or "0"


for base in range(2, 37):
    for magnitude in ((1 << 63)-1, 1 << 63, (1 << 63)+1,
                      (1 << 64)-1, 1 << 64):
        for neg in (False, True):
            text = ("-" if neg else "") + digits(magnitude, base)
            value = -magnitude if neg else magnitude
            signed = min((1 << 63)-1, max(-(1 << 63), value))
            unsigned = ((-magnitude if neg else magnitude) % (1 << 64)
                        if magnitude < 1 << 64 else (1 << 64)-1)
            CASES.append((text + "!", base, signed, unsigned, len(text),
                          int(signed != value), int(magnitude >= 1 << 64)))


def probe():
    import json
    lines = ['#include <stdio.h>', '#include <stdlib.h>', '#include <errno.h>',
             '#include <limits.h>', 'int main(void) { char *e; const char *s; long v; unsigned long u;']
    for s, base, *_ in CASES:
        lines += ['s = ' + json.dumps(s) + '; errno = 7;',
                  'v = strtol(s, &e, %d);' % base,
                  'printf("%ld %ld %d\\n", v, (long)(e-s), errno == ERANGE ? 1 : 0);',
                  'errno = 7; u = strtoul(s, &e, %d);' % base,
                  'printf("%lu %ld %d\\n", u, (long)(e-s), errno == ERANGE ? 1 : 0);']
    # atol overflow is undefined: only representable inputs are checked.
    lines += ['printf("%ld %ld %ld\\n", atol("-9223372036854775808"), atol("9223372036854775807"), atol("  +42tail"));',
              'errno = 7; v = strtol("12", 0, 10); u = strtoul("12", 0, 10);',
              'printf("%ld %lu %d\\n", v, u, errno); return 0; }']
    return "\n".join(lines) + "\n"


def expected():
    lines = []
    for _, _, s, u, end, se, ue in CASES:
        lines += [f"{s} {end} {se}", f"{u} {end} {ue}"]
    lines += [f"{-1 << 63} {(1 << 63)-1} 42", "12 12 7"]
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit-probe", type=Path)
    ap.add_argument("--check-output", type=Path)
    args = ap.parse_args()
    if args.emit_probe:
        args.emit_probe.write_text(probe())
        return 0
    if args.check_output:
        assert args.check_output.read_text() == expected(), "route output differs"
        print(f"strconvert: route {len(CASES)} cases and atol/null-end passed")
        return 0
    block = (ROOT / "include/stdlib.h").read_text().split("/* strtol/strtoul/strtod:", 1)[1]
    block = "/* strtol/strtoul/strtod:" + block.split("/* The fractional part", 1)[0]
    with tempfile.TemporaryDirectory(prefix="unisacc-r9-convert-") as tmp:
        tmp = Path(tmp)
        for kind in ("oracle", "candidate"):
            src = probe()
            if kind == "candidate":
                src = src.replace('#include <stdlib.h>', block)
            path = tmp / (kind + ".c")
            path.write_text(src)
            subprocess.run(["cc", "-std=c99", "-include", "errno.h", "-include", "limits.h",
                            "-fsanitize=undefined", "-o", str(tmp/kind), str(path)], check=True, timeout=20)
            out = subprocess.run([str(tmp/kind)], capture_output=True, text=True, check=True, timeout=10)
            if out.stdout != expected() or out.stderr:
                print(kind + " failed")
                for n, (got, want) in enumerate(zip(out.stdout.splitlines(), expected().splitlines())):
                    if got != want:
                        print(f"row {n}: got {got}; want {want}")
                print(out.stderr)
                return 1
    print(f"strconvert: host oracle/candidate {len(CASES)} cases and atol/null-end passed (UBSan)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
