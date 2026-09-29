#!/usr/bin/env python3
"""Mechanical model certificate seam; manually supplied fixture is not ABI proof."""
import argparse,hashlib,json,pathlib,shutil,subprocess,tempfile
from librarysignature3check import U,d,entry,agg,wire
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=pathlib.Path);a=ap.parse_args();out=a.out or pathlib.Path(tempfile.mkdtemp(prefix='r10-ordered-host-'));out.mkdir(parents=True,exist_ok=True)
 h=out/'headers';h.mkdir(exist_ok=True)
 for p in (ROOT/'exec/c').glob('*.h'):shutil.copy2(p,h/p.name)
 shutil.copy2(ROOT/'tests/libraryorderedcarriercheck.c',out/'probe.c')
 t=agg([entry(0,1,d(),bw=3),entry(1,2,d(),bo=3,bw=5),entry(2,3,d(),off=4)])
 o=wire('round',t,[t]);c=wire('round',d(version=2),[d(version=2)],support=1,version=2)
 def cert(original=o,carrier=c,target=b'osx/arm64'):return b'USLNCAR1\n'+U(len(target))+target+U(len(original))+original+U(len(carrier))+carrier
 good=cert();cases=[(0,good)]+[(1,good[:i]) for i in range(len(good))]
 cases += [(1,x) for x in [good+b'x',cert(target=b'lnx/arm64'),cert(carrier=wire('other',d(version=2),[d(version=2)],support=1,version=2)),cert(carrier=wire('round',d(1,8,8,version=2),[d(1,8,8,version=2)],support=1,version=2)),cert(carrier=c[:-1]+bytes([0])),cert(original=o[:-1]+bytes([1]))]]
 (out/'cases').write_bytes(U(len(cases))+b''.join(U(w)+U(len(b))+b for w,b in cases))
 commands=[]
 for cmd in [['cc','-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',str(h),str(out/'probe.c'),'-lffi','-o',str(out/'probe')],[str(out/'probe'),str(out/'cases')]]:
  p=subprocess.run(cmd,capture_output=True,text=True,timeout=20);commands.append(dict(command=cmd,rc=p.returncode,stdout=p.stdout,stderr=p.stderr))
  if p.returncode:break
 evidence=dict(status='passed' if len(commands)==2 and not commands[-1]['rc'] else 'failed',cases=len(cases),commands=commands,headers={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in h.glob('*.h')},scope='internal mechanical certificate, not public ABI certification')
 (out/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n');print(json.dumps(evidence));return evidence['status']!='passed'
if __name__=='__main__':raise SystemExit(main())
