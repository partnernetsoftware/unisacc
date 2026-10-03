#!/usr/bin/env python3
"""Independent recursive canonical-byte oracle; C network versus simulator."""
import importlib.util,json,pathlib,struct,sys,tempfile,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec'),str(ROOT/'tests'),str(ROOT)]
from modeltypedcandidatescheck import U,d2,I,D,F,PTR,PAIR,sig,Files
import assemble
install=lambda E,fail='DEAD':assemble.run(assemble.FACTS.parent/'modelsignature-manifest.tsv',E,E.P,{},dict(fail=fail))  # modelsignature-manifest.tsv
from exec.pp.sim import run,load
spec=importlib.util.spec_from_file_location('canonicalbase',ROOT/'exec/parse/gen.py');E=importlib.util.module_from_spec(spec);spec.loader.exec_module(E)
def canonical(blob):
 p=16;n=struct.unpack_from('<Q',blob,p)[0];p+=8+n;var=blob[p+2];p+=4;count=struct.unpack_from('<Q',blob,p)[0];p+=8
 def desc():
  nonlocal p
  f=list(struct.unpack_from('<7Q',blob,p));p+=56;f[1]=f[2]=0;tag=blob[p];p+=1;length=struct.unpack_from('<Q',blob,p)[0];p+=8;end=p+length
  q=struct.pack('<7Q',*f)+bytes([tag])+U(length)
  if tag in (1,2):
   m=struct.unpack_from('<Q',blob,p)[0];p+=8;q+=U(m)
   for _ in range(m):q+=blob[p:p+32];p+=32;q+=desc()
  elif tag==3:q+=blob[p:p+16];p+=16;q+=desc()
  else:assert length==0
  assert p==end;return q
 out=bytes([var])+U(count)+desc();stored=struct.unpack_from('<Q',blob,p)[0];p+=8;assert stored==count
 for _ in range(count):out+=desc()
 assert p+1==len(blob);return out

def main():
 import sys as _sys
 if len(_sys.argv)-1 < 1:
     _sys.stderr.write("usage: %s MODEL.json\n" % _sys.argv[0])
     _sys.exit(2)

 runtime=sys.argv[1];install(E)
 E.P('START').a(('SBCLR',),*[('SBOUT',x) for x in b'input'],('SBFIND','ms_blob'),('BLEN','ms_len','ms_blob')).call('MS.canonical').a(('INPUSH','ms_canon'),('LDI','ms_zero',0),('SPAN2','ms_zero','ms_canonlen'),('INPOP',)).goto('DONE')
 E.P('DONE').a(('ACCEPT',)).goto('DONE');E.g.finish();d={'start':'START','states':{n:[m,{str(k):v for k,v in r.items()}] for n,(m,r) in E.g.st.items()},'seqs':[list(map(list,s)) for s in E.g.seqs]};loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-canonical-') as name:
  t=pathlib.Path(name);model=t/'m.json';model.write_text(json.dumps(d));tbl=t/'m.tbl';net=t/'m.net'
  def cmd(*args):
   p=subprocess.run(list(map(str,args)),capture_output=True,timeout=20);assert p.returncode==0,(args,p.stderr);return p.stdout
  cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net);full=cmd(runtime,'--check-net',tbl,net).decode().strip()
  sys.path.insert(0,str(ROOT/'exec/c'));from pack import build
  mf=t/'routes';mf.write_text('test\tcanonical\tbytes\tbytes\tm.net\n');rd=t/'r';rd.mkdir();src=t/'empty';src.write_bytes(b'');pkg=t/'p'
  def check(blob,expected=None):
   status,out,_=run(d,b'','fixture',files=Files(blob),loaded=loaded,maxsteps=2000000)
   (rd/'input').write_bytes(blob);pkg.write_bytes(build([mf],[('',rd)],cache=False));p=subprocess.run([runtime,'--bundle',str(pkg),'test',str(src)],capture_output=True,timeout=10)
   assert (p.returncode==0)==(status=='accept'),(p.returncode,status,p.stderr)
   if status=='accept':assert p.stdout==out
   if expected is not None:assert status=='accept' and out==expected
   return status,out
  array=d2(5,32,8,U(2)+U(16)+PAIR,3,base=8001)
  nested=d2(5,32,8,U(1)+bytes(24)+U(32)+array,1,base=9001)
  mixed=sig(params=(I,D,F,PTR,PAIR,I,D,I,I),result=PAIR)
  shapes=[sig(),mixed,sig(params=(array,),result=nested),sig(params=(d2(4,8,8,depth=1),),supported=0)]
  for b in shapes:check(b,canonical(b))
  # Independently patch only parser-local identifiers in every recursive node.
  changed=bytearray(mixed)
  def rewrite(start):
   struct.pack_into('<QQ',changed,start+8,123456,987654);tag=changed[start+56];length=struct.unpack_from('<Q',changed,start+57)[0];p=start+65;end=p+length
   if tag in (1,2):
    count=struct.unpack_from('<Q',changed,p)[0];p+=8
    for _ in range(count):p=rewrite(p+32)
   elif tag==3:p=rewrite(p+16)
   assert p==end;return end
  p=24+len(b'host')+4+8;p=rewrite(p);p+=8
  for _ in range(9):p=rewrite(p)
  assert p+1==len(changed);check(bytes(changed),canonical(mixed));assert canonical(bytes(changed))==canonical(mixed)
  deep=I
  for _ in range(32):deep=d2(5,8,8,U(1)+bytes(24)+U(8)+deep,1)
  assert check(sig(result=deep))[0]=='reject'
  bad=[sig(result=d2(5,8,8,U(65),1)),sig(result=d2(5,8,8,U(1)+bytes(32)+I[:-1],1)),sig(result=d2(1,8,3)),sig(result=d2(5,16,8,U(3)+U(8)+I,3)),sig(result=d2(payload=b'x'))]
  bad += [sig(result=d2(kind=(1<<32)|1)),sig(result=d2(width=(1<<32)|8)),sig(result=d2(2,(1<<32)|8,8,depth=1)),b'USLSIG2\n'+U((1<<32)|1)+sig()[16:]]
  for b in bad:assert check(b)[0]=='reject'
  print(json.dumps({'states':len(d['states']),'full_domain':full,'exact_recursive_canonical_shapes':len(shapes)+1,'parser_local_id_invariance':True,'depth33_rejected':True,'malformed_recursive_controls':len(bad),'executors':'sim+C constructed network'}))
if __name__=='__main__':main()
