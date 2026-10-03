"""Tape templates for the product's undeclared-printf fallback.
The pfconv truth table selects a routine. These templates preserve the
reference fallback's ignored widths/precision and zero return, not libc's
full printf contract. Declared printf still uses the ordinary call path.
"""
import sys as _s; from pathlib import Path as _P; _s.path.insert(0, str(_P(__file__).resolve().parent.parent / 'facts')); from load import facts
globals().update((r['name'], r['value']) for r in facts('printfallback'))
def install(E, P):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules, install_template
    root = Path(__file__).parent
    def rows(suffix):
        return (line.split('\t') for line in (root/('printfallback-'+suffix+'.tsv')).read_text().splitlines()
                if not line.startswith('#'))
    texts = {'SLEN':SLEN, 'ITOAB':ITOAB}
    sequences = {name:E.O(texts[value] if kind=='binding' else json.loads(value))
                 for name,kind,value in rows('text')}
    templates = {kind:json.loads(value) for kind,value in rows('kinds')}
    bindings = {}
    def rules(section):
        for selected,name,prefix,kind in rows('names'):
            if selected == section:
                bindings[name] = P(prefix+'.pf_'+name).fresh(kind)
        install_rules(E.g, root, 'printfallback', bindings=bindings,
                      sequences=sequences, section=section)
    rules('head')
    for index,kind in enumerate(KINDS):
        bindings['convert'] = 'PF.convert.'+kind
        install_template(E.g, root, 'printfallback', {'kind':[{'i':index,'name':kind}]}, None,
                         bindings=bindings, section='dispatch')
        sequences['body'] = [op for action in templates.get(kind, [])
                             for op in (E.O(action[1]) if action[0]=='text' else [tuple(action)])]
        rules('body')
    rules('tail')
