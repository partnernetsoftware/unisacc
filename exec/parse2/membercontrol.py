"""Bind member and assignment declarations to shared generators and dynamic type facts."""
import json
import re
from pathlib import Path
from finite_rules import install as install_rules, install_template


class _Holder:
    def __init__(self, cur):
        self.cur = cur


def install(E, P, warnings, templates, facts, shape_control):
    root = Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root / ('membercontrol-' + suffix + '.tsv')).read_text().splitlines()[1:]]
    b = dict(facts)
    sequences = {name:E.O(json.loads(value)) for name,value in rows('text')}
    for name,value in rows('template'):
        template,index=json.loads(value)
        sequences[name]=E.O(re.split(r'(\{[^}]*\})',templates[template])[index])
    p=P('membercontrol.bindings')
    for name,value in rows('stack'):
        method,slots=json.loads(value);p.acts=[]
        sequences[name]=getattr(p,method)(*slots).acts
    tokens=dict(E.TK,identifier=E.TK_ID)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    sections={line.split('\t')[0] for line in (root/'membercontrol-result.tsv').read_text().splitlines()[1:]}
    def control(name, facts, cur):
        holder = _Holder(cur)
        install_template(E.g, root, 'membercontrol-control', facts,
                         lambda k: E.P.fresh(holder, k), section=name)
    def section(name, **extra):
        b.update(extra)
        for part,mode,owner,kind,key in rows('fresh'):
            if part==name and (mode=='all' or mode==('warnings' if warnings else 'plain')):
                p.cur=b[owner[1:]] if owner.startswith('$') else owner
                b[key]=p.fresh(kind)
        for part,state_key,load_state in rows('operators'):
            if part==name:
                for op in E.CASOPS:   # one instance per operator: its entry edge follows its states
                    control('operator', dict(entry=[b[state_key]], load=[load_state],
                                             OP=[dict(op=op, token=tokens[op+'='])]), b[state_key])
        for mode in ('all','warnings' if warnings else 'plain'):
            if name+'.'+mode in sections:
                install_rules(E.g,root,'membercontrol',bindings=b,sequences=sequences,classes=classes,section=name+'.'+mode)
    section('part0')
    shape_control('subscript')
    section('part2')
    # Assignment expression values are narrowed only when the right value's
    # width exceeds the left width or their signedness differs.  Generate the
    # finite comparison states from tyinfo, the same type facts used by the
    # conversion network; an unused `a = x;` skips this path in the table.
    tyint = facts['TYINT']
    sources = [(code, size, uns) for _, code, size, uns, _ in tyint]
    sources += [(facts['FLT'], 4, 0), (facts['DBL'], 8, 0), (facts['BOOL'], 1, 0)]
    section('part3')
    # The E.CASOPS operator states in section() and the AS.mask chain below are
    # membercontrol-control-template.tsv, instantiated from operator and tyinfo facts.
    types = [dict(name=name, code=code,
                  wide=[1] if size >= 8 else [],
                  narrow=[] if size >= 8 else [dict(values=sorted(
                      {v for v, width, signedness in sources if width <= size and signedness == uns}))])
             for name, code, size, uns, _ in tyint]
    control('mask', dict(T=types), 'AS.mask.target')
