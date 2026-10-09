#!/usr/bin/env python3
"""0.0.38 scheduler controls (P3/P6/P7, full038/full038b): UNVERIFIED rc 77, parent deadline cap and
unschedulable budget, outer-kill INTERRUPTED, stall stop, window span, attempt kind at admission,
tail budget with SIGTERM-ignoring leftovers, segment stage events.  Split from queuecheck.py so each
stays inside its 60 s bound."""
import contextlib, importlib.util, io, json, os, pathlib, sys, tempfile
from unittest.mock import patch
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
def load(name, path):
    spec=importlib.util.spec_from_file_location(name,path); m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
q=load('queue_under_test',ROOT/'tests/gatequeue.py')
real_fingerprint=q.fingerprint
with tempfile.TemporaryDirectory() as td:
    t=pathlib.Path(td); stamp=['same']
    q.plan=lambda com:jobs
    q.fingerprint=lambda plan:stamp[0]
    def run(state,window=55,extra=()):
        sys.argv=['gatequeue.py','--state',str(t/state),'--window',str(window),*extra]
        with contextlib.redirect_stdout(io.StringIO()): return q.main()
    # 0.0.38 P3/P7: UNVERIFIED, parent deadline, outer-kill interruption, stall stop
    jobs={'hostonly':[sys.executable,'-c','print("UNVERIFIED: required=osx/arm64 observed=lnx/x86_64 missing=native Darwin"); raise SystemExit(77)']}
    assert run('unverified')==4, 'UNVERIFIED must not pass the queue'
    r=json.loads((t/'unverified/results.json').read_text())['results']['hostonly']
    assert r['rc']==77 and r['status']=='UNVERIFIED'
    import time as _t
    jobs={'short':[sys.executable,'-c','pass']}
    assert run('nobudget',55,('--parent-deadline',str(_t.monotonic()+3)))==3, 'no budget must be reported, not run'
    assert 'short' not in json.loads((t/'nobudget/results.json').read_text())['results']
    jobs={'quick':[sys.executable,'-c','pass']}
    assert run('capped',55,('--parent-deadline',str(_t.monotonic()+12)))==0
    lim=json.loads((t/'capped/results.json').read_text())['results']['quick']['limit']
    assert lim<=9, 'job limit must respect the parent deadline: %r'%lim
    # an outer kill leaves inflight: first counts an interruption (no synthetic rc), a killed window
    # that completed nothing stops the driver; the second interruption is the INTERRUPTED result
    jobs={'victim':[sys.executable,'-c','pass'],'other':[sys.executable,'-c','pass']}
    d=t/'killed'; d.mkdir()
    (d/'results.json').write_text(json.dumps({'stamp':stamp[0],'jobs':jobs,'exclusive':[],'results':{},'inflight':{'victim':{'limit':48}},'window':{'completed':0}}))
    assert run('killed')==3, 'killed window with no progress must stop'
    k=json.loads((d/'results.json').read_text())
    assert k['interrupted']=={'victim':1} and 'victim' not in k['results'] and 'inflight' not in k
    k.update(stalled=0, window={'completed':1}, inflight={'victim':{'limit':48}}); (d/'results.json').write_text(json.dumps(k))
    assert run('killed')==1
    v=json.loads((d/'results.json').read_text())['results']['victim']
    assert v['status']=='INTERRUPTED' and v['rc'] is None, v
    # three windows in a row without any job exit stop the driver instead of 75 forever
    jobs={'never':[sys.executable,'-c','pass']}
    d=t/'stall'; d.mkdir()
    (d/'results.json').write_text(json.dumps({'stamp':stamp[0],'jobs':jobs,'exclusive':[],'results':{},'stalled':3}))
    assert run('stall')==3
    # a job that had its early slot (0.0.37 rowcov) gets a full attempt, not endless deferrals
    jobs={'long':[sys.executable,'-c','import time; time.sleep(30)']}
    d=t/'redefer'; d.mkdir()
    (d/'results.json').write_text(json.dumps({'stamp':stamp[0],'jobs':jobs,'exclusive':[],'results':{},'fullwindow':['long'],'stalled':2}))
    rc=run('redefer',55,('--parent-deadline',str(_t.monotonic()+9)))
    st=json.loads((d/'results.json').read_text())
    # at a window's start a long job is a full attempt: it is retried alone, not deferred again
    assert rc==75 and 'long' in st.get('retried',[]) and not st.get('deferred'), (rc, st.get('retried'), st.get('deferred'))
    # an external header the host plan declares is part of the identity: editing it voids reuse
    inc=t/'csinc'; inc.mkdir(); (inc/'csmith.h').write_text('a\n')
    with patch.dict(os.environ,{'CSMITH_INCLUDE':str(inc)}):
        one=real_fingerprint({'x':['true']}); (inc/'csmith.h').write_text('b\n'); two=real_fingerprint({'x':['true']})
    assert one!=two, 'csmith header change kept the old identity'
    # a different deadline alone is scheduling, not identity: the finished result is reused
    jobs={'quick':[sys.executable,'-c','pass']}
    before=json.loads((t/'capped/results.json').read_text())['results']['quick']
    assert run('capped',55,('--parent-deadline',str(_t.monotonic()+40)))==0
    assert json.loads((t/'capped/results.json').read_text())['results']['quick']==before, 'deadline change reran a finished job'
    # P6: each window records prologue/jobs/epilogue and logs them as paired events when asked
    home=t/'home'; home.mkdir()
    with patch.dict(os.environ,{'HOME':str(home)}):
        assert run('segments',55,('--stagelog-run','qtest','--stagelog-parent','e-0123'))==0
    w=json.loads((t/'segments/results.json').read_text())['window']
    assert all(isinstance(w[k],float) for k in ('prologue_s','jobs_s','epilogue_s')), w
    ev=[json.loads(l) for l in (home/'.unisacc/stagelog/events.jsonl').read_text().splitlines()]
    assert [e['subphase'] for e in ev if e['event']=='begin']==['prologue','jobs','epilogue'] and len(ev)==6, ev
    # full038: under a parent deadline a job longer than any window gets a full attempt, a solo retry,
    # then a timeout result -- never 'late fill' deferrals until nothing is admissible and the queue stalls
    jobs={'toolong':[sys.executable,'-c','import time; time.sleep(60)']}
    seen=[]
    for _ in range(4):
        rc=run('span',55,('--parent-deadline',str(_t.monotonic()+8))); seen.append(rc)
        if rc!=75: break
    st=json.loads((t/'span/results.json').read_text())
    assert st['results'].get('toolong',{}).get('rc')==142 and not st.get('deferred'), (seen, st)
    # combined boundary (cdx2): parent deadline x exclusive x an inflated history estimate x a pending
    # late-fill marker.  Every path must end in a decided result within a bounded number of windows,
    # no job limit may exceed the parent's span, and no window may sit idle with work pending.
    jobs={'hog':[sys.executable,'-c','import time; time.sleep(60)'], 'quick':[sys.executable,'-c','pass']}
    d=t/'combo'; d.mkdir()
    (d/'results.json').write_text(json.dumps({'stamp':stamp[0],'jobs':jobs,'exclusive':['hog'],'results':{},
                                               'fullwindow':['hog'],'deferred':[{'name':'hog','limit':5}]}))
    with patch.dict(os.environ,{'TMPDIR':str(d)}):   # private history: the estimate starts cold
        for _ in range(6):
            dl=_t.monotonic()+8
            rc=run('combo',55,('--exclusive-suite','hog','--parent-deadline',str(dl)))
            st=json.loads((d/'results.json').read_text())
            if rc!=75: break
    assert rc==1 and st['results']['hog']['rc']==142 and st['results']['quick']['rc']==0, (rc, st)
    assert all(r['limit']<=6 for r in st['results'].values()), 'a limit exceeded the parent span: %r'%st['results']
    # full038b: four leftovers that ignore SIGTERM at the window's end are reaped within the tail budget;
    # the whole tail (reaping, saves, refingerprint) is measured and reserved for the next window
    stub='import signal,time; signal.signal(signal.SIGTERM, signal.SIG_IGN); time.sleep(120)'
    jobs={'deaf%d'%k:[sys.executable,'-c',stub] for k in range(4)}
    t0=_t.monotonic(); rc=run('deaf',55,('--parent-deadline',str(_t.monotonic()+9))); took=_t.monotonic()-t0
    st=json.loads((t/'deaf/results.json').read_text())
    assert took < 9+1, 'window overran its parent deadline: %.1fs'%took
    assert st.get('epilogue_max',0) > 0 and 'inflight' not in st or not st['inflight'], st.get('epilogue_max')
    # full038b rounding: a fractional span (x.9 s) admitted at the window's start is a full attempt --
    # recorded as such at admission -- so its timeout is a solo retry, not a tail-fill deferral
    jobs={'frac':[sys.executable,'-c','import time; time.sleep(60)']}
    assert run('frac',55,('--parent-deadline',str(_t.monotonic()+7.9)))==75
    st=json.loads((t/'frac/results.json').read_text())
    assert st.get('retried')==['frac'] and not st.get('deferred'), (st.get('retried'), st.get('deferred'))
    print('queue: UNVERIFIED rc77, parent deadline cap/unschedulable, outer-kill INTERRUPTED and stall controls pass')
