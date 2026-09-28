#!/usr/bin/env python3
"""Explicit graph-pair fixture; no model/public callback qualification claim."""
import argparse,hashlib,json,os,platform,shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--arch',choices=['arm64','x86_64']);p.add_argument('--evidence',type=Path);a=p.parse_args();host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host;flags=['-arch',arch] if platform.system()=='Darwin' else [];records=[]
 with tempfile.TemporaryDirectory(prefix='r10-carrier-callback-') as tmp:
  t=Path(tmp);rt=t/'headers';rt.mkdir()
  for f in (ROOT/'exec/c').glob('*.h'):shutil.copy2(f,rt/f.name)
  shutil.copy2(ROOT/'tests/librarycarriercallbackcheck.c',t/'probe.c');inputs={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in rt.iterdir()}
  for san in (False,True):
   exe=t/('san' if san else 'normal');commands=[[os.environ.get('CC','cc'),*flags,'-std=c11','-O1','-Wall','-Wextra','-Wno-unused-function','-Wno-misleading-indentation','-I',str(rt),str(t/'probe.c'),'-lffi','-o',str(exe),*(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if san else [])], [*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),str(exe)]]
   for command in commands:
    r=subprocess.run([str(ROOT/'tests/bound'),'20',*command],capture_output=True,text=True);records.append({'arch':arch,'sanitize':san,'command':command,'rc':r.returncode,'stdout':r.stdout,'stderr':r.stderr});print(r.stdout,end='')
    if r.returncode:
     if a.evidence:a.evidence.write_text(json.dumps({'status':'failed','commands':records,'inputs':inputs},indent=2)+'\n')
     raise AssertionError(records[-1])
  if a.evidence:a.evidence.write_text(json.dumps({'status':'passed','scope':'mechanical explicit graph-pair fixture, not model/public acceptance','commands':records,'inputs':inputs,'probe_sha256':hashlib.sha256((t/'probe.c').read_bytes()).hexdigest()},indent=2)+'\n')
 print('paired carrier callbacks normal + ASan/UBSan passed on '+arch)
if __name__=='__main__':main()
