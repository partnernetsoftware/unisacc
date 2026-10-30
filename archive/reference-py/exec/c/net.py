#!/usr/bin/env python3
"""Construct state-conditioned integer threshold networks, without training.

For each finite row f, base=f(lo), h_t(x)=[x>=t], W_t=f(t)-f(t-1).
Then f(x)=base+sum W_t*h_t(x), exactly, by telescoping. Two linear outputs
encode next state and action-sequence ID. Missing transitions remain (-1,0).
A stack bank's rows that send a continuation state k back to k with one
shared sequence are a return (pop and continue there), which is control flow,
not a decision: they become a declared return set (H mode 3) instead of step
units, and the bank keeps units only for its other rows.
S/Q records are unchanged declarations. N/H records contain weights, not
per-observation answers. Runtime must evaluate them, not expand them to R.
"""
import collections,pathlib,sys
from tbl import CODE,OPS

def convert(src):
 lines=src.splitlines();head=lines[0].split();assert head[0]=='T' and len(head)==6
 ns,nq,nr,nstr,start=map(int,head[1:]);assert ns>0 and nq>0 and 0<=start<ns
 prefix=lines[1:1+nstr+nq];rows=lines[1+nstr+nq:];assert len(rows)==ns
 top=ns-1
 for line in prefix[nstr:]:
  a=line.split();assert a[0]=='Q';count=int(a[1]);v=list(map(int,a[2:]));i=0
  for _ in range(count):
   op=v[i];n=len(OPS[op][1]);assert i+1+n<=len(v)
   if op==CODE['PUSH']:top=max(top,v[i+1])
   i+=1+n
  assert i==len(v)
 parsed=[]
 for line in rows:
  a=line.split();assert a[0]=='R';v=list(map(int,a[1:]));mode,n,dn,dq=v[:4]
  assert mode in (0,1,2) and n>=0 and len(v)==4+3*n
  row={v[i]:(v[i+1],v[i+2]) for i in range(4,len(v),3)};assert len(row)==n
  if mode==1:
   assert all(k>=-1 for k in row);top=max([top,*row]);assert (dn,dq)==(-1,0)
  else:assert all(0<=k<=256 for k in row)
  assert all(-1<=nx<ns and 0<=sq<nq for nx,sq in [*row.values(),(dn,dq)])
  parsed.append((mode,row,(dn,dq)))
 out=['N '+' '.join(head[1:])+' '+str(top),*prefix];units=0
 for mode,row,default in parsed:
  lo,hi=(-1,top) if mode==1 else (0,256)
  ret=None
  if mode==1:
   same=collections.Counter(sq for k,(nx,sq) in row.items() if k>=0 and nx==k)
   if same:
    rs,c=max(same.items(),key=lambda t:(t[1],-t[0]))
    if c>=2:ret=(rs,sorted(k for k,(nx,sq) in row.items() if k>=0 and nx==k and sq==rs))
  skip=set(ret[1]) if ret else set()
  base=row.get(lo,default);last=base;hidden=[]
  for x in range(lo+1,hi+1):
   if x in skip:continue   # answered by the return declaration; the units never see it
   y=row.get(x,default)
   if y!=last:hidden.append((x,y[0]-last[0],y[1]-last[1]))
   last=y
  units_text=' '.join('%d %d %d'%h for h in hidden)
  if ret:out.append('H 3 %d %d %d %d %d %d %d %s %s'%(lo,hi,len(hidden),*base,ret[0],len(ret[1]),' '.join(map(str,ret[1])),units_text))
  else:out.append('H %d %d %d %d %d %d %s'%(mode,lo,hi,len(hidden),*base,units_text))
  units+=len(hidden)
 return '\n'.join(out)+'\n',ns,units

if __name__=='__main__':
 assert len(sys.argv)==3,'usage: net.py INPUT.tbl OUTPUT.net'
 out,rows,units=convert(pathlib.Path(sys.argv[1]).read_text());pathlib.Path(sys.argv[2]).write_text(out)
 print('threshold network:',rows,'banks,',units,'hidden units,',len(out.encode()),'bytes',file=sys.stderr)
