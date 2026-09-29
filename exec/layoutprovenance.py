"""Stage-generated layout provenance framing, in ordinary model actions.

This is transport evidence about skipped syntax, not native ABI classification.
The storage policy describes this compiler; nativeabi must still certify it.
"""
MAGIC = b'USLFACT1\n'
RESOURCE = b'\0library/sourcefacts'
POLICY = 1


def decoder(E, P, prefix, stage, end, ready, fail):
    """At current cursor, read an exact frame bounded by register `end`.

    Return status, payload begin and payload length registers. No input frame
    push or output occurs; caller chooses a whole-input SWAP or unit copying.
    """
    s = prefix
    status, begin, size = s+'_status', s+'_begin', s+'_size'
    P(s+'.bound').a(('MARK',s+'_at')).branch({2:fail},s+'.headerbound',
                                          [('C64U',s+'_at',end)])
    P(s+'.headerbound').a(('A64','sub',s+'_left',end,s+'_at'),
                         ('LDI',s+'_header',20)).branch({0:fail},s+'.magic0',
                                                     [('C64U',s+'_left',s+'_header')])
    for i, byte in enumerate(MAGIC + bytes([stage])):
        P(s+'.magic'+str(i)).branch({byte:s+'.adv'+str(i)}, fail,
                                   [('BYTE',s+'_byte'),('RLD',s+'_byte')])
        P(s+'.adv'+str(i)).a(('ADV',)).goto(s+'.magic'+str(i+1) if i+1<len(MAGIC)+1 else s+'.status')
    P(s+'.status').branch({(0,1):s+'.policy'}, fail,
                         [('BYTE',status),('ADV',),('RLD',status)])
    P(s+'.policy').branch({POLICY:s+'.wordbound'}, fail,
                         [('BYTE',s+'_byte'),('ADV',),('RLD',s+'_byte')])
    P(s+'.wordbound').a(('MARK',s+'_at'),('A64','sub',s+'_left',end,s+'_at'),
                        ('LDI',s+'_eight',8)).branch({0:fail},s+'.word',
                                                  [('C64U',s+'_left',s+'_eight')])
    P(s+'.word').a(('LDI',size,0),*[a for i in range(8) for a in
        [('BYTE',s+'_byte'),('A64I','shl',s+'_byte',s+'_byte',8*i),
         ('A64','or',size,size,s+'_byte'),('ADV',)]]).goto(s+'.extent')
    # Subtraction precedes comparison: do not accept overflowed begin+size.
    P(s+'.extent').a(('MARK',begin)).branch({2:fail},s+'.sizebound',
                                         [('C64U',begin,end)])
    P(s+'.sizebound').a(('A64','sub',s+'_left',end,begin)).branch(
        {1:ready},fail,[('C64U',size,s+'_left')])
    return s+'.bound', status, begin, size


def writer(E, P, prefix, stage, status, payload, ready):
    """Write an owned payload blob without changing its bytes or attributes."""
    p=P(prefix).o(MAGIC.decode('ascii')).a(('LDI',prefix+'_stage',stage),
        ('OUTW',prefix+'_stage'),('OUTW',status),('LDI',prefix+'_policy',POLICY),
        ('OUTW',prefix+'_policy'),('BLEN',prefix+'_len',payload))
    for i in range(8):
        p.a(('A64I','shr',prefix+'_byte',prefix+'_len',8*i),
            ('ALUI','and',prefix+'_byte',prefix+'_byte',255),('OUTW',prefix+'_byte'))
    p.a(('INPUSH',payload),('LDI',prefix+'_zero',0),
        ('SPAN2',prefix+'_zero',prefix+'_len'),('INPOP',)).goto(ready)


def parser(E, P, start):
    from modelinput import u64
    u64(E,'SF3.resource',RESOURCE,'sf3_flag','sf3_present','SF3.fail')
    P('SF3.start').a(('LDI','sf3_status',0)).call('SF3.resource').branch(
        {1:start},'SF3.enabled',[('CMPI','sf3_present',0)])
    P('SF3.enabled').branch({1:'SF3.open'},'SF3.fail',[('RLD','sf3_flag')])
    P('SF3.fail').a(E.rej('not covered: malformed source layout provenance')).goto('DEAD')
    entry,status,begin,size=decoder(E,P,'SF3.frame',1,'sf3_end','SF3.payload','SF3.fail')
    P('SF3.open').a(('XLEN','sf3_end')).goto(entry)
    P('SF3.payload').a(('COPYW','sf3_status',status),('SPAN2',begin,'sf3_end'),('SWAP',)).goto(start)
    return 'SF3.start'


