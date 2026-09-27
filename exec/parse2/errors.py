"""Located parser errors and top-level balanced recovery, as reference unit().
Only mapped language errors are recovered; unsupported prototype constructs
keep their explicit reason, gain a position, and stop without recovery. Recovery discards the failed continuation and
scope, never its emitted tape (a nonzero error count prevents publication).
"""
from tokenlocations import TOKEN_POS
NAMES=50<<40


def install(E,P,warnings=False):
    g=E.g
    import json
    from pathlib import Path
    from finite_rules import install as install_rules, load as load_rules
    root=Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root/('errors-'+suffix+'.tsv')).read_text().splitlines()[1:]]
    messages={reason:(json.loads(message),bool(int(identifier))) for reason,message,identifier in rows('messages')}
    bindings=dict(NAMES=NAMES,TOKEN_POS=TOKEN_POS,recorded='WU.token' if warnings else 'RET')
    sequences={name:E.O(json.loads(value)) for name,value in rows('text')}
    positions=load_rules(root/'errors-position.tsv',{},domain=(0,1),bindings=bindings)['position']
    # Locate prototype rejects; only mapped reference diagnostics recover.
    # Valid parsing and the branch that classifies each error stay unchanged.
    mapped={}
    for name,(mode,row) in list(g.st.items()):
        if name == 'DL.bad': continue  # malformed transport cannot locate its own error
        for key,(nxt,seq) in list(row.items()):
            acts=list(g.seqs[seq])
            if acts and acts[-1][0]=='REJECT' and acts[-1][1].startswith('not covered: '):
                reason=acts[-1][1]
                message,identifier=messages.get(reason,(reason,False))
                recover=reason in messages
                info=(message,identifier,recover)
                tag=mapped.setdefault(info,'ER.message'+str(len(mapped)))
                row[key]=(tag,g.seq(acts[:-1]))
    for (message,identifier,recover),tag in mapped.items():
        sequences.update(position=positions[int(identifier)][1],message=[('SBOUT',c) for c in message.encode()])
        install_rules(g,root,'errors',bindings=dict(bindings,entry=tag,recover=int(recover)),sequences=sequences,section='message')
    p=P('errors.bindings')
    for owner,kind,key in rows('fresh'):
        p.cur=owner;bindings[key]=p.fresh(kind)
    tokens=dict(E.TK,identifier=E.TK_ID)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    for name in ('UNIT','START','END'):
        g.st[name+'.original']=g.st.pop(name)
    install_rules(g,root,'errors',bindings=bindings,sequences=sequences,classes=classes,section='main')
    # Clear the actual return alphabet after all continuation labels exist.
    for state,row in load_rules(root/'errors-stack.tsv',{},domain=sorted(g.labels)+['BOT']).items():
        g.st[state]=['t',{key:(target,g.seq(acts)) for key,(target,acts) in row.items()}]
    # Track every input view in the complete graph, including the renderer.
    for _,row in g.st.values():
        for key,(nxt,seq) in list(row.items()):
            acts=[]
            for a in g.seqs[seq]:
                acts.append(a)
                if a[0] in ('INPUSH','INPUSHX','INPUSHXE'):acts.append(('ALUI','add','er_depth','er_depth',1))
                elif a[0]=='INPOP':acts.append(('ALUI','sub','er_depth','er_depth',1))
            row[key]=(nxt,g.seq(acts))
