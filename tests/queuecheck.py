#!/usr/bin/env python3
"""Small scheduler/cache controls; no compiler workload or repository edits."""
import contextlib, importlib.util, io, json, os, pathlib, subprocess, sys, tempfile
from unittest.mock import patch
ROOT = pathlib.Path(__file__).resolve().parents[1]
def load(name, path):
    spec=importlib.util.spec_from_file_location(name,path); m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
q=load('queue_under_test',ROOT/'tests/gatequeue.py')
real_fingerprint=q.fingerprint
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
    # Exercise real queue identity and resume, not a mocked candidate stamp.
    q.fingerprint=real_fingerprint
    selectors=('MODEL_COM','UA','UA_RUN','TOOLS_UA','CORPUS_UA')
    environment={k:v for k,v in os.environ.items() if k not in selectors}
    candidate=t/'compiler with spaces';candidate.write_bytes(b'#!/bin/sh\nexit 0\n');candidate.chmod(0o755)
    marker=t/'candidate-ran'
    jobs={'candidate':[sys.executable,'-c','import pathlib,sys; pathlib.Path(sys.argv[1]).write_text("ran")',str(marker)]}
    with patch.dict(os.environ,environment,clear=True):
        assert not q.executable_inputs(q.execution_settings()), 'implicit compiler file guessed'
        for key in selectors:
            with patch.dict(os.environ,{key:str(candidate)}):
                assert run(key)==0
                before=marker.stat().st_mtime_ns
                original=candidate.read_bytes();candidate.write_bytes(original)
                assert run(key)==0 and marker.stat().st_mtime_ns==before, 'same bytes did not resume'
                candidate.write_bytes(original+b'# changed\n')
                try: run(key)
                except SystemExit as e: assert 'input changed' in str(e)
                else: raise AssertionError('changed '+key+' reused completed result')
                candidate.write_bytes(original)
            with patch.dict(os.environ,{key:str(t/'missing')}):
                try: run('missing-'+key)
                except SystemExit as e: assert 'queue '+key+':' in str(e)
                else: raise AssertionError('missing explicit '+key+' accepted')
        for key in selectors[1:]:
            assert q.executable_inputs({key:''})=={}
        try: q.executable_inputs({'MODEL_COM':''})
        except SystemExit: pass
        else: raise AssertionError('empty explicit MODEL_COM accepted')
        with patch.dict(os.environ,{'PATH':str(t)}):
            assert q.executable_inputs({'UA_RUN':candidate.name})['UA_RUN']['path']==str(candidate.resolve())
        for value in ['sh '+str(candidate), 'arch -x86_64 '+str(candidate), 'sh -c true']:
            try: q.executable_inputs({'UA_RUN':value})
            except SystemExit: pass
            else: raise AssertionError('shell command guessed as a file')
        link=t/'linked';link.symlink_to(candidate)
        assert q.executable_inputs({'MODEL_COM':str(link)})['MODEL_COM']['path']==str(candidate.resolve())
        fifo=t/'fifo';os.mkfifo(fifo)
        nonexecutable=t/'nonexecutable';nonexecutable.write_bytes(b'not executable')
        for invalid in (t,nonexecutable,fifo):
            try: q.executable_inputs({'MODEL_COM':str(invalid)})
            except SystemExit: pass
            else: raise AssertionError('invalid candidate accepted')
    print('queue: five explicit executable hashes, same-byte resume, changed/missing rejection, spaces/PATH/symlink and command-string controls pass')
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
