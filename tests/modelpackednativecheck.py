#!/usr/bin/env python3
"""Measured external packed layouts -> model certificate -> true C calls/closures.
AAPCS64 (osx/arm64) certifies declared-alignment integer-array carriers; SysV
(osx/x86_64) must refuse every packed object. Not source pack, FP inside packed,
packed bitfields, Win64 native execution or six-platform qualification.
"""
import argparse,hashlib,json,pathlib,platform,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run,load
from buildlibrary import provider_flags
from pack import build
U=lambda n:struct.pack('<Q',n)
def descriptor(k,w,a,uns=0,tag=0,payload=b'',version=3,natural=None):
 metadata=bytes((2 if k==3 else 0,2 if k==3 else 0))+U(a if natural is None else natural)+bytes((0,3,2)) if version==3 else b''
 return b''.join(U(x) for x in (0,0,0,k,w,uns,a))+metadata+bytes((tag,))+U(len(payload))+payload
def obj(entries,w,a,version=3,natural=None):
 payload=U(len(entries))
 for ordinal,(kind,off,bit,width,storage,child,*rest) in enumerate(entries):
  if version==3:payload+=U(ordinal)+bytes((kind,))+U(rest[0] if rest else struct.unpack_from('<Q',child,48)[0])
  payload+=U(off)+U(bit)+U(width)+U(storage)+child
 return descriptor(5,w,a,tag=1,payload=payload,version=version,natural=natural)
def signature(params,result,version=3):
 return ('USLSIG'+str(version)+'\n').encode()+U(1)+U(5)+b'entry'+bytes((0,1,0,int(len(params)>6)))+U(len(params))+result+U(len(params))+b''.join(params)+bytes((version==2,))
def graphs(facts):
 assert facts=={'p5':[5,1,0,1],'p6':[6,2,0,2],'p11':[11,1,0,2,10],'pn':[9,1,0,1],'n8':[8,4]},facts
 C1=descriptor(1,1,1,1);S2=descriptor(1,2,2,1);I4=descriptor(1,4,4,1);I8=descriptor(1,8,8,1)
 n8=obj([(0,0,0,0,4,I4),(0,4,0,0,4,I4)],8,4)
 return {'p5':obj([(0,0,0,0,1,C1),(0,1,0,0,4,I4,1)],5,1,natural=4),
         'p6':obj([(0,0,0,0,1,C1),(0,2,0,0,4,I4,2)],6,2,natural=4),
         'p11':obj([(0,0,0,0,2,S2,1),(0,2,0,0,8,I8,1),(0,10,0,0,1,C1)],11,1,natural=8),
         'pn':obj([(0,0,0,0,1,C1),(0,1,0,0,8,n8,1)],9,1,natural=4)}
def expected_carrier(name):
 size,unit={'p5':(5,1),'p6':(6,2),'p11':(11,1),'pn':(9,1)}[name];E=descriptor(1,unit,unit,1,version=2)
 return obj([(0,off,0,0,unit,E) for off in range(0,size,unit)],size,unit,version=2)
