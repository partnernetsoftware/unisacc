#!/usr/bin/env python3
"""Actual E2 network vs the scratch reference: directive diagnostics and limits.

#error (live and skipped), #else/#endif without #if, string prefixes, and the
4000-splice #include limit.  The first diagnostic line must equal the
reference's; constructs E2 does not cover must be refused by name.
"""
import os, pathlib, subprocess, sys, tempfile
R = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(R/'exec/c'))
from refsource import source_text, compile_command

def run(args, cwd=None):
    return subprocess.run(list(map(str, args)), capture_output=True, timeout=55, cwd=cwd)
def call(args):
    p = run(args); assert p.returncode == 0, (p.args, p.stderr[-2000:]); return p.stdout

cases = {  # name: (source, extra files)
    'error-live': ('int a;\n  #  error  bad thing  \t\nint b;\n', {}),
    'error-empty': ('#error\n', {}),
    'error-linedir': ('int q;\n#line 40\n # error  a   b /* c */ d\n', {}),
    'error-skipped': ('#if 0\n#error no\n#elif 0\n# error no\n#else\nyes\n#endif\n', {}),
    'line-macro-if': ('#undef  line\n#define line 1000\n\n#line line\n#if 1000 != __LINE__\n\t#error "  # line line" not work as expected\n#endif\nint main() { return 0; }\n', {}),
    'line-num-if': ('int a;\n#line 50\n#if __LINE__ != 50\n#error bad\n#endif\nint b = __LINE__;\n', {}),
    'line-num-if-hit': ('#line 50\n#if __LINE__ == 50\n#error hit\n#endif\n', {}),
    'line-macro-elif': ('#define N 7\n#line N\n#if 0\n#elif __LINE__ != 8\n#error bad\n#elif __LINE__ == 8\n#error eight\n#endif\n', {}),
    'line-if-splice': ('int a;\n#if 1 && \\\n __LINE__ == 3\n#error three\n#endif\nint b = __LINE__;\n', {}),
    'line-if-macro': ('#define L __LINE__\n#line 20\n#if L != 20\n#error bad\n#endif\n', {}),
    'line-if-plain': ('int a;\n\n#if __LINE__ != 3\n#error bad\n#endif\n', {}),
    'endif-alone': ('int a;\n#endif\n', {}),
    'endif-extra': ('#if 1\n#else\n#endif\n#endif\n', {}),
    'else-alone': ('#else\nint a;\n', {}),
    'prefix': ('#define LUA_POF "p"\n#define Lx "m"\n#define U8 "n"\n'
               'char *s = LUA_POF"%s"; int v = Lx"q"; int y = U8"z";\n'
               'int w = L"r"; int u = u8"k" u"a" U"b"; int c = L\'c\';\n', {}),
}
refused = {  # name: (source, extra files, reason)
    'endif-in-header': ('#include "eh.h"\n', {'eh.h': '#endif\n'},
                        'a preprocessing diagnostic inside an included header'),
    'error-in-header': ('#include "er.h"\n', {'er.h': '#error x\n'},
                        'a preprocessing diagnostic inside an included header'),
    'error-long': ('#error ' + 'x' * 300 + '\n', {}, '#error text longer than 283 bytes'),
    'include-loop': ('#include "self.h"\n', {'self.h': '#include "self.h"\n'},
                     'more than 4000 #include splices'),
}
with tempfile.TemporaryDirectory(prefix='pp-directives-') as td:
    t = pathlib.Path(td)
    (t/'ref.c').write_text(source_text(R))
    call(compile_command(R, t/'ref.c', t/'ref'))
    call([os.environ.get('CC', 'cc'), '-O2', R/'exec/c/run.c', '-o', t/'run'])
    n = 0
    for mode in ([], ['--locations']):
        call([sys.executable, R/'exec/build/gen.py', 'pp', t/'pp.json', *mode])
        call([sys.executable, R/'exec/c/tbl.py', t/'pp.json', t/'pp.tbl'])
        call([sys.executable, R/'exec/c/net.py', t/'pp.tbl', t/'pp.net'])
        call([t/'run', '--check-net', t/'pp.tbl', t/'pp.net'])
        for name, (text, extra) in cases.items():
            src = t/(name + '.c'); src.write_text(text)
            for k, v in extra.items(): (t/k).write_text(v)
            ref = run([t/'ref', src.name, '-E'], cwd=t)
            got = run([t/'run', t/'pp.net', src.name, src.name, R/'include'], cwd=t)
            want = ref.stderr.split(b'\n')[0]
            if want:
                assert got.returncode == 1 and got.stdout == b'', (name, got.returncode)
                assert got.stderr == want + b'\n', (name, got.stderr, want)
            else:
                assert got.returncode == 0 and ref.returncode == 0, (name, got.stderr)
                if not mode:
                    assert got.stdout == ref.stdout, (name, got.stdout, ref.stdout)
            n += 1
        for name, (text, extra, why) in refused.items():
            src = t/(name + '.c'); src.write_text(text)
            for k, v in extra.items(): (t/k).write_text(v)
            ref = run([t/'ref', src.name, '-E'], cwd=t)
            assert b'error' in ref.stderr, (name, ref.stderr[:200])
            got = run([t/'run', t/'pp.net', src.name, src.name, R/'include'], cwd=t)
            assert got.returncode == 1 and b'not covered: ' + why.encode() in got.stderr, (name, got.stderr)
            n += 1
    print('E2 directives:', n, 'verdicts (reference diagnostics or named refusals); two variants check-net')
