#!/usr/bin/env python3
"""Measured external V3 facts -> model certificate -> true C calls/closures.
Not public source refinement, full BANK, or six-platform qualification.
"""
import argparse,hashlib,json,pathlib,platform,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c')]
from exec.pp.sim import run,load
from buildlibrary import provider_flags
from pack import build
U=lambda n:struct.pack('<Q',n)
def descriptor(k,w,a,uns=0,tag=0,payload=b'',version=3):
 metadata=bytes((2 if k==3 else 0,2 if k==3 else 0))+U(a)+bytes((0,3,2)) if version==3 else b''
 return b''.join(U(x) for x in (0,0,0,k,w,uns,a))+metadata+bytes((tag,))+U(len(payload))+payload
def obj(entries,w,a,version=3):
 payload=U(len(entries))
 for ordinal,(kind,off,bit,width,storage,child) in enumerate(entries):
  if version==3:payload+=U(ordinal)+bytes((kind,))+U(struct.unpack_from('<Q',child,48)[0])
  payload+=U(off)+U(bit)+U(width)+U(storage)+child
 return descriptor(5,w,a,tag=1,payload=payload,version=version)
def signature(params,result,version=3):
 return ('USLSIG'+str(version)+'\n').encode()+U(1)+U(5)+b'entry'+bytes((0,1,0,int(len(params)>6)))+U(len(params))+result+U(len(params))+b''.join(params)+bytes((version==2,))
