"""Object-route facts: tape encounter order and linkage, computed by delta actions.
No text parser or symbol plan executes in the Python constructor.
"""
ORD, NAMES, GLOBAL, EXTERN, SHAPES = (i << 40 for i in range(240,245))

def install(E):
    from unisa.tape import SHAPE
    g=E.g
    class P(E.P):
        def b(self,cases,other):
            used=set()
            for key,target in cases.items():
                g.on(self.cur,[key],target,self.acts,'b');used.add(key)
            g.on(self.cur,set(range(257))-used,other,self.acts,'b')
            self.cur=None;self.acts=[]
            return self
    def move(state):
        name='OF.original.'+state;assert name not in g.st
        g.st[name]=g.st.pop(state);g.labels.add(name);return name
    start=move('START')
    p=P('START').a(('MARK','of_zero'),('LDI','of_count',0),('SBCLR',),[('SBOUT',c) for c in b'\0cli/funit'],('SBFIND','of_resource'),('BLEN','of_unit','of_resource'))
    words=list(SHAPE)+['.global','.extern','.bss','.str']
    for i,w in enumerate(words):
        p.a(('SBCLR',),[('SBOUT',c) for c in w.encode()],('SBINTERN','of_key'+str(i)))
        if w in SHAPE:
            for j,k in enumerate(SHAPE[w]):
                if k in 'Ls':p.a(('ALUI','mul','of_ix','of_key'+str(i),8),('ALUI','add','of_ix','of_ix',j),('LDI','of_one',1),('STX','of_ix',SHAPES,'of_one'))
    key={w:'of_key'+str(i) for i,w in enumerate(words)}
    p.goto('OF.line')
    P('OF.line').b({32:'OF.space',9:'OF.space',10:'OF.space',256:'OF.done'},'OF.op')
    P('OF.space').a(('ADV',)).goto('OF.line')
    P('OF.op').call('OF.word').branch({1:'OF.label'},'OF.dispatch',[('CMPI','of_last',58)])
    P('OF.label').a(('ALUI','sub','of_we','of_we',1)).call('OF.name').goto('OF.skip')
    p=P('OF.dispatch').a(('INTERN','of_op','of_ws','of_we'),('LDI','of_arg',0))
    for w in ['.global','.extern','.bss','.str']:
        nxt=p.fresh('r');p.branch({1:'OF.directive.'+w},nxt,[('CMP','of_op',key[w])]);p.label(nxt)
    p.goto('OF.args')
    for w in ['.global','.extern','.bss','.str']:
        p=P('OF.directive.'+w).call('OF.whitespace').call('OF.word').call('OF.name')
        if w in ('.global','.extern'):
            p.a(('LDI','of_one',1),('STX','of_nameid',GLOBAL if w=='.global' else EXTERN,'of_one'))
        p.goto('OF.skip')
    P('OF.whitespace').b({32:'OF.wsadv',9:'OF.wsadv'},'RET')
    P('OF.wsadv').a(('ADV',)).goto('OF.whitespace')
    P('OF.args').b({32:'OF.argspace',9:'OF.argspace',44:'OF.argspace',91:'OF.argspace',93:'OF.argspace',43:'OF.argspace',10:'OF.next',256:'OF.done',59:'OF.skip'},'OF.argument')
    P('OF.argspace').a(('ADV',)).goto('OF.args')
    P('OF.next').a(('ADV',)).goto('OF.line')
    P('OF.argument').call('OF.word').a(('ALUI','mul','of_ix','of_op',8),('ALU','add','of_ix','of_ix','of_arg'),('LDX','of_isname','of_ix',SHAPES),('ALUI','add','of_arg','of_arg',1)).branch({1:'OF.namedarg'},'OF.args',[('RLD','of_isname')])
    P('OF.namedarg').a(('MARK','of_after'),('JUMP','of_ws')).b({c:'OF.notname' for c in list(range(48,58))+[43,45]},'OF.argname')
    P('OF.notname').a(('JUMP','of_after')).goto('OF.args')
    P('OF.argname').a(('JUMP','of_after')).call('OF.name').goto('OF.args')
    P('OF.word').a(('MARK','of_ws'),('LDI','of_last',0)).goto('OF.wordloop')
    P('OF.wordloop').b({c:'OF.wordend' for c in [32,9,10,44,91,93,43,59,256]},'OF.wordbyte')
    P('OF.wordbyte').a(('BYTE','of_last'),('ADV',)).goto('OF.wordloop')
    P('OF.wordend').a(('MARK','of_we')).ret()
    P('OF.name').a(('CMP','of_ws','of_we')).branch({1:'OF.fail'},'OF.intern')
    P('OF.intern').a(('INTERN','of_nameid','of_ws','of_we'),('LDX','of_ordinal','of_nameid',ORD)).branch({0:'OF.addname'},'RET',[('RLD','of_ordinal')])
    P('OF.addname').a(('BLOBSAVE','of_blob','of_ws','of_we'),('STX','of_count',NAMES,'of_blob'),('ALUI','add','of_count','of_count',1),('STX','of_nameid',ORD,'of_count')).ret()
    P('OF.skip').b({10:'OF.next',256:'OF.done'},'OF.skipbyte')
    P('OF.skipbyte').a(('ADV',)).goto('OF.skip')
    P('OF.done').a(('MARK','of_end'),('BLOBSAVE','of_tape','of_zero','of_end'),('JUMP','of_zero')).goto(start)
    P('OF.fail').a(*E.rej('not covered: object tape metadata')).goto('DEAD')
    # Link attributes carry no target instruction, but still define symbol order.
    previous=move('WORD.end')
    p=P('WORD.end').a(('INTERN','wi','ws','we'))
    for w in ('.global','.extern'):
        nxt=p.fresh('r');p.branch({1:'SKIP'},nxt,[('CMP','wi',key[w])]);p.label(nxt)
    p.goto(previous)
    second=move('R.second');P('R.second').a(('COPYW','of_nzend','outn')).goto(second)
    head=move('HEAD');P('HEAD').a(('COPYW','stored','of_nzend')).goto(head)
    end=move('H.end')
    P('H.end').o('@obj_unit ').a(('COPYW','n','of_unit')).call('PRN').o('\n@obj_nzend ').a(('COPYW','n','of_nzend')).call('PRN').o('\n').a(('LDI','of_i',0)).goto('OF.headers')
    P('OF.headers').branch({0:'OF.header'},'OF.raw',[('CMP','of_i','of_count')])
    P('OF.header').o('@obj_name ').a(('LDX','of_blob','of_i',NAMES),('INPUSH','of_blob')).call('OF.internblob').a(('INPOP',),('INPUSH','of_blob')).call('OF.printblob').a(('INPOP',),('LDX','n','of_nameid',GLOBAL)).o(' ').call('PRN').a(('LDX','n','of_nameid',EXTERN)).o(' ').call('PRN').o('\n').a(('ALUI','add','of_i','of_i',1)).goto('OF.headers')
    P('OF.internblob').a(('LDI','of_zero2',0)).goto('OF.ib')
    P('OF.ib').b({256:'OF.ibend'},'OF.ibbyte')
    P('OF.ibbyte').a(('ADV',)).goto('OF.ib')
    P('OF.ibend').a(('MARK','of_size'),('INTERN','of_nameid','of_zero2','of_size')).ret()
    P('OF.printblob').b({256:'RET'},'OF.printbyte')
    P('OF.printbyte').a(('COPY',),('ADV',)).goto('OF.printblob')
    P('OF.raw').o('@obj_tape ').a(('INPUSH','of_tape')).call('OF.hexblob').a(('INPOP',)).o('\n').goto(end)
    P('OF.hexblob').b({256:'RET'},'OF.hexbyte')
    P('OF.hexbyte').a(('BYTE','of_byte'),('ALUI','sar','h','of_byte',4)).call('HEX').a(('ALUI','and','h','of_byte',15)).call('HEX').a(('ADV',)).goto('OF.hexblob')
