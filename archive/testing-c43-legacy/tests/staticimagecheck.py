#!/usr/bin/env python3
"""staticimagecheck.py COM [--selftest] -- the Linux compiler images inside a product are static.

A product (.com) carries one compressed compiler image per POSIX target.  The Linux ones must be
static ELF: no PT_INTERP and no PT_DYNAMIC.  A dynamic image needs the system's loader and libc
before it can print a usage line; a static one does not.  (Structurally static is all this
shows: it does not prove the image starts on every kernel.)  0.0.42 shipped a candidate whose Linux images had silently
become dynamic (a loader import in src/host_dl.h); the behaviour gate could not see it, because
every gate host has glibc.  This reads program headers only: it does not run the product, so it
answers in milliseconds, before a gate run is spent on the candidate.

Exit 0: exactly one Linux image per architecture, each static with at least one PT_LOAD.
Exit 1: a Linux image is dynamic or has no loadable segment, an architecture is missing (nothing
checked is not a pass) or appears more than once (an extra member must not stand in for the
shipped one), or the file cannot be read as a product.
"""
import re, struct, sys, zlib

PT_LOAD, PT_DYNAMIC, PT_INTERP = 1, 2, 3
MACHINES = {62: 'x86_64', 183: 'aarch64'}


def members(data):
    """Formal Linux launch slots, never gzip magic outside the launch table."""
    if not data.startswith(b"MZqFpD='"):
        raise ValueError('not an APE launch script')
    end = data[:4096].find(b'\nesac\n')
    if end < 0:
        raise ValueError('missing launch table')
    script = data[:end + 6]
    if script.count(b'case "$u" in\n') != 1:
        raise ValueError('missing/duplicate platform switch')
    for line in script.splitlines():
        if line.lstrip(b' ').startswith(b'Linux') and not re.match(rb'^ *(Linuxx86_64|Linuxaarch64\|Linuxarm64)\) o=', line):
            raise ValueError('unknown Linux launch slot')
    slots = []
    for label, machine in ((b'Linuxx86_64', 62), (b'Linuxaarch64|Linuxarm64', 183)):
        pattern = rb'^ *' + re.escape(label) + rb'\) o=([0-9]+) n=([0-9]+) k=[0-9a-f]{16};;$'
        matches = list(re.finditer(pattern, script, re.M))
        if len(matches) != 1:
            raise ValueError('missing/duplicate Linux launch slot')
        o, n = map(int, matches[0].groups())
        at = o - 1
        if o <= 0 or n <= 0 or at < len(script) or at > len(data) or n > len(data) - at:
            raise ValueError('launch slot outside product')
        slots.append((at, n, machine))
    a, b = slots
    if a[0] < b[0] + b[1] and b[0] < a[0] + a[1]:
        raise ValueError('overlapping launch slots')
    for at, n, machine in slots:
        stream = zlib.decompressobj(31)
        try:
            out = stream.decompress(data[at:at + n])
        except zlib.error as e:
            raise ValueError('invalid gzip launch slot') from e
        if not stream.eof or stream.unused_data:
            raise ValueError('gzip length differs from launch slot')
        if len(out) < 20 or struct.unpack_from('<H', out, 18)[0] != machine:
            raise ValueError('launch slot ELF architecture mismatch')
        yield at, out


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
    lines, bad, seen = [], False, {}
    try:
        images = list(members(data))
    except ValueError as error:
        return ["  FAIL " + str(error)], True
    for at, image in images:
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
        seen[machine] = seen.get(machine, 0) + 1
        dynamic = [name for kind, name in ((PT_INTERP, 'PT_INTERP'), (PT_DYNAMIC, 'PT_DYNAMIC')) if kind in types]
        if PT_LOAD not in types:
            lines.append('  FAIL lnx/%s image at %d has no PT_LOAD (%d program headers)' % (machine, at, len(types))); bad = True
        elif dynamic:
            lines.append('  FAIL lnx/%s image at %d is dynamic: %s' % (machine, at, ' '.join(dynamic))); bad = True
        else:
            lines.append('  ok   lnx/%s image at %d: static, %d program headers' % (machine, at, len(types)))
    for machine in sorted(MACHINES.values()):
        if machine not in seen:
            lines.append('  FAIL no lnx/%s image found' % machine); bad = True
        elif seen[machine] != 1:
            lines.append('  FAIL %d lnx/%s images found, want exactly 1' % (seen[machine], machine)); bad = True
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
    def pack(*images):
        chunks = [gzip.compress(i) for i in images]
        off = 512
        slots = []
        for chunk in chunks[:2]:
            slots.append((off + 1, len(chunk)))
            off += len(chunk)
        slots += [(0, 0)] * (2 - len(slots))
        header = ("MZqFpD='\nu=x\ncase \"$u\" in\n"
                  "  Linuxx86_64) o=%d n=%d k=0000000000000000;;\n"
                  "  Linuxaarch64|Linuxarm64) o=%d n=%d k=0000000000000000;;\nesac\n" % (*slots[0], *slots[1])).encode()
        return header.ljust(512, b' ') + b''.join(chunks)
    static = pack(elf(62, [1, 1]), elf(183, [1, 1]), b'\xcf\xfa\xed\xfe' + bytes(60))
    assert check(static)[1] is False, 'two static Linux images must pass'
    for name, data in (('PT_INTERP', pack(elf(62, [1, 3, 1]), elf(183, [1, 1]))),
                       ('PT_DYNAMIC', pack(elf(62, [1, 1]), elf(183, [1, 2]))),
                       ('one architecture missing', pack(elf(62, [1, 1]))),
                       ('no image at all', b'MZ-fixture' + bytes(100)),
                       ('headers outside the image', pack(elf(62, [1, 1])[:80], elf(183, [1, 1]))),
                       ('no program headers', pack(elf(62, []), elf(183, [1, 1]))),
                       ('no PT_LOAD', pack(elf(62, [4]), elf(183, [1, 1]))),
                       ('an extra static member beside a dynamic one', pack(elf(62, [1, 3]), elf(183, [1, 1]), elf(62, [1, 1]))),
                       ('wrong architecture slot', pack(elf(62, [1, 1]), elf(62, [1, 1])))):
        assert check(data)[1] is True, name + ' must fail'
    assert not check(pack(elf(62, [1, 1]), elf(183, [1, 1]), elf(62, [1, 1])))[1], 'unselected gzip is ignored'
    print('staticimagecheck: formal slots pass; PT_INTERP, PT_DYNAMIC, missing or duplicate architecture, no image, no PT_LOAD, empty and truncated headers fail')
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
