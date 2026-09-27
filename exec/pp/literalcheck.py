#!/usr/bin/env python3
"""Actual E2 network vs host: typed literals, #if macros, and explicit limits."""
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
expressions += [
    '-1 > 0u', '-1 < 0u', '-1 == 18446744073709551615u',
    '18446744073709551615u / 2 == 9223372036854775807',
    '18446744073709551615u % 7 == 1', '0xffffffffffffffff == -1',
    '0xffffffffffffffff > 0', '(0x8000000000000000 >> 63) == 1',
    '01777777777777777777777 > 0',
    '01777777777777777777777 == -1', '000000000000000000001u == 1',
    '1u + 2L == 3', '1Ul + 2lU + 3uLL + 4LLu == 10',
    '1uL + 2Lu + 3Ull + 4llU == 10',
    '(~0u >> 63) == 1', '(-1 >> 1u) == -1',
    '(1 ? -1 : 0u) > 0', '(0 ? 0u : -1) > 0',
    '(1 ? -1 : 1u/0) > 0', '(0 ? 1u/0 : -1) > 0',
    '0 && 1u/0', '1 || 1u/0', '(1 || 0u) < 0u',
    '(-1 < 0u) == 0', '(-1 <= 0u) == 0', '(-1 >= 0u) == 1',
    '(0u - 1) > 0', '(0u - 1) / -1 == 1', '(0u - 1) % -1 == 0',
    '(-1 & 1u) == 1', '(-1 ^ 0u) > 0', '(-1 | 0u) > 0',
    '(-1 * 1u) > 0', '(+1u - 2) > 0', '(-1u) > 0',
    '(!0u - 2) < 0', '((1u == 1) - 2) < 0',
    '((1u && 1) - 2) < 0', '((0u || 1) - 2) < 0',
]
# These remain outside this slice, including dead-arm literals:
# poison skips arithmetic faults, never turns unsupported syntax into a value.
rejected = ['18446744073709551615', '9223372036854775808L',
            '0x10000000000000000', '02000000000000000000000', '08', '1lL',
            '184467440737095516160L', '18446744073709551616u',
            '1uu', '1lul', '1UlL', '1LLL', r"'ab'", r"'\x100'", r"'\400'",
            '0 && 18446744073709551616u', '1/0', '(0 ? 1 : 1u/0)']

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
        macro_prefix = """#define M(a,b) ((a)>(b)?(a):(b))
#define ID(x) x
#define INC(x) ((x)+1)
#define APPLY(f,x) f(x)
#define FN INC
#define OPEN M(
#define SELF SELF
#define A B
#define B A
#define X 41
#define EMPTY
#define DUP(x) x+x
"""
        macro_exprs = ['M(3,7)==7', 'M(9,M(2,4))==9', 'M(M(1,8),2)==8',
            'defined(X) && defined M && !defined NOPE',
            'SELF==0 && A==0', 'M==0', '0 && ID(1/0)', '1 || ID(1/0)',
            'OPEN 1,2)==2', 'APPLY(INC,M(1,2))==3', 'FN(3)==4',
            '(EMPTY 1)==1', '3*DUP(2)==8', '(1 ? M(1,2) : ID(1/0))==2']
        f = t/'ifmacro.c'
        f.write_text(macro_prefix + ''.join('#if '+e+'\nYES'+str(i)+'\n#else\nNO'+str(i)+'\n#endif\n'
                                           for i,e in enumerate(macro_exprs)) + 'INC(4) SELF\n')
        got = call([t/'run', t/'pp.net', f, f, R/'include'])
        if mode:
            length, _, _, ns, ni = struct.unpack_from('<5I', got, 7)
            start = 27+4*ns
            for _ in range(ni): start += 12+struct.unpack_from('<I', got, start+8)[0]
            got = got[start:]; assert len(got) == length
        sys.path.insert(0, str(R))
        from unisa.front.pp import _pieces
        tokens = lambda value: [s for s in _pieces(value.decode()) if not s.isspace()]
        want = call([os.environ.get('CC', 'cc'), '-E', '-P', f])
        assert tokens(got) == tokens(want), (got, want)
        f.write_text(macro_prefix + '#if 0 && ID(1,2)\na\n#endif\n')
        p = run([t/'run', t/'pp.net', f, f, R/'include'])
        assert p.returncode != 0 and b'argument count' in p.stderr, p.stderr
        assert run([os.environ.get('CC', 'cc'), '-E', '-P', f]).returncode != 0
        f = R/'tests/c/b_ppif.c'
        got = call([t/'run', t/'pp.net', f, f, R/'include'])
        if mode:
            length, _, _, ns, ni = struct.unpack_from('<5I', got, 7)
            start = 27+4*ns
            for _ in range(ni): start += 12+struct.unpack_from('<I', got, start+8)[0]
            got = got[start:]; assert len(got) == length
        # Same repository headers and target predefinitions; compare the entire
        # original source's preprocessing output, including every header token.
        defines = []
        for line in (R/'exec/pp/predefines.tsv').read_text().splitlines():
            if not line or line.startswith('#'): continue
            kind, target, *names = line.split('\t')
            if (kind, target) in [('common', '*'), ('os', 'lnx'), ('arch', 'x86_64')]:
                defines += ['-D'+name for name in names]
        want = call([os.environ.get('CC', 'cc'), '-E', '-P', '-undef', '-nostdinc',
                     '-I'+str(R/'include'), *defines, f])
        sys.path.insert(0, str(R))
        from unisa.front.pp import _pieces
        tokens = lambda value: [s for s in _pieces(value.decode()) if not s.isspace()]
        assert tokens(got) == tokens(want), (tokens(got)[-180:], tokens(want)[-180:])
        print(mode or 'plain', len(expressions), 'host selections;', len(rejected),
              'literal refusals; 14 macro selections + dead-arm arity refusal; original b_ppif full header/source tokens match host', flush=True)
