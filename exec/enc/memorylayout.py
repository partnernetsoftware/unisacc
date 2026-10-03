"""Optional native-memory binding, expressed as ordinary model actions.
A reserved text base lets the model derive the aligned data base and encode
once. The retained explicit text/data interface also supports the old two-pass
driver. Neither path asks a reference compiler.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from modelinput import u64
from unisa.tape import DATA_BASE
from unisa.image.pe import DLL, IMPORTS
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from exec.facts.load import facts


def install(E, fail, code_size='endo'):
    P=E.P
    for r in facts('enc-memorylayout-resources'):
        u64(E,r['entry'],b'\0'+r['resource'].encode(),r['value'],r['present'],fail)
    from finite_rules import install as install_rules, load as load_rules
    root = Path(__file__).parent
    bindings = dict(fail=fail, DATA_BASE=DATA_BASE, code_size=code_size,
                    reserve_choice="ML.reserve-choice", iat_size=8*len(IMPORTS))
    for line in (root/'memorylayout-names.tsv').read_text().splitlines():
        if line and not line.startswith('#'):
            name, owner, kind = line.split('\t')
            bindings[name] = P(owner).fresh(kind)
    install_rules(E.g, root, 'memorylayout', bindings=bindings, section='base')
    pending = load_rules(root/'memorylayout-result.tsv', {}, bindings=bindings,
                         section='prefix')['actions'][0][1]
    current = 'ML.darwin'
    for i in range(4):
        label='ML.dl.'+str(i); value='ml_dl_'+str(i)
        u64(E,label,b'\0process/dl/'+str(i).encode(),value,'ml_found',fail)
        resume=P(current).fresh('r'); found=P(resume).fresh('b')
        nonzero=P(label+'.present').fresh('b')
        install_rules(E.g, root, 'memorylayout', section='import',
                      bindings=dict(bindings, entry=current, read=label, value=value,
                                    resume=resume, found=found, nonzero=nonzero,
                                    present=label+'.present', next=label+'.nonzero'),
                      sequences={'pending': []})
        current=label+'.nonzero'
    install_rules(E.g, root, 'memorylayout', section='finish',
                  bindings={'entry': current}, sequences={'pending': []})
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
