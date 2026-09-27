"""Block-scope static storage, using the shared scope/type/load/store procedures.
LOC encodes a static object's label as -(token ordinal + 1); positive slots
still mean frame offsets or GMARK. No language primitive is added to run.c.
"""

def install(E, P, TIX, SINIT, SIEND, LOC, SKIPS, BOOL):
    import pathlib,re
    # Same bounded ordinal namespace as the product; single-unit labels stay
    # unchanged. This is a declaration constant, not emitted reference code.
    source=(pathlib.Path(E.ROOT)/'src/front_pp.c').read_text()
    limits=re.findall(r'^#define MAXTOK ([0-9]+)\b',source,re.M)
    assert len(limits)==1 and 0<int(limits[0])<(1<<25)
    unit_span=int(limits[0])
    g = E.g
    # A first token walk assigns source ordinals, including skipped qualifiers.
    # Cache by byte position: later rewinds must not allocate another ordinal.
    del g.st['NEXT']       # replace the reader entry, preserving NX and its lexer
    g.on('NEXT', range(257), 'IX.have', [('MARK','tpos'),('LDX','ixn','tpos',TIX)], 'r')
    P('IX.have').branch({1:'IX.new'}, 'NX', [('CMPI','ixn',0)])
    P('IX.new').a(('ALUI','add','ixcount','ixcount',1),('STX','tpos',TIX,'ixcount')).goto('NX')
    P('INDEX').call('NEXT').tok({'eof':'RET'},'INDEX')
    P('SC.start').call('NEXT').call('TSPEC').goto('SC.name')
    P('SC.name').tok({E.TK_ID:'SC.id','(':'SC.fp'}, ('rej','not covered: static declarator'))
    P('SC.fp').call('FPDECL').a(('COPYW','si_pos','fppos'),
        ('LDX','si_lab','fppos',TIX),('ALUI','sub','si_lab','si_lab',1),
        ('ALUI','mul','si_unit','unit_epoch',unit_span),('ALU','add','si_lab','si_lab','si_unit'),
        ('COPYW','prd','fpn')).branch({1:'SC.fpscalar'},'SC.size',[('CMPI','fpn',0)])
    P('SC.fpscalar').a(('LDI','prd',1)).goto('SC.size')
    P('SC.id').a(('COPYW','ips','ps'),('COPYW','ipe','pe'),('COPYW','si_pos','tpos'),
        ('LDX','si_lab','tpos',TIX),('ALUI','sub','si_lab','si_lab',1),
        ('ALUI','mul','si_unit','unit_epoch',unit_span),('ALU','add','si_lab','si_lab','si_unit'),
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
    # Scalar and aggregate initialization both emit once into the deferred __init blob.
    saved = ('si_pos','si_lab','si_out','v','bd','tb','td','dar','dsz')
    P('SC.init').a(('OLEN','si_out'),('LDI','si_active',1)).vpush(*saved).call('NEXT').tok({'{':'SC.aggr',E.TK_STR:'SC.string'},'SC.scalar')
    P('SC.aggr').a(('LDI','imode',2),('COPYW','inlabel','si_lab'),('COPYW','ivv','v'),('COPYW','ibytes','dsz')).call('INITLIST').vpop(*saved).goto('SC.cache')
    P('SC.string').branch({1:'SC.scalar'},'SC.char',[('CMPI','dar',0)])
    P('SC.char').call('CHARR').branch({1:'SC.strinit'},'SC.scalar',[('CMPI','u',1)])
    P('SC.strinit').a(('LDI','t',1),('STX','tpos',SKIPS,'t'),('LDI','imode',2),
        ('COPYW','inlabel','si_lab'),('COPYW','ibytes','dsz')).call('STRINGINIT').vpop(*saved).goto('SC.cache')
    P('SC.scalar').branch({1:'SC.expr'},('rej','not covered: static array initializer'),[('CMPI','dar',0)])
    P('SC.expr').call('EXPR').a(('COPYW','rvt','vt'),('COPYW','rvb','vb')).call('ISDV').a(('COPYW','sdv','u')).vpop(*saved).a(
        ('LDX','vt','v',E.PTR),('LDX','vb','v',E.BASE)).branch({1:'SC.booltest'},'SC.kind',[('CMPI','vt',0)])
    P('SC.booltest').branch({1:'SC.bool'},'SC.kind',[('CMPI','vb',BOOL)])
    P('SC.bool').call('BOOLCV').goto('SC.store')
    P('SC.kind').vpush('vt','vb').a(('COPYW','vt','rvt'),('COPYW','vb','rvb')).call('SC.nof32').vpop('vt','vb').call('SC.nof32').call('ISDV').branch({1:'SC.store'},'DEAD.dbl',[('CMP','u','sdv')])
    P('SC.store').o('  .lea r1, ls').num('si_lab').o('\n').call('STOREV').goto('SC.cache')
    P('SC.cache').a(('LDI','si_active',0),('OCUT','si_blob','si_out'),('STX','si_pos',SINIT,'si_blob'),('STX','si_pos',SIEND,'tpos')).goto('SC.after')
    P('IN.static').a(('LDX','si_end','tpos',SIEND),('INPUSH','si_blob')).goto('IN.scopy')
    g.on('IN.scopy',[256],'IN.sdone',[('INPOP',),('JUMP','si_end')])
    g.els('IN.scopy','IN.scopy',[('COPY',),('ADV',)])
    P('IN.sdone').call('NEXT').goto('IN.l')

    P('SC.nof32').branch({1:'SC.nof32v'},'RET',[('CMPI','vt',0)])
    P('SC.nof32v').branch({1:'DEAD.dbl'},'RET',[('CMPI','vb',E.FLT)])
