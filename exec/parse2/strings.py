"""E3 token-span and byte decoding for adjacent ordinary string literals.
Hand parsing rules compiled into the same generic delta; no runtime Python.
"""

def token_span(E, P):
    g = E.g
    # Replace only the old reader's one-line SPANSTR row, deliberately.
    del g.st['SPANSTR']
    g.on('SPANSTR', [34], 'QS.body', [('ADV',)])
    g.els('SPANSTR', 'DEAD', E.rej('not covered: string prefix'))
    g.on('QS.body', [92], 'QS.escape', [('ADV',)])
    g.on('QS.body', [34], 'QS.gap', [('ADV',), ('MARK','pe')])
    g.on('QS.body', [256], 'DEAD', E.rej('not covered: truncated string token'))
    g.els('QS.body', 'QS.body', [('ADV',)])
    g.on('QS.escape', [256], 'DEAD', E.rej('not covered: truncated string escape'))
    g.els('QS.escape', 'QS.body', [('ADV',)])
    g.on('QS.gap', [32,9,10,13,11,12], 'QS.gap', [('ADV',)])
    g.on('QS.gap', [34], 'QS.body', [('ADV',)])
    # The token dump ends the final quoted span with one newline. Rewind the
    # lookahead, leaving the next token untouched, including at EOF.
    g.els('QS.gap', 'RET', [('JUMP','pe'), ('ADV',), ('LDI','tk',E.TK_STR)])


def initializer(E, P, esc):
    """Shared byte stores for automatic, global and block-static arrays.
    The caller sets ibytes and INITADDR's storage descriptor. NEXT is read
    once after the literal; address spelling remains in the shared helper.
    """
    P('STRINGINIT').a(('LDI','ioff',0),('LDI','str_done',0)).call('INITADDR').o('  .zero r1, 0, ').num('ibytes').o('\n').goto('SI.walk')
    walk(E,P,esc,'SI.walk','SI.byte','SI.end')
    P('SI.byte').branch({0:'SI.store'},'SI.bad',[('CMP','ioff','ibytes')])
    P('SI.bad').branch({1:'DEAD.ginit'},'DEAD.linit',[('CMPI','imode',1)])
    p=P('SI.store').o('  imm r0, ').num('bv').o('\n').call('INITADDR')
    p.o('  .st [r1+0], r0, 1\n').a(('ALUI','add','ioff','ioff',1)).branch({1:'SI.done'},'SI.walk.w',[('CMPI','str_done',1)])
    P('SI.end').branch({0:'SI.nul'},'SI.done',[('CMP','ioff','ibytes')])
    P('SI.nul').a(('LDI','bv',0),('LDI','str_done',1)).goto('SI.store')
    P('SI.done').call('NEXT').ret()


def walk(E, P, esc, pre, body, done):
    """Decode [ps+1,pe-1) a byte at a time in bv; resume at pre+'.w'."""
    g = E.g
    P(pre).a(('ALUI','add','fs','ps',1),('ALUI','sub','fe','pe',1),('INPUSHXE','fs','fe')).goto(pre+'.w')
    g.on(pre+'.w',[92],pre+'.es',[('ADV',)])
    g.on(pre+'.w',[34],pre+'.gap',[('ADV',)])
    g.on(pre+'.w',[256],done,[('INPOP',)])
    for c in range(256):
        if c not in (34,92):g.on(pre+'.w',[c],body,[('ADV',),('LDI','bv',c)])
    g.on(pre+'.gap',[32,9,10,13,11,12],pre+'.gap',[('ADV',)])
    g.on(pre+'.gap',[34],pre+'.w',[('ADV',)])
    g.els(pre+'.gap','DEAD',E.rej('not covered: adjacent string prefix'))
    for ch,v in esc.items():
        if ch not in '01234567x':g.on(pre+'.es',[ord(ch)],body,[('ADV',),('LDI','bv',v)])
    g.on(pre+'.es',[120],pre+'.hex',[('ADV',),('LDI','bv',0),('LDI','sn',0)])
    for c in range(48,56):g.on(pre+'.es',[c],pre+'.oct',[('ADV',),('LDI','bv',c-48),('LDI','sn',1)])
    g.els(pre+'.es','DEAD',E.rej('not covered: string escape'))
    for c in range(48,58):g.on(pre+'.hex',[c],pre+'.hex',[('ALUI','mul','bv','bv',16),('ALUI','add','bv','bv',c-48),('ALUI','and','bv','bv',255),('LDI','sn',1),('ADV',)])
    for off in (65,97):
        for d in range(6):g.on(pre+'.hex',[off+d],pre+'.hex',[('ALUI','mul','bv','bv',16),('ALUI','add','bv','bv',10+d),('ALUI','and','bv','bv',255),('LDI','sn',1),('ADV',)])
    g.els(pre+'.hex',pre+'.hexend',[])
    P(pre+'.hexend').branch({1:body},'DEAD',[('CMPI','sn',1)])
    for c in range(48,56):g.on(pre+'.oct',[c],pre+'.octstep',[('ALUI','mul','bv','bv',8),('ALUI','add','bv','bv',c-48),('ALUI','and','bv','bv',255),('ALUI','add','sn','sn',1),('ADV',)])
    g.els(pre+'.oct',body,[])
    P(pre+'.octstep').branch({1:body},pre+'.oct',[('CMPI','sn',3)])
