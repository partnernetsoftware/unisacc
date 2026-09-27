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
    P,g=E.P,E.g
    p=P('ITO.store').branch({1:'ITO.args'},'DEAD.itoa',[('CMPI','na',3)])
    p=P('ITO.args')
    for i in range(3):
        p.branch({1:'ITO.kind'+str(i)},'DEAD.itoa',[('CMPI','ak'+str(i),1)])
        p=P('ITO.kind'+str(i)).a(('LDI','limit',2147483647))
        p.branch({2:'DEAD.itoa'},'ITO.bound'+str(i),[('C64U','a'+str(i),'limit')]);p=P('ITO.bound'+str(i))
    p.a(('STX','npc',VALUE,'a0'),('STX','npc',VALUE2,'a1'),('STX','npc',DEST,'a2'),('LDI','t',8),('STX','npc',KND,'t'),('LDI','t',SIZE),('STX','npc',SZ,'t'),('ALUI','add','npc','npc',1)).goto('SKIPL')
    p=P('WR.itoa')
    def rip(reg,op,table):
        p.a(('LDI','ad_r',reg),('LDI','ad_o',op),('LDX','ad_v','q',table),('A64','add','ad_v','ad_v','data_shift')).call('RIP')
    rip(0,0x8b,VALUE)
    p.a([('OUT',b) for b in PREFIX])
    rip(14,0x89,DEST)
    rip(3,0x8d,VALUE2)
    p.a([('OUT',b) for b in SUFFIX]).goto('WR.nx')
    g.on('DEAD.itoa',range(257),'DEAD',E.rej('not covered: itoa address/operands'),'r')
