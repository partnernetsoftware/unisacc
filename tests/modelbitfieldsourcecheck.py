#!/usr/bin/env python3
"""Actual source bitfields -> exact USLSIG2 storage facts, sim and C network."""
import argparse,hashlib,json,os,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
from modelcallbacksourcecheck import decode,U,Files
SOURCE='\n'.join('struct B%d {%s a:%d;}; struct B%d f%d(struct B%d x){return x;}'%(i,ty,bits,i,i,i) for i,(ty,bits,size,uns) in enumerate([('unsigned int',3,4,1),('signed int',3,4,0),('unsigned long',33,8,1),('signed long',33,8,0)]))
CASES=[(3,4,1),(3,4,0),(33,8,1),(33,8,0)]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--model',type=pathlib.Path);ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args()
 evidence={'scope':'source signature facts only; bitfield support remains zero','status':'failed','commands':[]}
 with tempfile.TemporaryDirectory(prefix='r10-bitfield-source-') as name:
  t=pathlib.Path(name)
  def cmd(*args):
   c=[str(ROOT/'tests/bound'),'20',*map(str,args)];p=subprocess.run(c,capture_output=True,timeout=25,env=dict(os.environ,UA_TYPESPELL='1'))
   evidence['commands'].append({'command':c,'rc':p.returncode,'stderr':p.stderr.decode(errors='replace')});assert p.returncode==0,(c,p.returncode,p.stderr);return p.stdout
  try:
   cmd(ROOT/'tests/build_ref.sh',t/'ref.c',t/'ref')
   s=(t/'ref.c').read_bytes();old=b'        if (tkind[i] == 2) { __write(1, "=", 1);'
   new=b'        if (L == 4 && TOKV[p] == 116 && TOKV[p+1] == 121 && TOKV[p+2] == 112 && TOKV[p+3] == 101 && getenv("UA_TYPESPELL")) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }\n'+old
   assert s.count(old)==1;s=s.replace(old,new);(t/'dump.c').write_bytes(s)
   cmd('cc','-w','-std=c99','-O2',t/'dump.c','-o',t/'dump');cmd('cc','-O2',ROOT/'exec/c/run.c','-o',t/'run')
   model=a.model or t/'e3.json'
   if not a.model:cmd(sys.executable,ROOT/'exec/parse2/gen2.py',model)
   evidence['model_sha256']=hashlib.sha256(model.read_bytes()).hexdigest()
   d=json.loads(model.read_text());loaded=load(d);cmd(sys.executable,ROOT/'exec/c/tbl.py',model,t/'e3.tbl');cmd(sys.executable,ROOT/'exec/c/net.py',t/'e3.tbl',t/'e3.net')
   evidence['full_domain']=cmd(t/'run','--check-net',t/'e3.tbl',t/'e3.net').decode().strip()
   (t/'routes').write_text('parse\te3\ttokens\ttape\te3.net\n');rd=t/'r';rd.mkdir();(rd/'module').write_bytes(U(1));(rd/'symbols').write_bytes(U(1));(t/'p').write_bytes(build([t/'routes'],[('006c6962726172792f',rd)],cache=False))
   (t/'s.c').write_text(SOURCE);raw=cmd(t/'dump','-dump-tokens',t/'s.c');(t/'tok').write_bytes(raw)
   status,out,_=run(d,raw,'source',files=Files(),loaded=loaded,maxsteps=5000000);assert status=='accept',(status,out[:300]);native=cmd(t/'run','--bundle',t/'p','parse',t/'tok');assert native==out
   _,records=decode(out);evidence['observed']=[]
   for i,(bits,size,uns) in enumerate(CASES):
    rec=records['f'+str(i)];assert rec['count']==1 and rec['support']==0
    for role,obj in [('result',rec['result']),('parameter',rec['params'][0])]:
     assert obj['tag']==1 and obj['fields'][3:]==(5,size,0,size)
     assert len(obj['child'])==1;layout,leaf=obj['child'][0];evidence['observed'].append({'case':i,'role':role,'layout':layout,'leaf':leaf['fields']})
     assert layout==(0,0,bits,size),(i,role,layout,'expected',(0,0,bits,size))
     assert leaf['fields'][3:]==(1,size,uns,size)
   evidence['status']='passed'
  except Exception as e:evidence['error']=repr(e)
 if a.evidence:a.evidence.write_text(json.dumps(evidence,indent=2)+'\n')
 print(json.dumps(evidence));return 0 if evidence['status']=='passed' else 1
if __name__=='__main__':raise SystemExit(main())
