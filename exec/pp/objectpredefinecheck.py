#!/usr/bin/env python3
"""The object-mode predefine is driven by E2 resources, before -U."""
import json
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from exec.pp.sim import Files, run
sys.path.insert(0, str(ROOT / 'exec'))
import assemble


class Inputs(Files):
    def __init__(self, object_mode=0, unit_mode=0, undef=0, extra=None):
        super().__init__()
        self.resources = {
            b'\0cli/object': b'\1' if object_mode else None,
            b'\0cli/funit': b'\1' if unit_mode else None,
            b'\0cli/undefines': b'__UNISA_OBJECT\0' if undef else None,
        }

        self.resources.update(extra or {})

    def get(self, key):
        if key in self.resources:
            return self.resources[key]
        return super().get(key)


def call(*args):
    p = subprocess.run(list(map(str, args)), capture_output=True, timeout=55)
    assert p.returncode == 0, (args, p.returncode, p.stderr[-500:])


source = b'#ifdef __UNISA_OBJECT\nOBJECT\n#else\nORDINARY\n#endif\n'
with tempfile.TemporaryDirectory(prefix='e2-object-predefine-') as name:
    tmp = pathlib.Path(name)
    runtime = tmp / 'run'
    call('cc', '-O2', ROOT / 'exec/c/run.c', '-o', runtime)
    count = 0
    standard_count = 0
    predefs = assemble.load_facts("pp-gen")["predefres"]
    for mode in ((), ('--locations',), ('--shared-predefines',)):
        model = tmp / 'pp.json'
        table = tmp / 'pp.tbl'
        net = tmp / 'pp.net'
        call(sys.executable, ROOT / 'exec/build/gen.py', 'pp', model, *mode)
        call(sys.executable, ROOT / 'exec/c/tbl.py', model, table)
        call(sys.executable, ROOT / 'exec/c/net.py', table, net)
        call(runtime, '--check-net', table, net)
        delta = json.loads(model.read_text())
        # Independent C99 macro contracts, including CLI ordering.  Values are
        # specified here, rather than taken from the constructor's facts.
        targets = tuple(predefs) if '--shared-predefines' in mode else ('lnx/x86_64',)
        for target in targets:
            resources = {b'\0cli/target': target.encode(),
                         b'\0predefines/'+target.encode(): predefs[target].encode()}
            cases = (
                (b'__STDC__ __STDC_VERSION__ __STDC_HOSTED__\n', {}, b'1 199901L 1'),
                (b'#if defined(__STDC__) && __STDC_VERSION__ == 199901L && __STDC_HOSTED__ == 1\nGOOD\n#else\nBAD\n#endif\n', {}, b'GOOD'),
                (b'__STDC__ __STDC_VERSION__ __STDC_HOSTED__\n',
                 {b'\0cli/defines': b'__STDC__=2\0__STDC_VERSION__=42\0__STDC_HOSTED__=0\0'}, b'2 42 0'),
                (b'__STDC__ __STDC_VERSION__ __STDC_HOSTED__\n',
                 {b'\0cli/undefines': b'__STDC__\0__STDC_VERSION__\0__STDC_HOSTED__\0'},
                 b'__STDC__ __STDC_VERSION__ __STDC_HOSTED__'),
                (b'#ifdef __STDC_VERSION__\nBAD\n#else\nGOOD\n#endif\n',
                 {b'\0cli/defines': b'__STDC_VERSION__=42\0',
                  b'\0cli/undefines': b'__STDC_VERSION__\0'}, b'GOOD'),
            )
            for text, overrides, expected in cases:
                status, out, why = run(delta, text, '/tmp/e2-standard.c',
                    files=Inputs(extra=resources | overrides), maxsteps=2000000)
                assert status == 'accept', (mode, target, status, why)
                if '--locations' not in mode:
                    assert b' '.join(out.split()) == expected, (mode, target, out, expected)
                else:
                    assert all(word in out for word in expected.split()) and b'BAD' not in out, (mode, out)
                standard_count += 1
        if mode == ('--shared-predefines',):
            continue
        for obj, unit, undef in ((0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0), (1, 0, 1)):
            status, out, why = run(delta, source, '/tmp/e2-object.c',
                                   files=Inputs(obj, unit, undef), maxsteps=2000000)
            assert status == 'accept', (mode, obj, unit, undef, status, why)
            expected = b'OBJECT' if (obj or unit) and not undef else b'ORDINARY'
            assert expected in out and (b'ORDINARY' if expected == b'OBJECT' else b'OBJECT') not in out, (mode, obj, unit, undef, out)
            count += 1
print('E2 object predefine:', count, 'resource/undef verdicts; three variants check-net')

print('E2 standard predefines:', standard_count, 'value/condition/override/undef contracts')
