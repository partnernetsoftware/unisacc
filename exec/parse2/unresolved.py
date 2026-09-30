"""Resolve ordinary tape calls after every source unit, as undef_calls does.
Two byte scans collect labels and then diagnose unresolved calls in emission
order, once per name. No parser/diagnostic primitive is added to the executor.
R13-0b #31: the call scan runs twice.  The first time (ud_mode 0) it only
records which names are called (REFERENCED); the second time it diagnoses,
except inside a block whose label is a file-scope static nobody calls
(STATICF and not REFERENCED): a call there is not a reference, as gcc never
emits such a function.  The block in hand is the last label line the scan
passed (ud_cur).
"""
DEFINED = 33 << 40
REFERENCED = 34 << 40   # REFERENCED[id] = 1: some `call NAME` names it (first call scan)
STATICF = 35 << 40      # STATICF[id] = 1: the parser saw `static` on this function definition

def install(E, P):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    root = Path(__file__).parent
    sequences = {name: E.O(json.loads(text)) for name, text in
                 (line.split('\t') for line in (root/'unresolved-text.tsv').read_text().splitlines()
                  if not line.startswith('#'))}
    def rules(section, bindings=None, classes=None, instance=''):
        bindings = dict(bindings or {}, DEFINED=DEFINED, REFERENCED=REFERENCED, STATICF=STATICF)
        for line in (root/'unresolved-names.tsv').read_text().splitlines():
            if not line.startswith('#'):
                selected, name, prefix, kind = line.split('\t')
                if selected == section:
                    bindings[name] = P(prefix+'.ud_'+name+instance).fresh(kind)
        install_rules(E.g, root, 'unresolved', bindings=bindings,
                      sequences=sequences, classes=classes, section=section)
    rules('head')
    for i, char in enumerate(b'  call '):
        rules('prefix', {'prefix':'UD.prefix'+str(i), 'match':'UD.match'+str(i),
                        'next':'UD.prefix'+str(i+1) if i<6 else 'UD.name0',
                        # a line that does not start `  call`: at its first byte it may be a label
                        'prefix_miss':'UD.linestart' if i == 0 else 'UD.nextline'},
              {'char':[char]}, str(i))
    rules('tail')
