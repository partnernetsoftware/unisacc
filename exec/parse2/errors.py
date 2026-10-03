"""Located parser errors and top-level balanced recovery, as reference unit().
Only mapped language errors are recovered; unsupported prototype constructs
keep their explicit reason, gain a position, and stop without recovery. Recovery discards the failed continuation and
scope, never its emitted tape (a nonzero error count prevents publication).
"""
from tokenlocations import TOKEN_POS
import sys as _s,pathlib as _p
_s.path.insert(0,str(_p.Path(__file__).parents[1]/'facts'))
from load import facts as _facts
_t=lambda v:tuple(map(_t,v)) if isinstance(v,list) else v
globals().update((_r['name'],_t(_r['value'])) for _r in _facts('errors'))


def install(E,P,warnings=False):
    g=E.g
    import json
    from pathlib import Path
    from finite_rules import install as install_rules, install_rows, install_template, load as load_rules
    root=Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root/('errors-'+suffix+'.tsv')).read_text().splitlines()[1:]]
    messages={reason:(json.loads(message),bool(int(identifier))) for reason,message,identifier in rows('messages')}
    bindings=dict(NAMES=NAMES,TOKEN_POS=TOKEN_POS,recorded='WU.token' if warnings else 'RET')
    sequences={name:E.O(json.loads(value)) for name,value in rows('text')}
    holder=P('errors.fresh')  # unregistered: only names fresh labels
    def fresh(kind):  # KIND is OWNER_H: label OWNER.H<n>
        holder.cur,h=kind.split('_');return holder.fresh(h)
    def declare(section,**facts):
        install_template(g,root,'errors',facts,fresh,bindings=bindings,section=section)
    positions=load_rules(root/'errors-position.tsv',{},domain=(0,1),bindings=bindings)['position']
    # Locate prototype rejects; only mapped reference diagnostics recover.
    # Valid parsing and the branch that classifies each error stay unchanged.
    # One tag per distinct reason (messages are distinct per reason).
    groups=install_template(g,root,'errors',{},fresh,bindings=bindings,section='reasons')
    mapped={(*messages.get(reason,(reason,False)),reason in messages):tag for reason,tag in groups.items()}
    for (message,identifier,recover),tag in mapped.items():
        sequences.update(position=positions[int(identifier)][1],message=[('SBOUT',c) for c in message.encode()])
        install_rules(g,root,'errors',bindings=dict(bindings,entry=tag,recover=int(recover)),sequences=sequences,section='message')
        mode,row=g.st[tag]
        assert mode=='r' and len(row)==257
        # 'unknown identifier' first tests for _Complex; every message clears uc_kind.
        if message=='unknown identifier':
            declare('unknown.move',tag=[tag]);declare('unknown',tag=[tag])
        else:
            declare('plain',tag=[tag])
    p=P('errors.bindings')
    for owner,kind,key in rows('fresh'):
        p.cur=owner;bindings[key]=p.fresh(kind)
    # The located renderer has already resolved the include/continuation map
    # when it reaches DP.emit.  Reuse those coordinates for the first machine
    # readable coverage record, then let the ordinary human diagnostic run.
    declare('emit.move')
    declare('emit')  # also moves UNIT/START/END to *.original
    tokens=dict(E.TK,identifier=E.TK_ID)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    install_rules(g,root,'errors',bindings=bindings,sequences=sequences,classes=classes,section='main')
    declare('start')
    # Clear the actual return alphabet after all continuation labels exist.
    stack=load_rules(root/'errors-stack.tsv',{},domain=sorted(g.labels)+['BOT'])
    assert not set(stack)&set(g.st)
    install_rows(g,root/'errors-stack.tsv',domain=sorted(g.labels)+['BOT'],mode='t')
    # Track every input view in the complete graph, including the renderer.
    declare('depth')
