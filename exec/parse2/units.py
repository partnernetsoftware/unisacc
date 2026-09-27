"""Model prepass for independently lexed translation units.
Input: repeated LE32 length + typed-token bytes. Output: one typed stream.
The model performs framing validation and file-static name isolation. C only
frames bytes. Declaration forms outside this scanner are named refusals.
"""
import importlib.util,json,pathlib,sys
s=importlib.util.spec_from_file_location('unit_grammar',pathlib.Path(__file__).parents[1]/'parse/gen.py')
E=importlib.util.module_from_spec(s);s.loader.exec_module(E)
P=E.P;g=E.g;TK_ID=E.TK_ID
STATIC=1<<40
BAD=('rej','not covered: multi-unit static declarator')

def build(locations=False):
    # This pass copies physical tokens, including qualifiers. The parser's
    # reader skips qualifiers, but doing that here merges two framing spans.
    for name in ('type=const','type=volatile','type=restrict','type=inline','type=_Bool'):
        E.WORDS.append(name);E.TK[name]=max(E.TK.values())+1
    E.tokenizer();E.prn();E.fconv()
    for name in ('type=const','type=volatile','type=restrict','type=inline','type=_Bool'):
        g.st['NX'+name][1][10]=('RET',g.seq([('ADV',),('LDI','tk',E.TK[name])]))
    from strings import token_span
    token_span(E,P)
    P('START').a(('LDI','unit',0),('LDI','zero',0)).goto('FRAME')
    g.on('FRAME',[256],'FINAL',[])
    g.on('FRAME',range(256),'L0',[('LDI','len',0)])
    for i in range(4):
        g.on('L'+str(i),range(256),'L'+str(i)+'b',[('BYTE','ch'),('ADV',),('A64I','shl','ch','ch',8*i),('A64','or','len','len','ch')])
        g.on('L'+str(i),[256],'DEAD',E.rej('not covered: truncated unit frame'))
        P('L'+str(i)+'b').goto('L'+str(i+1) if i<3 else 'EXTENT')
    P('EXTENT').branch({(1,2):'DEAD.frame'},'COUNT.ok',[('CMPI','unit',64)])
    P('COUNT.ok').branch({1:'DEAD.frame'},'EXTENT2',[('CMPI','len',0)])
    P('EXTENT2').a(('MARK','begin'),('XLEN','total'),('A64','add','end','begin','len')).branch({2:'DEAD.frame'},'SCAN',[('C64','end','total')])
    P('SCAN').a(('INPUSHXE','begin','end'),('LDI','dep',0)).call('NEXT').goto('SCAN.loop')
    P('SCAN.loop').tok({'eof':'SCAN.end','{':'SCAN.open','}':'SCAN.close','type=static':'SCAN.static'},'SCAN.next')
    P('SCAN.open').a(('ALUI','add','dep','dep',1)).goto('SCAN.next')
    P('SCAN.close').a(('ALUI','sub','dep','dep',1)).branch({0:'DEAD.frame'},'SCAN.next',[('CMPI','dep',0)])
    P('SCAN.next').call('NEXT').goto('SCAN.loop')
    P('SCAN.static').branch({1:'SD.type'},'SCAN.next',[('CMPI','dep',0)])
    # A declaration specifier followed by pointer stars and an identifier.
    # Tagged types, typedef names and parenthesised pointer declarators are
    # accepted; inline aggregate specifiers remain a named refusal.
    P('SD.type').a(('LDI','nest',0)).call('NEXT').tok({'struct':'SD.tag','union':'SD.tag','enum':'SD.tag',TK_ID:'SD.typedef'},'SD.builtin')
    builtin={w:'SD.more' for w in E.WORDS if w.startswith('type=') and w!='type=static'}
    P('SD.builtin').tok(builtin,BAD)
    P('SD.more').call('NEXT').tok(builtin|{'*':'SD.star','(':'SD.nested',TK_ID:'SD.name'},BAD)
    P('SD.tag').call('NEXT').tok({TK_ID:'SD.typedef'},BAD)
    P('SD.typedef').call('NEXT').goto('SD.decl')
    P('SD.decl').tok({'*':'SD.star','(':'SD.nested',TK_ID:'SD.name'},BAD)
    P('SD.nested').a(('ALUI','add','nest','nest',1)).call('NEXT').goto('SD.decl')
    P('SD.star').call('NEXT').goto('SD.decl')
    P('SD.name').a(('INTERN','id','ps','pe'),('ALUI','add','u','unit',1),('STX','id',STATIC,'u')).call('NEXT').a(('COPYW','par','nest'),('LDI','br',0),('LDI','curly',0),('LDI','init',0)).goto('SD.tail')
    P('SD.tail').tok({'(':'SD.po',')':'SD.pc','[':'SD.bo',']':'SD.bc','{':'SD.co','}':'SD.cc','=':'SD.eq',',':'SD.comma',';':'SD.semi','eof':'DEAD.frame'},'SD.next')
    for token,slot,op in [('po','par','add'),('pc','par','sub'),('bo','br','add'),('bc','br','sub'),('cc','curly','sub')]:
        p=P('SD.'+token).a(('ALUI',op,slot,slot,1))
        if op=='sub':p.branch({0:'DEAD.frame'},'SD.'+token+'ok',[('CMPI',slot,0)]);p=P('SD.'+token+'ok')
        p.goto('SD.next')
    P('SD.eq').a(('LDI','init',1)).goto('SD.next')
    P('SD.co').branch({1:'SD.body'},'SD.initbrace',[('CMPI','init',0)])
    P('SD.body').a(('LDI','dep',1)).goto('SCAN.next')
    P('SD.initbrace').a(('ALUI','add','curly','curly',1)).goto('SD.next')
    for name,dest in [('comma','SD.nextdecl'),('semi','SCAN.next')]:
        P('SD.'+name).a(('ALU','or','u','par','br'),('ALU','or','u','u','curly')).branch({1:dest},'SD.next',[('CMPI','u',0)])
    P('SD.nextdecl').a(('LDI','nest',0)).call('NEXT').goto('SD.decl')
    P('SD.next').call('NEXT').goto('SD.tail')
    # The eof token must be the last token in its bounded view.
    P('SCAN.end').goto('TRAIL')
    g.on('TRAIL',range(48,58),'TRAILdigits',[('ADV',)])
    g.on('TRAIL',[256],'COPY.start',[])
    g.els('TRAIL','DEAD.frame')
    g.on('TRAILdigits',range(48,58),'TRAILdigits',[('ADV',)])
    g.on('TRAILdigits',[32],'TRAIL0',[('ADV',)])
    g.els('TRAILdigits','DEAD.frame')
    for i,c in enumerate(b'tokens\n'):
        g.on('TRAIL'+str(i),[c],'TRAIL'+str(i+1),[('ADV',)])
        g.els('TRAIL'+str(i),'DEAD.frame')
    g.on('TRAIL7',[256],'COPY.start',[])
    g.els('TRAIL7','DEAD.frame')
    P('COPY.start').branch({1:'COPY.first'},'COPY.later',[('CMPI','unit',0)])
    P('COPY.first').o('@unit0\n').goto('COPY.tokens')
    P('COPY.later').o('@unit+\n').goto('COPY.tokens')
    P('COPY.tokens').a(('JUMP','begin'),('MARK','copy_begin')).call('NEXT').goto('COPY.loop')
    P('COPY.loop').tok({'eof':'COPY.end',TK_ID:'COPY.id'},'COPY.raw')
    P('COPY.id').a(('INTERN','id','ps','pe'),('LDX','seen','id',STATIC),('ALUI','add','u','unit',1)).branch({1:'COPY.private'},'COPY.raw',[('CMP','seen','u')])
    P('COPY.private').branch({1:'COPY.raw'},'COPY.rename',[('CMPI','unit',0)])
    P('COPY.rename').a(('SPAN2','copy_begin','tpos')).o('id=').a(('SPAN2','ps','pe')).o('__u').num('unit').o('\n').goto('COPY.next')
    P('COPY.raw').a(('MARK','at'),('SPAN2','copy_begin','at')).goto('COPY.next')
    P('COPY.next').a(('MARK','copy_begin')).call('NEXT').goto('COPY.loop')
    P('COPY.end').a(('INPOP',),('JUMP','end'),('ALUI','add','unit','unit',1)).goto('FRAME')
    P('FINAL').branch({1:'DEAD.frame'},'DONE',[('CMPI','unit',0)])
    P('DONE').o('eof\n').a(('ACCEPT',)).goto('DEAD')
    g.on('DEAD.frame',range(257),'DEAD',E.rej('not covered: malformed unit frame'),'r')
    if locations:
        from unitlocations import install
        install(E,P)
    g.finish()
    return {'start':'START','states':{n:[m,{str(k):v for k,v in r.items()}] for n,(m,r) in g.st.items()},'seqs':[list(map(list,s)) for s in g.seqs]}

if __name__=='__main__':
    d=build("--locations" in sys.argv);pathlib.Path(sys.argv[1]).write_text(json.dumps(d,separators=(',',':')))
    print('unit framing/static isolation:',len(d['states']),'states')
