"""Located parser errors and top-level balanced recovery, as reference unit().
Only mapped language errors are recovered; unsupported prototype constructs
keep their explicit reason, gain a position, and stop without recovery. Recovery discards the failed continuation and
scope, never its emitted tape (a nonzero error count prevents publication).
"""
from tokenlocations import TOKEN_POS
NAMES=50<<40


def install(E,P,warnings=False):
    g=E.g
    Replace=P.__bases__[0]  # explicit wrappers of existing, renamed entries
    messages={
        'not covered: identifier is not a local':('unknown identifier',True),
        'not covered: expression':('this is not the start of an expression',False),
    }
    for mark in (';',')','(',']','[','{','}',':'):
        messages['not covered: expected '+mark]=("expected '"+mark+"'",False)
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
        p=P(tag)
        if identifier:p.a(('LDX','er_at','ips',NAMES))
        else:p.a(('COPYW','er_at','tpos'))
        p.a(('LDI','er_recover',int(recover)),('SBCLR',),*[('SBOUT',c) for c in message.encode()],('SBSAVE','diag_message')).goto('ER.frames')

    def before(name,acts):
        g.st[name+'.original']=g.st.pop(name)
        Replace(name).a(*acts).goto(name+'.original')
    before('UNIT',[('COPYW','er_top','tpos'),('COPYW','er_usp','usp')])
    g.st['START.original']=g.st.pop('START')
    Replace('START').a(('LDI','er_count',0),('LDI','er_depth',0),('LDI','er_limit',20),('SBCLR',),
        *[('SBOUT',c) for c in b'\0cli/error-limit'],('SBFIND','er_flag'),('BLEN','er_len','er_flag')).branch(
        {1:'START.original'},'ER.limitstart',[('CMPI','er_len',0)])
    P('ER.limitstart').a(('INPUSH','er_flag'),('LDI','er_limit',0)).goto('ER.limitdigit')
    for c in range(10):g.on('ER.limitdigit',[48+c],'ER.limitdigit',[
        ('ALUI','mul','er_limit','er_limit',10),('ALUI','add','er_limit','er_limit',c),('ADV',)])
    g.els('ER.limitdigit','START.original',[('INPOP',)])
    P('ER.token').tok({E.TK_ID:'ER.name'},'ER.recorded')
    P('ER.name').a(('STX','ps',NAMES,'tpos')).goto('ER.recorded')
    P('ER.recorded').goto('WU.token' if warnings else 'RET')
    # Return to the root token buffer before positioning or recovering. The
    # counter below mirrors generic input-frame operations, not parser guesses.
    P('ER.frames').branch({1:'ER.position'},'ER.popframe',[('CMPI','er_depth',0)])
    P('ER.popframe').a(('INPOP',)).goto('ER.frames')
    P('ER.position').a(('JUMP','er_at'),('MARK','tpos')).call('DL.prefix').a(('LDX','diag_pos','er_at',TOKEN_POS),
        ('LDI','diag_warning',0)).call('DIAG.report').a(('ALUI','add','er_count','er_count',1)).branch(
        {(0,1):'ER.decide'},'ER.limitcheck',[('CMPI','er_limit',0)])
    P('ER.limitcheck').branch({0:'ER.decide'},'ER.limitstop',[('CMP','er_count','er_limit')])
    P('ER.decide').branch({1:'ER.clear'},'ER.summary',[('CMPI','er_recover',1)])
    P('ER.limitstop').a(('OSEL',1)).o('too many errors emitted, stopping now\n').a(('OSEL',0),('REJECT','')).goto('DEAD')
    # Scope cleanup uses the production undo records; no discarded function's
    # local or enum-shadow binding can leak into the next declaration.
    P('ER.unwind').a(('COPYW','sv','er_usp'),('LDI','vsp',0),('LDI','csp',0),
        ('LDI','si_active',0)).call('UNWIND').a(('JUMP','er_top'),('LDI','er_braces',0)).call('NEXT').goto('ER.scan')
    P('ER.scan').tok({'eof':'ER.summary','{':'ER.open','}':'ER.close',';':'ER.semicolon'},'ER.next')
    P('ER.open').a(('ALUI','add','er_braces','er_braces',1)).goto('ER.next')
    P('ER.close').a(('ALUI','sub','er_braces','er_braces',1)).branch({(0,1):'ER.afterblock'},'ER.next',[('CMPI','er_braces',0)])
    P('ER.afterblock').call('NEXT').tok({';':'ER.aftersemicolon'},'UNIT')
    P('ER.semicolon').branch({1:'ER.aftersemicolon'},'ER.next',[('CMPI','er_braces',0)])
    P('ER.aftersemicolon').call('NEXT').goto('UNIT')
    P('ER.next').call('NEXT').goto('ER.scan')
    g.st['END.original']=g.st.pop('END')
    Replace('END').branch({1:'END.original'},'ER.summary',[('CMPI','er_count',0)])
    P('ER.summary').a(('OSEL',1),('COPYW','dp_num','er_count')).call('DP.number').branch(
        {1:'ER.one'},'ER.many',[('CMPI','er_count',1)])
    P('ER.one').o(' error generated.\n').goto('ER.reject')
    P('ER.many').o(' errors generated.\n').goto('ER.reject')
    P('ER.reject').a(('OSEL',0),('REJECT','')).goto('DEAD')
    # All return labels have now been installed. The value stack has its own
    # pointer; the generic call stack is cleared explicitly using its alphabet.
    g.st['ER.clear']=['t',{lab:('ER.clear',g.seq([('POP',)])) for lab in sorted(g.labels)}]
    g.st['ER.clear'][1]['BOT']=('ER.unwind',g.seq([]))
    # Track every input view in the complete graph, including the renderer.
    for _,row in g.st.values():
        for key,(nxt,seq) in list(row.items()):
            acts=[]
            for a in g.seqs[seq]:
                acts.append(a)
                if a[0] in ('INPUSH','INPUSHX','INPUSHXE'):acts.append(('ALUI','add','er_depth','er_depth',1))
                elif a[0]=='INPOP':acts.append(('ALUI','sub','er_depth','er_depth',1))
            row[key]=(nxt,g.seq(acts))
