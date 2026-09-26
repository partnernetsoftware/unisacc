"""Block-scope static storage, using the shared scope/type/load/store procedures.
LOC encodes a static object's label as -(token ordinal + 1); positive slots
still mean frame offsets or GMARK. No language primitive is added to run.c.
"""

def install(E, P, TIX, SINIT, SIEND, LOC):
    g = E.g
    # A first token walk assigns source ordinals, including skipped qualifiers.
    # Cache by byte position: later rewinds must not allocate another ordinal.
    del g.st['NEXT']       # replace the reader entry, preserving NX and its lexer
    g.on('NEXT', range(257), 'IX.have', [('MARK','tpos'),('LDX','ixn','tpos',TIX)], 'r')
    P('IX.have').branch({1:'IX.new'}, 'NX', [('CMPI','ixn',0)])
    P('IX.new').a(('ALUI','add','ixcount','ixcount',1),('STX','tpos',TIX,'ixcount')).goto('NX')
    P('INDEX').call('NEXT').tok({'eof':'RET'},'INDEX')
    P('SC.start').call('NEXT').call('TSPEC').goto('SC.name')
    P('SC.name').tok({E.TK_ID:'SC.id'}, ('rej','not covered: static declarator'))
    P('SC.id').a(('COPYW','ips','ps'),('COPYW','ipe','pe'),('COPYW','si_pos','tpos'),
        ('LDX','si_lab','tpos',TIX),('ALUI','sub','si_lab','si_lab',1),
        ('LDI','prd',1),('LDI','drk',0)).call('NEXT').tok({'[':'SC.arr'},'SC.size')
    P('SC.arr').call('DIMS').goto('SC.size')
    P('SC.size').call('ELSZ').a(('ALU','mul','dsz','prd','es'),('COPYW','dar','drk'),
        ('COPYW','ps','ips'),('COPYW','pe','ipe')).call('BIND').a(
        ('LDI','s',-1),('ALU','sub','s','s','si_lab'),('STX','v',LOC,'s'),
        ('STX','v',E.BASE,'tb'),('STX','v',E.ARR,'dar'),('COPYW','t','td')).call('SC.desc').goto('SC.emit')
    P('SC.desc').branch({1:'DC.p'},'DC.a',[('CMPI','dar',0)])
    # Emit after the descriptor returns to SC.size's continuation.
    # P.call chains resume automatically; finish SC.size at a distinct state.
    P('SC.emit').o('.bss ls').num('si_lab').o(' ').a(('COPYW','si_bytes','dsz')).branch({0:'SC.min'},'SC.bytes',[('CMPI','si_bytes',8)])
    P('SC.min').a(('LDI','si_bytes',8)).goto('SC.bytes')
    P('SC.bytes').num('si_bytes').o('\n').goto('SC.after')
    P('SC.after').tok({'=':'SC.init',',':'SC.more'},'SC.end')
    P('SC.more').call('DSTARS').goto('SC.name')
    P('SC.end').expect(';').call('NEXT').ret()
    P('SC.init').branch({1:'SC.scalar'},('rej','not covered: static aggregate initializer'),[('CMPI','dar',0)])
    P('SC.scalar').a(('OLEN','si_out'),('LDI','si_active',1)).vpush('si_pos','si_lab','si_out','v','bd','tb').call('NEXT').call('EXPR').a(('LDI','si_active',0)).call('SC.nof32').call('ISDV').a(('COPYW','sdv','u')).vpop('si_pos','si_lab','si_out','v','bd','tb').a(
        ('LDX','vt','v',E.PTR),('LDX','vb','v',E.BASE)).call('SC.nof32').call('ISDV').branch({1:'SC.store'},'DEAD.dbl',[('CMP','u','sdv')])
    P('SC.store').o('  .lea r1, ls').num('si_lab').o('\n').call('STOREV').a(
        ('OCUT','si_blob','si_out'),('STX','si_pos',SINIT,'si_blob'),('STX','si_pos',SIEND,'tpos')).goto('SC.after')
    P('IN.static').a(('LDX','si_end','tpos',SIEND),('INPUSH','si_blob')).goto('IN.scopy')
    g.on('IN.scopy',[256],'IN.sdone',[('INPOP',),('JUMP','si_end')])
    g.els('IN.scopy','IN.scopy',[('COPY',),('ADV',)])
    P('IN.sdone').call('NEXT').goto('IN.l')

    P('SC.nof32').branch({1:'SC.nof32v'},'RET',[('CMPI','vt',0)])
    P('SC.nof32v').branch({1:'DEAD.dbl'},'RET',[('CMPI','vb',E.FLT)])
