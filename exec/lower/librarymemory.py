"""Library mapping callback facts and declared routing, not a host compiler pass."""
from pathlib import Path
from finite_rules import install as rules
from modelinput import u64
def install(E, os_, regmap, sysa):
    for op in ('mmap','munmap'):
        u64(E,'LM.read.'+op,('\0library/'+op).encode(),'library_'+op,'library_has'+op,'C.fail')
    from unisa.emit_x86 import SCR
    from unisa.emit_arm import IP0
    scratch='x'+str(IP0) if regmap['r0'].startswith('x') else SCR
    assert scratch not in regmap.values(), 'library zero scratch aliases tape register'
    rules(E.g,Path(__file__).parent,'librarymemory',section='route',
          bindings=dict(supported='C.prelude.base' if os_ in ('osx','lnx','win') else 'C.fail', SYSA=sysa),
          sequences=dict(fn=E.O('setreg '+regmap['r1']+', imm '),
                         argv=E.O('setreg '+regmap['r0']+', addr '), zero=E.O(', '+scratch+'\n'), zeroinit=E.O('setreg '+scratch+', imm 0\n'),
                         store=E.O('setmem '), call=E.O('hostcall '+regmap['r1']+', '+regmap['r0']+'\n'),
                         retname=[('SBOUT',b) for b in regmap['r0'].encode()]))
