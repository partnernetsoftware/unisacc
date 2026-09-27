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
UNIT_MAP=48<<40
MAP_STRIDE=1<<26
MAP_FIELDS=('source','textlen','forced','auto','nsplice','ninclude','filename')


def install(E,P,ordinal_table=None,token_record=None,ready="START",multi=True):
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
    magic('DL.magic',b'UNITOK','DL.version')
    g.on('DL.version',[49],'DL.singlezero',[('ADV',)])
    g.on('DL.singlezero',[0],'DL.single',[('ADV',)])
    g.els('DL.singlezero',bad)
    P('DL.single').a(('LDI','diag_multi',0),('LDI','diag_mapbase',0),('LDI','diag_filename',1)).goto('DL.length')
    if multi:
        g.on('DL.version',[50],'DL.multizero',[('ADV',)])
        g.on('DL.multizero',[0],'DL.count',[('ADV',),('LDI','diag_multi',1)])
        g.els('DL.multizero',bad)
        word('DL.count','diag_count','DL.countcheck')
        P('DL.countcheck').branch({1:bad},'DL.countmax',[('CMPI','diag_count',0)])
        P('DL.countmax').branch({2:bad},'DL.multistart',[('CMPI','diag_count',64)])
        P('DL.multistart').a(('LDI','diag_unit',0)).goto('DL.multinext')
        P('DL.multinext').a(('ALUI','mul','diag_mapbase','diag_unit',MAP_STRIDE)).goto('DL.filelen')
        word('DL.filelen','diag_filelen','DL.filebound')
        P('DL.filebound').a(('MARK','diag_at'),('XLEN','diag_total'),('ALU','sub','diag_left','diag_total','diag_at')).branch({1:bad},'DL.fileextent',[('CMPI','diag_filelen',0)])
        le(P('DL.fileextent'),'diag_filelen','diag_left','DL.filecopy')
        P('DL.filecopy').a(('ALU','add','diag_end','diag_at','diag_filelen'),('BLOBSAVE','diag_filename','diag_at','diag_end'),('JUMP','diag_end')).goto('DL.length')
    g.els('DL.version',bad)
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
    P('DL.splice0').branch({(1,2):bad},'DL.includecap',[('CMPI','diag_nsplice',MAP_STRIDE)])
    P('DL.includecap').branch({(1,2):bad},'DL.spliceinit',[('CMPI','diag_ninclude',MAP_STRIDE)])
    P('DL.spliceinit').a(('LDI','diag_i',0)).goto('DL.splicecheck')
    P('DL.splicecheck').branch({0:'DL.splice'},'DL.include0',[('CMP','diag_i','diag_nsplice')])
    word('DL.splice','diag_v','DL.splicebound')
    le(P('DL.splicebound'),'diag_v','diag_textlen','DL.splicestore')
    P('DL.splicestore').a(('ALU','add','diag_slot','diag_mapbase','diag_i'),('STX','diag_slot',SPLICES,'diag_v'),('ALUI','add','diag_i','diag_i',1)).goto('DL.splicecheck')
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
        ('ALU','add','diag_slot','diag_mapbase','diag_i'),
        ('STX','diag_slot',INCLUDE_LINE,'diag_line'),('STX','diag_slot',INCLUDE_LINES,'diag_lines'),
        ('STX','diag_slot',INCLUDE_NAME,'diag_name'),('ALUI','add','diag_i','diag_i',1)).goto('DL.includecheck')
    P('DL.textbound').a(('MARK','diag_at'),('ALU','sub','diag_left','diag_pp_end','diag_at')).branch(
        {1:'DL.context'},bad,[('CMP','diag_left','diag_textlen')])
    P('DL.context').branch({1:'DL.ready'},'DL.multiready' if multi else bad,[('CMPI','diag_multi',0)])
    P('DL.ready').a(('BLOBSAVE','diag_source','diag_at','diag_pp_end'),
        ('INPOP',),('JUMP','diag_pp_end')).goto(ready)
    if multi:
        p=P('DL.multiready').a(('BLOBSAVE','diag_source','diag_at','diag_pp_end'),('INPOP',),('JUMP','diag_pp_end'),('ALUI','mul','diag_record','diag_unit',8))
        for k,f in enumerate(MAP_FIELDS):p.a(('ALUI','add','diag_slot','diag_record',k),('STX','diag_slot',UNIT_MAP,'diag_'+f))
        p.a(('ALUI','add','diag_unit','diag_unit',1)).branch({0:'DL.multinext'},ready,[('CMP','diag_unit','diag_count')])
    # Every parser rewind points to this prefix, not to the token-kind line.
    del g.st['NEXT']
    if token_record: P('NEXT').call('DL.read').call(token_record).ret()
    P('DL.read' if token_record else 'NEXT').a(('MARK','tpos')).call('DL.prefix').goto('IX.have' if ordinal_table is not None else 'NX')
    g.on('DL.prefix',[64],'DL.which' if multi else 'DL.offset',[('ADV',)])
    if multi:
        P('DL.which').branch({1:'DL.offset'},'DL.tokenunit',[('CMPI','diag_multi',0)])
        word('DL.tokenunit','diag_unit','DL.unitbound')
        P('DL.unitbound').branch({0:'DL.unitload'},bad,[('CMP','diag_unit','diag_count')])
        p=P('DL.unitload').a(('ALUI','mul','diag_record','diag_unit',8),('ALUI','mul','diag_mapbase','diag_unit',MAP_STRIDE))
        for k,f in enumerate(MAP_FIELDS):p.a(('ALUI','add','diag_slot','diag_record',k),('LDX','diag_'+f,'diag_slot',UNIT_MAP))
        p.goto('DL.offset')
    g.els('DL.prefix',bad)
    word('DL.offset','source_pos','DL.offsetbound')
    le(P('DL.offsetbound'),'source_pos','diag_textlen','DL.newline')
    # Preserve the ordinary reader's stable token ordinals used by static
    # storage. Located prefixes change byte offsets, not source ordinals.
    acts=[('ADV',),('STX','tpos',TOKEN_POS,'source_pos')]
    if ordinal_table is not None: acts.append(('LDX','ixn','tpos',ordinal_table))
    g.on('DL.newline',[10],'RET',acts)
    g.els('DL.newline',bad)
    g.on(bad,range(257),'DEAD',E.rej('not covered: malformed token locations'))
    return 'DL.magic0'
