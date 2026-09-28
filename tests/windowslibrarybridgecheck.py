#!/usr/bin/env python3
"""Windows bridge source interpreter + COFF/unwind checks, not Windows execution.
All assemblers/readobj children are bounded; no runtime or guest process runs.
Microsoft ABI references are recorded in the two assembly sources.
"""
import pathlib,re,random,subprocess,tempfile,sys,json,shutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
MASK=(1<<64)-1

def instructions(path):
 text=path.read_text();text=re.sub(r'/\*.*?\*/','',text,flags=re.S)
 return [line.strip() for line in text.splitlines() if line.strip() and not line.strip().startswith('.') and not line.strip().endswith(':')]

def arm(lines,seed):
 rng=random.Random(seed);r={f'x{i}':rng.getrandbits(64) for i in range(31)};r.update({f'd{i}':rng.getrandbits(64) for i in range(32)});r['sp']=0x100000
 old=dict(r);args=[rng.getrandbits(64) for _ in range(6)];m={0x200000+8*i:a for i,a in enumerate(args)};r.update(x0=0x300000,x1=0x200000,x2=0x400000);called=False
 for line in lines:
  parts=line.split(None,1);op=parts[0];rest=parts[1] if len(parts)>1 else ''
  if op in ('stp','ldp'):
   a,b,base,off=re.fullmatch(r'(\w+), (\w+), \[(\w+), #(\d+)\]',rest).groups();at=r[base]+int(off)
   if op=='stp':m[at]=r[a];m[at+8]=r[b]
   else:r[a],r[b]=m[at],m[at+8]
  elif op in ('sub','add'):
   a,b,n=re.fullmatch(r'(\w+), (\w+), #(\d+)',rest).groups();r[a]=(r[b]+int(n)*(1 if op=='add' else -1))&MASK
  elif op=='mov':
   a,b=rest.split(', ');r[a]=int(b[1:]) if b.startswith('#') else r[b]
  elif op=='adr':r[rest.split(', ')[0]]=0x500000
  elif op=='str':
   a,b=re.fullmatch(r'(\w+), \[(\w+)\]',rest).groups();m[r[b]]=r[a]
  elif op=='br':
   assert r[rest]==0x300000 and [r[f'x{i}'] for i in range(6)]==args
   assert r['sp']%16==0 and r['x7']==0x400000-8 and m[r['x7']]==0x500000
   # A script may destroy every saved register; software ret pops x7.
   for k in r:
    if k not in ('sp','x18','x7'):r[k]=rng.getrandbits(64)
   r['x7']+=8;r['x0']=0xfedcba9876543210;called=True
  elif op=='ret':pass
  else:raise AssertionError(line)
 assert called and r['x0']==0xfedcba9876543210 and r['sp']==old['sp'] and r['x18']==old['x18']
 assert all(r[f'x{i}']==old[f'x{i}'] for i in range(19,31))
 assert all(r[f'd{i}']==old[f'd{i}'] for i in range(8,16))

