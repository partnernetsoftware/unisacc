"""Fixed foreign ABI bridge: shared declarations, generic integer actions.
No host compiler/encoder is called by this model generator.
"""
from pathlib import Path
from finite_rules import install as install_rules, install_template
from unisa.hostabi import ARM_FN, ARM_ARGV, ARM_BODY, X86_BODY, WIN_ARM_BODY, WIN_X86_BODY
from unisa.catalog import REGMAP
from unisa.emit_x86 import NUM


def guard(E, entry, predicate, yes, accepted=(1,), no='HB.fail'):
    test=E.P(entry).fresh('b')
    fact=dict(entry=entry,test=test,yes=yes,no=no,acc=','.join(map(str,sorted(accepted))))
    install_template(E.g,Path(__file__).parent,'hostbridge',dict(g=[fact]),None,
                     sequences={'predicate':predicate},section='guard')


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
    root=Path(__file__).parent
    install_template(E.g,root,'hostbridge',{},None,sequences={'fail':E.rej('not covered: foreign host ABI target or operands')},section='fail')
    if arch=='arm64':
        bodies={'word':word(_Acts()).acts,
                'body':[x for v in ARM_BODY for x in [('LDI','w',v)]+word(_Acts()).acts],
                'winbody':[x for v in WIN_ARM_BODY for x in [('LDI','w',v)]+word(_Acts()).acts]}
        install_template(E.g,root,'hostbridge',{},E.P('HB.call.emit').fresh,
                         bindings=dict(armfn=ARM_FN,armargv=ARM_ARGV),sequences=bodies,section='arm')
    else:
        bodies={'body':[('OUT',b) for b in X86_BODY],'winbody':[('OUT',b) for b in WIN_X86_BODY]}
        install_template(E.g,root,'hostbridge',{},E.P('HB.call.emit').fresh,sequences=bodies,section='x86')


class _Acts:
    """Collects the actions a word writer appends (no graph is touched)."""
    def __init__(self): self.acts=[]
    def a(self,*xs):
        self.acts.extend(xs);return self
