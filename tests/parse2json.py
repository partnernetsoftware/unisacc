#!/usr/bin/env python3
"""Shared content-keyed cache of the plain `exec/build/gen.py parse2` JSON model.

The five lib-*-source checks each read the parse2 model as JSON (the simulator
needs it, not only .tbl/.net), and its cold construction alone is ~32 s, so a
20 s per-step watchdog never let them pass after K2.  fetch(dest) copies a
digest-checked cached copy (key: compilerpack's closure hash, so any generator
or facts change rebuilds); a miss constructs it under a lock.  Run as a script
(gate job lib-source-prep) it only warms the cache; the checks still self-build
on a miss, so queue order does not matter.   usage: parse2json.py [DEST]"""
import fcntl,hashlib,json,os,pathlib,shutil,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(R/'exec/c'))
import compilerpack
GEN=R/'exec/build/gen.py'
def _sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def fetch(dest):
    dest=pathlib.Path(dest);h,names=compilerpack._keybase()
    key=hashlib.sha256(json.dumps(['parse2json-v1',h,str(GEN),[(k,os.environ.get(k)) for k in names]]).encode()).hexdigest()
    base=pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'));base.mkdir(parents=True,exist_ok=True)
    m=base/('parse2json-'+key+'.json');d=base/('parse2json-'+key+'.sha')
    with (base/('parse2json-'+key+'.lock')).open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        try: ok=d.read_text()==_sha(m)
        except FileNotFoundError: ok=False
        if not ok:
            w=base/('parse2json-'+key+'.%d.tmp'%os.getpid())
            subprocess.run([str(R/'tests/bound'),'58',sys.executable,str(GEN),'parse2',str(w)],check=True,timeout=60)
            w.rename(m);d.write_text(_sha(m))
        shutil.copyfile(m,dest)
    return dest
if __name__=='__main__':
    with tempfile.TemporaryDirectory(prefix='parse2json-') as t:
        out=fetch(sys.argv[1] if len(sys.argv)>1 else pathlib.Path(t)/'e3.json');print('parse2json cached',_sha(pathlib.Path(out))[:16])
