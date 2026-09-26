#!/usr/bin/env python3
"""Construct state-conditioned integer threshold networks, without training.

For each finite row f, base=f(lo), h_t(x)=[x>=t], W_t=f(t)-f(t-1).
Then f(x)=base+sum W_t*h_t(x), exactly, by telescoping. Two linear outputs
encode next state and action-sequence ID. Missing transitions remain (-1,0).
S/Q records are unchanged declarations. N/H records contain weights, not
per-observation answers. Runtime must evaluate them, not expand them to R.
"""
import pathlib,sys
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
  base=row.get(lo,default);last=base;hidden=[]
  for x in range(lo+1,hi+1):
   y=row.get(x,default)
   if y!=last:hidden.append((x,y[0]-last[0],y[1]-last[1]))
   last=y
  out.append('H %d %d %d %d %d %d %s'%(mode,lo,hi,len(hidden),*base,' '.join('%d %d %d'%h for h in hidden)))
  units+=len(hidden)
 return '\n'.join(out)+'\n',ns,units

if __name__=='__main__':
 assert len(sys.argv)==3,'usage: net.py INPUT.tbl OUTPUT.net'
 out,rows,units=convert(pathlib.Path(sys.argv[1]).read_text());pathlib.Path(sys.argv[2]).write_text(out)
 print('threshold network:',rows,'banks,',units,'hidden units,',len(out.encode()),'bytes',file=sys.stderr)
