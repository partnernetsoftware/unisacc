"""ARM64 straight-line encoder delta, executed by the existing generic runtime.

ENCSPEC supplies ALU and inverted condition values. Instruction bit layouts,
MOVZ/MOVK selection and operand contracts below are hand-written rules compiled
into transitions, not new runtime primitives or constructed neural networks.
Input is the TIns line syntax with section-local labels and declared metadata.
Two passes resolve branches after all lengths are measured.
"""
import json
import sys
import importlib.util
from pathlib import Path
_spec = importlib.util.spec_from_file_location('encoder_builder', Path(__file__).with_name('gen.py'))
_enc = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_enc)
E, P, g = _enc.E, _enc.P, _enc.g
from unisa.catalog import ENCSPEC

OP, REG, BASE, LP = 70000000, 71000000, 72000000, 73000000
DIG = list(range(48, 58))
END = [10, 256]
SEP = [32, 44] + END


def word(p):
    for _ in range(4):
        p.a(('OUTW', 'w'), ('A64I', 'shr', 'w', 'w', 8))
    return p


def build(image=False):
    specs = {'mov': ('rr', 1), 'imm': ('ri', 2), 'mul64': ('rrr', 3),
             'ret': ('', 4), 'nop': ('', 5), 'callr': ('r', 6),
             'load64': ('rri',9), 'store64': ('rir',10),
             '.ld': ('rrii',11), '.st': ('riri',12),
             'jump': ('l',13), 'jumpz': ('rl',14), 'call': ('l',15),
             'setreg': ('rv',25), 'spinit': ('r',26), 'gate': ('',27),
             '.lea':('rl',28),'setmem':('ir',29),'argsave':('iib',30),'argvget':('rri',31),'.zero':('rii',32),'winsave':('i',33),'winrest':('ir',34),'winstdh':('i',35),'winargs':('iii',36)}
    from armint import SPECS, install as install_int
    specs.update(SPECS)
    from armfp import SPECS as FP_SPECS, install as install_fp
    specs.update(FP_SPECS)
    specs.update({k: ('rrr', 7) for k in ENCSPEC['arm64']['alu3']})
    specs.update({k: ('rrr', 8) for k in ENCSPEC['arm64']['invcond']})
    p = P('START')
    for i, (op, (shape, cls)) in enumerate(specs.items(), 1):
        p.a(('SBCLR',), [('SBOUT', c) for c in op.encode()], ('SBINTERN', 't'),
            ('LDI', 'u', i), ('STX', 't', OP, 'u'),
            ('LDI', 'u', shape.find('l')), ('STX', 't', LP, 'u'))
        val = ENCSPEC['arm64']['alu3'].get(op, ENCSPEC['arm64']['invcond'].get(op, 0))
        p.a(('LDI', 'u', val), ('STX', 't', BASE, 'u'))
    # x31 is intentionally excluded: SP/ZR interpretations differ by opcode.
    for i in range(31):
        p.a(('SBCLR',), [('SBOUT', c) for c in ('x%d' % i).encode()],
            ('SBINTERN', 't'), ('LDI', 'u', i+1), ('STX', 't', REG, 'u'))
    from arminput import init, install as install_input
    init(p)
    from armlayout import init as init_layout, install as install_layout
    init_layout(p)
    from armwin import init as init_win, reset as reset_win, install as install_win
    init_win(p)
    p.a(('LDI','pass',0),('LDI','lnum',0)).goto('LINE')
    g.on('LINE',[64],'HDR.key',[('MARK','hs'),('ADV',)])
    g.on('LINE', [256], 'FINISH', [])
    g.on('LINE', [10], 'LINE', [('ADV',)])
    g.els('LINE', 'OP.scan', [('MARK', 'start')])
    g.on('OP.scan', [58], 'LABEL', [('MARK','end'),('ADV',)])
    g.on('OP.scan', [32]+END, 'OP.end', [('MARK', 'end')])
    g.els('OP.scan', 'OP.scan', [('ADV',)])
    reset_win(P('OP.end')).a(('INTERN', 'oid', 'start', 'end'), ('LDX', 'cls', 'oid', OP),
                   ('LDX', 'base', 'oid', BASE), ('LDX','labelpos','oid',LP), ('LDI', 'n', 0),('LDI','started',1),('LDI','stag',0),('LDI','tagkind',0),('LDI','gcarry',0),('LDI','gkind',0),('ALUI','add','lnum','lnum',1)).goto('ARG')
    g.on('ARG', [32], 'ARG', [('ADV',)])
    g.on('ARG', END, 'ENC', [])
    g.els('ARG','ARG.type',[])
    P('ARG.type').branch({1:'NAME'},'VALUE',[('CMP','n','labelpos')])
    g.els('NAME','NAME.scan',[('MARK','start')])
    g.on('NAME.scan',SEP,'NAME.end',[('MARK','end')])
    g.els('NAME.scan','NAME.scan',[('ADV',)])
    P('NAME.end').a(('INTERN','v','start','end'),('LDI','kind',3)).goto('PUT')
    g.on('VALUE', [45]+DIG, 'NUM', [('LDI', 'neg', 0), ('LDI', 'v', 0), ('LDI', 'kind', 2)])
    g.els('VALUE', 'REG.scan', [('MARK', 'start')])
    g.on('REG.scan',[61],'META.begin',[('MARK','end'),('ADV',)])
    g.on('REG.scan', SEP, 'REG.end', [('MARK', 'end')])
    g.els('REG.scan', 'REG.scan', [('ADV',)])
    P('REG.end').a(('INTERN', 't', 'start', 'end')).goto('TAG.check')
    P('REG.value').a(('LDX', 'v', 't', REG)).branch({1:'FAIL'}, 'REG.ok', [('CMPI','v',0)])
    P('REG.ok').a(('ALUI','sub','v','v',1),('LDI','kind',1)).goto('PUT')
    g.on('NUM', [45], 'NUM.first', [('LDI','neg',1),('ADV',)])
    g.els('NUM', 'NUM.first', [])
    g.on('NUM.first', DIG, 'NUM.digit', [])
    g.els('NUM.first','FAIL',[])
    # Unsigned magnitude <= 2^64-1; overflow checked before each multiply.
    P('NUM.digit').a(('LDI','limit',1844674407370955161)).branch({2:'FAIL'},'NUM.mul',[('C64U','v','limit')])
    P('NUM.mul').a(('BYTE','digit'),('ALUI','sub','digit','digit',48)).branch({1:'NUM.last'},'NUM.add',[('C64U','v','limit')])
    P('NUM.last').branch({2:'FAIL'},'NUM.add',[('CMPI','digit',5)])
    P('NUM.add').a(('A64I','mul','v','v',10),('A64','add','v','v','digit'),('ADV',)).goto('NUM.more')
    g.on('NUM.more',DIG,'NUM.digit',[])
    g.on('NUM.more',SEP,'NUM.sign',[])
    g.els('NUM.more','FAIL',[])
    P('NUM.sign').branch({1:'NUM.neg'},'PUT',[('CMPI','neg',1)])
    P('NUM.neg').a(('LDI','limit',-9223372036854775808)).branch({2:'FAIL'},'NUM.negate',[('C64U','v','limit')])
    P('NUM.negate').a(('LDI','zero',0),('A64','sub','v','zero','v')).goto('PUT')
    p=P('PUT')
    for i in range(4):
        p.branch({1:'PUT.%d'%i},'PUT.next%d'%i,[('CMPI','n',i)])
        P('PUT.%d'%i).a(('COPYW','a%d'%i,'v'),('COPYW','k%d'%i,'kind'),('COPYW','neg%d'%i,'neg'),('ALUI','add','n','n',1)).goto('AFTER')
        p=P('PUT.next%d'%i)
    p.goto('FAIL')
    g.on('AFTER',[32],'AFTER',[('ADV',)])
    g.on('AFTER',[44],'REQUIRED',[('ADV',)])
    g.on('AFTER',END,'ENC',[])
    g.els('AFTER','META.key',[('MARK','start')])
    g.on('REQUIRED',[32],'REQUIRED',[('ADV',)])
    g.on('REQUIRED',END,'FAIL',[])
    g.els('REQUIRED','ARG',[])
    P('ENC').branch({i:'CHECK.'+op for i,op in enumerate(specs,1)},'FAIL',[('RLD','cls')])
    for op,(shape,cls) in specs.items():
        p=P('CHECK.'+op)
        if op=='spinit':
            p.branch({1:'SP.address'},'CHECK.spinit.one',[('CMPI','n',2)]);p=P('CHECK.spinit.one')
        p.branch({1:'CHECK.'+op+'.n'},'FAIL',[('CMPI','n',len(shape))]); p=P('CHECK.'+op+'.n')
        for i,k in enumerate(shape):
            nxt='CHECK.'+op+'.k%d'%i
            if k=='v':
                p.branch({1:nxt},'FAIL',[('CMP','k%d'%i,'tagkind')])
            else:
                p.branch({1:nxt},'FAIL',[('CMPI','k%d'%i,{'r':1,'i':2,'l':3,'b':4}[k])])
            p=P(nxt)
            if k=='i' and op!='imm':
                checked=nxt+'.signed'; bound=nxt+'.positive'
                p.branch({1:checked},bound,[('CMPI','neg%d'%i,1)])
                P(bound).a(('LDI','limit',9223372036854775807)).branch({2:'FAIL'},checked,[('C64U','a%d'%i,'limit')])
                p=P(checked)
        p.goto('EMIT.%d'%cls)
    word(P('EMIT.1').a(('ALUI','shl','w','a1',16),('ALUI','or','w','w',0xAA0003E0),('ALU','or','w','w','a0'))).goto('LINE')
    for cls,base in ((3,0x9B007C00),(7,None)):
        p=P('EMIT.%d'%cls).a(('ALUI','shl','w','a2',16),('ALUI','shl','t','a1',5),('ALU','or','w','w','t'),('ALU','or','w','w','a0'))
        p.a(('ALU','or','w','w','base') if base is None else ('ALUI','or','w','w',base))
        word(p).goto('LINE')
    # Tape ABI returns through x17 saved on x7, rather than host LR.
    p=P('EMIT.4')
    for base in (0xF94000F1, 0x910020E7, 0xD65F0220):
        word(p.a(('LDI','w',base)))
    p.goto('LINE')
    for cls,base in ((5,0xD503201F),):
        word(P('EMIT.%d'%cls).a(('LDI','w',base))).goto('LINE')
    P('EMIT.6').branch({1:'FAIL'},'CALLR.sp',[('CMPI','a0',17)])
    P('CALLR.sp').branch({1:'FAIL'},'CALLR.emit',[('CMPI','a0',7)])
    p=P('CALLR.emit')
    for base in (0x10000091, 0xD10020E7, 0xF90000F1):
        word(p.a(('LDI','w',base)))
    word(p.a(('ALUI','shl','w','a0',5),('ALUI','or','w','w',0xD63F0000))).goto('LINE')
    p=P('EMIT.8').a(('ALUI','shl','w','a2',16),('ALUI','shl','t','a1',5),('ALU','or','w','w','t'),('ALUI','or','w','w',0xEB00001F))
    word(p).a(('ALUI','shl','w','base',12),('ALUI','or','w','w',0x9A9F07E0),('ALU','or','w','w','a0'))
    word(p).goto('LINE')
    P('EMIT.2').a(('COPYW','md','a0'),('COPYW','mv','a1')).call('MOVIMM').goto('LINE')
    p=P('MOVIMM').a(('A64I','and','w','mv',65535),('ALUI','shl','w','w',5),('ALUI','or','w','w',0xD2800000),('ALU','or','w','w','md'))
    word(p).goto('IMM.1')
    for sh in (1,2,3):
        nxt='IMM.%d'%(sh+1) if sh<3 else 'RET'
        P('IMM.%d'%sh).a(('A64I','shr','w','mv',16*sh),('ALUI','and','w','w',65535)).branch({1:nxt},'IMM.put%d'%sh,[('CMPI','w',0)])
        word(P('IMM.put%d'%sh).a(('ALUI','shl','w','w',5),('ALUI','or','w','w',0xF2800000|(sh<<21)),('ALU','or','w','w','md'))).goto(nxt)
    from armmem import install
    install(E,word)
    from armbranch import install as install_branch
    install_branch(E,word,image)
    install_int(E,word)
    install_fp(E,word)
    install_input(E,word)
    install_layout(E,word)
    install_win(E,word)
    if image:
        from elfimage import install as install_elf
        from armbranch import LABELS
        install_elf(E,_enc.byte,0,LABELS,arch='arm64',direct_labels=True,image_format=image if image in ('macho','pe') else 'elf')
    g.on('FAIL',range(257),'DEAD',E.rej('not covered: ARM64 operand or instruction'),'r')
    g.finish()
    return {'start':'START','states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in g.st.items()},'seqs':[list(map(list,s)) for s in g.seqs]}

if __name__=='__main__':
    if len(sys.argv) not in (2,3) or (len(sys.argv)==3 and sys.argv[2] not in ('--elf','--macho','--pe')):
        sys.exit('usage: arm.py OUT.json [--elf|--macho|--pe]')
    d=build(image=sys.argv[2][2:] if len(sys.argv)==3 else False);open(sys.argv[1],'w').write(json.dumps(d,separators=(',',':')))
    print('ARM64 states',len(d['states']),file=sys.stderr)
