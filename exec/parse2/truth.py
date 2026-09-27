"""Bind scalar condition/conversion declarations to current type and opcode facts."""
import json
from pathlib import Path
from finite_rules import install as install_rules

ROOT = Path(__file__).parent


def rows(name):
    return [line.split('\t') for line in (ROOT / ('scalar-' + name + '.tsv')).read_text().splitlines()[1:]]


def rules(E, section, bindings=None, sequences=None):
    install_rules(E.g, ROOT, 'scalar', bindings=bindings, sequences=sequences, section=section)


def classify(E, P, DBL, FLT, entry, base, floating, double, single, integer, pointer):
    b = dict(entry=entry, base=base, float=floating, double=double, single=single,
             integer=integer, pointer=pointer, DBL=DBL, FLT=FLT)
    b.update((key + '_test', P(b[key]).fresh('b')) for key in ('entry', 'base', 'float'))
    rules(E, 'classify', b)


def outputs(E, group):
    texts = {name:json.loads(text) for name,text in rows('text')}
    for selected,entry,template,argument,result in rows('outputs'):
        if selected == group:
            text = texts[template] if argument == '-' else texts[template] % argument
            rules(E, 'output', dict(entry=entry), dict(output=E.O(text), result=json.loads(result)))


def install(E, P, DBL, FLT):
    classify(E, P, DBL, FLT, 'FTRUTH', 'FT.kind', 'FT.single', 'FT.double', 'FT.float', 'RET', 'RET')
    outputs(E, 'truth')
    classify(E, P, DBL, FLT, 'FNOT', 'FN.kind', 'FN.single', 'FN.double', 'FN.float', 'FN.integer', 'FN.integer')
    outputs(E, 'not')


def conversions(E, P, DBL, FLT, unsigned_wide, fpu):
    paths = {(source,target):ops.split(',') for source,target,ops in rows('conversions')}
    text = dict((name,json.loads(value)) for name,value in rows('text'))['convert']
    for suffix in ('d', 's', 'i', 'u'):
        cv = 'TO.' + suffix
        classify(E, P, DBL, FLT, cv, cv+'.base', cv+'.float', cv+'.d', cv+'.s', cv+'.int', cv+'.u')
        rules(E, 'unsigned', dict(entry=cv+'.int', test=P(cv+'.int').fresh('b'),
              UNSIGNED_WIDE=unsigned_wide, unsigned=cv+'.u', integer=cv+'.i'))
        for source in ('d', 's', 'i', 'u'):
            output = E.O(''.join(text % fpu[op] for op in paths.get((source,suffix), [])))
            rules(E, 'output', dict(entry=cv+'.'+source), dict(output=output, result=[]))