def x86(lines,seed):
 rng=random.Random(seed);names='rax rbx rcx rdx rdi rsi rbp rsp r8 r9 r10 r11 r12 r13 r14 r15'.split();r={k:rng.getrandbits(64) for k in names};r.update({f'xmm{i}':rng.getrandbits(128) for i in range(16)});r['rsp']=0x100008
 old=dict(r);args=[rng.getrandbits(64) for _ in range(6)];m={0x200000+8*i:a for i,a in enumerate(args)};r.update(rcx=0x300000,rdx=0x200000,r8=0x400000);called=False
 def address(s):
  off,reg=re.fullmatch(r'(-?\d*)\(%(\w+)\)',s).groups();return r[reg]+int(off or 0)
 def value(s):
  if s.startswith('%'):return r[s[1:]]
  if s.startswith('$'):return int(s[1:])
  return m[address(s)]
 def put(s,v):
  if s.startswith('%'):r[s[1:]]=v
  else:m[address(s)]=v
 for line in lines:
  op,*rr=line.split(None,1);rest=rr[0] if rr else ''
  if op=='pushq':r['rsp']-=8;m[r['rsp']]=value(rest)
  elif op=='popq':put(rest,m[r['rsp']]);r['rsp']+=8
  elif op in ('movq','movdqu'):
   a,b=rest.split(', ');put(b,value(a))
  elif op in ('subq','addq'):
   a,b=rest.split(', ');put(b,(value(b)+value(a)*(1 if op=='addq' else -1))&MASK)
  elif op=='leaq':a,b=rest.split(', ');put(b,address(a))
  elif op=='xorq':a,b=rest.split(', ');put(b,value(a)^value(b))
  elif op=='callq':
   assert value(rest[1:])==0x300000 and [r[k] for k in ('rax','rdi','rsi','rdx','rcx','r8')]==args
   assert r['rsp']==0x400000-16 and r['rsp']%16==0
   host=m[r['rsp']];assert host%16==0
   r['rsp']-=8;m[r['rsp']]=0x500000
   for k in r:
    if k!='rsp':r[k]=rng.getrandbits(128 if k.startswith('xmm') else 64)
   r['rax']=0xfedcba9876543210;assert m[r['rsp']]==0x500000;r['rsp']+=8;called=True
  elif op=='retq':pass
  else:raise AssertionError(line)
 assert called and r['rax']==0xfedcba9876543210 and r['rsp']==old['rsp']
 assert all(r[k]==old[k] for k in ('rbx','rbp','rdi','rsi','r12','r13','r14','r15'))
 assert all(r[f'xmm{i}']==old[f'xmm{i}'] for i in range(6,16))

def command(args):
 p=subprocess.run(list(map(str,args)),capture_output=True,timeout=15);assert p.returncode==0,(args,p.stderr.decode());return p.stdout.decode()
def main():
 a=ROOT/'exec/c/librarycall_windows_arm64.S';x=ROOT/'exec/c/librarycall_windows_x86_64.S';aa=instructions(a);xx=instructions(x)
 for n in range(128):arm(aa,n);x86(xx,n)
 controls=0
 for lines,check,bad in [(aa,arm,'ldp d8,'),(aa,arm,'ldp x19,'),(xx,x86,'popq %rdi'),(xx,x86,'movdqu 32(%rsp), %xmm6'),(xx,x86,'movq (%rsp), %rsp')]:
  changed=[l for l in lines if not l.startswith(bad)]
  try:check(changed,5)
  except (AssertionError,KeyError):controls+=1
  else:raise AssertionError('lost restore escaped control')
 clang=shutil.which('clang');assert clang
 readobj=shutil.which('llvm-readobj') or '/opt/homebrew/opt/llvm/bin/llvm-readobj'
 with tempfile.TemporaryDirectory(prefix='windows-library-bridge-') as tmp:
  d=pathlib.Path(tmp);out=[]
  for target,src,kind in [('aarch64-pc-windows-msvc',a,'COFF-ARM64'),('x86_64-pc-windows-msvc',x,'COFF-x86-64')]:
   obj=d/(target+'.obj');command([clang,'-target',target,'-c',src,'-o',obj]);info=command([readobj,'--unwind',obj]);assert kind in info and 'us_library_bridge_raw' in info
   if src==x:
    assert 'ALLOC_LARGE size=200' in info
    for n in range(6,16):assert f'SAVE_XMM128 reg=XMM{n},' in info
    for reg in ('RBX','RBP','RDI','RSI','R12','R13','R14','R15'):assert f'PUSH_NONVOL reg={reg}' in info
   else:assert 'mov fp, sp' in info and 'sub sp, #160' in info and 'ldp d14, d15' in info
   out.append({'target':target,'object_bytes':obj.stat().st_size,'unwind':info})
  if len(sys.argv)>1:pathlib.Path(sys.argv[1]).write_text(json.dumps({'model_cases':256,'negative_controls':controls,'objects':out,'scope':'source interpreter + COFF construction/unwind; no native Windows execution; guest SEH unsupported'},indent=2)+'\n')
 print('Windows library bridges: 256 register/stack cases, 5 restore controls, two COFF unwind records; native Windows/guest SEH not claimed')
if __name__=='__main__':main()
