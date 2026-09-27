#!/usr/bin/env python3
"""Small scheduler/cache controls; no compiler workload or repository edits."""
import contextlib, importlib.util, io, json, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
def load(name, path):
    spec=importlib.util.spec_from_file_location(name,path); m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
q=load('queue_under_test',ROOT/'tests/gatequeue.py')
c=load('cache_under_test',ROOT/'exec/pipeline/models.py')
with tempfile.TemporaryDirectory() as td:
    t=pathlib.Path(td); stamp=['same']
    code='import pathlib,sys,time; p=pathlib.Path(sys.argv[1]); p.write_text(str(time.monotonic())); time.sleep(float(sys.argv[2])); p.with_suffix(".end").write_text(str(time.monotonic()))'
    jobs={n:[sys.executable,'-c',code,str(t/n),delay] for n,delay in [('fast','0.1'),('slow','0.8'),('refill','0.1')]}
    q.plan=lambda com:jobs
    q.fingerprint=lambda plan:stamp[0]
    def run(state,window=55):
        sys.argv=['gatequeue.py','--state',str(t/state),'--window',str(window)]
        with contextlib.redirect_stdout(io.StringIO()): return q.main()
    assert run('rolling')==0
    assert float((t/'refill').read_text()) < float((t/'slow.end').read_text()), 'empty slot waited for slow job'
    before=(t/'fast').stat().st_mtime_ns
    assert run('rolling')==0 and before==(t/'fast').stat().st_mtime_ns, 'resume reran completed job'
    stamp[0]='changed'
    try: run('rolling')
    except SystemExit as e: assert 'input changed' in str(e)
    else: raise AssertionError('stale results accepted')
    jobs={'bad':[sys.executable,'-c','print("PASS"); raise SystemExit(2)']}
    assert run('failure')==1
    assert json.loads((t/'failure/results.json').read_text())['results']['bad']['rc']==2
    jobs={'timeout':[sys.executable,'-c','import time; time.sleep(30)']}
    assert run('timeout',5)==1
    assert json.loads((t/'timeout/results.json').read_text())['results']['timeout']['rc']==142
    print('queue: rolling refill, resume, changed inputs, nonzero exit and timeout controls pass')
    cache=t/'cache';cache.mkdir()
    names={'run','models.pkg','route.tsv'} | {s+'.'+e for s in ('e2','e1','e3','e4','lower','elf') for e in ('json','tbl','net')}
    for n in names: (cache/n).write_bytes(b'fixture')
    manifest={n:c.digest(cache/n) for n in names}
    (cache/'manifest.json').write_text(json.dumps(manifest))
    assert c.valid(cache,'1')
    (cache/'e3.net').write_bytes(b'corrupt'); assert not c.valid(cache,'1')
    (cache/'e3.net').write_bytes(b'fixture'); del manifest['e3.net']
    (cache/'manifest.json').write_text(json.dumps(manifest)); assert not c.valid(cache,'1')
    seed=t/'source';seed.mkdir()
    for d in ('exec','unisa','src','kernel','include','weights'): (seed/d).mkdir()
    header=seed/'include/test.h';header.write_text('one')
    keywords=seed/'iterate/kernel/typekw.tsv'
    keywords.parent.mkdir(parents=True)
    keywords.write_text('kw\tint\n')
    c.ROOT=seed
    first=c.identity('osx/arm64','1','cc')
    header.write_text('two')
    assert first!=c.identity('osx/arm64','1','cc')
    second=c.identity('osx/arm64','1','cc')
    keywords.write_text('kw\tlong\n')
    assert second!=c.identity('osx/arm64','1','cc')
    print('cache: artifact corruption, incomplete manifest, changed header and keyword controls pass')
