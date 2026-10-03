#!/usr/bin/env python3
"""Original union support0 + frozen supported plan enters real E3, sim/C exact."""
import json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
from modelnativecarriercheck import U,MIX,signature,carrier,plan
from modelcallablewirecheck import decode
SOURCE='union U {double d;unsigned long long u;};union U host_flip(union U x);union U round(union U x){return host_flip(x);}'
def binding(supported=1,handle=12288):
 name=b'host_flip';graph=signature(name=name)
 payload=U(len(name))+name+bytes([0,0])+U(0)+bytes([0,0])+U(4096)+U(1)+bytes([1])+U(8192)+U(handle)+U(len(graph))+graph+bytes([supported])
 return b'USBIND3\n'+U(1)+U(len(payload))+payload
class Files:
 def __init__(self,b):self.b=b
 def get(self,key):return self.b if key==b'\0library/bindings' else {b'\0library/module':U(1),b'\0library/symbols':U(1),b'\0library/callables':U(1),b'\0library/callablemake':U(16384),b'\0library/callablecall':U(20480)}.get(key)

def main():
 with tempfile.TemporaryDirectory(prefix='r10-native-carrier-import-') as name:
  t=pathlib.Path(name)
  def cmd(*args):
   p=subprocess.run(list(map(str,args)),capture_output=True,timeout=45,env=dict(os.environ,UA_TYPESPELL='1'));assert p.returncode==0,(args,p.returncode,p.stderr);return p.stdout
  if len(sys.argv)==4:model,runtime,dumper=map(pathlib.Path,sys.argv[1:])
  else:
   assert len(sys.argv)==1
   model,runtime,dumper=t/'e3.json',t/'run',t/'dump'
   cmd(ROOT/'tests/bound',20,ROOT/'tests/build_ref.sh',t/'ref.c',t/'ref')
   s=(t/'ref.c').read_bytes();a=b'        if (tkind[i] == 2) { __write(1, "=", 1);'
   b=b'        if (L == 4 && TOKV[p] == 116 && TOKV[p+1] == 121 && TOKV[p+2] == 112 && TOKV[p+3] == 101 && getenv("UA_TYPESPELL")) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }\n'+a
   assert s.count(a)==1;s=s.replace(a,b)
   a=b'    splice();\n    decomment();\n    preprocess();\n    expandsrc();\n    if (lex() < 0) return 1;\n    i = 0;';b=a.replace(b'    preprocess();',b'    autoinc();\n    preprocess();')
   assert s.count(a)==1;s=s.replace(a,b);(t/'dump.c').write_bytes(s)
   cmd(ROOT/'tests/bound',20,'cc','-w','-std=c99','-O0',t/'dump.c','-o',dumper)
   cmd(ROOT/'tests/bound',20,'cc','-O2',ROOT/'exec/c/run.c','-o',runtime)
   cmd(sys.executable,ROOT/'exec/build/gen.py','parse2',model)
  d=json.loads(model.read_text());loaded=load(d);tbl,net=t/'e3.tbl',t/'e3.net';cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net)
  full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  route=t/'routes';route.write_text('parse\te3\ttokens\ttape\te3.net\n');rd=t/'resources';rd.mkdir();src,tok,pkg=t/'source.c',t/'tokens',t/'p'
  def check(source,bound,accepted):
   src.write_text(source);raw=cmd(dumper,'-dump-tokens',src);tok.write_bytes(raw);files=Files(bound)
   for key in ('module','symbols','bindings','callables','callablemake','callablecall'):(rd/key).write_bytes(files.get(b'\0library/'+key.encode()))
   pkg.write_bytes(build([route],[('006c6962726172792f',rd)],cache=False));status,out,_=run(d,raw,'union-source',files=files,loaded=loaded,maxsteps=5000000)
   p=subprocess.run([str(runtime),'--bundle',str(pkg),'parse',str(tok)],capture_output=True,timeout=45)
   assert (status=='accept')==accepted==(p.returncode==0),(status,p.returncode,p.stderr,out[:200])
   if accepted:assert out==p.stdout;return out
   assert not p.stdout;return b''
  out=check(SOURCE,binding(),True)
  assert out[:9]==b'USLTAPE3\n';tn,mn,cn,kn=struct.unpack_from('<4Q',out,9);tape=out[41:41+tn];meta=out[41+tn:41+tn+mn]
  _,records=decode(b'USLTAPE1\n'+U(0)+U(len(meta))+meta)
  record=records['round'];assert record['count']==1 and record['support']==0
  for original in [record['result'],record['params'][0]]:
   assert original['tag']==2 and original['fields'][3:]==(5,8,0,8)
   assert [m[0] for m in original['child']]==[(0,0,0,8),(0,0,0,8)]
   assert [m[1]['fields'][3:] for m in original['child']]==[(3,8,0,8),(1,8,1,8)]
  assert b'host_flip:' in tape and b'.librarycall' in tape and b'12288' in tape and b'callr ' not in tape
  check(SOURCE,binding(0,0),False);check(SOURCE.replace('unsigned long long u','double other'),binding(),False)
  # Package construction seam: every target shares exactly one carrier model.
  ncmodel,nctbl,ncnet=t/'nc.json',t/'nc.tbl',t/'nc.net'
  cmd(sys.executable,ROOT/'exec/build/gen.py','nativeabi',ncmodel);cmd(sys.executable,ROOT/'exec/c/tbl.py',ncmodel,nctbl);cmd(sys.executable,ROOT/'exec/c/net.py',nctbl,ncnet)
  ncfull=cmd(runtime,'--check-net',nctbl,ncnet).decode().strip()
  import compilerpack
  oldbuilt,oldbuild=compilerpack.built_model,compilerpack.build;calls=[];captured=[]
  def fakebuilt(td,name,script,args,env=None):calls.append((name,str(script)));return ncnet
  cli=t/'cli';cli.mkdir();(cli/'target').write_bytes(b'osx/arm64')
  def capture(manifests,mounts,**kwargs):
   captured.extend(pathlib.Path(manifests[0]).read_text().splitlines());return oldbuild(manifests,[*mounts,('00636c692f',cli)],**kwargs)
  compilerpack.built_model,compilerpack.build=fakebuilt,capture
  includes=t/'includes';includes.mkdir();(includes/'test.h').write_text('/* private package fixture */\n')
  manifests=[]
  for osname in ('osx','lnx','win'):
   for arch in ('arm64','x86_64'):
    target=osname+'/'+arch;path=t/(osname+'-'+arch+'.tsv');path.write_text(''.join('\t'.join([target,s,'bytes','bytes',str(ncnet)])+'\n' for s in ('e2','e1','e3','e4','prune','lower','elf')));manifests.append(path)
  try:
   payload=compilerpack.compiler_package(manifests,ncnet,includes,compressed=False,shared_e2=ncnet,shared_nativeabi=ncnet)
   rows=[line.split('\t') for line in captured if line.split('\t')[1]=='nativeabi']
   assert {r[0] for r in rows}=={f'{osname}/{arch}/nativeabi' for osname in ('osx','lnx','win') for arch in ('arm64','x86_64')}
   assert all(r[1:4]==['nativeabi','USLSIG2','USLNCAR1'] and pathlib.Path(r[4]).resolve()==ncnet.resolve() for r in rows)
   assert not any(name=='nativeabi' for name,script in calls)
   pp=t/'compiler.pkg';pp.write_bytes(payload);original=signature();wire=t/'signature';wire.write_bytes(original)
   assert cmd(runtime,'--bundle',pp,'osx/arm64/nativeabi',wire)==plan(b'osx/arm64',original,carrier((MIX,),MIX))
   calls.clear();captured.clear();compilerpack.compiler_package(manifests,ncnet,includes,compressed=False,shared_e2=ncnet)
   assert sum(name=='nativeabi' for name,script in calls)==1
  finally:compilerpack.built_model,compilerpack.build=oldbuilt,oldbuild
  print(json.dumps({'prototype_only':True,'actual_union_original_import_sim_C_exact':True,'original_union_support0_preserved':True,'frozen_plan_outer_support0_rejected':True,'source_same_size_union_mismatch_rejected':True,'e3_full_domain':full,'carrier_full_domain':ncfull,'six_routes_one_network':True,'supplied_stage_not_rebuilt':True,'default_stage_built_once':True,'actual_packaged_carrier_route':True,'scope':'package and E3 frozen-plan protocol; public native union execution is separate'}))
if __name__=='__main__':main()
