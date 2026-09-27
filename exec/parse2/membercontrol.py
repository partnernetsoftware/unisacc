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
        for part,state_key in rows('operators'):
            if part==name:
                for op in E.CASOPS:
                    rules=load_rules(root/'membercontrol-operator-rule.tsv',{},domain=[tokens[op+'=']],
                                     bindings=dict(entry=b[state_key],target='LV.c'+op))
                    for state,row in rules.items():
                        for key,(target,actions) in row.items():
                            E.g.on(state,[key],target,actions,'r')
        for mode in ('all','warnings' if warnings else 'plain'):
            if name+'.'+mode in sections:
                install_rules(E.g,root,'membercontrol',bindings=b,sequences=sequences,classes=classes,section=name+'.'+mode)
    section('part0')
    shape_control('subscript')
    section('part2')
