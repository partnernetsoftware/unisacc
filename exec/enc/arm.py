"""ARM64 straight-line encoder delta, executed by the existing generic runtime.

ENCSPEC supplies ALU and inverted condition values. armbase-*.tsv declares
basic instruction packing and MOVZ/MOVK selection. armcontract-*.tsv declares
input scanning and operand checks; live specs bind its shared templates.
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


def word(p):
    for _ in range(4):
        p.a(('OUTW', 'w'), ('A64I', 'shr', 'w', 'w', 8))
    return p


def build(image=False, object_mode=False):
    if object_mode:
        E.prn()
    specs = {'mov': ('rr', 1), 'imm': ('ri', 2), 'mul64': ('rrr', 3),
             'ret': ('', 4), 'nop': ('', 5), 'callr': ('r', 6),
             'hostcall': ('rr',60), 'hostaddr': ('ri',61),
             'load64': ('rri',9), 'store64': ('rir',10),
             '.ld': ('rrii',11), '.st': ('riri',12),
             'jump': ('l',13), 'jumpz': ('rl',14), 'call': ('l',15),
             'setreg': ('rv',25), 'spinit': ('r',26), 'gate': ('',27),
             '.lea':('rl',28),'setmem':('ir',29),'argsave':('iib',30),'argvget':('rri',31),'.zero':('rii',32),'winsave':('i',33),'winrest':('ir',34),'winstdh':('i',35),'winargs':('iii',36),'itoa':('iii',37)}
    from exec.facts.load import facts
    import assemble
    here = Path(__file__).parent
    specs.update({r['op']: (r['shape'], r['cls']) for r in facts('enc-armint-specs')})
    specs.update({r['op']: (r['shape'], r['cls']) for r in facts('enc-armfp-specs')})
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
    from tins import META
    from unisa.image.pe import IMPORTS
    # START metadata (was arminput.init / armwin.init / armwin.reset)
    KEYS, IMP = next(r['value'] for r in facts('enc-arminput-bindings') if r['name'] == 'KEYS'), next(r['value'] for r in facts('enc-armwin-bindings') if r['name'] == 'IMP')
    for key in facts('enc-arminput-keys'):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','id_'+key))
    for n,key in enumerate(META,1):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','t'),('LDI','u',n),('STX','t',KEYS,'u'))
    from armlayout import init as init_layout, install as install_layout
    init_layout(p)
    for key in facts('enc-armwin-keys'):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','wi_'+key))
    for i,name in enumerate(IMPORTS):
        p.a(('SBCLR',),[('SBOUT',c) for c in name.encode()],('SBINTERN','t'),('LDI','u',i+1),('STX','t',IMP,'u'))

    def reset_win(r):
        for k in facts('enc-armwin-reset'):
            r.a(('LDI','wm_'+k,0))
        return r
    from finite_rules import install as install_rules
    from functools import partial
    contract = partial(install_rules, g, Path(__file__).parent, 'armcontract')
    contract(section='start', sequences={'initial': p.acts})
    bindings = dict(OP=OP, REG=REG, BASE=BASE, LP=LP)
    for line in Path(__file__).with_name('armcontract-names.tsv').read_text().splitlines():
        if not line.startswith('#'):
            name, owner, kind = line.split('\t')
            bindings[name] = P(owner).fresh(kind)
    contract(section='scan', bindings=bindings, sequences={'reset': reset_win(P('reset.binding')).acts})
    for i in range(4):
        entry = 'PUT' if i == 0 else 'PUT.next%d' % (i - 1)
        contract(section='put', bindings=dict(entry=entry, test=P(entry).fresh('b'),
            hit='PUT.%d' % i, next='PUT.next%d' % i, index=i,
            value='a%d' % i, kind='k%d' % i, negative='neg%d' % i))
    contract(section='after')
    from finite_rules import install_rows
    dispatch_test = P('ENC').fresh('b')
    contract(section='dispatch', bindings={'test': dispatch_test})
    routes = [('dispatch-route', (i,), {'entry': 'CHECK.'+op}) for i, op in enumerate(specs,1)]
    routes.append(('dispatch-unknown', set(range(257))-set(range(1,len(specs)+1)), {}))
    for section, domain, values in routes:
        install_rows(g, Path(__file__).with_name('armcontract-result.tsv'), domain=domain, section=section,
                     bindings={'test': dispatch_test, **values}, mode='r')
    for op, (shape, cls) in specs.items():
        entry = 'CHECK.' + op
        if op == 'spinit':
            contract(section='check', bindings=dict(entry=entry, test=P(entry).fresh('b'),
                yes='SP.address', no='CHECK.spinit.one'), sequences={'predicate': [('CMPI', 'n', 2)]})
            entry = 'CHECK.spinit.one'
        contract(section='check', bindings=dict(entry=entry, test=P(entry).fresh('b'),
            yes='CHECK.' + op + '.n', no='FAIL'), sequences={'predicate': [('CMPI', 'n', len(shape))]})
        entry = 'CHECK.' + op + '.n'
        for i, kind in enumerate(shape):
            nxt = 'CHECK.' + op + '.k%d' % i
            predicate = ('CMP', 'k%d' % i, 'tagkind') if kind == 'v' else (
                'CMPI', 'k%d' % i, {'r': 1, 'i': 2, 'l': 3, 'b': 4}[kind])
            contract(section='check', bindings=dict(entry=entry, test=P(entry).fresh('b'),
                yes=nxt, no='FAIL'), sequences={'predicate': [predicate]})
            entry = nxt
            if kind == 'i' and op != 'imm':
                contract(section='signed', bindings=dict(entry=nxt, test=P(nxt).fresh('b'),
                    positive=nxt + '.positive', bound=P(nxt + '.positive').fresh('b'),
                    next=nxt + '.signed', negative='neg%d' % i, value='a%d' % i))
                entry = nxt + '.signed'
        contract(section='finish', bindings=dict(entry=entry, next='EMIT.%d' % cls))
    sequences = {'word': word(P('word.binding')).acts}
    install_rules(g, Path(__file__).parent, 'armbase', sequences=sequences, section='move')
    for cls, base in ((3, ('ALUI', 'or', 'w', 'w', 0x9B007C00)),
                      (7, ('ALU', 'or', 'w', 'w', 'base'))):
        install_rules(g, Path(__file__).parent, 'armbase', section='rrr',
                      bindings={'entry': 'EMIT.%d' % cls}, sequences={**sequences, 'base': [base]})
    bindings = {name: P(owner).fresh(kind) for name, owner, kind in
                (('label0', 'EMIT.6', 'b'), ('label1', 'CALLR.sp', 'b'), ('label2', 'EMIT.2', 'r'))}
    install_rules(g, Path(__file__).parent, 'armbase', bindings=bindings,
                  sequences=sequences, section='base')
    for halfword in (1, 2, 3):
        state = 'IMM.%d' % halfword
        install_rules(g, Path(__file__).parent, 'armbase', section='movk', sequences=sequences,
                      bindings=dict(state=state, put='IMM.put%d' % halfword,
                                    branch=P(state).fresh('b'),
                                    next='IMM.%d' % (halfword + 1) if halfword < 3 else 'RET',
                                    shift=16 * halfword, movk=0xF2800000 | (halfword << 21)))
    env = {'word': word(P('word.binding')).acts}
    for stem in ('armmem', 'armbranch', 'armint', 'armfp'):
        assemble.run(here/(stem+'-manifest.tsv'), E, P, dict(image=image), env)
    env['meta_classes'] = {"meta_"+key: [index] for index, key in enumerate(META, 1)}
    assemble.run(here/'arminput-manifest.tsv', E, P, {}, env)
    install_layout(E,word)
    assemble.run(here/'armitoa-manifest.tsv', E, P, {}, env)
    assemble.run(here/'armwin-manifest.tsv', E, P, {}, env)
    from hostbridge import install as install_hostbridge
    install_hostbridge(E,'arm64',word)
    if image:
        from elfimage import install as install_elf
        LABELS = next(r['value'] for r in facts('enc-armbranch-bindings') if r['name'] == 'LABELS')
        install_elf(E,_enc.byte,0,LABELS,arch='arm64',direct_labels=True,image_format=image if image in ('macho','pe') else 'elf')
    if object_mode:
        from armlayout import SYM, PRESENT
        import assemble
        assemble.run(Path(__file__).parent/'objectplan-manifest.tsv', E, E.P, dict(direct_labels=True),
                     dict(OFF=0, LABD=LABELS, SYM=SYM, PRESENT=PRESENT, arch='arm64'))
        assemble.run(here/'armobject-manifest.tsv', E, P, {},
                     dict(OBJ_RELOCS=assemble.load_facts('enc-objectplan')['OBJ_RELOCS'], LABELS=LABELS))
    contract(section='fail', sequences={'reject': E.rej('not covered: ARM64 operand or instruction')})
    g.finish()
    return {'start':'START','states':{n:[m,{str(k):v for k,v in row.items()}] for n,(m,row) in g.st.items()},'seqs':[list(map(list,s)) for s in g.seqs]}

if __name__=='__main__':
    if len(sys.argv) not in (2,3) or (len(sys.argv)==3 and sys.argv[2] not in ('--elf','--macho','--pe','--object')):
        sys.exit('usage: arm.py OUT.json [--elf|--macho|--pe|--object]')
    mode=sys.argv[2][2:] if len(sys.argv)==3 else False
    d=build(image='elf' if mode=='object' else mode, object_mode=mode=='object');open(sys.argv[1],'w').write(json.dumps(d,separators=(',',':')))
    print('ARM64 states',len(d['states']),file=sys.stderr)
