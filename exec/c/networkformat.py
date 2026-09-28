"""UNINETB1 storage only: record schema and expanded actions stay unchanged."""
from tbl import OPS
import zlib

def require(ok):
 if not ok: raise ValueError("invalid binary network")

def varint(x):
 require(-(1<<63)<=x<(1<<63));v=(x<<1)^(x>>63);out=bytearray()
 while v>=128:out.append((v&127)|128);v>>=7
 out.append(v);return out
def encode(raw):
 out=bytearray(b'UNINETB1')
 for line in raw.splitlines():
  fields=line.split();tag=fields[0];out.extend(tag)
  if tag==b'S':
   require(len(fields)==2);value=bytes.fromhex(fields[1].decode()) if fields[1]!=b'-' else b''
   out.extend(varint(len(value)));out.extend(value)
  else:
   for v in fields[1:]:out.extend(varint(int(v)))
 return bytes(out)
def decode(raw):
 require(raw[:8]==b'UNINETB1');p=8;lines=[]
 def tag(t):
  nonlocal p
  require(raw[p:p+1]==t);p+=1
 def integer():
  nonlocal p
  u=0
  for k in range(10):
   require(p<len(raw));c=raw[p];p+=1;require(k!=9 or c<=1);u|=(c&127)<<(7*k)
   if c<128:
    require(k==0 or c!=0)
    return (u>>1)^-(u&1)
  raise ValueError('long varint')
 def numbers(n):return [integer() for _ in range(n)]
 def line(t,v):lines.append(t+b' '+b' '.join(str(x).encode() for x in v)+(b' ' if (t==b'Q' and v[0]==0) or (t==b'H' and v[3]==0) else b'')+b'\n')
 tag(b'N');head=numbers(6);ns,nq,_,nstr,_,_=head;require(0<ns<=len(raw) and 0<nq<=len(raw) and 0<=nstr<=len(raw));line(b'N',head)
 for _ in range(nstr):
  tag(b'S');n=integer();require(0<=n<=len(raw)-p);value=raw[p:p+n];p+=n;lines.append(b'S '+(value.hex().encode() if n else b'-')+b'\n')
 for _ in range(nq):
  t=raw[p:p+1];p+=1;require(t in (b'Q',b'C'));v=numbers(1 if t==b'Q' else 3);actions=v[-1]
  for _ in range(actions):
   op=integer();require(0<=op<len(OPS));v.append(op);v.extend(numbers(len(OPS[op][1])))
  line(t,v)
 for _ in range(ns):
  tag(b'H');v=numbers(6);count=v[3]
  if v[0]==3:
   extra=numbers(2);v.extend(extra);v.extend(numbers(extra[1]))
  v.extend(numbers(3*count));line(b'H',v)
 require(p==len(raw));return b''.join(lines)

def inflate(blob, rawlen, checksum):
 d = zlib.decompressobj(-15)
 raw = d.decompress(blob, rawlen + 1)
 if len(raw) != rawlen or not d.eof or d.unused_data or d.unconsumed_tail or zlib.crc32(raw) != checksum:
  raise ValueError('invalid compressed model extent/CRC')
 return raw