def graphs(facts):
 assert facts['positions']==[0,5,12,0,9,26] and facts['units']==[4,8],facts
 assert facts['b4']==[4,4] and facts['b8']==[8,8] and facts['bd16']==[16,8,8] and facts['db16']==[16,8,8],facts
 result={}
 for name,unit,widths,positions in [('b4',4,[5,7,20],facts['positions'][:3]),('b8',8,[9,17,38],facts['positions'][3:])]:
  entries=[(1,pos//(unit*8)*unit,pos%(unit*8),width,unit,descriptor(1,unit,unit,uns=int(i!=1))) for i,(pos,width) in enumerate(zip(positions,widths))]
  result[name]=obj(entries,unit,unit)
 D=descriptor(3,8,8);result['bd16']=obj([(0,0,0,0,8,result['b8']),(0,8,0,0,8,D)],16,8)
 result['db16']=obj([(0,0,0,0,8,D),(0,8,0,0,8,result['b8'])],16,8)
 return result
def expected_carrier(name,target):
 J=descriptor(1,4,4,1,version=2);I=descriptor(1,8,8,1,version=2);D=descriptor(3,8,8,version=2)
 members={'b4':[J],'b8':[I],'bd16':[I,I] if target.endswith('arm64') else [I,D],'db16':[I,I] if target.endswith('arm64') else [D,I]}[name]
 entries=[];off=0
 for child in members:
  width=struct.unpack_from('<Q',child,32)[0];entries.append((0,off,0,0,width,child));off+=width
 return obj(entries,off,4 if name=='b4' else 8,version=2)
class Files:
 def __init__(self,target):self.target=target.encode()
 def get(self,key):return self.target if key==b'\0cli/target' else None
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--model',type=pathlib.Path);ap.add_argument('--target',choices=('osx/arm64','osx/x86_64'),required=True);ap.add_argument('--provider',type=pathlib.Path);ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args()
 evidence={'schema':1,'status':'failed','scope':__doc__,'target':a.target,'commands':[]};fixture=ROOT/'tests/libraryabi/ordered-bitfield.c'
 def cmd(*args):
  argv=[str(ROOT/'tests/bound'),'20',*map(str,args)];p=subprocess.run(argv,capture_output=True,timeout=23);evidence['commands'].append({'command':argv,'rc':p.returncode,'stdout':p.stdout.decode(errors='replace'),'stderr':p.stderr.decode(errors='replace')});assert p.returncode==0,evidence['commands'][-1];return p.stdout
 with tempfile.TemporaryDirectory(prefix='r10-ordered-native-') as tmp:
  t=pathlib.Path(tmp)
  try:
   arch=a.target.split('/')[1];assert platform.system()=='Darwin';invoke=['arch','-'+arch];flags=['-arch',arch]
   cmd('cc',*flags,'-std=c11','-DFACTS_ONLY',fixture,'-o',t/'facts');facts=json.loads(cmd(*invoke,t/'facts'));evidence['actual_layout']=facts;objects=graphs(facts)
   modelpath=a.model or t/'model.json'
   if not a.model:cmd(sys.executable,ROOT/'exec/build/gen.py','nativeabi',modelpath)
   model=json.loads(modelpath.read_text());loaded=load(model);evidence['model_sha256']=hashlib.sha256(modelpath.read_bytes()).hexdigest();paths=[];rows=[]
   cmd('cc','-O2',ROOT/'exec/c/run.c','-o',t/'runtime')
   cmd(sys.executable,ROOT/'exec/c/tbl.py',modelpath,t/'model.tbl')
   cmd(sys.executable,ROOT/'exec/c/net.py',t/'model.tbl',t/'model.net')
   route=t/'route.tsv';route.write_text('ordered\tnativeabi\tbytes\tbytes\tmodel.net\n')
   resources=t/'resources';(resources/'cli').mkdir(parents=True);(resources/'cli/target').write_bytes(a.target.encode())
   package=t/'package';package.write_bytes(build([route],[('00',resources)],cache=False))
   for name,value in objects.items():
    for pressure,(gp,fp) in enumerate(((0,0),(5,7),(7,8))):
     params=[descriptor(1,8,8,1)]*gp+[descriptor(3,8,8)]*fp+[value,descriptor(1,8,8,1),descriptor(3,8,8)]
     original=signature(params,value);converted=expected_carrier(name,a.target)
     cp=[descriptor(1,8,8,1,version=2)]*gp+[descriptor(3,8,8,version=2)]*fp+[converted,descriptor(1,8,8,1,version=2),descriptor(3,8,8,version=2)]
     carrier=signature(cp,converted,version=2);expected=b'USLNCAR1\n'+U(len(a.target))+a.target.encode()+U(len(original))+original+U(len(carrier))+carrier
     status,out,_=run(model,original,name,files=Files(a.target),loaded=loaded,maxsteps=2000000)
     assert status=='accept',{'case':name,'pressure':pressure,'status':status}
     assert out==expected,{'case':name,'pressure':pressure,'actual_sha':hashlib.sha256(out).hexdigest(),'expected_sha':hashlib.sha256(expected).hexdigest()}
     raw=t/'original';raw.write_bytes(original);network_out=cmd(t/'runtime','--bundle',package,'ordered',raw)
     assert network_out==out==expected,'actual C network certificate differs'
     p=t/f'{name}-{pressure}.certificate';p.write_bytes(network_out);paths.append((name,pressure,p));rows.append({'case':name,'pressure':pressure,'original_sha256':hashlib.sha256(original).hexdigest(),'certificate_sha256':hashlib.sha256(out).hexdigest(),'independent_carrier_bytes_equal':True,'actual_C_network_bytes_equal':True})
   includes,links,manifest=provider_flags(a.provider,a.target) if a.provider else ([],['-lffi'],None)
   if arch=='x86_64':assert manifest is not None,'qualified x86 provider required'
   evidence['provider']=manifest;cmd('cc',*flags,'-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',ROOT,*includes,fixture,*links,'-o',t/'native')
   for (name,pressure,path),row in zip(paths,rows):
    native=json.loads(cmd(*invoke,t/'native',name,pressure,path,a.target));assert native=={'case':name,'pressure':pressure,'native_calls':100,'script_calls':100,'rc':0};row['actual_native']=native
   evidence.update(status='passed',cases=rows,total_native_calls=1200,total_script_calls=1200,source_fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest());print(json.dumps({k:v for k,v in evidence.items() if k not in ('commands','provider')}))
  except BaseException as ex:evidence['error']=repr(ex);raise
  finally:
   if a.evidence:a.evidence.write_text(json.dumps(evidence,indent=2)+'\n')
if __name__=='__main__':main()
