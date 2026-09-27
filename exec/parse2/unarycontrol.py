"""Bind unary-expression declarations to shared generators and dynamic type facts."""
import json
import re
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P, warnings, templates, addr, printf, compound, facts, words):
    root = Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root / ('unarycontrol-' + suffix + '.tsv')).read_text().splitlines()[1:]]
    b = dict(facts)
    sequences = {name:E.O(json.loads(value)) for name,value in rows('text')}
    for name,value in rows('template'):
        template,index=json.loads(value)
        sequences[name]=E.O(re.split(r'(\{[^}]*\})',templates[template])[index])
    p=P('unarycontrol.bindings')
    for name,value in rows('stack'):
        method,slots=json.loads(value);p.acts=[]
        sequences[name]=getattr(p,method)(*slots).acts
    tokens=dict(E.TK,identifier=E.TK_ID)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    classes['typewords']=[tokens[token] for token in (*words,'struct','union','enum')]
    sections={line.split('\t')[0] for line in (root/'unarycontrol-result.tsv').read_text().splitlines()[1:]}
    def section(name, **extra):
        b.update(extra)
        for part,mode,owner,kind,key in rows('fresh'):
            if part==name and (mode=='all' or warnings):
                p.cur=b[owner[1:]] if owner.startswith('$') else owner
                b[key]=p.fresh(kind)
        for mode in ('all','warnings' if warnings else 'plain'):
            if name+'.'+mode in sections:
                install_rules(E.g,root,'unarycontrol',bindings=b,sequences=sequences,classes=classes,section=name+'.'+mode)
    section('part0')
    compound('compound',False,dict(TIX=b['TIX'],MAXTOK=b['MAXTOK']))
    install_rules(E.g,root,'width',section='cast-void')
    section('part4')
    for suffix, rule in rows('conversions'):
        section(rule,
                convert_entry='UC.to_'+suffix,convert_target='TO.'+suffix)
    section('part6')
    printf(warnings)
    section('part8')
    b['f_part9_1217_U_ad_1']=addr(P(b['f_part8_1216_U_r_1'])).cur
    section('part10')
