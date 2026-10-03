#!/usr/bin/env python3
"""Variadic-template wire facts on simulator and constructed-network C executor.
No concrete call cif, tail promotion, source parsing or native call is claimed.
"""
import importlib.util,json,pathlib,struct,sys,tempfile,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec'),str(ROOT/'tests'),str(ROOT)]
from modeltypedcandidatescheck import U,I,F,D,PAIR,typed,wire3,normalized3,legacy3,rec
from modeltypedcanonicalcheck import canonical
import assemble
_MB=assemble.load_facts('top-modelbindings-banks')['top-modelbindings-banks!']
FORMAT,DISPATCH,PLAN,ARGC,VARIADIC,CANON,CANONLEN=(_MB[k] for k in ('FORMAT','DISPATCH','PLAN','ARGC','VARIADIC','CANON','CANONLEN'))
install=lambda E:assemble.run(assemble.FACTS.parent/'modelbindings-manifest.tsv',E,E.P,{})  # modelbindings-manifest.tsv
from exec.pp.sim import run,load
spec=importlib.util.spec_from_file_location('variadicprotocolbase',ROOT/'exec/build/parsebase.py');E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
def signature(params=(F,I),var=1,mode=1,support=0,name=b'host'):
 return b'USLSIG2\n'+U(1)+U(len(name))+name+bytes([0,1,var,mode])+U(len(params))+PAIR+U(len(params))+b''.join(params)+bytes([support])
def template(params=(F,I),var=1,mode=1,fmt=2,support=1,dispatcher=8192,handle=12288,count=None,origin=0,ordinal=0,prototype_support=0):
 name=b'host';s=signature(params,var,mode,prototype_support);n=len(params) if count is None else count
 p=U(len(name))+name+bytes([0,origin])+U(ordinal)+bytes([0,var])+U(4096)+U(n)+bytes([fmt])+U(dispatcher)+U(handle)+U(len(s))+s+bytes([support])
 return U(len(p))+p
class Files:
 def __init__(self,blob):self.blob=blob
 def get(self,k):return self.blob if k==b'\0library/bindings' else None

def main():
 runtime=sys.argv[1];install(E)
 E.P('START').goto('LBI.read')
 p=E.P('LBI.emit').a(('LDI','lbi_i',0))
 for bank in (FORMAT,DISPATCH,PLAN,ARGC,VARIADIC):
  p.a(('LDX','mc_value','lbi_i',bank)).call('MC.write64')
 p.a(('LDX','ms_canon','lbi_i',CANON),('LDX','ms_canonlen','lbi_i',CANONLEN),('INPUSH','ms_canon'),('LDI','ms_zero',0),('SPAN2','ms_zero','ms_canonlen'),('INPOP',),('ACCEPT',)).goto('DONE')
 E.P('DONE').a(('ACCEPT',)).goto('DONE');E.g.finish()
 d={'start':'START','states':{n:[m,{str(k):v for k,v in r.items()}] for n,(m,r) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]};loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-variadic-protocol-') as name:
  t=pathlib.Path(name);model=t/'m.json';model.write_text(json.dumps(d));tbl=t/'m.tbl';net=t/'m.net'
  def cmd(*args):
   p=subprocess.run(list(map(str,args)),capture_output=True,timeout=20);assert p.returncode==0,(args,p.stderr);return p.stdout
  cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net);full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  sys.path.insert(0,str(ROOT/'exec/c'));from pack import build
  mf=t/'routes';mf.write_text('test\tdecode\tbytes\tbytes\tm.net\n');rd=t/'r';rd.mkdir();src=t/'empty';src.write_bytes(b'');pkg=t/'p'
  def check(blob,expected=None):
   status,out,_=run(d,b'','fixture',files=Files(blob),loaded=loaded,maxsteps=2000000)
   (rd/'bindings').write_bytes(blob);pkg.write_bytes(build([mf],[(b'\0library/'.hex(),rd)],cache=False));p=subprocess.run([runtime,'--bundle',str(pkg),'test',str(src)],capture_output=True,timeout=10)
   assert (p.returncode==0)==(status=='accept'),(p.returncode,status,p.stderr)
   if status=='accept':assert p.stdout==out
   if expected is not None:assert status=='accept' and out==expected,(status,out,expected)
   return status,out
  expected=b''.join(U(n) for n in (2,8192,12288,2,1))+canonical(signature())
  valid=wire3(template());check(valid,expected)
  # A narrow fixed parameter is legal: promotions apply to later concrete tails.
  for rows in ([template((F,I)),template(origin=2,ordinal=1,handle=0,support=0)], [template(origin=2,ordinal=1,handle=0,support=0),template()]):check(wire3(*rows),expected)
  check(wire3(template(support=0,handle=0)),b''.join(U(n) for n in (2,8192,0,2,1))+canonical(signature()))
  bad=[wire3(template(var=0,mode=0)),wire3(template(params=())),wire3(template(mode=0)),wire3(template(dispatcher=0)),wire3(template(handle=0)),wire3(template(prototype_support=1)),wire3(template(count=3)),wire3(template(fmt=3)),valid+b'x',wire3(template(),template())]
  for b in bad:assert check(b)[0]=='reject'
  # Old fixed typed decoder metadata and canonical bytes stay unchanged.
  fixed=typed(signature=signature(var=0,mode=0,support=1),count=2)
  check(wire3(fixed),b''.join(U(n) for n in (1,8192,12288,2,0))+canonical(signature(var=0,mode=0,support=1)))
  # Legacy format0 remains accepted and leaves no typed graph.
  check(wire3(legacy3(rec())))
  for n in range(len(valid)):assert check(valid[:n])[0]=='reject'
  print(json.dumps({'states':len(d['states']),'full_domain':full,'format2_valid':4,'malformed':len(bad),'truncations':len(valid),'old_format1_and0':True,'executors':'sim+C constructed network','scope':'template only; fixed float32 legal; no concrete cif or promotions'}))
if __name__=='__main__':main()
