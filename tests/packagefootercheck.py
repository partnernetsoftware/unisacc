#!/usr/bin/env python3
"""Private frozen locator probe, synthetic corruptions, optional signed samples.
Every compiler/process call has a <= 45 second timeout; no shared caches.
"""
import argparse
import importlib.util
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("apeinspect", ROOT/"release/apeinspect.py")
APE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(APE)
PROBE = r'''#include <stdio.h>
#include <stdlib.h>
#include "packagefooter.h"
static void dirty(PackageFooter *p) {
    p->file_size = -1; p->footer_offset = -1; p->package_offset = -1;
    p->package_length = -1; p->certificate_offset = -1;
    p->certificate_length = -1; p->padding = -1;
}
static int result(int valid, PackageFooter *p) {
    if (valid) {
        printf("1 %ld %ld %ld %ld %ld %ld %d\n", p->file_size,
               p->footer_offset, p->package_offset, p->package_length,
               p->certificate_offset, p->certificate_length, p->padding);
    } else {
        if (p->file_size || p->footer_offset || p->package_offset ||
            p->package_length || p->certificate_offset ||
            p->certificate_length || p->padding) return 0;
        printf("0\n");
    }
    return 1;
}
int main(int argc, char **argv) {
    int i, valid; FILE *f; PackageFooter p;
    long size; unsigned char *data;
    i = 1;
    while (i < argc) {
        f = fopen(argv[i], "rb");
        if (!f) return 2;
        dirty(&p);
        valid = packagefooter_locate(f, &p);
        if (!result(valid, &p)) return 3;
        if (fseek(f, 0, SEEK_END)) return 4;
        size = ftell(f); if (size < 0) return 4;
        data = malloc(size ? size : 1); if (!data) return 4;
        if (fseek(f, 0, SEEK_SET)) return 4;
        if ((long)fread(data, 1, size, f) != size) return 4;
        fclose(f);
        dirty(&p);
        valid = packagefooter_locate_bytes(data, size, &p);
        if (!result(valid, &p)) return 3;
        free(data); i = i + 1;
    }
    dirty(&p);
    if (packagefooter_locate_bytes(0, 100, &p) || p.file_size ||
        p.footer_offset || p.package_offset || p.package_length ||
        p.certificate_offset || p.certificate_length || p.padding) return 5;
    dirty(&p);
    if (packagefooter_locate_bytes(0, -1, &p) || p.file_size ||
        p.footer_offset || p.package_offset || p.package_length ||
        p.certificate_offset || p.certificate_length || p.padding) return 5;
    dirty(&p);
    if (packagefooter_locate(0, &p) || p.file_size || p.footer_offset ||
        p.package_offset || p.package_length || p.certificate_offset ||
        p.certificate_length || p.padding) return 5;
    return 0;
}
'''


