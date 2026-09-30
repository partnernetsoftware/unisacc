#!/usr/bin/env python3
"""Exact USLSIG2 graph fixtures, normal/sanitized native host decoder oracle."""
import argparse
import os
import platform
import struct
import subprocess
import tempfile
from pathlib import Path
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import bounded as run
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--arch', choices=['arm64', 'x86_64'])
o = p.parse_args()
host = 'arm64' if platform.machine() in ('arm64', 'aarch64') else 'x86_64'
target = o.arch or host
if target != host and platform.system() != 'Darwin':
    raise SystemExit('cross execution requires native runner')
flags = ['-arch', target] if platform.system() == 'Darwin' else []
def u(n): return struct.pack('<Q', n)
def desc(kind=1, width=4, align=4, depth=0, tag=0, payload=b'', local=0):
    return b''.join(u(n) for n in (depth, local, local, kind, width, 0, align))+bytes([tag])+u(len(payload))+payload
I = desc()
D = desc(3, 8, 8)
F = desc(3, 4, 4)
P = desc(2, 8, 8, depth=1)
PAIR = desc(5, 16, 8, tag=1, payload=u(2)+u(0)+u(0)+u(0)+u(8)+D+u(8)+u(0)+u(0)+u(4)+I)
def callback(payload=b'', **kw): return desc(4, 8, kw.pop('align', 8), depth=kw.pop('depth', 1), tag=4, payload=payload, **kw)
def ref(i, **kw): return callback(bytes([1, 1])+u(i), **kw)
def definition(i, args=(), result=I, var=0, mode=1, support=0, **kw):
    return callback(bytes([1, 0])+u(i)+bytes([var, mode])+u(len(args))+result+u(len(args))+b''.join(args)+bytes([support]), **kw)
def pack(args, support=0):
    return b'USLSIG2\n'+u(1)+u(5)+b'graph'+bytes([0, 1, 0, 1])+u(len(args))+I+u(len(args))+b''.join(args)+bytes([support])
def chain(n, i=1): return definition(i, [chain(n-1, i+1)] if n > 1 else [])
with tempfile.TemporaryDirectory(prefix='r10-callbackgraph-') as tmp:
    t = Path(tmp)
    def write(name, data):
        f = t/(name+'.sig'); f.write_bytes(data); return f
    selfgraph = pack([definition(1, [ref(1)])])
    shared = pack([definition(1, [I]), ref(1)])
    mutual = pack([definition(1, [definition(2, [ref(1)])])])
    renamed = pack([definition(1, [ref(1, local=89)], result=desc(local=71), local=12)])
    fixtures = [
        ('self', write('self', selfgraph), write('renamed', renamed)),
        ('shared', write('shared', shared), write('sharedcopy', shared)),
        ('mutual', write('mutual', mutual), write('mutualcopy', mutual)),
        ('nine', write('nine', pack([definition(1, [definition(2, [I, D, F, P, PAIR, I, D, I, I], PAIR, support=1)], PAIR)])), None),
        ('limit', write('limit', pack([definition(1, [definition(i) for i in range(2, 1025)])])), None),
        ('depth', write('depth', pack([chain(31)])), None),
        ('empty', write('empty', pack([callback()])), None),
        ('unequal', write('self2', selfgraph), write('changed', pack([definition(1, [ref(1)], D)]))),
        ('equal', write('mutual2', mutual), write('collapsed', selfgraph)),
        ('equal', write('leaf-support0', pack([definition(1, [I], support=0)])), write('leaf-support1', pack([definition(1, [I], support=1)]))),
        ('equal', write('shared2', pack([definition(1, [ref(1), ref(1)])])), write('unshared', pack([definition(1, [ref(1), definition(2, [ref(2), ref(2)])])]))),
    ]
    malformed = [pack([ref(1)]), pack([definition(1), definition(1)]), pack([definition(2)]),
        pack([definition(1, [ref(2)]), definition(2)]), pack([ref(0)]), pack([ref(1025)]),
        pack([callback(bytes([2, 1])+u(1))]), pack([callback(bytes([1, 2])+u(1))]),
        pack([definition(1, depth=2)]), pack([definition(1, align=4)]),
        pack([definition(1, var=2)]), pack([definition(1, mode=2)]), pack([definition(1, support=2)]),
        pack([definition(1, [I]*1025)]), pack([chain(32)]),
        pack([definition(1)], 1), pack([definition(1, [ref(1)], support=1)]), pack([definition(1, [definition(i) for i in range(2, 1026)])]),
        pack([definition(i, [I]*15) for i in range(1, 1025)]),
        pack([callback(bytes([1, 1])+u(1)+b'x')]),
        pack([callback(bytes([1, 0])+u(1)+bytes([0, 1])+u(0)+I+u(1)+bytes([0]))]),
        pack([callback(b'x'*16777217)]),
    ]
    bad = [write('bad'+str(i), b) for i, b in enumerate(malformed)]
    exe = t/'probe'
    for sanitized in (False, True):
        run(os.environ.get('CC', 'cc'), *flags, '-std=c99', '-I', ROOT, '-Wall', '-Wextra', '-Wno-unused-function', ROOT/'tests/librarycallbackgraphcheck.c', '-lffi', '-o', exe, *(['-fsanitize=address,undefined', '-g'] if sanitized else []))
        for name, f, other in fixtures:
            run(*(['arch', '-x86_64'] if host == 'arm64' and target == 'x86_64' else []), exe, name, f, other or f, *bad)
print('callback graph host: normal + ASan/UBSan passed on '+target)
