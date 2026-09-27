"""Resolve ordinary tape calls after every source unit, as undef_calls does.
Two byte scans collect labels and then diagnose unresolved calls in emission
order, once per name. No parser/diagnostic primitive is added to the executor.
"""
DEFINED = 33 << 40

def install(P):
    # BYTE returns zero at EOF; generated textual tape has no literal NUL.
    P('UD.check').a(('LDI','ud_zero',0),('OCUT','ud_tape','ud_zero'),
        ('INPUSH','ud_tape'),('XLEN','ud_len'),('LDI','ud_bad',0)).goto('UD.labels')
    P('UD.labels').branch({0:'UD.calls0',32:'UD.skip',46:'UD.skip'},'UD.label', [('BYTE','ud_byte'),('RLD','ud_byte')])
    P('UD.label').a(('MARK','ud_start'),('LDI','ud_last',0)).goto('UD.scan')
    P('UD.scan').branch({10:'UD.endlabel',0:'UD.endlabel'},'UD.char',[('BYTE','ud_byte'),('RLD','ud_byte')])
    P('UD.char').a(('COPYW','ud_last','ud_byte'),('ADV',)).goto('UD.scan')
    P('UD.endlabel').branch({58:'UD.define'},'UD.advance',[('RLD','ud_last')])
    P('UD.define').a(('MARK','ud_end'),('ALUI','sub','ud_end','ud_end',1),
        ('INTERN','ud_id','ud_start','ud_end'),('LDI','ud_one',1),('STX','ud_id',DEFINED,'ud_one')).goto('UD.advance')
    P('UD.skip').branch({10:'UD.advance',0:'UD.calls0'},'UD.skip1',[('BYTE','ud_byte'),('RLD','ud_byte')])
    P('UD.skip1').a(('ADV',)).goto('UD.skip')
    P('UD.advance').branch({0:'UD.calls0'},'UD.advance1',[('BYTE','ud_byte'),('RLD','ud_byte')])
    P('UD.advance1').a(('ADV',)).goto('UD.labels')
    P('UD.calls0').a(('JUMP','ud_zero')).goto('UD.prefix0')
    for i,c in enumerate(b'  call '):
        P('UD.prefix'+str(i)).branch({c:'UD.match'+str(i),0:'UD.done'},'UD.nextline',[('BYTE','ud_byte'),('RLD','ud_byte')])
        P('UD.match'+str(i)).a(('ADV',)).goto('UD.prefix'+str(i+1) if i<6 else 'UD.name0')
    P('UD.name0').a(('MARK','ud_start')).goto('UD.name')
    P('UD.name').branch({10:'UD.lookup',0:'UD.lookup'},'UD.name1',[('BYTE','ud_byte'),('RLD','ud_byte')])
    P('UD.name1').a(('ADV',)).goto('UD.name')
    P('UD.lookup').a(('MARK','ud_end'),('INTERN','ud_id','ud_start','ud_end')).branch({1:'UD.nextline'},'UD.error',[
        ('LDX','ud_def','ud_id',DEFINED),('CMPI','ud_def',1)])
    P('UD.error').a(('LDI','ud_one',1),('STX','ud_id',DEFINED,'ud_one'),('OSEL',1)).o(
        "unisacc: error: undefined function '").a(('SPAN2','ud_start','ud_end')).o("'\n").a(
        ('OSEL',0),('ALUI','add','ud_bad','ud_bad',1)).goto('UD.nextline')
    P('UD.nextline').branch({10:'UD.next',0:'UD.done'},'UD.nextchar',[('BYTE','ud_byte'),('RLD','ud_byte')])
    P('UD.nextchar').a(('ADV',)).goto('UD.nextline')
    P('UD.next').a(('ADV',)).goto('UD.prefix0')
    P('UD.done').branch({1:'UD.ok'},'UD.summary',[('CMPI','ud_bad',0)])
    P('UD.ok').a(('SPAN2','ud_zero','ud_len'),('INPOP',)).ret()
    P('UD.summary').a(('OSEL',1)).num('ud_bad').branch({1:'UD.singular'},'UD.plural',[('CMPI','ud_bad',1)])
    P('UD.singular').o(' error generated.\n').goto('UD.reject')
    P('UD.plural').o(' errors generated.\n').goto('UD.reject')
    P('UD.reject').a(('OSEL',0),('REJECT','')).goto('DEAD')
