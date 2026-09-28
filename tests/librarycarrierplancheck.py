#!/usr/bin/env python3
"""Internal model-certificate framing, not native union ABI qualification."""
import argparse
import json
import hashlib
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
U=lambda n:struct.pack('<Q',n)
def descriptor(k,w,al,uns=0,tag=0,payload=b''):
    return b''.join(U(n) for n in (0,0,0,k,w,uns,al))+bytes([tag])+U(len(payload))+payload
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--evidence',type=Path);ap.add_argument('--model-plan',type=Path);args=ap.parse_args()
    d=descriptor(3,8,8);u=descriptor(1,8,8,1)
    union=descriptor(5,8,8,tag=2,payload=U(2)+U(0)*3+U(8)+d+U(0)*3+U(8)+u)
    def signature(t,name=b'round',mode=0,supported=0):
        return b'USLSIG2\n'+U(1)+U(len(name))+name+bytes([0,1,0,mode])+U(1)+t+U(1)+t+bytes([supported])
    original=signature(union);carrier=signature(u,supported=1)
    def certificate(o=original,c=carrier,target=b'osx/arm64'):
        return b'USLNCAR1\n'+U(len(target))+target+U(len(o))+o+U(len(c))+c
    good=certificate();cases=[(0,good)]
    cases.extend((1,good[:i]) for i in range(len(good)))
    cases.extend((1,p) for p in [good+b'x',b'X'+good[1:],certificate(target=b'osx/x86_64'),
        certificate(c=signature(u,name=b'other',supported=1)),
        certificate(c=signature(descriptor(1,4,4,1),supported=1)),
        certificate(c=signature(u,mode=1,supported=1)),certificate(c=signature(u,supported=0)),
        certificate(o=original[:-1]),certificate(c=carrier[:-1])])
    cases.append((0,good))
    if args.model_plan:cases.append((0,args.model_plan.read_bytes()))
    with tempfile.TemporaryDirectory(prefix='r10-carrier-frame-') as td:
        t=Path(td);rt=t/'exec/c';rt.mkdir(parents=True)
        for p in (ROOT/'exec/c').glob('*.h'):shutil.copy2(p,rt/p.name)
        shutil.copy2(ROOT/'tests/librarycarrierplancheck.c',t/'probe.c')
        (t/'cases').write_bytes(U(len(cases))+b''.join(U(w)+U(len(p))+p for w,p in cases))
        commands=[]
        for cmd in [['cc','-std=c11','-O1','-fsanitize=address,undefined','-I',str(rt),str(t/'probe.c'),'-lffi','-o',str(t/'probe')],
                    [str(t/'probe'),str(t/'cases'),'osx/arm64']]:
            p=subprocess.run([str(ROOT/'tests/bound'),'20',*cmd],capture_output=True,text=True,timeout=25)
            commands.append({'command':cmd,'rc':p.returncode,'stdout':p.stdout,'stderr':p.stderr})
            if p.returncode:break
        record={'scope':'Internal framing/transaction test; fixture carrier is not model certification or public ABI acceptance',
                'model_plan_sha256':hashlib.sha256(args.model_plan.read_bytes()).hexdigest() if args.model_plan else None,'cases':len(cases),'commands':commands,'inputs':{str(p.relative_to(t)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(rt.glob('*.h'))},'probe_sha256':hashlib.sha256((t/'probe.c').read_bytes()).hexdigest(),'cases_sha256':hashlib.sha256((t/'cases').read_bytes()).hexdigest(),'status':'passed' if len(commands)==2 and not commands[-1]['rc'] else 'failed'}
        if args.evidence:args.evidence.write_text(json.dumps(record,indent=2)+'\n')
        print(json.dumps(record,indent=2));return 0 if record['status']=='passed' else 1
if __name__=='__main__':raise SystemExit(main())
