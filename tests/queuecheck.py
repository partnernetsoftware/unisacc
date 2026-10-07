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
    def run(state,window=55,extra=()):
        sys.argv=['gatequeue.py','--state',str(t/state),'--window',str(window),*extra]
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
    # 0.0.32: a full-limit timeout is retried once, alone; the second timeout is the result
    assert run('timeout',5)==75
    first=json.loads((t/'timeout/results.json').read_text())
    assert 'timeout' not in first['results'] and first['retried']==['timeout']
    assert run('timeout',5)==1
    assert json.loads((t/'timeout/results.json').read_text())['results']['timeout']['rc']==142
    print('queue: rolling refill, resume, changed inputs, nonzero exit and timeout controls pass')
    # Exclusive jobs must precede and never overlap ordinary two-slot work.
    jobs={n:[sys.executable,'-c',code,str(t/n),delay] for n,delay in
          [('normal-a','0.4'),('normal-b','0.4'),('exclusive-a','0.2'),('exclusive-b','0.2')]}
    exclusive_args=['--exclusive-suite','exclusive-a','--exclusive-suite','exclusive-b']
    jobs['bad-exclusive']=[sys.executable,'-c','print("PASS"); raise SystemExit(3)']
    exclusive_args+=['--exclusive-suite','bad-exclusive']
    assert run('exclusive',extra=exclusive_args)==1
    result=json.loads((t/'exclusive/results.json').read_text())
    assert len(result['results'])==5 and result['results']['bad-exclusive']['rc']==3
    intervals={n:(float((t/n).read_text()),float((t/n).with_suffix('.end').read_text()))
               for n in jobs if n!='bad-exclusive'}
    for name in ('exclusive-a','exclusive-b'):
        lo,hi=intervals[name]
        for other,(start,end) in intervals.items():
            if other!=name:assert hi<=start or end<=lo,'exclusive overlap'
        assert hi<=intervals['normal-a'][0] and hi<=intervals['normal-b'][0], 'exclusive not prioritized'
    assert max(intervals[n][0] for n in ('normal-a','normal-b')) < min(intervals[n][1] for n in ('normal-a','normal-b')), 'ordinary jobs did not overlap'
    for extra in (['--exclusive-suite','unknown'],['--suite','normal-a','--exclusive-suite','exclusive-a']):
        try:run('invalid-exclusive',extra=extra)
        except SystemExit as e:assert e.code==2
        else:raise AssertionError('unselected exclusive accepted')
    # 0.0.21: a changed exclusive set keeps results whose job and fingerprint are unchanged
    before=json.loads((t/'exclusive/results.json').read_text())['results']
    assert run('exclusive')==1, 'kept failing result lost'
    assert json.loads((t/'exclusive/results.json').read_text())['results']==before, 'changed exclusivity reran unchanged jobs'
    print('queue: exclusive priority/no overlap, ordinary concurrency, nonzero exclusive and selected-name validation pass')
    # Exercise real queue identity and resume, not a mocked candidate stamp.
    q.fingerprint=real_fingerprint
    selectors=('MODEL_COM','UA','UA_RUN','TOOLS_UA','CORPUS_UA')
    runtime_selectors=('UNISA_MAXSTEPS','UNISA_CONTAINER','UNISA_KERNEL')
    environment={k:v for k,v in os.environ.items() if k not in (*selectors,*runtime_selectors)}
    candidate=t/'compiler with spaces';candidate.write_bytes(b'#!/bin/sh\nexit 0\n');candidate.chmod(0o755)
    marker=t/'candidate-ran'
    jobs={'candidate':[sys.executable,'-c','import pathlib,sys; pathlib.Path(sys.argv[1]).write_text("ran")',str(marker)]}
    # Keep real content hashing independent of concurrent repository edits.
    fixture=t/'queue-source';(fixture/'tests').mkdir(parents=True)
    (fixture/'tests/bound.py').write_bytes((ROOT/'tests/bound.py').read_bytes())
    subprocess.run(['git','init','-q',str(fixture)],check=True,timeout=5)
    subprocess.run(['git','-C',str(fixture),'add','tests/bound.py'],check=True,timeout=5)
    previous_cwd=pathlib.Path.cwd()
    with patch.object(q,'ROOT',fixture), patch.dict(os.environ,environment,clear=True):
        os.chdir(fixture)
        assert not q.executable_inputs(q.execution_settings()), 'implicit compiler file guessed'
        # Session transport changes preserve results; jobs see no session ID.
        session_marker=t/'session-env'
        jobs={'session':[sys.executable,'-c',
              'import os,pathlib,sys; assert "TERM_SESSION_ID" not in os.environ; pathlib.Path(sys.argv[1]).write_text("ran")',str(session_marker)]}
        with patch.dict(os.environ,{'TERM_SESSION_ID':'window-a'}):
            assert run('session')==0
            before=session_marker.stat().st_mtime_ns
            session_stamp=q.fingerprint(jobs)
        with patch.dict(os.environ,{'TERM_SESSION_ID':'window-b'}):
            assert q.fingerprint(jobs)==session_stamp
            assert run('session')==0 and session_marker.stat().st_mtime_ns==before
        with patch.dict(os.environ,{'TERM_SESSION_ID':'window-b','UNISA_MAXSTEPS':'17'}):
            assert q.fingerprint(jobs)!=session_stamp
            assert run('session')==0 and session_marker.stat().st_mtime_ns!=before
        jobs={'candidate':[sys.executable,'-c','import pathlib,sys; pathlib.Path(sys.argv[1]).write_text("ran")',str(marker)]}
        print('queue: Terminal session changes reuse results, suites receive no transport ID, real fuel change reruns')

        # Fuel and model inputs affect execution even when the compiler is unchanged.
        for key in runtime_selectors:
            assert key not in q.execution_settings()
            with patch.dict(os.environ,{key:''}):
                assert q.execution_settings().get(key)=='' , 'empty '+key+' was normalised away'
                empty=q.fingerprint(jobs)
            assert empty!=q.fingerprint(jobs), 'unset/empty '+key+' shared an identity'
        with patch.dict(os.environ,{'UNISA_MAXSTEPS':'2'}):
            assert run('fuel')==0
            before=marker.stat().st_mtime_ns
            assert run('fuel')==0 and marker.stat().st_mtime_ns==before
        with patch.dict(os.environ,{'UNISA_MAXSTEPS':'3'}):
            assert run('fuel')==0 and marker.stat().st_mtime_ns!=before, 'changed fuel reused completed result'
        for key in runtime_selectors[1:]:
            data=t/(key+' data');data.write_bytes(b'model input');data.chmod(0o600)
            alternate=t/(key+' alternate');alternate.write_bytes(data.read_bytes());alternate.chmod(0o600)
            with patch.dict(os.environ,{key:str(data)}):
                assert run(key)==0
                before=marker.stat().st_mtime_ns
                data.write_bytes(b'model input')
                assert run(key)==0 and marker.stat().st_mtime_ns==before, 'same model bytes did not resume'
                for change in ('content','mode'):
                    if change=='content': data.write_bytes(b'changed input')
                    else: data.chmod(0o640)
                    assert run(key)==0 and marker.stat().st_mtime_ns!=before, 'changed '+key+' '+change+' reused completed result'
                    before=marker.stat().st_mtime_ns
                    data.write_bytes(b'model input');data.chmod(0o600)
            with patch.dict(os.environ,{key:str(alternate)}):
                assert run(key)==0 and marker.stat().st_mtime_ns!=before, 'changed '+key+' path reused completed result'
            invalid_fifo=t/(key+' fifo');os.mkfifo(invalid_fifo)
            for invalid in (t,t/'absent-model',invalid_fifo):
                with patch.dict(os.environ,{key:str(invalid)}):
                    try: run('invalid-'+key)
                    except SystemExit as e: assert 'queue '+key+':' in str(e)
                    else: raise AssertionError('invalid '+key+' accepted')
        print('queue: fuel and external model identity, nonexecutables, resume, changed path/content/mode and invalid files pass')
        for key in selectors:
            with patch.dict(os.environ,{key:str(candidate)}):
                assert run(key)==0
                before=marker.stat().st_mtime_ns
                original=candidate.read_bytes();candidate.write_bytes(original)
                assert run(key)==0 and marker.stat().st_mtime_ns==before, 'same bytes did not resume'
                candidate.write_bytes(original+b'# changed\n')
                assert run(key)==0 and marker.stat().st_mtime_ns!=before, 'changed '+key+' reused completed result'
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
    os.chdir(previous_cwd)
    print('queue: five explicit executable hashes, same-byte resume, changed/missing rejection, spaces/PATH/symlink and command-string controls pass')
    # Small audited closures, with unknown suites retaining the global fallback.
    # This private tree is independent of concurrent product source edits.
    import hashlib, time
    private=t/'closures';(private/'tests').mkdir(parents=True);(private/'src').mkdir()
    subprocess.run(['git','init','-q',str(private)],check=True,timeout=5)
    (private/'tests/bound.py').write_bytes((ROOT/'tests/bound.py').read_bytes())
    for n in ('docs', 'bound'):
        (private/('tests/'+n+'check.py')).write_text('pass\n')
    (private/'release').mkdir(); (private/'release/apeinspect.py').write_text('pass\n')
    for n in ('prd.md','README.md','src/product.c'):(private/n).write_text('v1\n')
    probe={n:[sys.executable,'-c','pass',n] for n in ('docs','bound','product-com')}
    declarations={'version':1,'suites':{n:{'command':probe[n],
        'files':['tests/'+n+'check.py']+(['prd.md','README.md'] if n=='docs' else []),
        'guards':{'tests/'+n+'check.py':hashlib.sha256((private/('tests/'+n+'check.py')).read_bytes()).hexdigest()}}
        for n in ('docs','bound')}}
    dep=private/'tests/gatedeps.json';dep.write_text(json.dumps(declarations))
    with patch.object(q,'ROOT',private), patch.dict(os.environ,environment,clear=True):
        os.chdir(private)
        def changed(path, expected):
            before=q.fingerprint(probe);original=path.read_bytes()
            path.write_bytes(original+(b' ' if path.suffix=='.json' else b'# edit\n'))
            after=q.fingerprint(probe)
            assert {n for n in probe if before[n]!=after[n]}==set(expected), (path, expected)
            saved={'stamp':before,'jobs':probe,'exclusive':[],'results':{n:{'rc':0} for n in probe}}
            q.resume(saved,after,probe,set())
            assert set(saved['results'])==set(probe)-set(expected)
            path.write_bytes(original)
        changed(private/'prd.md', ['docs'])
        # Unknown product's global closure intentionally includes README.
        changed(private/'README.md', ['docs','product-com'])
        changed(private/'tests/bound.py', list(probe)) # shared watchdog
        changed(private/'tests/docscheck.py', ['docs','product-com'])
        changed(private/'tests/boundcheck.py', ['bound','product-com'])
        # 0.0.29 P7': the declaration file is no longer a global identity -- declared suites keep their
        # results when their own entries are unchanged; only undeclared ones (global closure) rerun
        changed(private/'tests/gatedeps.json', ['product-com'])
        changed(private/'src/product.c', ['product-com'])
        changed(private/'release/apeinspect.py', ['product-com'])
        footer_plan={'package-footer':['python3','./tests/packagefootercheck.py']}
        footer_before=q.fingerprint(footer_plan)
        footer=private/'release/apeinspect.py';footer_original=footer.read_bytes()
        footer.write_bytes(footer_original+b'# changed imported locator\n')
        footer_after=q.fingerprint(footer_plan)
        footer_saved={'stamp':footer_before,'jobs':footer_plan,'exclusive':[],
                      'results':{'package-footer':{'rc':0}}}
        q.resume(footer_saved,footer_after,footer_plan,set())
        assert not footer_saved['results'], 'package-footer reused a changed imported locator'
        footer.write_bytes(footer_original)
        print('queue: package-footer loses its completed result when release/apeinspect.py changes')

        base=q.fingerprint(probe)
        with patch.dict(os.environ,{'UNISA_MAXSTEPS':'123'}):
            assert all(q.fingerprint(probe)[n]!=base[n] for n in probe), 'environment reused'
        with patch.object(q.platform,'platform',return_value='different platform'):
            assert all(q.fingerprint(probe)[n]!=base[n] for n in probe), 'platform reused'
        removed=private/'README.md';original=removed.read_bytes();removed.unlink()
        missing=q.fingerprint(probe)
        assert missing['docs']!=base['docs'], 'missing declared file reused'
        removed.write_bytes(original)
        with patch.dict(os.environ,{'MODEL_COM':str(candidate)}):
            base=q.fingerprint(probe);original=candidate.read_bytes();candidate.write_bytes(original+b'# candidate edit\n')
            after=q.fingerprint(probe)
            assert {n for n in probe if base[n]!=after[n]}=={'product-com'}
            candidate.write_bytes(original)
        # A checker with a new undeclared input cannot keep its audited closure.
        check=private/'tests/docscheck.py';original=check.read_bytes();check.write_bytes(original+b'# new dependency\n')
        changed(private/'src/product.c', ['docs','product-com'])
        check.write_bytes(original)
        # An unrecognised invocation likewise falls back, even under a known name.
        probe['docs']+=["new-option"]
        changed(private/'src/product.c', ['docs','product-com'])
        probe['docs'].pop()
        # Successful runs made while a script changed must not survive the
        # final identity check (nor reappear after restoring that script).
        supplied=iter(['before','after'])
        q.plan=lambda com:{'racing':[sys.executable,'-c','pass']}
        q.fingerprint=lambda jobs:next(supplied)
        try: run('racing')
        except SystemExit as error: assert 'inputs changed during queue' in str(error)
        else: raise AssertionError('mid-run input change accepted')
        assert not json.loads((t/'racing/results.json').read_text())['results']
        q.fingerprint=real_fingerprint
        started=time.monotonic();snap=q.fingerprint(probe)
        saved={'stamp':snap,'jobs':probe,'exclusive':[],'results':{n:{'rc':0} for n in probe}}
        q.resume(saved,q.fingerprint(probe),probe,set())
        assert len(saved['results'])==3
        elapsed=time.monotonic()-started
        print('queue: 3/3 synthetic results reused in %.3fs; docs/checker/candidate/environment/platform/missing/declaration and unreviewed-code/command fallback controls pass'%elapsed)
    os.chdir(previous_cwd)
    cache=t/'cache';cache.mkdir()
    names={'run','models.pkg','route.tsv'} | {s+'.'+e for s in ('e2','e1','e3','e4','prune','lower','elf') for e in ('json','tbl','net')}
    for n in names: (cache/n).write_bytes(b'fixture')
    manifest={n:c.digest(cache/n) for n in names}
    (cache/'manifest.json').write_text(json.dumps(manifest))
    assert c.valid(cache,'1')
    (cache/'e3.net').write_bytes(b'corrupt'); assert not c.valid(cache,'1')
    (cache/'e3.net').write_bytes(b'fixture'); del manifest['e3.net']
    (cache/'manifest.json').write_text(json.dumps(manifest)); assert not c.valid(cache,'1')
    seed=t/'source';seed.mkdir()
    for d in ('exec','unisa','src','kernel','include','weights','seed'): (seed/d).mkdir()
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

