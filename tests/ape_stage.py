#!/usr/bin/env python3
"""Bounded classic APE test preparation; no shipping artifact or answer cache."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / 'exec/pipeline'))
from models import closure, digest
from sourceflat import export_source,verify_source,identity as source_identity

TARGETS = ('lnx/x86_64', 'lnx/arm64', 'osx/x86_64', 'osx/arm64', 'win/x86_64')


def identity(ua):
    h = closure(hashlib.sha256()); h.update(bytes.fromhex(source_identity()))
    for path in (ROOT / 'unisacc.c', Path(ua), Path(__file__)):
        h.update(bytes.fromhex(digest(path)))
    return h.hexdigest()


def directory(state, target):
    if target not in TARGETS:
        raise ValueError('unknown APE target: ' + target)
    return Path(state) / target.replace('/', '_')


def prepare(state, target, ua):
    from unisa.__main__ import _oracle
    from unisa.tape import parse, DATA_BASE
    from unisa.lower import lower
    from unisa.assemble import assemble
    from unisa import image
    source = identity(ua)
    dest = directory(state,target); dest.mkdir(parents=True,exist_ok=True)
    flat=export_source(dest/'unisacc.flat.c')
    result = subprocess.run([ua, '-O2', str(flat), '-t', target],
                            capture_output=True, timeout=25, check=True)
    if not result.stdout:
        raise ValueError('empty compiler tape')
    tape = parse(result.stdout.decode('latin-1'))
    tape.src_os = target.split('/')[0]
    program = lower(tape, target, _oracle('built'), drive='built', prune_input=True)
    text, layout = assemble(program)
    data = image.relocate(program, program.data, layout['data_va'] - DATA_BASE)
    dest = directory(state, target)
    dest.mkdir(parents=True, exist_ok=True)
    (dest / 'text.bin').write_bytes(text)
    (dest / 'data.bin').write_bytes(data)
    # image.build reads program.code for Linux (b08ab96b: hostcall/hostaddr make
    # the ELF dynamic); pack has no code, so the decision is sealed here.
    dynamic = any(i.op in ('hostcall', 'hostaddr') for i in program.code)
    record = dict(schema=2, source=source, target=target, entry=layout['entry'], dynamic=dynamic,
                  bss=getattr(program, 'bss', 0), relocs=getattr(program, 'relocs', []),
                  files={name: digest(dest / name) for name in ('text.bin', 'data.bin')})
    if identity(ua) != source:
        raise ValueError('APE inputs changed while preparing')
    (dest / 'record.json').write_text(json.dumps(record, sort_keys=True) + '\n')
    print('ape prepare: ' + target + ' fresh text/data sealed')


def pack(state, output, ua):
    from unisa import ape, image
    source = identity(ua)
    prepared = {}
    for target in TARGETS:
        dest = directory(state, target)
        record = json.loads((dest / 'record.json').read_text())
        if record['schema'] != 2 or record['source'] != source or record['target'] != target:
            raise ValueError('stale APE preparation: ' + target)
        if set(record['files']) != {'text.bin', 'data.bin'}:
            raise ValueError('incomplete APE preparation: ' + target)
        for name, expected in record['files'].items():
            if digest(dest / name) != expected:
                raise ValueError('damaged APE preparation: ' + target)
        os_, arch = target.split('/')
        code = [SimpleNamespace(op='hostcall')] if record['dynamic'] else []
        program = SimpleNamespace(os=os_, arch=arch, bss=record['bss'], relocs=record['relocs'],
                                  code=code)
        prepared[target] = (program, (dest / 'text.bin').read_bytes(),
                            (dest / 'data.bin').read_bytes(), record['entry'])

    def one(target, stub=b''):
        program, text, data, entry = prepared[target]
        return image.build(program, text, data, entry, stub=stub)

    blob = ape.build(one, output)
    if identity(ua) != source:
        raise ValueError('APE inputs changed while packaging')
    print('ape pack: 5 targets, original writer, ' + str(len(blob)) + ' B')


if __name__ == '__main__':
    try:
        if len(sys.argv) == 5 and sys.argv[1] == 'prepare':
            prepare(*sys.argv[2:])
        elif len(sys.argv) == 5 and sys.argv[1] == 'pack':
            pack(*sys.argv[2:])
        else:
            raise ValueError('ape_stage.py prepare STATE TARGET UA | pack STATE OUTPUT UA')
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as exc:
        print('ape stage: FAIL ' + str(exc), file=sys.stderr)
        if isinstance(exc, subprocess.CalledProcessError):
            sys.stderr.buffer.write(exc.stderr)
        sys.exit(1)
