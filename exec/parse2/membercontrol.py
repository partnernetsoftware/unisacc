"""Bind member and assignment declarations to shared generators and dynamic type facts."""
import json
import re
from pathlib import Path
from finite_rules import install as install_rules, load as load_rules


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
    def section(name, **extra):
        b.update(extra)
        for part,mode,owner,kind,key in rows('fresh'):
            if part==name and (mode=='all' or mode==('warnings' if warnings else 'plain')):
                p.cur=b[owner[1:]] if owner.startswith('$') else owner
                b[key]=p.fresh(kind)
        for part,state_key,load_state in rows('operators'):
            if part==name:
                for op in E.CASOPS:
                    # below a unary * (deref=1) the walk leaves `op=` to the caller: `*x.p += 2`
                    check=b[state_key]+'.q'+op
                    P(check).branch({1:check+'.l'},'LV.c'+op,[('CMPI','deref',1)])
                    P(check+'.l').a(('LDI','deref',0)).goto(load_state)
                    rules=load_rules(root/'membercontrol-operator-rule.tsv',{},domain=[tokens[op+'=']],
                                     bindings=dict(entry=b[state_key],target='LV.c'+op,check=check))
                    for state,row in rules.items():
                        for key,(target,actions) in row.items():
                            E.g.on(state,[key],target,actions,'r')
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
    P('AS.mask').branch({1:'AS.mask.target'}, 'AS.store', [('CMPI','lt',0)])
    target = P('AS.mask.target')
    for name, code, size, uns, _ in tyint:
        next_target = target.fresh('b')
        state = 'AS.mask.' + name
        target.branch({1:state}, next_target, [('CMPI','lb',code)])
        target.label(next_target)
        if size >= 8:
            P(state).goto('AS.store')
            continue
        P(state).branch({1:state+'.scalar'}, 'AS.needmask', [('CMPI','rvt',0)])
        no_mask = sorted({v for v, width, signedness in sources
                          if width <= size and signedness == uns})
        rhs = P(state+'.scalar')
        for value in no_mask:
            next_rhs = rhs.fresh('b')
            rhs.branch({1:'AS.store'}, next_rhs, [('CMPI','rvb',value)])
            rhs.label(next_rhs)
        rhs.goto('AS.needmask')
    target.goto('AS.store')
