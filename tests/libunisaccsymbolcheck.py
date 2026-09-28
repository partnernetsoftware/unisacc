#!/usr/bin/env python3
"""Private true compiler-produced text; synthetic known declarations test the ABI
adapter independently. Complete E3 metadata integration is a separate gate.
Usage: PRIVATE_UNISACC ARCH; no default/shared compiler paths. All children 25s.
"""
import hashlib,json,pathlib,sys,tempfile,struct,subprocess,os,signal
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path[:0]=[str(ROOT/'exec/c'),str(ROOT)]
from librarycallcheck import labels
from unisa.tape import parse
from unisa.lower import lower
from unisa.__main__ import _oracle
from unisa.assemble import assemble
from unisa.image.macho import HDRS
SOURCE="""unsigned long mixed(signed char a,unsigned short b,int c,unsigned long d,long *p,signed char f){return a+b+c+d+*p+f;}
signed char s8(signed char x){return x-1;}
unsigned char u8(unsigned char x){return x+1;}
short s16(short x){return x-1;}
unsigned short u16(unsigned short x){return x+1;}
int s32(int x){return x-1;}
unsigned int u32(unsigned int x){return x+1;}
long *identity(long *p){return p;}
void bump(int *p){*p=*p+1;}
long recursive(long n){if(n==0)return 5;return recursive(n-1)+n;}
int main(void){long n=1;int k=2;mixed(-7,65535,-123,4,&n,-2);s8(-122);u8(255);s16(-32000);u16(65535);s32(-1234567);u32(1);identity(&n);bump(&k);recursive(4);return k;}
"""
def run(cmd):
 p=subprocess.Popen(list(map(str,cmd)),cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
 try:o,e=p.communicate(timeout=25)
 except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.communicate();raise
 assert p.returncode==0,(cmd,p.returncode,e);return o
def main(ua,arch):
 assert arch in ('arm64','x86_64');ua=pathlib.Path(ua).resolve();assert ua.is_file()
 u=lambda n:struct.pack('<Q',n)
 def integer(width,uns=0):return (0,4,0,1,width,uns)
 pointer=(1,4,0,2,8,0);void=(0,0,0,0,0,0)
 declarations=[('mixed',integer(8,1),[integer(1),integer(2,1),integer(4),integer(8,1),pointer,integer(1)]),
  *[(n,integer(w,z),[integer(w,z)]) for n,w,z in [('s8',1,0),('u8',1,1),('s16',2,0),('u16',2,1),('s32',4,0),('u32',4,1)]],
  ('identity',pointer,[pointer]),('bump',void,[pointer]),('recursive',integer(8),[integer(8)])]
 refused=['internal','floating','aggregate','variadic','too_many','missing']
 extra=[('internal',integer(8),[],1,0,0),('floating',(0,8,0,3,8,0),[],0,0,0),('aggregate',(0,8,1,5,0,0),[],0,0,0),('variadic',integer(8),[],0,1,0),('too_many',integer(8),[integer(8)]*7,0,0,0)]
 with tempfile.TemporaryDirectory(prefix='r10-symbol-native-') as name:
  d=pathlib.Path(name);src=d/'p.c';src.write_text(SOURCE);image=d/'image';target='osx/'+arch
  tape=run(['/bin/sh',ua,'-nostdinc','-b',target,'-O0','-S',src]);run(['/bin/sh',ua,'-nostdinc','-b',target,'-O0','-o',image,src])
  tp=lower(parse(tape.decode()),target,_oracle('built'),drive='built',prune_input=True);ref,stats=assemble(tp);assert stats['encoded']==stats['insns'];machine=image.read_bytes()[HDRS(arch):HDRS(arch)+len(ref)];assert machine==ref
  offsets=labels(tp);blob=d/'blob.bin';blob.write_bytes(machine);(d/'blob.S').write_text('.text\n.p2align 4\n.globl _script_blob\n_script_blob:\n.incbin "'+str(blob)+'"\n')
  records=[(*x,0,0,1) for x in declarations]+extra;metadata=b'USLSIG1\n'+u(len(records));mutations=[]
  for n,result,args,link,var,supported in records:
   nb=n.encode();start=len(metadata);metadata+=u(len(nb))+nb+bytes([link,1,var])+u(len(args))+struct.pack('<6Q',*result)+u(len(args))+b''.join(struct.pack('<6Q',*a) for a in args)+bytes([supported])
   if n=='u8':mutations.append((start+8,ord('s')))
   if n=='mixed':mutations += [(start+8+len(nb),2),(start+8+len(nb)+1,0),(start+8+len(nb)+2,2),(start+8+len(nb)+3+8+24,7),(len(metadata)-1,2)]
  fixture='static const unsigned char metadata[]={'+','.join(map(str,metadata))+'};\nstruct Position{const char*name;unsigned offset;};static const struct Position positions[]={'+','.join('{"%s",%d}'%(n,offsets[n]) for n in [x[0] for x in declarations]+['__init'])+'};\nstatic const char *refused[]={'+','.join('"'+n+'"' for n in refused)+'};\nstruct Mutation{unsigned offset,value;};static const struct Mutation mutations[]={'+','.join('{%d,%d}'%x for x in mutations)+'};\n'
  (d/'fixture.inc').write_text(fixture);exe=d/'check'
  run(['cc','-arch',arch,'-O2','-Wall','-Wextra','-I',ROOT/'exec/c','-I',d,ROOT/'tests/libunisaccsymbolcheck.c',d/'blob.S',ROOT/'exec/c'/('librarycall_'+arch+'.S'),'-lffi','-o',exe]);out=run(['/usr/bin/arch','-x86_64',exe] if arch=='x86_64' else [exe])
  print(json.dumps({'arch':arch,'private_compiler_sha256':hashlib.sha256(ua.read_bytes()).hexdigest(),'actual_text_sha256':hashlib.sha256(machine).hexdigest(),'metadata_fixture_bytes':len(metadata),'evidence':out.decode().strip(),'scope':'actual compiler-produced text; known synthetic declarations, not E3 metadata proof'}))
if __name__=='__main__':main(*sys.argv[1:])
