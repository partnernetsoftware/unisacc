"""ARM FP bit-pattern operations, with no FP primitive in the executor.
FARITH/FCMP_INV/FP_OPS are opcode declarations; movement/packing sequences
below are hand-written generation rules. Runtime emits bytes using integer ops.
"""
from unisa.emit_arm import FARITH, FCMP_INV, FP_OPS
SPECS={op:('rrr' if op[:4] in FARITH or op[:3] in FCMP_INV else 'rr',100+i) for i,op in enumerate(FP_OPS)}


def install(E,word):
    def to(p,v,r,dbl):
        return word(p.a(('ALUI','shl','w',r,5),('ALUI','or','w','w',(0x9E670000 if dbl else 0x1E270000)|v)))
    def out(p,dbl):
        return word(p.a(('ALUI','or','w','a0',(0x9E660000 if dbl else 0x1E260000)|(16<<5))))
    def fixed(p,v): return word(p.a(('LDI','w',v)))
    for op,(_,cls) in SPECS.items():
        p=E.P('EMIT.%d'%cls)
        if op[:4] in FARITH or op[:3] in FCMP_INV:
            dbl=op.endswith('64');ty=0x00400000 if dbl else 0
            to(p,16,'a1',dbl);to(p,17,'a2',dbl)
            if op[:4] in FARITH:
                fixed(p,FARITH[op[:4]]|ty|(17<<16)|(16<<5)|16);out(p,dbl)
            else:
                fixed(p,0x1E202000|ty|(17<<16)|(16<<5))
                word(p.a(('ALUI','or','w','a0',0x9A9F07E0|(FCMP_INV[op[:3]]<<12))))
        elif op in ('cvtid','cvtud','cvtis','cvtus'):
            base={'cvtid':0x9E620000,'cvtud':0x9E630000,'cvtis':0x9E220000,'cvtus':0x9E230000}[op]
            word(p.a(('ALUI','shl','w','a1',5),('ALUI','or','w','w',base|16)));out(p,op[-1]=='d')
        elif op in ('cvtdi','cvtdu'):
            to(p,16,'a1',True)
            word(p.a(('ALUI','or','w','a0',(0x9E780000 if op=='cvtdi' else 0x9E790000)|(16<<5))))
        elif op in ('cvtsd','cvtds'):
            to(p,16,'a1',op=='cvtds')
            fixed(p,(0x1E22C000 if op=='cvtsd' else 0x1E624000)|(16<<5)|16);out(p,op=='cvtsd')
        else:
            dbl=op=='fsqrt64';to(p,16,'a1',dbl)
            fixed(p,(0x1E61C000 if dbl else 0x1E21C000)|(16<<5)|16);out(p,dbl)
        p.goto('LINE')
