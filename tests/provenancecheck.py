#!/usr/bin/env python3
"""Stale source/artifact controls, plus actual gate admission before any suite."""
import importlib.util, json, os, pathlib, subprocess, sys, tempfile
from unittest.mock import patch
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('provenance', ROOT/'exec/c/provenance.py')
p = importlib.util.module_from_spec(spec); spec.loader.exec_module(p)

def refused(path):
    try: p.verify(path)
    except SystemExit: return
    raise AssertionError('stale product accepted')

with tempfile.TemporaryDirectory() as td:
    t = pathlib.Path(td)
    for d in ('exec/c/asm', 'unisa', 'src', 'kernel', 'include', 'weights', 'iterate/kernel'):
        (t/d).mkdir(parents=True, exist_ok=True)
    header=t/'include/test.h'; header.write_text('header v1')
    asm=t/'exec/c/asm/test.S'; asm.write_text('assembly v1')
    source=t/'src/test.c'; source.write_text('source v1')
    generator=t/'exec/generator.py'; generator.write_text('generator v1')
    output=t/'exec/build/ua_ref.c'; output.parent.mkdir(); output.write_text('generated v1')
    (t/'iterate/kernel/typekw.tsv').write_text('int\n')
    binary=t/'candidate'; binary.write_bytes(b'candidate v1'); binary.chmod(0o755)
    with patch.object(p, 'ROOT', t), patch.object(sys.modules['models'], 'ROOT', t), \
         patch.object(p.subprocess, 'check_output', return_value=b'fixture-commit\n'):
        refused(binary)  # absence is not auto-attested
        start=p.source_digest(); p.write(binary, start); p.verify(binary)
        output.write_text('generated v2')
        assert p.source_digest()==start, 'generated build output invalidated source identity'
        p.verify(binary)
        output.unlink(); assert p.source_digest()==start
        new_output=output.parent/'nested/model.json'; new_output.parent.mkdir(); new_output.write_text('{}')
        assert p.source_digest()==start, 'new generated output invalidated source identity'
        for source in (header, asm, source, generator):
            old=source.read_bytes(); source.write_bytes(old+b'changed'); refused(binary)
            source.write_bytes(old); p.verify(binary)
        binary.write_bytes(b'candidate v2'); refused(binary)
        binary.write_bytes(b'candidate v1'); p.verify(binary)
        try: p.write(binary, 'wrong-start')
        except SystemExit: pass
        else: raise AssertionError('source change during packaging accepted')
    # A record from the real current source, but altered bytes, must stop both
    # entry points before they can build a reference or schedule a test job.
    record=dict(schema=1, sources_sha256=p.source_digest(), bytes=12,
                artifact_sha256='0'*64)  # intentional invalid fixture, no product required
    pathlib.Path(str(binary)+'.build.json').write_text(json.dumps(record))
    env=dict(os.environ, MODEL_COM=str(binary), TERM_SH='0', GATE_BOUND='1')
    for cmd in ([sys.executable, 'tests/gatequeue.py', '--com', '--suite', 'com-cli', '--state', str(t/'queue')],
                ['sh', 'tests/gate.sh', '--com', '--suite', 'com-cli']):
        r=subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, timeout=10)
        assert r.returncode != 0 and b'product freshness:' in r.stderr, r
    assert not (t/'queue/results.json').exists(), 'stale candidate entered the queue'
print('product freshness: absent record, changed header/assembly/artifact, build race and both gate admissions rejected')
