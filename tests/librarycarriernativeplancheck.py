#!/usr/bin/env python3
"""Native plan carrier mechanics with explicit fixtures, not public acceptance."""
import argparse,hashlib,json,os,platform,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
U=lambda n:struct.pack('<Q',n)
def desc(k,w,a,uns=0,depth=0,tag=0,payload=b''):
 return b''.join(U(n) for n in (depth,0,0,k,w,uns,a))+bytes([tag])+U(len(payload))+payload
def main():
 p=argparse.ArgumentParser();p.add_argument('--arch',choices=['arm64','x86_64']);p.add_argument('--evidence',type=Path);a=p.parse_args()
 host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host;flags=['-arch',arch] if platform.system()=='Darwin' else []
 I=desc(1,8,8,1);D=desc(3,8,8);union=desc(5,8,8,tag=2,payload=U(2)+U(0)*3+U(8)+D+U(0)*3+U(8)+I)
 def sig(t,support=0,var=0,result=None):return b'USLSIG2\n'+U(1)+U(5)+b'round'+bytes([0,1,var,1])+U(1)+(result if result is not None else t)+U(1)+t+bytes([support])
 original=sig(union);carrier=sig(I,1);target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch;tb=target.encode()
 certificate=b'USLNCAR1\n'+U(len(tb))+tb+U(len(original))+original+U(len(carrier))+carrier
 leaf=bytes([1,0])+U(1)+bytes([0,0])+U(1)+I+U(1)+I+bytes([1])
 callback=desc(4,8,8,depth=1,tag=4,payload=leaf)
 files=[certificate,original,carrier,sig(I),sig(union,var=1),sig(callback,result=I),sig(union,1)]
 records=[]
 with tempfile.TemporaryDirectory(prefix='r10-carrier-nativeplan-') as tmp:
  t=Path(tmp);rt=t/'headers';rt.mkdir()
  for f in (ROOT/'exec/c').glob('*.h'):shutil.copy2(f,rt/f.name)
  shutil.copy2(ROOT/'tests/librarycarriernativeplancheck.c',t/'probe.c')
  names=[]
  for i,data in enumerate(files):f=t/str(i);f.write_bytes(data);names.append(f)
  for san in (False,True):
   exe=t/('san' if san else 'normal');commands=[[os.environ.get('CC','cc'),*flags,'-std=c11','-O1','-Wall','-Wextra','-Wno-unused-function','-Wno-misleading-indentation','-I',str(rt),str(t/'probe.c'),'-lffi','-o',str(exe),*(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if san else [])], [*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),str(exe),target,*map(str,names)]]
   for command in commands:
    r=subprocess.run([str(ROOT/'tests/bound'),'20',*command],capture_output=True,text=True);records.append({'arch':arch,'sanitize':san,'command':command,'rc':r.returncode,'stdout':r.stdout,'stderr':r.stderr});print(r.stdout,end='')
    if r.returncode:raise AssertionError(records[-1])
 record={'scope':'explicit fixtures; carrier native plan mechanism only, no model/public acceptance','commands':records,'inputs':{str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest() for f in [ROOT/'exec/c/librarynative.h',ROOT/'exec/c/librarycarrierplan.h',ROOT/'tests/librarycarriernativeplancheck.c']},'status':'passed'}
 if a.evidence:a.evidence.write_text(json.dumps(record,indent=2)+'\n')
 print('carrier native plan normal + ASan/UBSan passed on '+arch)
if __name__=='__main__':main()
