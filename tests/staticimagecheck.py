#!/usr/bin/env python3
"""staticimagecheck.py COM [--selftest] -- the Linux compiler images inside a product are static.

A product (.com) carries one compressed compiler image per POSIX target.  The Linux ones must be
static ELF: no PT_INTERP and no PT_DYNAMIC.  Such an image starts on any Linux kernel of its
architecture, whatever libc the system has; a dynamic one needs that system's loader and libc
before it can print a usage line.  0.0.42 shipped a candidate whose Linux images had silently
become dynamic (a loader import in src/host_dl.h); the behaviour gate could not see it, because
every gate host has glibc.  This reads program headers only: it does not run the product, so it
answers in milliseconds, before a gate run is spent on the candidate.

Exit 0: every Linux image found is static, and both architectures were found.
Exit 1: a Linux image is dynamic, or one of the two architectures is missing (nothing checked is
not a pass), or the file cannot be read as a product.
"""
import struct, sys, zlib

PT_DYNAMIC, PT_INTERP = 2, 3
MACHINES = {62: 'x86_64', 183: 'aarch64'}


def members(data):
    """Every gzip member in the file that inflates completely, as (offset, bytes)."""
    at = 0
    while True:
        at = data.find(b'\x1f\x8b\x08', at)
        if at < 0:
            return
        try:
            stream = zlib.decompressobj(31)
            out = stream.decompress(data[at:])
            if not stream.eof:
                raise zlib.error('truncated')
        except zlib.error:
            at += 1
            continue
        yield at, out
        at += max(len(data) - at - len(stream.unused_data), 1)


def program_types(image):
    """Program header types of a little-endian ELF64 image, or None if it is not one."""
    if len(image) < 64 or image[:4] != b'\x7fELF' or image[4] != 2 or image[5] != 1:
        return None
    phoff = struct.unpack_from('<Q', image, 32)[0]
    phentsize, phnum = struct.unpack_from('<HH', image, 54)
    if phentsize < 56 or phoff + phentsize * phnum > len(image):
        raise ValueError('program headers outside the image')
    return [struct.unpack_from('<I', image, phoff + k * phentsize)[0] for k in range(phnum)]


def check(data):
    """(lines, bad): one line per Linux image, and whether the product fails."""
    lines, bad, seen = [], False, set()
    for at, image in members(data):
        try:
            types = program_types(image)
        except ValueError as error:
            lines.append('  FAIL member at %d: %s' % (at, error)); bad = True
            continue
        if types is None:
            continue
        machine = MACHINES.get(struct.unpack_from('<H', image, 18)[0])
        if machine is None:
            continue
        seen.add(machine)
        dynamic = [name for kind, name in ((PT_INTERP, 'PT_INTERP'), (PT_DYNAMIC, 'PT_DYNAMIC')) if kind in types]
        if dynamic:
            lines.append('  FAIL lnx/%s image at %d is dynamic: %s' % (machine, at, ' '.join(dynamic))); bad = True
        else:
            lines.append('  ok   lnx/%s image at %d: static, %d program headers' % (machine, at, len(types)))
    for machine in sorted(set(MACHINES.values()) - seen):
        lines.append('  FAIL no lnx/%s image found' % machine); bad = True
    return lines, bad


def elf(machine, types):
    """A minimal ELF64 header with the given program header types (self-test fixture)."""
    head = bytearray(64)
    head[:6] = b'\x7fELF\x02\x01'
    struct.pack_into('<H', head, 18, machine)
    struct.pack_into('<Q', head, 32, 64)
    struct.pack_into('<HH', head, 54, 56, len(types))
    return bytes(head) + b''.join(struct.pack('<I', t) + bytes(52) for t in types)


def selftest():
    import gzip
    pack = lambda *images: b'MZ-fixture' + b''.join(gzip.compress(i) + b'pad' for i in images)
    static = pack(elf(62, [1, 1]), elf(183, [1, 1]), b'\xcf\xfa\xed\xfe' + bytes(60))
    assert check(static)[1] is False, 'two static Linux images must pass'
    for name, data in (('PT_INTERP', pack(elf(62, [1, 3, 1]), elf(183, [1, 1]))),
                       ('PT_DYNAMIC', pack(elf(62, [1, 1]), elf(183, [1, 2]))),
                       ('one architecture missing', pack(elf(62, [1, 1]))),
                       ('no image at all', b'MZ-fixture' + bytes(100)),
                       ('headers outside the image', pack(elf(62, [1, 1])[:80], elf(183, [1, 1])))):
        assert check(data)[1] is True, name + ' must fail'
    print('staticimagecheck: static pass; PT_INTERP, PT_DYNAMIC, missing architecture, no image and truncated headers fail')
    return 0


def main(argv):
    if argv == ['--selftest']:
        return selftest()
    if len(argv) != 1:
        print(__doc__.split('\n')[0], file=sys.stderr)
        return 2
    try:
        data = open(argv[0], 'rb').read()
    except FileNotFoundError:
        print('staticimagecheck: no such product: ' + argv[0]); return 1
    lines, bad = check(data)
    print('\n'.join(lines))
    print('staticimagecheck  %s  %s' % (argv[0], 'FAILED' if bad else 'linux images static'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
