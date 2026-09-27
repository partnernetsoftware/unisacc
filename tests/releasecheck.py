#!/usr/bin/env python3
"""Exercise release acceptance contracts with a private queue stub; no real gate/VM."""
import os,pathlib,shutil,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='release-contract-') as td:
    p=pathlib.Path(td);t=p/'tests';t.mkdir()
    for name in ('release.sh','bound.pl'):shutil.copy2(ROOT/'tests'/name,t/name)
    candidate=p/'candidate';candidate.write_text('#!/bin/sh\nexit 0\n');candidate.chmod(0o755)
    reference=p/'reference';shutil.copy2(candidate,reference)
    (t/'gatequeue.py').write_text('''import json,os,pathlib,sys
assert sys.argv[1:6]==['--com','--jobs','2','--window','50']
assert sys.argv[6:8]==['--exclusive-suite','exec-warningdriver']
if os.uname().sysname=='Darwin':assert sys.argv[8:10]==['--exclusive-suite','exec-bindx86']
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
    for i,log in enumerate(('SKIPPED 2: target', 'skip 3', '(skipped 1)', '  skip windows (not running)', 'fat skipped (macOS only)', 'ARM64 native: SKIPPED', 'clone failed -- skipped')):
        check('positive'+str(i),1,PROBE_LOG=log)
    check('pending',75);x=check('pending',0)
    assert 'LOCAL gate passed' in x.stdout and 'release NOT ready' in x.stdout
    assert (p/'out-pending/unisacc.com').read_bytes()==candidate.read_bytes()
    check('mutate',1)
    print('release contracts: missing/disabled/failed/skip/empty/mutation rejected; pending resumed; exact candidate copied; external platforms remain unverified')
