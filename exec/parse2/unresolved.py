"""Resolve ordinary tape calls after every source unit, as undef_calls does.
Two byte scans collect labels and then diagnose unresolved calls in emission
order, once per name. No parser/diagnostic primitive is added to the executor.
"""
DEFINED = 33 << 40

def install(E, P):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    root = Path(__file__).parent
    sequences = {name: E.O(json.loads(text)) for name, text in
                 (line.split('\t') for line in (root/'unresolved-text.tsv').read_text().splitlines()
                  if not line.startswith('#'))}
    def rules(section, bindings=None, classes=None, instance=''):
        bindings = dict(bindings or {}, DEFINED=DEFINED)
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
                        'next':'UD.prefix'+str(i+1) if i<6 else 'UD.name0'},
              {'char':[char]}, str(i))
    rules('tail')
