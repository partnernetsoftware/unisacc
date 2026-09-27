"""Optional native-memory binding, expressed as ordinary model actions.
The first pass without mapped bases supplies a size plan; the second pass
encodes at the actual bases. Neither pass asks a reference compiler.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from modelinput import u64
from unisa.tape import DATA_BASE
from unisa.image.pe import DLL, IMPORTS


def install(E, fail, code_size='endo'):
    P=E.P
    u64(E,'ML.mode',b'\0process/argc','ml_argc','memory_mode',fail)
    u64(E,'ML.argv',b'\0process/argv','ml_argv','ml_hasargv',fail)
    u64(E,'ML.text',b'\0memory/text','ml_text','ml_hast',fail)
    u64(E,'ML.data',b'\0memory/data','ml_data','ml_hasd',fail)
    from finite_rules import install as install_rules, load as load_rules
    root = Path(__file__).parent
    bindings = dict(fail=fail, DATA_BASE=DATA_BASE, code_size=code_size)
    for line in (root/'memorylayout-names.tsv').read_text().splitlines():
        if line and not line.startswith('#'):
            name, owner, kind = line.split('\t')
            bindings[name] = P(owner).fresh(kind)
    install_rules(E.g, root, 'memorylayout', bindings=bindings, section='base')
    pending = load_rules(root/'memorylayout-result.tsv', {}, bindings=bindings,
                         section='prefix')['actions'][0][1]
    current = 'ML.windows'
    for i,name in enumerate(IMPORTS):
        label='ML.import.'+str(i); value='ml_imp_'+str(i)
        u64(E,label,b'\0process/import/'+DLL.lower()+b'/'+name.encode(),value,'ml_found',fail)
        resume = P(current).fresh('r')
        found = P(resume).fresh('b')
        nonzero = P(label+'.present').fresh('b')
        install_rules(E.g, root, 'memorylayout', section='import',
                      bindings=dict(bindings, entry=current, read=label, value=value,
                                    resume=resume, found=found, nonzero=nonzero,
                                    present=label+'.present', next=label+'.nonzero'),
                      sequences={'pending': pending})
        current, pending = label+'.nonzero', []
    install_rules(E.g, root, 'memorylayout', section='finish',
                  bindings={'entry': current}, sequences={'pending': pending})
