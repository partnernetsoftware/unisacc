#!/usr/bin/env python3
"""Exercise release acceptance contracts with a private queue stub; no real gate/VM."""
import hashlib,json,os,pathlib,shutil,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='release-contract-') as td:
    p=pathlib.Path(td);t=p/'tests';t.mkdir()
    for name in ('release.sh','bound.py'):shutil.copy2(ROOT/'tests'/name,t/name)
    candidate=p/'candidate';candidate.write_text('#!/bin/sh\nexit 0\n');candidate.chmod(0o755)
    reference=p/'reference';shutil.copy2(candidate,reference)
    (t/'gatequeue.py').write_text('''import json,os,pathlib,sys
assert sys.argv[1:6]==['--com','--jobs','2','--window','50']
assert sys.argv[6:12]==[part for flag in ('Wall','Wextra','Werror') for part in ('--exclusive-suite','exec-warningdriver-ua-'+flag)]
if os.uname().sysname=='Darwin':assert sys.argv[12:14]==['--exclusive-suite','exec-bindx86']
assert os.environ['STRICT']=='1'
p=pathlib.Path(sys.argv[-1]);p.mkdir(parents=True,exist_ok=True)
mode=os.environ['PROBE_MODE']
if mode=='mutate':
 with open(os.environ['MODEL_COM'],'a') as f:f.write('# changed\\n')
if mode=='pending' and not (p/'continued').exists():
 (p/'continued').touch();sys.exit(75)
if mode=='failed':sys.exit(7)
jobs={} if mode=='empty' else {'com-probe':['true']}
(p/'results.json').write_text(json.dumps({'jobs':jobs,'results':{n:{'rc':0} for n in jobs}}))
(p/'com-probe.log').write_text(os.environ.get('PROBE_LOG','SKIPPED 1: target' if mode=='skip' else 'probe passed; skip 0; (no target skipped)'))
''')
    base=dict(os.environ,MODEL_COM=str(candidate),UA=str(reference),TERM_SH='0')
    base.pop('RELEASE_BOUND',None);base.pop('SUITES',None)
    def check(mode,want,**extra):
        env=dict(base,PROBE_MODE=mode,GATE_STATE=str(p/('queue-'+mode)),RELEASE_OUT=str(p/('out-'+mode)),**extra)
        x=subprocess.run(['bash',str(t/'release.sh')],env=env,capture_output=True,text=True,timeout=12)
        assert x.returncode==want,(mode,x.returncode,x.stdout,x.stderr)
        if want:assert not (pathlib.Path(env['RELEASE_OUT'])/'unisacc.com').exists(),mode
        return x
    check('missing',2,MODEL_COM=str(p/'absent'))
    check('disabled',2,SUITES='0')
    check('failed',7);check('skip',1);check('empty',1)
    for i,log in enumerate(('SKIPPED 0', 'skip 0', '  skip 0', '  skip  0', '(skipped 0)', 'no target skipped', '(no target skipped)')):
        check('zero'+str(i),0,PROBE_LOG=log)
    for i,log in enumerate(('SKIPPED 2: target', 'skip 3', '(skipped 1)', '  skip windows (not running)', 'fat skipped (macOS only)', 'ARM64 native: SKIPPED', 'clone failed -- skipped', 'probe skipped unknown target', 'probe SkIp unknown target')):
        check('positive'+str(i),1,PROBE_LOG=log)
    check('pending',75);x=check('pending',0)
    assert 'LOCAL gate passed' in x.stdout and 'release NOT ready' in x.stdout
    assert (p/'out-pending/unisacc.com').read_bytes()==candidate.read_bytes()
    # Run the real final evidence checker against private copied states. Queue
    # scheduling is tested above; these fixtures isolate local scope policy.
    checker=(t/'release.sh').read_text().split("<<'CHECK'\n",1)[1].rsplit('\nCHECK',1)[0]
    run_notice='  skip windows (-run needs the UTM machine started)'
    boot_notice='  skip Windows self-build (not run; independent proofs: --windows win/arm64 and --windows win/x86_64)'
    baseline=p/'scope-baseline';baseline.mkdir()
    names=('run','com-run','nativeboot')
    data={'jobs':{n:['true'] for n in names},'results':{n:{'rc':0} for n in names}}
    (baseline/'results.json').write_text(json.dumps(data))
    for n in names:
        notice=boot_notice if n=='nativeboot' else run_notice
        (baseline/(n+'.log')).write_text('local checks passed (skipped 0)\n'+notice+'\n')
    digest=hashlib.sha256(candidate.read_bytes()).hexdigest()
    def scopecheck(mode,want,mutate=None):
        state=p/('scope-'+mode);shutil.copytree(baseline,state)
        if mutate:mutate(state)
        out=p/('scope-out-'+mode)
        x=subprocess.run(['python3','-',str(state),str(candidate),digest,str(out)],
                         input=checker,capture_output=True,text=True,timeout=12)
        assert x.returncode==want,(mode,x.returncode,x.stdout,x.stderr)
        if want:assert not (out/'unisacc.com').exists(),mode
        return x
    x=scopecheck('normal',0)
    assert x.stdout.count('UNVERIFIED Windows obligation outside LOCAL gate:')==3,x.stdout
    assert 'LOCAL gate passed' in x.stdout and 'release NOT ready' in x.stdout
    def append_unknown(state):
        with (state/'run.log').open('a') as f:f.write('  skip unknown Windows target\n')
    scopecheck('unknown',1,append_unknown)
    def delete_result(state):
        q=state/'results.json';obj=json.loads(q.read_text());del obj['results']['com-run'];q.write_text(json.dumps(obj))
    scopecheck('missing-result',1,delete_result)
    scopecheck('missing-log',1,lambda state:(state/'run.log').unlink())
    scopecheck('empty-log',1,lambda state:(state/'run.log').write_text(''))
    scopecheck('notice-only',1,lambda state:(state/'run.log').write_text(run_notice+'\n'))
    scopecheck('wrong-suite',1,lambda state:(state/'nativeboot.log').write_text('passed\n'+run_notice+'\n'))
    scopecheck('duplicate',1,lambda state:(state/'run.log').write_text('passed\n'+run_notice+'\n'+run_notice+'\n'))
    scopecheck('nonzero-skip',1,lambda state:(state/'run.log').write_text('local (skipped 1)\n'+run_notice+'\n'))
    scopecheck('nonzero-rc',1,lambda state:(state/'results.json').write_text(json.dumps(dict(data,results={n:{'rc':1 if n=='run' else 0} for n in names}))))
    check('mutate',1)
    print('release contracts: missing/disabled/failed/skip/empty/mutation rejected; pending resumed; exact candidate copied; three exact scoped Windows notices reported unverified; copied-state faults rejected; external platforms remain unverified')
