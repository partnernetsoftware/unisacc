"""Module lowering disables implicit first-function process setup."""
from pathlib import Path
from modelinput import u64
from finite_rules import install as rules

def install(E, regmap):
    g=E.g
    from code import REG
    from unisa.emit_arm import IP0, IP1
    from unisa.emit_x86 import SCR, SCR2
    scratches=('x'+str(IP0),'x'+str(IP1)) if regmap['r0'].startswith('x') else (SCR,SCR2)
    assert not set(scratches)&set(regmap.values()), 'process scratches alias tape values'
    s0,s1=scratches
    u64(E,'LMD.processread',b'\0library/process','library_process','lmd_hasprocess','C.fail')

    g.st['LMD.original.prelude']=g.st.pop('C.prelude');g.labels.add('LMD.original.prelude')
    u64(E,'LMD.read',b'\0library/module','library_module','lmd_present','C.fail')
    u64(E,'LMD.symbolsread',b'\0library/symbols','lmd_symbols','lmd_hassymbols','C.fail')
    rules(g,Path(__file__).parent,'librarymodule',section='entry')
    g.st['LMD.original.default']=g.st.pop('C.default');g.labels.add('LMD.original.default')
    rules(g,Path(__file__).parent,'librarymodule',section='default')
    g.st['LMD.original.setup']=g.st.pop('C.setup');g.labels.add('LMD.original.setup')
    rules(g,Path(__file__).parent,'librarymodule',section='setup')
    for name in ('DO.argc','DO.argv'):
        original='LMD.original.'+name
        g.st[original]=g.st.pop(name);g.labels.add(original)
        rules(g,Path(__file__).parent,'librarymodule',section='process',bindings=dict(entry=name,select=name+'.module',original=original,check=name+'.process',emit='LMD.argc' if name=='DO.argc' else 'LMD.argv'))
    rules(g,Path(__file__).parent,'librarymodule',section='dynamic',bindings=dict(REG=REG),
          sequences=dict(base=E.O('setreg '+s0+', imm '),
                         argc_load=E.O(', '+s0+', 0\n'),
                         argv_prefix=E.O('\nload64 '+s0+', '+s0+', 8\nsetreg '+s1+', imm 8\nmul64 '+s1+', '+s1+', '),
                         argv_add=E.O('\nadd64 '+s0+', '+s0+', '+s1+'\nload64 '),
                         newline=E.O('\n'),load=E.O('load64 ')))
