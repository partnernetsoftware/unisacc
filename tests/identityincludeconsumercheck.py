#!/usr/bin/env python3
"""Original buildlibrary consumer: include refusal and staging-only control; no library compilation."""
import argparse,hashlib,json,os,pathlib,subprocess,sys,tarfile,tempfile,time
sys.dont_write_bytecode=True
ROOT=pathlib.Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
a=argparse.ArgumentParser(description=__doc__);a.add_argument('--package',type=pathlib.Path,required=True);args=a.parse_args()
start=time.monotonic();package=args.package.resolve(strict=True);package_sha=sha(package);steps=[]
with tempfile.TemporaryDirectory(prefix='include-consumer-smoke-') as tmp:
 tmp=pathlib.Path(tmp);root=tmp/'source';root.mkdir();archive=tmp/'source.tar'
 with archive.open('wb') as f:
  q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'3','git','-c','tar.umask=022','archive','HEAD'],cwd=ROOT,stdout=f,stderr=subprocess.PIPE,timeout=4)
 assert q.returncode==0,('archive',q.returncode,q.stderr)
 with tarfile.open(archive) as tf:tf.extractall(root,filter='data')
 header=root/'include/unisa_sha256.h';saved=header.read_bytes();marker=tmp/'staging.json';sentinel=tmp/'stop-before-compiler.py'
 sentinel.write_text('import json,pathlib,sys\np=pathlib.Path(sys.argv[-1]).parent\nassert (p/"exec/c/libunisacc.c").is_file()\nassert (p/"include/unisa_sha256.h").read_bytes()==pathlib.Path('+repr(str(header))+').read_bytes()\npathlib.Path('+repr(str(marker))+').write_text(json.dumps({"staging_complete":True,"library_compilation":False}))\nprint("STAGING_COMPLETE_STOP_BEFORE_COMPILATION",file=sys.stderr)\nraise SystemExit(86)\n')
 def invoke(case):
  out=tmp/case;cmd=[sys.executable,str(ROOT/'tests/bound.py'),'3',sys.executable,str(root/'exec/c/buildlibrary.py'),'--package',str(package),'--output',str(out),'--cc',sys.executable+' '+str(sentinel),'--ar','/bin/false'];t=time.monotonic()
  q=subprocess.run(cmd,capture_output=True,timeout=4,env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1'))
  steps.append({'case':case,'rc':q.returncode,'seconds_monotonic':time.monotonic()-t,'stderr':q.stderr.decode(errors='replace'),'stdout_sha256':hashlib.sha256(q.stdout).hexdigest()})
  for name in ('manifest.json','libunisacc.a','libunisacc.so','libunisacc.dylib'):
   try:(out/name).stat()
   except FileNotFoundError:continue
   raise AssertionError('unexpected library output '+name)
  return q
 header.unlink();q=invoke('missing');assert q.returncode==1 and not q.stdout and q.stderr==('buildlibrary: runtime include missing: '+str(header)+'\n').encode(),steps[-1]
 try:marker.stat()
 except FileNotFoundError:pass
 else:raise AssertionError('missing header reached compiler boundary')
 header.write_bytes(saved);q=invoke('complete');assert q.returncode==1 and b'STAGING_COMPLETE_STOP_BEFORE_COMPILATION' in q.stderr,steps[-1]
 assert json.loads(marker.read_text())=={'staging_complete':True,'library_compilation':False}
assert sha(package)==package_sha
elapsed=time.monotonic()-start;assert elapsed<=10,elapsed
print(json.dumps({'status':'PASS_EXPECTED_REFUSAL_AND_STAGING_CONTROL','scope':'STAGING_ONLY_NOT_LIBRARY_COMPILATION','no_product_execution_or_cold_build':True,'package_sha256':package_sha,'seconds_monotonic':elapsed,'steps':steps},sort_keys=True))