def fixture(padding=0, pe64=True):
    optsize = 240 if pe64 else 224
    b = bytearray(512)
    b[:2] = b"MZ"
    struct.pack_into("<I", b, 60, 64)
    b[64:68] = b"PE\0\0"
    struct.pack_into("<HH", b, 68, 0x8664, 1)
    struct.pack_into("<H", b, 84, optsize)
    struct.pack_into("<H", b, 88, 0x20b if pe64 else 0x10b)
    struct.pack_into("<I", b, 148, 512)
    dirs = 112 if pe64 else 96
    struct.pack_into("<I", b, 88+dirs-4, 16)
    payload = b"P 3 private package" + b"x"*((padding-len(b"P 3 private package")) % 8)
    b += payload+b"UNIPKG1\n"+struct.pack("<Q", len(payload))
    if padding:
        # Choose payload length so requested zero padding aligns cert start.
        extra = ((-len(b)-padding) % 8)
        payload += b"y"*extra
        b = b[:512]+payload+b"UNIPKG1\n"+struct.pack("<Q", len(payload))
    else:
        extra = (-len(b)) % 8
        payload += b"y"*extra
        b = b[:512]+payload+b"UNIPKG1\n"+struct.pack("<Q", len(payload))
    unsigned = bytes(b)
    cert = len(b)+padding
    b += bytes(padding)+struct.pack("<IHH", 13, 0x200, 2)+b"abcde"+bytes(3)
    struct.pack_into("<II", b, 88+dirs+32, cert, 16)
    return unsigned, bytes(b), cert, 88+dirs+32


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, help="private root.com snapshot")
    parser.add_argument("--include", type=Path, help="frozen subset headers (required with compiler)")
    parser.add_argument("--sample", action="append", type=Path, default=[])
    args = parser.parse_args()
    cases = []
    def add(name, b, valid): cases.append((name, bytes(b), valid))
    def corrupt(name, b, off, fmt, value):
        b = bytearray(b); struct.pack_into(fmt, b, off, value); add(name, b, False)
    add("legacy", b"stubP 1"+b"UNIPKG1\n"+struct.pack("<Q", 3), True)
    for pe64 in (True, False):
        for padding in range(8):
            u, s, c, d = fixture(padding, pe64)
            add(f"unsigned-{pe64}-{padding}", u, True)
            add(f"signed-{pe64}-{padding}", s, True)
            if padding: corrupt(f"padding-{pe64}-{padding}", s, c-1, "B", 1)
    u, s, c, d = fixture(1)
    for n in range(0, 70): add(f"tiny-{n}", s[:n], False)
    for n in range(1, 17): add(f"truncated-{n}", s[:-n], False)
    for name, off, fmt, value in [
        ("pe-offset", 60, "<I", 0xffffffff), ("pe-low", 60, "<I", 1),
        ("optional-short", 84, "<H", 100), ("optional-long", 84, "<H", 65535),
        ("bad-magic", 88, "<H", 0x999), ("dirs-short", 196, "<I", 4),
        ("dirs-long", 196, "<I", 17), ("section-count", 70, "<H", 65535),
        ("header-extent", 148, "<I", 1), ("cert-offset", d, "<I", 0xffffffff),
        ("cert-align", d, "<I", c-1), ("cert-size", d+4, "<I", 17),
        ("cert-zero", d+4, "<I", 0), ("cert-length-small", c, "<I", 7),
        ("cert-length-large", c, "<I", 0xffffffff), ("revision", c+4, "<H", 0x100),
        ("cert-type", c+6, "<H", 1), ("cert-pad", c+15, "B", 1),
        ("package-zero", c-9, "<Q", 0), ("package-overflow", c-9, "<Q", (1<<64)-1),
        ("package-overlap-headers", c-9, "<Q", c),
    ]: corrupt(name, s, off, fmt, value)
    add("double-footer", s+b"UNIPKG1\n"+struct.pack("<Q", 4), False)
    add("trailing-data", s+b"x", False)
    # Valid certificate chain, and a malformed second entry.
    chain = bytearray(s+struct.pack("<IHH", 8, 0x200, 2))
    struct.pack_into("<I", chain, d+4, 24)
    add("two-certificates", chain, True)
    corrupt("bad-second-cert", chain, c+16, "<I", 9)
    for path in args.sample: add("sample-"+str(len(cases)), path.read_bytes(), True)
    with tempfile.TemporaryDirectory(prefix="unisacc-footer-") as td:
        tmp = Path(td)
        if args.compiler and not args.include:
            parser.error("--compiler requires --include frozen headers")
        if args.include:
            shutil.copytree(args.include.resolve(), tmp/"include")
        shutil.copy2(ROOT/"exec/c/packagefooter.h", tmp/"packagefooter.h")
        (tmp/"probe.c").write_text(PROBE)
        commands = [("host-asan-ubsan", ["cc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                     "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"])]
        if args.compiler:
            compiler = tmp/"compiler.com"
            shutil.copy2(args.compiler.resolve(), compiler)
            commands.append(("rootcom-frozen", ["/bin/bash", str(compiler), "-nostdinc", "-I", str(tmp/"include")]))
        paths, expected = [], []
        keys = ("file_size", "footer_offset", "package_offset", "package_length",
                "certificate_offset", "certificate_length", "padding")
        for i, (name, data, valid) in enumerate(cases):
            path = tmp/f"case-{i}"
            path.write_bytes(data); paths.append(str(path))
            try: result = APE.inspect(data)
            except ValueError:
                assert not valid, name
                expected.append("0")
            else:
                assert valid, name
                expected.append("1 "+" ".join(str(result[k]) for k in keys))
        for name, command in commands:
            binary = tmp/name
            built = subprocess.run(command+[str(tmp/"probe.c"), "-o", str(binary)],
                           cwd=tmp, capture_output=True, text=True, timeout=45)
            assert built.returncode == 0, (name, built.stdout, built.stderr)
            run = subprocess.run([str(binary), *paths], check=True, capture_output=True,
                                 text=True, timeout=45)
            actual = run.stdout.splitlines()
            assert len(actual) == 2*len(expected), (name, len(actual), len(expected), run.stderr)
            for i, want in enumerate(expected):
                for api, got in zip(("FILE", "bytes"), actual[2*i:2*i+2]):
                    assert got == want, (name, api, cases[i][0], got, want)
            print(f"{name}: {len(cases)} cases passed for both APIs; null/negative guards passed")


if __name__ == "__main__":
    main()
