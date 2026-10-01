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
        mode,row=g.st[tag]
        assert mode=='r' and len(row)==257
        if message=='unknown identifier':
            original=tag+'.ordinary'
            g.st[original]=(mode,{key:(nxt,g.seq([('LDI','uc_kind',0),*g.seqs[seq]])) for key,(nxt,seq) in row.items()})
            del g.st[tag]
            P(tag).a(('SBCLR',),*(('SBOUT',c) for c in b'_Complex'),('SBINTERN','uc_expected'),
                     ('INTERN','uc_actual','ips','ipe')).branch({1:'UC.complex'},original,
                                                               [('CMP','uc_actual','uc_expected')])
            P('UC.complex').a(('LDI','uc_kind',2),('LDX','er_at','ips',NAMES),('LDI','er_recover',0),
                              ('SBCLR',),*(('SBOUT',c) for c in b'not covered: complex types (C99 6.2.5p11 _Complex is not supported)'),
                              ('SBSAVE','diag_message')).goto('ER.frames')
        else:
            g.st[tag]=(mode,{key:(nxt,g.seq([('LDI','uc_kind',0),*g.seqs[seq]])) for key,(nxt,seq) in row.items()})
    p=P('errors.bindings')
    for owner,kind,key in rows('fresh'):
        p.cur=owner;bindings[key]=p.fresh(kind)
    # The located renderer has already resolved the include/continuation map
    # when it reaches DP.emit.  Reuse those coordinates for the first machine
    # readable coverage record, then let the ordinary human diagnostic run.
    g.st['DP.emit.original']=g.st.pop('DP.emit')
    P('DP.emit').branch({1:'UC.frame.start',2:'UC.frame.start'},'DP.emit.original',
                        [('RLD','uc_kind')])
    P('UC.frame.start').a(('OSEL',1)).o('UNCOVERED\te3\t-\t-\t').a(
        ('INPUSH','dp_file'),('BLEN','dp_len','dp_file'),('SPAN2','dp_zero','dp_len'),('INPOP',)
    ).o(':').a(('COPYW','dp_num','dp_line')).call('DP.number').o(':').a(
        ('COPYW','dp_num','dp_col')).call('DP.number').o('\t').branch(
            {1:'UC.frame.struct',2:'UC.frame.complex'},'DP.emit.original',
            [('RLD','uc_kind')])
    P('UC.frame.struct').o('expr.call.ret-struct\tstruct return expression outside local lvalue\n').goto('DP.emit.original')
    P('UC.frame.complex').o('type.complex\tcomplex types (C99 6.2.5p11 _Complex is not supported)\n').goto('DP.emit.original')
    tokens=dict(E.TK,identifier=E.TK_ID)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    for name in ('UNIT','START','END'):
        g.st[name+'.original']=g.st.pop(name)
    install_rules(g,root,'errors',bindings=bindings,sequences=sequences,classes=classes,section='main')
    mode,row=g.st['START']
    g.st['START']=(mode,{key:(nxt,g.seq([('LDI','uc_kind',0),*g.seqs[seq]])) for key,(nxt,seq) in row.items()})
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