class Files:
 def __init__(self,target):self.target=target.encode()
 def get(self,key):return self.target if key==b'\0cli/target' else None
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--model',type=pathlib.Path);ap.add_argument('--target',choices=('osx/arm64','osx/x86_64'),required=True);ap.add_argument('--provider',type=pathlib.Path);ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args()
 evidence={'schema':1,'status':'failed','scope':__doc__,'target':a.target,'commands':[]};fixture=ROOT/'tests/libraryabi/packed-external.c'
 def cmd(*args):
  argv=[str(ROOT/'tests/bound'),'20',*map(str,args)];p=subprocess.run(argv,capture_output=True,timeout=23);evidence['commands'].append({'command':argv,'rc':p.returncode,'stdout':p.stdout.decode(errors='replace'),'stderr':p.stderr.decode(errors='replace')});assert p.returncode==0,(argv,p.stderr.decode(errors='replace'));return p.stdout
 with tempfile.TemporaryDirectory(prefix='r11-packed-native-') as tmp:
  t=pathlib.Path(tmp)
  try:
   arch=a.target.split('/')[1];assert platform.system()=='Darwin';invoke=['arch','-'+arch];flags=['-arch',arch]
   cmd('cc',*flags,'-std=c11','-DFACTS_ONLY',fixture,'-o',t/'facts');facts=json.loads(cmd(*invoke,t/'facts'));evidence['actual_layout']=facts;objects=graphs(facts)
   modelpath=a.model or t/'model.json'
   if not a.model:cmd(sys.executable,ROOT/'exec/nativeabi/gen.py',modelpath)
   model=json.loads(modelpath.read_text());loaded=load(model);evidence['model_sha256']=hashlib.sha256(modelpath.read_bytes()).hexdigest();paths=[];rows=[]
   cmd('cc','-O2',ROOT/'exec/c/run.c','-o',t/'runtime')
   cmd(sys.executable,ROOT/'exec/c/tbl.py',modelpath,t/'model.tbl')
   cmd(sys.executable,ROOT/'exec/c/net.py',t/'model.tbl',t/'model.net')
   route=t/'route.tsv';route.write_text('ordered\tnativeabi\tbytes\tbytes\tmodel.net\n')
   resources=t/'resources';(resources/'cli').mkdir(parents=True);(resources/'cli/target').write_bytes(a.target.encode())
   package=t/'package';package.write_bytes(build([route],[('00',resources)],cache=False))
   accept=a.target=='osx/arm64';refused=0
   for name,value in objects.items():
    for pressure,(gp,fp) in enumerate(((0,0),(5,7),(7,8))):
     params=[descriptor(1,8,8,1)]*gp+[descriptor(3,8,8)]*fp+[value,descriptor(1,8,8,1),descriptor(3,8,8)]
     original=signature(params,value);raw=t/'original';raw.write_bytes(original)
     status,out,_=run(model,original,name,files=Files(a.target),loaded=loaded,maxsteps=2000000)
     if not accept:
      assert status=='reject',{'case':name,'pressure':pressure,'status':status}
      p=subprocess.run([str(t/'runtime'),'--bundle',str(package),'ordered',str(raw)],capture_output=True,timeout=10);assert p.returncode!=0 and not p.stdout,'C network accepted a packed SysV object'
      refused+=1;rows.append({'case':name,'pressure':pressure,'status':'refused'});continue
     converted=expected_carrier(name)
     cp=[descriptor(1,8,8,1,version=2)]*gp+[descriptor(3,8,8,version=2)]*fp+[converted,descriptor(1,8,8,1,version=2),descriptor(3,8,8,version=2)]
     carrier=signature(cp,converted,version=2);expected=b'USLNCAR1\n'+U(len(a.target))+a.target.encode()+U(len(original))+original+U(len(carrier))+carrier
     assert status=='accept',{'case':name,'pressure':pressure,'status':status}
     assert out==expected,{'case':name,'pressure':pressure,'actual_sha':hashlib.sha256(out).hexdigest(),'expected_sha':hashlib.sha256(expected).hexdigest()}
     network_out=cmd(t/'runtime','--bundle',package,'ordered',raw)
     assert network_out==out==expected,'actual C network certificate differs'
     p=t/f'{name}-{pressure}.certificate';p.write_bytes(network_out);paths.append((name,pressure,p));rows.append({'case':name,'pressure':pressure,'original_sha256':hashlib.sha256(original).hexdigest(),'certificate_sha256':hashlib.sha256(out).hexdigest()})
   if not accept:
    assert refused==12;evidence.update(status='passed',cases=rows,refused=refused,total_native_calls=0,source_fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest());print(json.dumps({k:v for k,v in evidence.items() if k not in ('commands','provider')}));return
   includes,links,manifest=provider_flags(a.provider,a.target) if a.provider else ([],['-lffi'],None)
   evidence['provider']=manifest;cmd('cc',*flags,'-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',ROOT,*includes,fixture,*links,'-o',t/'native')
   for (name,pressure,path),row in zip(paths,rows):
    native=json.loads(cmd(*invoke,t/'native',name,pressure,path,a.target));assert native=={'case':name,'pressure':pressure,'native_calls':100,'script_calls':100,'rc':0};row['actual_native']=native
   evidence.update(status='passed',cases=rows,total_native_calls=1200,total_script_calls=1200,source_fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest());print(json.dumps({k:v for k,v in evidence.items() if k not in ('commands','provider')}))
  except BaseException as ex:evidence['error']=repr(ex);raise
  finally:
   if a.evidence:a.evidence.write_text(json.dumps(evidence,indent=2)+'\n')
if __name__=='__main__':main()
