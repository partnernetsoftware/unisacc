"""Fixed foreign ABI bridge: shared declarations, generic integer actions.
No host compiler/encoder is called by this model generator.
"""
from pathlib import Path
from finite_rules import install as install_rules, load as load_rules
from unisa.hostabi import ARM_FN, ARM_ARGV, ARM_BODY, X86_BODY, WIN_X86_BODY
from unisa.catalog import REGMAP
from unisa.emit_x86 import NUM


def guard(E, entry, predicate, yes, accepted=(1,), no='HB.fail'):
    test=E.P(entry).fresh('b')
    bindings=dict(entry=entry,test=test,yes=yes,no=no)
    root=Path(__file__).parent
    install_rules(E.g,root,'hostbridge',section='guard',bindings=bindings,sequences={'predicate':predicate})
    for section,domain in [('guard-yes',accepted),('guard-no',set(range(257))-set(accepted))]:
        rules=load_rules(root/'hostbridge-result.tsv',{},section=section,bindings=bindings,domain=domain)
        for state,row in rules.items():
            for key,(target,actions) in row.items():E.g.on(state,[key],target,actions,'r')


def install(E, arch, word=None):
    P=E.P
    entry='EMIT.60' if arch=='arm64' else 'HB.call'
    count,kind=('n','k') if arch=='arm64' else ('na','ak')
    regkind,intkind=(1,2) if arch=='arm64' else (0,1)
    allowed=tuple(range(8)) if arch=='arm64' else tuple(NUM[r] for r in REGMAP['x86_64'])
    for op,start in [('call',entry),('addr','EMIT.61' if arch=='arm64' else 'HB.addr')]:
        # macOS and Linux use the same fixed integer/pointer ABI bridge. Linux
        # additionally needs a dynamic ELF and four loader slots in the image.
        guard(E,start,[('RLD','target_os')],'HB.'+op+'.arity',
              accepted=(1,2), no='HB.call.librarytarget' if op=='call' else 'HB.fail')
        guard(E,'HB.'+op+'.arity',[('CMPI',count,2)],'HB.'+op+'.kind0')
        guard(E,'HB.'+op+'.kind0',[('CMPI',kind+'0',regkind)],'HB.'+op+'.kind1')
        guard(E,'HB.'+op+'.kind1',[('CMPI',kind+'1',regkind if op=='call' else intkind)],'HB.'+op+'.reg0')
        guard(E,'HB.'+op+'.reg0',[('RLD','a0')],'HB.'+op+'.value1',allowed)
        if op=='call':guard(E,'HB.call.value1',[('RLD','a1')],'HB.call.emit',allowed)
        else:guard(E,'HB.addr.value1',[('RLD','a1')],'HB.addr.emit',(0,1,2,3))
    from modelinput import u64
    u64(E,'HB.libraryread',b'\0library/exit','hb_libraryexit','hb_hasexit','HB.fail')
    u64(E,'HB.librarymmap',b'\0library/mmap','hb_librarymmap','hb_hasmmap','HB.fail')
    u64(E,'HB.librarymunmap',b'\0library/munmap','hb_librarymunmap','hb_hasmunmap','HB.fail')
    guard(E,'HB.call.librarytarget',[('RLD','target_os')],'HB.call.libraryread',(1,3))
    install_rules(E.g,Path(__file__).parent,'hostbridge',section='library')
    P('HB.fail').a(E.rej('not covered: foreign host ABI target or operands')).goto('DEAD')
    p=P('HB.call.emit').a(('LDI','host_dyn',1))
    if arch=='x86_64':
        p.branch({1:'HB.call.emitwin'},'HB.call.emitposix',[('CMPI','target_os',3)])
        p=P('HB.call.emitposix')
    if arch=='arm64':
        for base,arg in [(ARM_FN,'a0'),(ARM_ARGV,'a1')]:
            p.a(('ALUI','shl','w',arg,16),('ALUI','or','w','w',base));word(p)
        for value in ARM_BODY:p.a(('LDI','w',value));word(p)
        p.goto('LINE')
        p=P('HB.addr.emit').a(('LDI','host_dyn',1))
        p.a(('COPYW','ad_r','a0'),('A64I','mul','ad_v','a1',8),
            ('A64I','add','ad_v','ad_v',224)).call('AD.data').call('ADRP')
        p.a(('ALUI','shl','w','a0',5),('ALU','or','w','w','a0'),('ALUI','or','w','w',0xF9400000))
        word(p);p.goto('LINE')
    else:
        for dest,arg in [(11,'a0'),(0,'a1')]:
            p.a(('ALUI','sar','t',arg,3),('ALUI','shl','t','t',2),
                ('ALUI','or','t','t',0x48|(dest>>3)),('OUTW','t'),('OUT',0x89),
                ('ALUI','and','t',arg,7),('ALUI','shl','t','t',3),
                ('ALUI','or','t','t',0xC0|(dest&7)),('OUTW','t'))
        p.a([('OUT',b) for b in X86_BODY]).goto('NEXTL')
        win=P('HB.call.emitwin')
        # Operand prefix has no OS-specific rule; emit the identical moves.
        for dest,arg in [(11,'a0'),(0,'a1')]:
            win.a(('ALUI','sar','t',arg,3),('ALUI','shl','t','t',2),
                ('ALUI','or','t','t',0x48|(dest>>3)),('OUTW','t'),('OUT',0x89),
                ('ALUI','and','t',arg,7),('ALUI','shl','t','t',3),
                ('ALUI','or','t','t',0xC0|(dest&7)),('OUTW','t'))
        win.a([('OUT',b) for b in WIN_X86_BODY]).goto('NEXTL')
        P('HB.addr.emit').a(('LDI','host_dyn',1),('COPYW','ad_r','a0'),('A64I','mul','ad_v','a1',8),
            ('A64I','add','ad_v','ad_v',224),('LDI','ad_o',0x8B),('LDI','anamed',0)).goto('AD.store')
