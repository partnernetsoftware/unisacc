#!/usr/bin/env python3
"""Actual E3 callsite graphs: simulator and constructed-network executor."""
import json,os,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
from exec.pp.sim import run,load
from pack import build
from modeltypedcandidatescheck import U,I,D,d2
class Files:
 def __init__(self,v):self.v=v
 def get(self,k):return self.v.get(k)
def binding():
 name=b'host';params=(D,d2(1,4,4));sig=b'USLSIG2\n'+U(1)+U(4)+name+bytes([0,1,1,1])+U(2)+I+U(2)+b''.join(params)+b'\0'
 p=U(4)+name+bytes([0,0])+U(0)+bytes([0,1])+U(4096)+U(2)+bytes([2])+U(8192)+U(12288)+U(len(sig))+sig+b'\1'
 return b'USBIND3\n'+U(1)+U(len(p))+p

def main():
 model,runtime,dumper=sys.argv[1:];d=json.loads(pathlib.Path(model).read_text());loaded=load(d)
 with tempfile.TemporaryDirectory(prefix='r10-model-varcall-') as name:
  t=pathlib.Path(name)
  def cmd(*a):
   p=subprocess.run(list(map(str,a)),capture_output=True,timeout=25,env=dict(os.environ,UA_TYPESPELL='1'));assert p.returncode==0,(a,p.returncode,p.stderr);return p.stdout
  tbl=t/'m.tbl';net=t/'m.net';cmd(sys.executable,ROOT/'exec/c/tbl.py',model,tbl);cmd(sys.executable,ROOT/'exec/c/net.py',tbl,net)
  mf=t/'routes';mf.write_text('parse\tparse\ttokens\ttape\tm.net\n');rd=t/'r';rd.mkdir();pkg=t/'p';src=t/'s.c';tok=t/'tokens'
  vals={b'\0library/module':U(1),b'\0library/symbols':U(1),b'\0library/bindings':binding()}
  for k,v in vals.items():(rd/k.split(b'/')[-1].decode()).write_bytes(v)
  pkg.write_bytes(build([mf],[('006c6962726172792f',rd)],cache=False))
  def check(source,accepted=True):
   src.write_text(source);raw=cmd(dumper,'-dump-tokens',src);tok.write_bytes(raw)
   status,out,_=run(d,raw,'probe',files=Files(vals),loaded=loaded,maxsteps=2000000)
   p=subprocess.run([runtime,'--bundle',str(pkg),'parse',str(tok)],capture_output=True,timeout=15)
   assert (status=='accept')==(p.returncode==0),(status,p.returncode,p.stderr)
   assert (status=='accept')==accepted,(source,status,out,p.stderr)
   if accepted:assert out==p.stdout
   return out
  prefix='long host(double x,int k,...);'
  out=check(prefix+'long probe(void){return host(1.5,7,(signed char)-3,(unsigned short)24,(_Bool)1,(float)2.5);}')
  assert out.startswith(b'USLTAPE2\n'),out[:100]
  lens=struct.unpack_from('<3Q',out,9);at=33;tape=out[at:at+lens[0]];calls=out[at+lens[0]+lens[1]:];assert calls[:8]==b'USCPLAN1';assert struct.unpack_from('<Q',calls,8)[0]==1
  assert b'__us_vcall_1:' in tape and b'store64 [r7+32], r1' in tape
  # Independent graph reader checks actual promoted descriptors, not only count.
  size,site,template,fixed,slen=struct.unpack_from('<5Q',calls,16)
  assert (site,template,fixed)==(1,12288,2) and size==32+slen
  graph=calls[56:56+slen];assert graph[:8]==b'USLSIG2\n'
  pos=16;n=struct.unpack_from('<Q',graph,pos)[0];pos+=8+n
  assert graph[pos:pos+4]==bytes([0,1,0,1]);pos+=4
  total=struct.unpack_from('<Q',graph,pos)[0];pos+=8;assert total==6,(total,graph[:80].hex(),size,slen)
  def desc():
   nonlocal pos
   values=struct.unpack_from('<7Q',graph,pos);pos+=56
   tag=graph[pos];length=struct.unpack_from('<Q',graph,pos+1)[0];pos+=9+length
   return values,tag
  result,tag=desc();assert (result[3:7],tag)==((1,8,0,8),0)
  assert struct.unpack_from('<Q',graph,pos)[0]==6;pos+=8
  shapes=[desc()[0][3:7] for _ in range(6)]
  assert shapes==[(3,8,0,8),(1,4,0,4),(1,4,0,4),(1,4,0,4),(1,4,0,4),(3,8,0,8)],shapes
  assert graph[pos:]==b'\1'

  zero=check(prefix+'long probe(void){return host(1.5,7);}')
  assert zero.startswith(b'USLTAPE2\n')
  nested=check(prefix+'long probe(void){return host(1.5,7,host(2.5,8),3.5)+host(4.5,9,4);}')
  ls=struct.unpack_from('<3Q',nested,9);calls=nested[33+ls[0]+ls[1]:];assert struct.unpack_from('<Q',calls,8)[0]==3
  late=check(prefix+'long probe(void){return host(1.5,7,2.5);}long host(double x,int k,...){return k;}')
  assert late.startswith(b'USLTAPE1\n') and b'jump host\n' in late
  check(prefix+'long probe(void){return host(1.5);}',False)
  check('long host(int x,int k,...);long probe(void){return host(1,2,3);}',False)
  check(prefix+'long __us_vcall_1(void){return 0;}long probe(void){return host(1.5,7);}',False)
  print(json.dumps(dict(actual_model_sim_and_network=True,native_sites=5,late_definition_priority=True,zero_tail=True,nested_three_sites=True,missing_fixed_and_bad_prefix_rejected=True)))
if __name__=='__main__':main()
