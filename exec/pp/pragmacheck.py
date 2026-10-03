#!/usr/bin/env python3
"""Declared pragma stacks: actual E2 networks against host preprocessing tokens.
Only literal ASCII identifier names are covered; malformed/escaped names reject.
"""
import os, pathlib, platform, struct, subprocess, sys, tempfile
R = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(R))
from unisa.front.pp import _pieces
TARGET = ('osx' if sys.platform == 'darwin' else 'lnx') + '/' + (
    'arm64' if platform.machine() in ('arm64', 'aarch64') else 'x86_64')
TFLAGS = ['--osx'] * TARGET.startswith('osx/') + ['--arm64'] * TARGET.endswith('/arm64')

def call(args):
    p = subprocess.run(list(map(str, args)), capture_output=True, timeout=15)
    assert p.returncode == 0, (p.args, p.stderr[-2000:])
    return p.stdout

def tokens(b):
    return [x for x in _pieces(b.decode()) if not x.isspace()]

def unpack(b):
    assert b[:7] == b'UNIPP1\0'
    length, _, _, ns, ni = struct.unpack_from('<5I', b, 7)
    i = 27 + 4 * ns
    for _ in range(ni):
        i += 12 + struct.unpack_from('<I', b, i + 8)[0]
    assert len(b) == i + length
    return b[i:]

cases = {
    'nested': '#define push_macro wrong\n#define pop_macro wrong\n#define X 1\nX\n#pragma push_macro("X")\n#undef X\n#define X 2\nX\n#pragma push_macro("X")\n#define X 3\nX\n#pragma pop_macro("X")\nX\n#pragma pop_macro("X")\nX\n',
    'undefined': '#pragma push_macro("X")\n#define X 9\nX\n#pragma pop_macro("X")\n#if defined(X)\nWRONG\n#else\nRIGHT\n#endif\nX\n',
    'interleaved': '#define X 1\n#define Y 2\n#pragma push_macro("X")\n#pragma push_macro("Y")\n#define X 3\n#define Y 4\nX Y\n#pragma pop_macro("X")\nX Y\n#pragma pop_macro("Y")\nX Y\n#pragma pop_macro("Z")\n',
    'function': '#define F(x,...) x + __VA_ARGS__\n#pragma push_macro("F")\n#undef F\n#define F 9\nF\n#pragma pop_macro("F")\nF(1,2)\n',
    'hash': '#define CAT(a,b) a ## b\n#pragma push_macro("CAT")\n#undef CAT\n#pragma pop_macro("CAT")\nCAT(he,llo)\n',
    'inactive': '#if 0\n#pragma push_macro("X")\n#endif\n#define X 7\n#pragma pop_macro("X")\nX\n',
    'restore-undefined': '#define X 8\n#pragma push_macro("X")\n#undef X\n#pragma push_macro("X")\n#define X 9\n#pragma pop_macro("X")\nX\n#pragma pop_macro("X")\nX\n',
}
with tempfile.TemporaryDirectory(prefix='pp-pragma-') as td:
    t = pathlib.Path(td)
    call([os.environ.get('EXEC_CC', 'cc'), '-O2', R/'exec/c/run.c', '-o', t/'run'])
    for located in (False, True):
        call([sys.executable, R/'exec/build/gen.py', 'pp', t/'pp.json', *TFLAGS, *(['--locations'] if located else [])])
        call([sys.executable, R/'exec/c/tbl.py', t/'pp.json', t/'pp.tbl'])
        call([sys.executable, R/'exec/c/net.py', t/'pp.tbl', t/'pp.net'])
        call([t/'run', '--check-net', t/'pp.tbl', t/'pp.net'])
        inputs = []
        for name, source in cases.items():
            f = t/(name+'.c'); f.write_text(source); inputs.append(f)
        inputs += [pathlib.Path(x).resolve() for x in sys.argv[1:]]
        for f in inputs:
            got = call([t/'run', t/'pp.net', f, f, R/'include'])
            if located: got = unpack(got)
            host = call(['cc', '-E', '-P', '-nostdinc', '-I'+str(R/'include'), f])
            assert tokens(got) == tokens(host), (f.name, located, got[-1500:], host[-1500:])
        for source in ['#pragma push_macro("a-b")\n', '#pragma pop_macro(X)\n']:
            f=t/'invalid.c';f.write_text(source)
            p=subprocess.run([t/'run', t/'pp.net', f, f, R/'include'],capture_output=True,timeout=15)
            assert p.returncode and b'pragma macro stack requires' in p.stderr
        print('pragma', 'locations' if located else 'plain', len(inputs), 'host token matches; 2 named refusals', flush=True)
