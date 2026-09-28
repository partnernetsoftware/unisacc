"""Offline package audit shared by byte ledgers and compatibility referees.
The production reader is independent C; this reader never executes a model.
"""
from networkformat import inflate, decode

def read_package(data):
    at=0
    def line():
        nonlocal at
        end=data.index(b'\n',at);v=data[at:end].split();at=end+1;return v
    def take(n):
        nonlocal at
        if n<0 or n>len(data)-at:raise ValueError('package extent')
        b=data[at:at+n];at+=n;return b
    h=line()
    if len(h)<2 or h[0]!=b'P' or h[1] not in (b'1',b'2',b'3'):raise ValueError('unknown package version')
    version=int(h[1])
    if len(h)!=(4 if version==1 else 5):raise ValueError('package header')
    nm,nd=map(int,h[2:4]);nr=0 if version==1 else int(h[4])
    if nm<=0 or nd<=0 or nr<0 or max(nm,nd,nr)>len(data):raise ValueError('package count')
    rows=[];seen=set();last={}
    for _ in range(nd):
        row=line()
        if len(row)!=6 or row[0]!=b'D' or not 0<=int(row[5])<nm:raise ValueError('stage row')
        route,stage,inp,out=row[1:5]
        if (route,stage) in seen or (route in last and last[route]!=inp):raise ValueError('stage order')
        seen.add((route,stage));last[route]=out;rows.append(row)
    models=[];wires=[]
    for _ in range(nm):
        row=line()
        if len(row)!=(5 if version==3 else 2) or row[0]!=b'M':raise ValueError('model record')
        n=int(row[1])
        if n<=0:raise ValueError('model length')
        wire=take(n)
        if version==3:
            length,codec,checksum=map(int,row[2:])
            if length<=0 or length>=2**31 or codec!=1 or not 0<=checksum<2**32:raise ValueError('model metadata')
            raw=inflate(wire,length,checksum)
            if not raw.startswith(b'UNINETB1N'):raise ValueError('binary network required')
            text=decode(raw)
        else:raw=wire;text=wire
        if not text.startswith(b'N ') or not text.endswith(b'\n'):raise ValueError('network required')
        models.append(text);wires.append({'stored':wire,'raw':raw})
    resources={}
    for _ in range(nr):
        row=line()
        if len(row)!=3 or row[0]!=b'F':raise ValueError('resource record')
        key=take(int(row[1]));value=take(int(row[2]))
        if not key or key in resources:raise ValueError('resource key')
        resources[key]=value
    if at!=len(data):raise ValueError('trailing package bytes')
    return {'version':version,'rows':rows,'models':models,'wires':wires,'resources':resources}
