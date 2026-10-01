#!/usr/bin/env python3
"""Text tape reader bounds and its conservative prune/binary interfaces."""

import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from unisa.tapebin import encode


def run(args, **kwargs):
    return subprocess.run(args, cwd=ROOT, capture_output=True, timeout=10, **kwargs)


def main():
    valid = {
        "linkage": ".global\t_start\n.extern external.name\n.global payload\n.str payload \"x\"\n_start:\n  imm r0, 0\n  .exit r0\n",
        "minimal": "_start:\n  imm r0, 0\n  .exit r0\n",
        "data": '.str payload "A\\x00"\n.bss zeros 8\n_start:\n  imm r0, 0\n  .exit r0\n',
        "comments": '.str payload "a;b" ; outside string\n_start: ; entry\n  imm r0, 0 ; value\n  .exit r0\n',
        "crlf": "_start:\r\n  imm r0, 0\r\n  .exit r0\r\n",
        "signed-min": "_start:\n  imm r0, -9223372036854775808\n  .exit r0\n",
        "unsigned-max": "_start:\n  imm r0, 18446744073709551615\n  .exit r0\n",
    }
    invalid = {
        "linkage-missing": ".global\n",
        "linkage-extra": ".extern foo bar\n",
        "linkage-name": ".global 12bad\n",
        "empty-hex": "_start:\n  imm r0, 0x\n  .exit r0\n",
        "junk-number": "_start:\n  imm r0, 12q\n  .exit r0\n",
        "overflow": "_start:\n  imm r0, 18446744073709551616\n  .exit r0\n",
        "negative-overflow": "_start:\n  imm r0, -9223372036854775809\n  .exit r0\n",
        "bad-register": "_start:\n  imm r9, 1\n  .exit r0\n",
        "missing-operand": "_start:\n  imm r0\n  .exit r0\n",
        "extra-operand": "_start:\n  imm r0, 1, r2\n  .exit r0\n",
        "short-bss": ".bss x\n_start:\n  .exit r0\n",
        "short-string": ".str x\n_start:\n  .exit r0\n",
        "unterminated-string": '.str x "abc\n_start:\n  .exit r0\n',
        "bad-string-hex": '.str x "\\xGG"\n_start:\n  .exit r0\n',
        "string-capacity": '.str x "' + 'a' * 1050000 + '"\n_start:\n  .exit r0\n',
    }
    matrix = {}
    rows = (ROOT / "tests/tapereader.matrix.tsv").read_text().splitlines()
    if rows[2] != "name\tbackend\tprune\ttapebin":
        raise AssertionError("tape reader matrix header changed")
    for row in rows[3:]:
        name, backend, prune_status, binary = row.split("\t")
        if name in matrix:
            raise AssertionError("duplicate tape reader matrix row: " + name)
        matrix[name] = (backend, prune_status, binary)
    if set(matrix) != set(valid) | set(invalid):
        raise AssertionError("tape reader matrix does not cover all probes")
    def check_encoder(name, text):
        try:
            encode(text)
            status = "accept"
        except (AssertionError, ValueError):
            status = "reject"
        if status != matrix[name][2]:
            raise AssertionError(name + ": tapebin text encoder contract changed")
    with tempfile.TemporaryDirectory(prefix="unisacc-tapereader-") as tmp:
        base = pathlib.Path(tmp)
        ref = base / "ref"
        subprocess.run([str(ROOT / "tests/build_ref.sh"), str(base / "ref.c"), str(ref)],
                       cwd=ROOT, check=True, timeout=30)
        prune = base / "prune"
        subprocess.run(["cc", "-O2", "-std=c99", "-o", str(prune),
                        str(ROOT / "tests/tapeprune_harness.c")], check=True, timeout=20)
        adapter_c = base / "adapter.c"
        adapter = base / "adapter"
        adapter_c.write_text('#define UNISA_RUNTIME_LIBRARY\n#include "' + str(ROOT / 'exec/c/run.c') + '"\n#include "' + str(ROOT / 'exec/c/tapebin.h') + '"\n#include "' + str(ROOT / 'exec/c/tapebin_emit.h') + '"\nint main(void){Buf b={0};int c;while((c=getchar())!=EOF)bput(&b,c,0);Buf t={0};if(tbc_decode(b.b,b.n,0,0,&t))return 1;if(tbc_encode_product(&t,0))return 2;return fwrite(t.b,1,t.n,stdout)==(size_t)t.n?0:3;}\n')
        subprocess.run(["cc", "-O2", "-o", str(adapter), str(adapter_c)], check=True, timeout=20)
        for name, text in valid.items():
            if matrix[name][0] != "accept":
                raise AssertionError(name + ": backend matrix status changed")
            check_encoder(name, text)
            plain = base / (name + ".tape")
            binary = base / (name + ".tapebin")
            plain.write_bytes(text.encode("ascii"))
            binary.write_bytes(encode(text))
            if name == "linkage":
                encoded = base / (name + ".c.tapebin")
                result = run([str(ref), str(plain), "--tapebin", "-o", str(encoded)])
                if result.returncode or encoded.read_bytes() != binary.read_bytes():
                    raise AssertionError(name + ": C/Python encoder differs")
                result = run([str(adapter)], input=binary.read_bytes())
                if result.returncode or result.stdout != binary.read_bytes():
                    raise AssertionError("product adapter linkage roundtrip differs")
            images = []
            for source in (plain, binary):
                image = base / (name + (".text.elf" if source == plain else ".binary.elf"))
                result = run([str(ref), str(source), "-b", "lnx/x86_64", "-o", str(image)])
                if result.returncode or not image.is_file():
                    raise AssertionError((name, source.suffix, result.returncode, result.stderr[:120]))
                images.append(image.read_bytes())
            if images[0] != images[1]:
                raise AssertionError(name + ": text/binary image differs")
            pruned = run([str(prune)], input=plain.read_bytes())
            # The conservative pruner declines unsigned immediates above
            # LONG_MAX; the reader still accepts their 64-bit bit pattern.
            expected = b"P" if matrix[name][1] == "pass" else b"R"
            if pruned.returncode or pruned.stdout[:1] != expected:
                raise AssertionError(name + ": unexpected prune decision")
        for name, text in invalid.items():
            if matrix[name][0] != "reject":
                raise AssertionError(name + ": backend matrix status changed")
            check_encoder(name, text)
            plain = base / (name + ".tape")
            plain.write_bytes(text.encode("ascii"))
            if name.startswith("linkage-"):
                rejected = run([str(ref), str(plain), "--tapebin", "-o", str(base / "bad.tapebin")])
                if rejected.returncode != 1:
                    raise AssertionError(name + ": C encoder accepted malformed linkage")
            result = run([str(ref), str(plain), "-b", "lnx/x86_64", "-o", str(base / (name + ".elf"))])
            if result.returncode != 1 or b"back end: malformed tape" not in result.stderr:
                raise AssertionError((name, result.returncode, result.stderr[:160]))
            pruned = run([str(prune)], input=plain.read_bytes())
            if matrix[name][1] != "pass" or pruned.returncode or pruned.stdout != b"P" + plain.read_bytes():
                raise AssertionError(name + ": malformed tape was changed by prune")
    print("tape reader: %d text/binary image pairs, %d malformed tapes rejected; prune preserves malformed input" %
          (len(valid), len(invalid)))


if __name__ == "__main__":
    main()
