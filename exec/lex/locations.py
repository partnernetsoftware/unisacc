"""Model framing for pp.locations -> tokens.locations (no host parser).
The complete preprocessing envelope is retained before the positioned tokens.
Offsets stay relative to its text. All lengths fit the executor's int cursors.
"""


def install(d):
    bad = [('REJECT', 'not covered: malformed preprocessing locations')]

    def row(name, mode, entries):
        r = d.state(name, mode)
        for key, (nxt, acts) in entries.items():
            r[key] = (nxt, d.seq(acts))

    def go(name, nxt, acts=()):
        row(name, 'b', {i: (nxt, list(acts)) for i in range(257)})

    def test(name, acts, ok, keys=(0, 1)):
        go(name, name+'.r', acts)
        row(name+'.r', 'r', {i: (ok, []) if i in keys else ('HALT', bad) for i in range(3)})

    def word(name, reg, nxt):
        go(name, name+'.0', [('LDI', reg, 0)])
        for k in range(4):
            row(name+'.'+str(k), 'b', {
                c: (name+'.'+str(k+1) if k<3 else nxt,
                    [('ALUI','add',reg,reg,c << (8*k)),('ADV',)])
                if c < (128 if k==3 else 256) else ('HALT',bad)
                for c in range(257)})

    def outword(reg):
        return sum(([('ALUI','sar','loc_byte',reg,s),('OUTW','loc_byte')]
                    for s in (0,8,16,24)), [])

    magic=b'UNIPP1\0'
    for k,c in enumerate(magic):
        row('LOC.magic'+str(k),'b', {b: ('LOC.magic'+str(k+1) if k+1<len(magic) else 'LOC.length', [('ADV',)])
                    if b==c else ('HALT',bad) for b in range(257)})
    fields=['textlen','forced','auto','splices','includes']
    for i,f in enumerate(fields):
        word('LOC.length' if i==0 else 'LOC.'+f,'loc_'+f,
             'LOC.'+fields[i+1] if i+1<len(fields) else 'LOC.splicecheck')
    test('LOC.splicecheck',[('CMPI','loc_splices',0)],'LOC.includecheck',keys=(1,))
    # Nonzero counts consume one fixed-width record before returning.
    r=d.states['LOC.splicecheck.r'][1]
    r[2]=('LOC.splice',d.seq([]))
    word('LOC.splice','loc_offset','LOC.splicebound')
    test('LOC.splicebound',[('CMP','loc_offset','loc_textlen')],'LOC.splicenext')
    go('LOC.splicenext','LOC.splicecheck',[('ALUI','sub','loc_splices','loc_splices',1)])
    test('LOC.includecheck',[('CMPI','loc_includes',0)],'LOC.textbound',keys=(1,))
    d.states['LOC.includecheck.r'][1][2]=('LOC.line',d.seq([]))
    word('LOC.line','loc_line','LOC.lines')
    word('LOC.lines','loc_lines','LOC.namelen')
    word('LOC.namelen','loc_namelen','LOC.namebound')
    test('LOC.namebound',[('CMPI','loc_namelen',62)],'LOC.nameextent')
    test('LOC.nameextent',[('MARK','loc_at'),('XLEN','loc_total'),
                         ('ALU','sub','loc_left','loc_total','loc_at'),
                         ('CMP','loc_namelen','loc_left')],'LOC.nameskip')
    go('LOC.nameskip','LOC.includecheck',[
        ('ALU','add','loc_at','loc_at','loc_namelen'),('JUMP','loc_at'),
        ('ALUI','sub','loc_includes','loc_includes',1)])
    test('LOC.textbound',[('MARK','loc_at'),('XLEN','loc_total'),
                         ('ALU','sub','loc_left','loc_total','loc_at'),
                         ('CMP','loc_textlen','loc_left')],'LOC.emit',keys=(1,))
    go('LOC.emit','DISPATCH', [('LDI','loc_zero',0)] +
       [('OUT',b) for b in b'UNITOK1\0'] + outword('loc_total') +
       [('SPAN2','loc_zero','loc_total'),('BLOBSAVE','loc_text','loc_at','loc_total'),('INPUSH','loc_text')])
    return 'LOC.magic0'
