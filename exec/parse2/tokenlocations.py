"""Optional located-token reader and retained diagnostic map.
Parsing stays on the original token buffer, so rewinds and bounded input
views keep their existing offsets. source_pos is a preprocessed-text offset;
TOKEN_POS preserves it for a saved token-buffer position after later reads.
"""
TOKEN_POS=34<<40
SPLICES=35<<40
INCLUDE_LINE=36<<40
INCLUDE_LINES=37<<40
INCLUDE_NAME=38<<40


def install(E,P,ordinal_table=None):
    g=E.g
    bad='DL.bad'
    def word(name,reg,nxt):
        P(name).a(('LDI',reg,0)).goto(name+'.0')
        for k in range(4):
            for c in range(128 if k==3 else 256):
                g.on(name+'.'+str(k),[c],name+'.'+str(k+1) if k<3 else nxt,
                     [('ALUI','add',reg,reg,c<<(8*k)),('ADV',)])
            g.els(name+'.'+str(k),bad)
    def magic(name,value,nxt):
        for k,c in enumerate(value):
            g.on(name+str(k),[c],name+str(k+1) if k+1<len(value) else nxt,[('ADV',)])
            g.els(name+str(k),bad)
    def le(p,left,right,nxt):
        p.branch({(0,1):nxt},bad,[('CMP',left,right)])
    magic('DL.magic',b'UNITOK1\0','DL.length')
    word('DL.length','diag_pp_len','DL.extent')
    p=P('DL.extent').a(('MARK','diag_pp_begin'),('XLEN','diag_total'),
                       ('ALU','sub','diag_left','diag_total','diag_pp_begin'))
    le(p,'diag_pp_len','diag_left','DL.bound')
    P('DL.bound').a(('ALU','add','diag_pp_end','diag_pp_begin','diag_pp_len'),
                     ('INPUSHXE','diag_pp_begin','diag_pp_end')).goto('DL.pp0')
    magic('DL.pp',b'UNIPP1\0','DL.textlen')
    fields=['textlen','forced','auto','nsplice','ninclude']
    for k,f in enumerate(fields):
        word('DL.'+f,'diag_'+f,'DL.'+fields[k+1] if k+1<len(fields) else 'DL.splice0')
    P('DL.splice0').a(('LDI','diag_i',0)).goto('DL.splicecheck')
    P('DL.splicecheck').branch({0:'DL.splice'},'DL.include0',[('CMP','diag_i','diag_nsplice')])
    word('DL.splice','diag_v','DL.splicebound')
    le(P('DL.splicebound'),'diag_v','diag_textlen','DL.splicestore')
    P('DL.splicestore').a(('STX','diag_i',SPLICES,'diag_v'),('ALUI','add','diag_i','diag_i',1)).goto('DL.splicecheck')
    P('DL.include0').a(('LDI','diag_i',0)).goto('DL.includecheck')
    P('DL.includecheck').branch({0:'DL.line'},'DL.textbound',[('CMP','diag_i','diag_ninclude')])
    word('DL.line','diag_line','DL.lines')
    word('DL.lines','diag_lines','DL.namelen')
    word('DL.namelen','diag_namelen','DL.namebound')
    P('DL.namebound').branch({(0,1):'DL.nameextent'},bad,[('CMPI','diag_namelen',62)])
    p=P('DL.nameextent').a(('MARK','diag_at'),('ALU','sub','diag_left','diag_pp_end','diag_at'))
    le(p,'diag_namelen','diag_left','DL.namestore')
    P('DL.namestore').a(('ALU','add','diag_end','diag_at','diag_namelen'),
        ('BLOBSAVE','diag_name','diag_at','diag_end'),('JUMP','diag_end'),
        ('STX','diag_i',INCLUDE_LINE,'diag_line'),('STX','diag_i',INCLUDE_LINES,'diag_lines'),
        ('STX','diag_i',INCLUDE_NAME,'diag_name'),('ALUI','add','diag_i','diag_i',1)).goto('DL.includecheck')
    P('DL.textbound').a(('MARK','diag_at'),('ALU','sub','diag_left','diag_pp_end','diag_at')).branch(
        {1:'DL.ready'},bad,[('CMP','diag_left','diag_textlen')])
    P('DL.ready').a(('BLOBSAVE','diag_source','diag_at','diag_pp_end'),
        ('INPOP',),('JUMP','diag_pp_end')).goto('START')
    # Every parser rewind points to this prefix, not to the token-kind line.
    del g.st['NEXT']
    P('NEXT').a(('MARK','tpos')).goto('DL.prefix')
    g.on('DL.prefix',[64],'DL.offset',[('ADV',)])
    g.els('DL.prefix',bad)
    word('DL.offset','source_pos','DL.offsetbound')
    le(P('DL.offsetbound'),'source_pos','diag_textlen','DL.newline')
    # Preserve the ordinary reader's stable token ordinals used by static
    # storage. Located prefixes change byte offsets, not source ordinals.
    acts=[('ADV',),('STX','tpos',TOKEN_POS,'source_pos')]
    if ordinal_table is not None: acts.append(('LDX','ixn','tpos',ordinal_table))
    g.on('DL.newline',[10],'IX.have' if ordinal_table is not None else 'NX',acts)
    g.els('DL.newline',bad)
    g.on(bad,range(257),'DEAD',E.rej('not covered: malformed token locations'))
    return 'DL.magic0'
