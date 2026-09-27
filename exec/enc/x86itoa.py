"""Signed decimal formatter as fixed x86 instruction templates plus delta RIP
relocations. The templates implement count/write loops; they do not contain
input-dependent precomputed addresses, digits, or reference encoder results.
"""
from address import VALUE, VALUE2, DEST

def rr(d,s):
    return bytes([0x48|((s>>3)<<2)|(d>>3),0x89,0xc0|((s&7)<<3)|(d&7)])
def imm(d,v):
    return (b'\x41' if d>=8 else b'')+bytes([0xb8+(d&7)])+v.to_bytes(4,'little')
NEG=bytes.fromhex('48f7d8')+imm(12,1)
PREFIX=bytes.fromhex('4d31e4 4885c0')+bytes([0x79,len(NEG)])+NEG
PREFIX+=rr(15,0)+imm(11,10)+rr(13,0)+bytes.fromhex('4d31f6')
COUNT=rr(0,13)+bytes.fromhex('4831d2 49f7f3')+rr(13,0)+bytes.fromhex('49ffc6 4d85ed')
PREFIX+=COUNT+bytes([0x75,(-len(COUNT)-2)&255])+bytes.fromhex('4d01e6')
DIGITS=rr(0,15)+bytes.fromhex('4831d2 49f7f3')+rr(15,0)+bytes.fromhex('4883c230 49ffcd 41885500 4d85ff')
SUFFIX=bytes.fromhex('4e8d2c33')+DIGITS+bytes([0x75,(-len(DIGITS)-2)&255])+bytes.fromhex('4d85e4 7403 c6032d')
SIZE=21+len(PREFIX)+len(SUFFIX)

def install(E,KND,SZ):
    from pathlib import Path
    from functools import partial
    from finite_rules import install as install_rules
    rules = partial(install_rules, E.g, Path(__file__).parent, 'x86itoa')
    rules(section='arity', bindings={'test': E.P('ITO').fresh('b')})
    for i in range(3):
        rules(section='argument', bindings=dict(entry='ITO.args' if i == 0 else 'ITO.bound'+str(i-1),
            kindtest=E.P('ITO').fresh('b'), kindentry='ITO.kind'+str(i), kind='ak'+str(i),
            boundtest=E.P('ITO').fresh('b'), value='a'+str(i), next='ITO.bound'+str(i)))
    rules(section='store', bindings=dict(VALUE=VALUE, VALUE2=VALUE2, DEST=DEST, KND=KND, SZ=SZ, SIZE=SIZE))
    entry = 'WR.itoa'
    for reg, opcode, table, prefix in ((0,0x8b,VALUE,b''),(14,0x89,DEST,PREFIX),(3,0x8d,VALUE2,b'')):
        nxt = E.P('WR').fresh('r')
        rules(section='rip', bindings=dict(entry=entry, next=nxt, reg=reg, opcode=opcode, table=table),
              sequences={'prefix': [('OUT', b) for b in prefix]})
        entry = nxt
    rules(section='finish', bindings={'entry': entry}, sequences={
        'suffix': [('OUT', b) for b in SUFFIX], 'reject': E.rej('not covered: itoa address/operands')})
