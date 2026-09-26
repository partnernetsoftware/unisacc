"""Two passes over the source: measure actual instruction lengths, then resolve.
No relaxation on ARM64. Labels store byte offset+1, keeping zero as undefined.
Relocation field widths/shifts are read from emit_arm.RELFIELD.
"""
from unisa.emit_arm import RELFIELD
LABELS=74000000


def install(E, word):
    P,g=E.P,E.g
    P('FINISH').branch({1:'ACCEPT'},'REWIND',[('CMPI','pass',1)])
    P('ACCEPT').a(('ACCEPT',)).goto('DEAD')
    P('REWIND').call('LAYOUT').a(('LDI','zero',0),('OCUT','discard','zero'),('LDI','pass',1),('JUMP','zero')).goto('LINE')
    P('LABEL').branch({1:'FAIL'},'LABEL.id',[('CMP','start','end')])
    P('LABEL.id').a(('LDI','started',1),('INTERN','lid','start','end'),('OLEN','pos'),('ALUI','add','pos','pos',1),('LDX','prev','lid',LABELS)).branch({1:'LABEL.verify'},'LABEL.new',[('CMPI','pass',1)])
    P('LABEL.new').branch({1:'LABEL.save'},'FAIL',[('CMPI','prev',0)])
    P('LABEL.save').a(('STX','lid',LABELS,'pos')).goto('LABEL.end')
    P('LABEL.verify').branch({1:'LABEL.end'},'FAIL',[('CMP','prev','pos')])
    g.on('LABEL.end',[32],'LABEL.end',[('ADV',)])
    g.on('LABEL.end',[10,256],'LINE',[])
    g.els('LABEL.end','FAIL',[])
    for cls,tag in ((13,'arm26'),(14,'arm19'),(15,'arm26')):
        bits,shift=RELFIELD[tag]
        p=P('EMIT.%d'%cls).a(('COPYW','target','a1' if cls==14 else 'a0'))
        if cls==15:
            # ADR x17,+16; push continuation on software stack x7.
            for v in (0x10000091,0xD10020E7,0xF90000F1):
                word(p.a(('LDI','w',v)))
        p.a(('LDI','bits',bits),('LDI','fieldshift',shift)).call('BR.disp')
        p.a(('ALUI','or','w','disp',{13:0x14000000,14:0xB4000000,15:0x94000000}[cls]))
        if cls==14: p.a(('ALU','or','w','w','a0'))
        word(p).goto('LINE')
    P('BR.disp').a(('LDI','disp',0)).branch({1:'BR.lookup'},'RET',[('CMPI','pass',1)])
    P('BR.lookup').a(('LDX','dest','target',LABELS)).branch({1:'FAIL'},'BR.calc',[('CMPI','dest',0)])
    P('BR.calc').a(('OLEN','pos'),('A64I','sub','dest','dest',1),('A64','sub','disp','dest','pos'),('ALUI','and','t','disp',3)).branch({1:'BR.range'},'FAIL',[('CMPI','t',0)])
    P('BR.range').a(('A64I','sar','disp','disp',2),('ALUI','sub','t','bits',1),('LDI','limit',1),('A64','shl','limit','limit','t'),('LDI','zero',0),('A64','sub','low','zero','limit')).branch({0:'FAIL'},'BR.high',[('C64','disp','low')])
    P('BR.high').branch({0:'BR.mask'},'FAIL',[('C64','disp','limit')])
    P('BR.mask').a(('A64I','shl','limit','limit',1),('A64I','sub','limit','limit',1),('A64','and','disp','disp','limit'),('A64','shl','disp','disp','fieldshift')).ret()
