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


class Inputs(Files):
    def __init__(self, object_mode, unit_mode, undef):
        super().__init__()
        self.resources = {
            b'\0cli/object': b'\1' if object_mode else None,
            b'\0cli/funit': b'\1' if unit_mode else None,
            b'\0cli/undefines': b'__UNISA_OBJECT\0' if undef else None,
        }

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
    for mode in ((), ('--locations',), ('--shared-predefines',)):
        model = tmp / 'pp.json'
        table = tmp / 'pp.tbl'
        net = tmp / 'pp.net'
        call(sys.executable, ROOT / 'exec/pp/gen.py', model, *mode)
        call(sys.executable, ROOT / 'exec/c/tbl.py', model, table)
        call(sys.executable, ROOT / 'exec/c/net.py', table, net)
        call(runtime, '--check-net', table, net)
        if mode == ('--shared-predefines',):
            # This variant also requires the target and target-name resources.
            continue
        delta = json.loads(model.read_text())
        for obj, unit, undef in ((0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0), (1, 0, 1)):
            status, out, why = run(delta, source, '/tmp/e2-object.c',
                                   files=Inputs(obj, unit, undef), maxsteps=2000000)
            assert status == 'accept', (mode, obj, unit, undef, status, why)
            expected = b'OBJECT' if (obj or unit) and not undef else b'ORDINARY'
            assert expected in out and (b'ORDINARY' if expected == b'OBJECT' else b'OBJECT') not in out, (mode, obj, unit, undef, out)
            count += 1
print('E2 object predefine:', count, 'resource/undef verdicts; three variants check-net')
