"""Located extension of the existing unit-framing/name-isolation model.
Frame payload = LE32 filename length, filename bytes, UNITOK1 envelope.
Output = UNITOK2 map directory plus positioned, statically renamed tokens.
No declaration logic is duplicated here; units.py remains the scanner.
"""
MAPS=49<<40

def install(E,P):
    from tokenlocations import install as locations
    g=E.g
    def replace(name):
        del g.st[name]
        return P(name)
    def word(p,reg):
        for k in range(4):p.a(('ALUI','sar','ls_byte',reg,8*k),('OUTW','ls_byte'))
        return p
    def prefix(p,offset):
        p.o('@');word(p,'unit');word(p,offset);return p.o('\n')
    locations(E,P,ready='LS.ready',multi=False)
    replace('SCAN').a(('INPUSHXE','begin','end'),('LDI','dep',0),('LDI','ls_namelen',0)).goto('LS.len0')
    for k in range(4):
        for c in range(128 if k==3 else 256):
            g.on('LS.len'+str(k),[c],'LS.len'+str(k+1) if k<3 else 'LS.extent',
                 [('ALUI','add','ls_namelen','ls_namelen',c<<(8*k)),('ADV',)])
        g.els('LS.len'+str(k),'DEAD.frame')
    P('LS.extent').a(('MARK','ls_namebegin'),('ALU','sub','ls_left','end','ls_namebegin')).branch({1:'DEAD.frame'},'LS.bound',[('CMPI','ls_namelen',0)])
    P('LS.bound').branch({2:'DEAD.frame'},'LS.name',[('CMP','ls_namelen','ls_left')])
    P('LS.name').a(('ALU','add','ls_nameend','ls_namebegin','ls_namelen'),
        ('BLOBSAVE','ls_name','ls_namebegin','ls_nameend'),('JUMP','ls_nameend')).goto('DL.magic0')
    p=P('LS.ready').a(('MARK','token_begin'),('OLEN','ls_mark'))
    word(p,'ls_namelen').a(('INPUSH','ls_name'),('LDI','ls_zero',0),('SPAN2','ls_zero','ls_namelen'),('INPOP',))
    word(p,'diag_pp_len').a(('SPAN2','diag_pp_begin','diag_pp_end'),('OCUT','ls_map','ls_mark'),('STX','unit',MAPS,'ls_map')).call('NEXT').goto('SCAN.loop')
    p=replace('COPY.first').a(('LDI','ls_zero',0));prefix(p,'ls_zero').o('@unit0\n').goto('COPY.tokens')
    p=replace('COPY.later').a(('LDI','ls_zero',0));prefix(p,'ls_zero').o('@unit+\n').goto('COPY.tokens')
    replace('COPY.tokens').a(('JUMP','token_begin'),('MARK','copy_begin')).call('NEXT').goto('COPY.loop')
    p=replace('COPY.rename');prefix(p,'source_pos').o('id=').a(('SPAN2','ps','pe')).o('__u').num('unit').o('\n').goto('COPY.next')
    p=replace('COPY.raw').o('@');word(p,'unit').a(('MARK','at'),('ALUI','add','ls_from','copy_begin',1),('SPAN2','ls_from','at')).goto('COPY.next')
    p=replace('DONE').a(('LDI','ls_zero',0),('OCUT','ls_tokens','ls_zero')).o('UNITOK2\0')
    word(p,'unit').a(('LDI','ls_i',0)).goto('LS.maps')
    P('LS.maps').branch({0:'LS.map'},'LS.tokens',[('CMP','ls_i','unit')])
    P('LS.map').a(('LDX','ls_map','ls_i',MAPS),('BLEN','ls_size','ls_map'),('INPUSH','ls_map'),('SPAN2','ls_zero','ls_size'),('INPOP',),('ALUI','add','ls_i','ls_i',1)).goto('LS.maps')
    p=P('LS.tokens').a(('BLEN','ls_size','ls_tokens'),('INPUSH','ls_tokens'),('SPAN2','ls_zero','ls_size'),('INPOP',),('ALUI','sub','unit','unit',1))
    prefix(p,'diag_textlen').o('eof\n').a(('ACCEPT',)).goto('DEAD')
