#!/usr/bin/env python3
"""Batch independent zlib oracle; compile outside this runner, bounded runs."""
import argparse
import pathlib
import random
import subprocess
import zlib


def vectors():
    rng = random.Random(9009)
    cases = []
    def add(blob, size):
        d = zlib.decompressobj(-15)
        try:
            raw = d.decompress(blob) + d.flush()
            valid = d.eof and not d.unused_data and len(raw) == size
        except zlib.error:
            valid, raw = False, b''
        cases.append((blob, size, zlib.crc32(raw) if valid else None))
        return valid
    for size in (0, 1, 2, 7, 8, 9, 55, 56, 63, 64, 65, 511, 512, 513, 4095, 4096, 4097, 32768, 65535, 65536):
        raw = (b'abcdef012345' * (size//12+1))[:size]
        for level, strategy in ((0,0), (9,0), (9,zlib.Z_FIXED)):
            c = zlib.compressobj(level, zlib.DEFLATED, -15, strategy=strategy)
            blob = c.compress(raw) + c.flush()
            add(blob, size)
            for end in sorted({0, 1, len(blob)//2, len(blob)-1}):
                add(blob[:end], size)
            add(blob + b'X', size)
            add(blob, size+1)
            if size: add(blob, size-1)
            for _ in range(3):
                changed = bytearray(blob)
                pos = rng.randrange(len(blob)); changed[pos] ^= 1 << rng.randrange(8)
                add(bytes(changed), size)
    # Reserved block, oversubscribed code-length tree, distance before output.
    assert not add(b'\x07', 0)
    # Four one-bit symbols oversubscribe the dynamic code-length tree.
    over = 5 + sum(1 << k for k in (17,20,23,26))
    assert not add(over.to_bytes(4, 'little'), 0)
    assert not add(bytes.fromhex('030200'), 3)
    c = zlib.compressobj(9, zlib.DEFLATED, -15)
    raw = b'first block' * 100 + b'second block' * 300
    blob = c.compress(raw[:1100]) + c.flush(zlib.Z_SYNC_FLUSH)
    blob += c.compress(raw[1100:]) + c.flush()
    assert add(blob, len(raw))
    for _ in range(1000):
        add(rng.randbytes(rng.randrange(1,80)), rng.randrange(0,300))
    return cases


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('runners', nargs='+', type=pathlib.Path)
    args = ap.parse_args()
    cases = vectors()
    stream = b''.join(f'{len(b)} {n}\n'.encode() + b for b,n,_ in cases)
    for runner in args.runners:
        p = subprocess.run([str(runner.resolve())], input=stream, capture_output=True, timeout=45)
        if p.returncode or p.stderr:
            raise SystemExit(f'{runner}: rc {p.returncode}: {p.stderr.decode(errors="replace")}')
        rows = p.stdout.splitlines()
        if len(rows) != len(cases): raise SystemExit(f'{runner}: short output')
        for i, (line, (_,_,expected)) in enumerate(zip(rows,cases)):
            rc, crc = map(int,line.split())
            if (expected is None and rc == 0) or (expected is not None and (rc != 0 or crc != expected)):
                raise SystemExit(f'{runner}: vector {i}: {rc}/{crc}, expected {expected}')
        print(f'{runner}: {len(cases)} vectors; exact extent/CRC and red zones ok')
if __name__ == '__main__': main()
