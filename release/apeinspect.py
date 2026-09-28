#!/usr/bin/env python3
"""Inspect bounded APE package/certificate extents; does not authenticate signatures."""
import argparse
import hashlib
import json
from pathlib import Path


def inspect(data):
    size = len(data)
    def span(off, n):
        if off < 0 or n < 0 or off > size or n > size - off:
            raise ValueError("extent outside file")
        return data[off:off+n]
    def number(off, n):
        return int.from_bytes(span(off, n), "little")
    cert = certlen = minimum = 0
    if size < 16:
        raise ValueError("short file")
    if span(0, 2) == b"MZ":
        pe = number(60, 4)
        if pe < 64 or span(pe, 4) != b"PE\0\0":
            raise ValueError("invalid PE offset/header")
        sections, optsize = number(pe+6, 2), number(pe+20, 2)
        opt = pe+24
        span(opt, optsize)
        magic = number(opt, 2)
        dirs = {0x10b: 96, 0x20b: 112}.get(magic)
        if not dirs or optsize < dirs:
            raise ValueError("invalid optional header")
        count = number(opt+dirs-4, 4)
        if count < 5 or count > (optsize-dirs)//8:
            raise ValueError("invalid data directories")
        cert, certlen = number(opt+dirs+32, 4), number(opt+dirs+36, 4)
        if bool(cert) != bool(certlen) or (cert and (cert % 8 or certlen < 8 or cert+certlen != size)):
            raise ValueError("invalid security extent")
        sec = opt+optsize
        if not sections:
            raise ValueError("no sections")
        span(sec, sections*40)
        headers = number(opt+60, 4)
        if headers < sec+sections*40 or headers > size:
            raise ValueError("invalid header extent")
        minimum = headers
        for i in range(sections):
            rawlen, raw = number(sec+i*40+16, 4), number(sec+i*40+20, 4)
            if rawlen:
                if raw < headers:
                    raise ValueError("section overlaps headers")
                span(raw, rawlen)
                minimum = max(minimum, raw+rawlen)
        if cert and cert < minimum:
            raise ValueError("certificate overlaps sections")
    if cert:
        pos = cert
        while pos < size:
            length = number(pos, 4)
            if length < 8 or span(pos+4, 4) != b"\0\2\2\0":
                raise ValueError("invalid WIN_CERTIFICATE")
            span(pos, length)
            padding = (-length) % 8
            if any(span(pos+length, padding)):
                raise ValueError("nonzero certificate padding")
            pos += length+padding
        if pos != size:
            raise ValueError("invalid certificate table end")
    end = cert or size
    found = []
    for padding in range(8 if cert else 1):
        footer = end-padding-16
        if footer < minimum or any(span(end-padding, padding)):
            continue
        if span(footer, 8) == b"UNIPKG1\n":
            length = number(footer+8, 8)
            if not length or length > footer-minimum:
                raise ValueError("invalid package extent")
            found.append((footer, length, padding))
    if len(found) != 1:
        raise ValueError("missing or ambiguous package footer")
    footer, length, padding = found[0]
    package = span(footer-length, length)
    return dict(file_size=size, footer_offset=footer, package_offset=footer-length,
                package_length=length, certificate_offset=cert,
                certificate_length=certlen, padding=padding,
                sha256=hashlib.sha256(data).hexdigest(),
                package_sha256=hashlib.sha256(package).hexdigest(),
                authentication="not_checked")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args()
    results = []
    for path in args.files:
        try:
            result = inspect(path.read_bytes())
        except (OSError, ValueError) as exc:
            parser.exit(1, f"{path}: {exc}\n")
        results.append(dict(path=str(path), **result))
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