def units(E, P, locations=False):
    """Validate every unit before name isolation and conservatively merge proof.

    Whole-input prepass retains filename framing and old token payloads. A
    single unknown unit makes the merged source unknown, preventing provenance
    from leaking from one TU to another. Defaults keep the old wire untouched.
    """
    from modelinput import u64
    g=E.g
    u64(E,'SFU.resource',RESOURCE,'sfu_flag','sfu_present','SFU.fail')
    P('SFU.start').call('SFU.resource').branch({1:'START'},'SFU.enabled',[('CMPI','sfu_present',0)])
    P('SFU.enabled').branch({1:'SFU.begin'},'SFU.fail',[('RLD','sfu_flag')])
    P('SFU.fail').a(E.rej('not covered: malformed unit source layout provenance')).goto('DEAD')
    P('SFU.begin').a(('LDI','sfu_status',1),('LDI','sfu_count',0),('XLEN','sfu_total')).goto('SFU.loop')
    P('SFU.loop').a(('MARK','sfu_at')).branch({1:'SFU.finish'},'SFU.bound',[('C64U','sfu_at','sfu_total')])
    P('SFU.bound').a(('A64','sub','sfu_left','sfu_total','sfu_at'),('LDI','sfu_four',4)).branch({0:'SFU.fail'},'SFU.length',[('C64U','sfu_left','sfu_four')])
    def word(name, register, ready):
        P(name).a(('LDI',register,0),*[a for i in range(4) for a in
          [('BYTE','sfu_byte'),('A64I','shl','sfu_byte','sfu_byte',8*i),
           ('A64','or',register,register,'sfu_byte'),('ADV',)]]).goto(ready)
    word('SFU.length','sfu_len','SFU.extent')
    P('SFU.extent').a(('MARK','sfu_framebegin'),('A64','sub','sfu_left','sfu_total','sfu_framebegin')).branch({2:'SFU.fail'},'SFU.frame',[('C64U','sfu_len','sfu_left')])
    P('SFU.frame').a(('A64','add','sfu_frameend','sfu_framebegin','sfu_len'),('LDI','sfu_prefixlen',0)).goto('SFU.namebound' if locations else 'SFU.decode')
    if locations:
        P('SFU.namebound').branch({0:'SFU.fail'},'SFU.namelen',[('C64U','sfu_len','sfu_four')])
        word('SFU.namelen','sfu_namelen','SFU.nameextent')
        P('SFU.nameextent').a(('MARK','sfu_namebegin'),('A64','sub','sfu_left','sfu_frameend','sfu_namebegin')).branch({2:'SFU.fail'},'SFU.name',[('C64U','sfu_namelen','sfu_left')])
        P('SFU.name').a(('A64','add','sfu_namend','sfu_namebegin','sfu_namelen'),('JUMP','sfu_namend'),('ALUI','add','sfu_prefixlen','sfu_namelen',4)).goto('SFU.decode')
    entry,status,begin,size=decoder(E,P,'SFU.frameproof',1,'sfu_frameend','SFU.copy','SFU.fail')
    P('SFU.decode').goto(entry)
    p=P('SFU.copy').a(('ALU','and','sfu_status','sfu_status',status),('A64','add','sfu_newlen',size,'sfu_prefixlen'))
    for i in range(4):p.a(('A64I','shr','sfu_byte','sfu_newlen',i*8),('ALUI','and','sfu_byte','sfu_byte',255),('OUTW','sfu_byte'))
    if locations:p.a(('SPAN2','sfu_framebegin','sfu_namend'))
    p.a(('SPAN2',begin,'sfu_frameend'),('JUMP','sfu_frameend'),('ALUI','add','sfu_count','sfu_count',1)).goto('SFU.loop')
    P('SFU.finish').branch({1:'SFU.fail'},'SFU.swap',[('CMPI','sfu_count',0)])
    P('SFU.swap').a(('SWAP',)).goto('START')
    # Wrap final output once, regardless of plain or located unit output.
    terminal='SFU.accept'
    P(terminal).a(('LDI','sfu_zero',0),('OCUT','sfu_payload','sfu_zero')).goto('SFU.write')
    writer(E,P,'SFU.write',1,'sfu_status','sfu_payload','SFU.done')
    P('SFU.done').a(('ACCEPT',)).goto('DEAD')
    accept_rows=[]
    for name,(mode,row) in list(g.st.items()):
        if name.startswith('SFU.'):continue
        for key,(target,q) in list(row.items()):
            acts=list(g.seqs[q])
            if any(a[0]=='ACCEPT' for a in acts):accept_rows.append((name,key,target,acts))
    for index,(name,key,target,acts) in enumerate(accept_rows):
        assert acts[-1]==('ACCEPT',)
        gate='SFU.acceptgate'+str(index)
        P(gate).branch({1:terminal},'SFU.oldaccept',[('CMPI','sfu_flag',1)])
        g.st[name][1][key]=(gate,g.seq(acts[:-1]))
    P('SFU.oldaccept').a(('ACCEPT',)).goto('DEAD')
    return 'SFU.start'
