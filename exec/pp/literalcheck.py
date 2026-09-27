#!/usr/bin/env python3
"""Actual E2 network vs host preprocessor: signed literals and explicit limits."""
import os, pathlib, subprocess, sys, tempfile
R = pathlib.Path(__file__).resolve().parents[2]
def run(args):
    return subprocess.run(list(map(str, args)), capture_output=True, timeout=55)
def call(args):
    p = run(args)
    assert p.returncode == 0, (p.args, p.stderr[-2000:])
    return p.stdout
expressions = [
    '017 == 15', '0777777777777777777777 == 9223372036854775807',
    '9223372036854775807L > 0', '0x7fffffffffffffffLL > 0',
    '1l + 2ll + 3L + 4LL == 10', '0L == 0', '017LL == 15',
    r"'a' == 97", r"'\x41' == 65", r"'\n' == 10", r"'\101' == 65",
    r"'\0' == 0", r"'\377' == -1", r"'\xff' == -1", r"'\?' == 63",
    r"'\\' == 92", r"'\a' == 7", r"'\b' == 8", r"'\f' == 12",
    r"'\v' == 11", r"'\r' == 13", r"'\t' == 9", r"'\'' == 39",
    '0 && 1/0', '1 || 1/0', '(1 ? 7L : 1/0) == 7',
    '(0 ? 1/0 : 017) == 15', '-7 / 2 == -3 && -7 % 2 == -1',
]
# These remain outside this signed-only slice, including dead-arm literals:
# poison skips arithmetic faults, never turns unsupported syntax into a value.
rejected = ['0u', '1UL', '18446744073709551615', '9223372036854775808L',
            '0xffffffffffffffffLL', '02000000000000000000000', '08', '1lL',
            '184467440737095516160L', r"'ab'", r"'\x100'", r"'\400'",
            '0 && 0u', '1/0', '(0 ? 1 : 1/0)']
with tempfile.TemporaryDirectory(prefix='pp-literals-') as td:
    t = pathlib.Path(td)
    call([os.environ.get('CC', 'cc'), '-O2', R/'exec/c/run.c', '-o', t/'run'])
    for mode in ([], ['--locations']):
        call([sys.executable, R/'exec/pp/gen.py', t/'pp.json', *mode])
        call([sys.executable, R/'exec/c/tbl.py', t/'pp.json', t/'pp.tbl'])
        call([sys.executable, R/'exec/c/net.py', t/'pp.tbl', t/'pp.net'])
        call([t/'run', '--check-net', t/'pp.tbl', t/'pp.net'])
        source = ''.join('#if '+e+'\nYES'+str(i)+'\n#else\nNO'+str(i)+'\n#endif\n'
                         for i, e in enumerate(expressions))
        f = t/'literal.c'; f.write_text(source)
        got = call([t/'run', t/'pp.net', f, f, R/'include'])
        if mode:
            import struct
            length, _, _, ns, ni = struct.unpack_from('<5I', got, 7)
            start = 27+4*ns
            for _ in range(ni): start += 12+struct.unpack_from('<I', got, start+8)[0]
            got = got[start:]; assert len(got) == length
        want = call([os.environ.get('CC', 'cc'), '-E', '-P', f])
        assert got.split() == want.split(), (got, want)
        for expr in rejected:
            f.write_text('#if '+expr+'\na\n#endif\n')
            p = run([t/'run', t/'pp.net', f, f, R/'include'])
            assert p.returncode != 0, (expr, p.stdout)
        f = R/'tests/c/b_ppif.c'
        p = run([t/'run', t/'pp.net', f, f, R/'include'])
        assert p.returncode != 0 and b'not covered: #if expression' in p.stderr, p.stderr
        print(mode or 'plain', len(expressions), 'host selections;', len(rejected),
              'explicit refusals; original b_ppif still refuses uintmax', flush=True)
