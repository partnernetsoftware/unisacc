#!/usr/bin/env python3
"""Compare Linux host-bridge ELF bytes against the current C reference."""
import pathlib
import json
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT), str(ROOT / 'exec/enc')]
from unisa.__main__ import _oracle
from unisa.lower import lower
from unisa.tape import DATA_BASE, parse
from unisa.assemble import assemble
from unisa import image
from tins import dump
from exec.pp import sim


def checked(*args):
    result = subprocess.run(args, capture_output=True, timeout=55)
    if result.returncode:
        raise RuntimeError((args, result.returncode, result.stderr.decode(errors='replace')))
    return result.stdout


def main():
    if len(sys.argv) != 6:
        raise SystemExit('usage: dynelfcheck.py REF RUN X86_TBL ARM_TBL X86_JSON')
    ref, run, x86, arm, x86_json = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix='unisa-dynelf-') as tmp:
        root = pathlib.Path(tmp)
        src = root / 'host.c'
        src.write_text('long __hostaddr0(void); int main(void){return __hostaddr0()!=0;}\n')
        tape = parse(checked(ref, '-S', str(src)).decode())
        oracle = _oracle('built')
        for arch, table in (('x86_64', x86), ('arm64', arm)):
            target = 'lnx/' + arch
            native = root / (arch + '.elf')
            checked(ref, '-b', target, '-o', str(native), str(src))
            tp = lower(tape, target, oracle, drive='built')
            assert any(i.op == 'hostaddr' for i in tp.code), target
            tins = root / (arch + '.tins')
            tins.write_text(dump(tp, full=True))
            got = checked(run, table, str(tins))
            want = native.read_bytes()
            code, facts = assemble(tp)
            seed = image.build(tp, code, image.relocate(tp, tp.data,
                               facts['data_va'] - DATA_BASE), facts['entry'])
            assert got == want, (target, len(got), len(want), next((i for i, (a, b) in enumerate(zip(got, want)) if a != b), None))
            assert seed == want, (target, 'Python seed differs from C reference')
            assert got[:4] == b'\x7fELF' and int.from_bytes(got[56:58], 'little') == 4
            print('dynamic ELF', target, len(got), 'bytes equal in C reference, delta and Python seed', flush=True)
            if arch == 'x86_64':
                argsave = next(i for i in tp.code if i.op == 'argsave')
                raw = dump(tp, full=True).replace('@target ' + target + '\n',
                    '@target ' + target + '\n@argc ' + str(argsave.args[0]) +
                    '\n@argv ' + str(argsave.args[1]) + '\n', 1).encode()
                model = json.loads(pathlib.Path(x86_json).read_text())
                fixed = {'process/argc': 1, 'process/argv': 0x12345000,
                         'memory/text': 0x100000000,
                         **{'process/dl/' + str(i): 0x1000 + i for i in range(4)}}
                def memory(extra):
                    resources = sim.Files()
                    for key, value in dict(fixed, **extra).items():
                        resources.cache[b'\0' + key.encode()] = struct.pack('<Q', value)
                    status, image, _ = sim.run(model, raw, 'dynamic-memory', resources, maxsteps=5000000)
                    assert status == 'accept', (status, image)
                    return image
                reserved = memory({'memory/reserve': 2147467264})
                textlen = struct.unpack_from('<Q', reserved, 8)[0]
                explicit_data = fixed['memory/text'] + ((textlen + 16383) & -16384) + 32
                explicit = memory({'memory/data': explicit_data})
                assert reserved == explicit
                assert reserved[:8] == b'UNIMEM1\n'
                textlen = struct.unpack_from('<Q', reserved, 8)[0]
                assert struct.unpack_from('<4Q', reserved, 40 + textlen) == tuple(0x1000 + i for i in range(4))
                print('dynamic Linux memory:', len(reserved), 'bytes, one-pass = explicit binding', flush=True)


if __name__ == '__main__':
    main()