subprocess.run([sys.executable, str(ROOT/"tests/provenancecheck.py")], cwd=ROOT, check=True, timeout=10)


def compilercheck_closure_controls(queue, source_root, declaration_path=None):
    """Real fingerprints on a private source fixture; no compiler execution."""
    import copy, hashlib, shutil
    source_root=pathlib.Path(source_root)
    declaration=json.loads(pathlib.Path(declaration_path or source_root/'tests/gatedeps.json').read_text())
    family={n:dict(declaration['families'][e['family']],command=e['command'],family=e['family']) for n,e in declaration['suites'].items() if n.startswith('exec-driver-') and e['command'][0]=='./exec/c/compilercheck.sh'}
    assert len(family)==7, 'compilercheck audited declarations missing'   # core-build added by 0.0.23 E
    records=[]
    with tempfile.TemporaryDirectory(prefix='compilercheck-closure-controls-') as td:
        temp=pathlib.Path(td); fixture=temp/'source';fixture.mkdir()
        required={'tests/gatequeue.py','tests/gate.sh','tests/bound.py','README.md'}
        for entry in family.values():
            required.update(entry['files']);required.update(entry['guards'])
            for tree in entry['reviewed_trees']:
                required.update(str(p.relative_to(source_root)) for p in (source_root/tree).rglob('*')
                    if p.is_file() and (tree in entry['all_files_trees'] or p.suffix in entry['inventory_suffixes']))
        for name in sorted(required-{''}):
            source=source_root/name
            try: source.stat()
            except FileNotFoundError: continue
            if source.is_file():
                target=fixture/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
        (fixture/'tests/docs-fixture.py').write_text('pass\n')
        (fixture/'tests/bound-fixture.py').write_text('pass\n')
        probe={n:e['command'] for n,e in family.items()}
        probe.update(docs=['python3','tests/docs-fixture.py'],bound=['python3','tests/bound-fixture.py'],unknown=['python3','unknown.py'])
        declared={'version':1,'families':copy.deepcopy(declaration['families']),
            'suites':{n:copy.deepcopy(declaration['suites'][n]) for n in family}}
        for n in ('docs','bound'):
            file='tests/'+n+'-fixture.py'
            declared['suites'][n]={'command':probe[n],'files':[file]+(['README.md'] if n=='docs' else []),
                'guards':{file:hashlib.sha256((fixture/file).read_bytes()).hexdigest()}}
        (fixture/'tests/gatedeps.json').write_text(json.dumps(declared))
        subprocess.run(['git','init','-q',str(fixture)],check=True,timeout=5)
        ua=temp/'ua';model=temp/'model';ua.write_text('#!/bin/sh\nexit 0\n');model.write_bytes(ua.read_bytes());ua.chmod(0o755);model.chmod(0o755)
        tool=temp/'tools';tool.mkdir();awk=tool/'awk';awk.write_text('#!/bin/sh\nexit 0\n');awk.chmod(0o755)
        environment={k:v for k,v in os.environ.items() if k not in ('UA','UA_RUN','MODEL_COM','TOOLS_UA','CORPUS_UA','UNISA_CONTAINER','UNISA_KERNEL','TERM_SESSION_ID')}
        environment.update(UA=str(ua),MODEL_COM=str(model),PATH=str(tool)+os.pathsep+environment.get('PATH',''))
        oldcwd=pathlib.Path.cwd()
        try:
            with patch.object(queue,'ROOT',fixture),patch.dict(os.environ,environment,clear=True):
                os.chdir(fixture)
                def compare(label, mutation, expected, restore):
                    before=queue.fingerprint(probe);mutation();after=queue.fingerprint(probe)
                    changed={n for n in probe if before[n]!=after[n]}
                    assert changed==set(expected),(label,changed,set(expected))
                    saved={'stamp':before,'jobs':probe,'exclusive':[],'results':{n:{'rc':0} for n in probe}}
                    queue.resume(saved,after,probe,set())
                    assert set(saved['results'])==set(probe)-set(expected)
                    restore();records.append({'control':label,'invalidated':sorted(changed),'pass':True})
                def edit(label,path,expected):
                    raw=path.read_bytes();compare(label,lambda:path.write_bytes(raw+b'\n# fixture edit\n'),expected,lambda:path.write_bytes(raw))
                all_family=set(family); unknown={'unknown'}
                # A real reviewed snapshot must qualify; README stability proves the branch.
                edit('README-real-reviewed-snapshot',fixture/'README.md',{'docs','unknown'})
                records.append({'control':'real-main-inventory-qualified','pass':True,
                    'reviewed_trees':declaration['families']['compilercheck']['reviewed_trees']})
                ignored=fixture/'exec/build/cache.c';ignored.parent.mkdir(exist_ok=True)
                compare('ignored-build-output-new',lambda:ignored.write_text('generated v1\n'),set(),lambda:ignored.unlink())
                ignored.write_text('generated v1\n')
                edit('ignored-build-output-change',ignored,set())
                ignored.unlink()
                edit('checker',fixture/'exec/c/compilercheck.py',all_family|unknown)
                edit('actual-generator',fixture/'exec/pp/gen-manifest.tsv',all_family|unknown)
                edit('same-path-UA',ua,all_family|unknown)
                edit('same-path-MODEL_COM',model,all_family|unknown)
                edit('actual-awk-tool',awk,all_family)
                added=fixture/'exec/c/new-unreviewed.py'
                compare('new-code',lambda:added.write_text('pass\n'),all_family|unknown,lambda:added.unlink())
                removed_file=fixture/'exec/c/compilercheck.py';original_bytes=removed_file.read_bytes();mode=removed_file.stat().st_mode
                compare('missing-code',lambda:removed_file.unlink(),all_family|unknown,lambda:(removed_file.write_bytes(original_bytes),removed_file.chmod(mode)))
                # Once a code guard changes, README must trigger conservative fallback.
                checker=fixture/'exec/c/compilercheck.py';raw=checker.read_bytes();checker.write_bytes(raw+b'\n# unreviewed\n')
                edit('guard-change-falls-back',fixture/'README.md',all_family|{'docs','unknown'});checker.write_bytes(raw)
                removed=os.environ.pop('UA')
                edit('missing-UA-falls-back',fixture/'README.md',all_family|{'docs','unknown'})
                edit('fallback-keeps-actual-tools',awk,all_family)
                os.environ['UA']=removed
                original=probe['exec-driver-core-modes'];probe['exec-driver-core-modes']=original+['unknown-argument']
                edit('unknown-command-falls-back',fixture/'README.md',{'exec-driver-core-modes','docs','unknown'});probe['exec-driver-core-modes']=original
                before=queue.fingerprint(probe)
                with patch.dict(os.environ,{'UNISA_MAXSTEPS':'123'}):
                    assert all(queue.fingerprint(probe)[n]!=before[n] for n in probe)
                records.append({'control':'environment','pass':True})
                with patch.dict(os.environ,{'TERM_SESSION_ID':'transport-only'}):assert queue.fingerprint(probe)==before
                records.append({'control':'transport-excluded','pass':True})
                with patch.object(queue.platform,'platform',return_value='different-platform'):
                    assert all(queue.fingerprint(probe)[n]!=before[n] for n in probe)
                records.append({'control':'platform','pass':True})
                real_which=queue.shutil.which
                with patch.object(queue.shutil,'which',side_effect=lambda name:None if name=='awk' else real_which(name)):
                    edit('missing-actual-tool-falls-back',fixture/'README.md',all_family|{'docs','unknown'})
                bound_before=queue.fingerprint(probe);ua_raw=ua.read_bytes();ua.write_bytes(ua_raw+b'# changed\n');bound_after=queue.fingerprint(probe);ua.write_bytes(ua_raw)
                assert bound_before['bound']==bound_after['bound'] and bound_before['docs']==bound_after['docs']
                records.append({'control':'docs-bound-external-byte-semantics-preserved','pass':True})
        finally:os.chdir(oldcwd)
    print('compilercheck closures: %d private real-fingerprint controls passed; unknown remains global'%len(records))
    return records

compilercheck_closure_controls(q, ROOT)
