"""Optional raw label/data map, computed by delta after final image layout.
UNILIB1 is not a host ABI. Code entries include internal labels. Data aliases
are declared here and collision-checked in the delta, never guessed by C.
"""
from pathlib import Path
SEEN = 12 << 40


def install(E, OFF, LABD, direct_labels, completion):
    from finite_rules import install as rules
    from modelinput import u64
    if direct_labels:
        from armlayout import SYM, PRESENT
    else:
        from address import SYM, PRESENT
    P,g=E.P,E.g
    # Hook only the image writer's final data-loop return, not EI.bytes returns.
    mode,row=g.st[completion]
    for k,(n,q) in list(row.items()):
        if n=='RET': row[k]=('LIB.finish',q)
    u64(E,'LIB.resource',b'\0library/symbols','lib_flag','lib_present','LIB.fail')
    # Validate before any writer, including file-format writers with own tail.
    g.st['LIB.imagebegin']=g.st.pop('ELF.begin')
    P('ELF.begin').call('LIB.resource').branch({1:'LIB.imagebegin'},'LIB.mode',[('CMPI','lib_present',0)])
    P('LIB.mode').branch({1:'LIB.memory'},'LIB.fail',[('CMPI','lib_flag',1)])
    P('LIB.memory').branch({1:'LIB.fail'},'LIB.imagebegin',[('CMPI','memory_mode',0)])
    P('LIB.finish').branch({1:'RET'},'LIB.begin',[('CMPI','lib_present',0)])
    P('LIB.fail').a(E.rej('not covered: library symbol declaration or collision')).goto('DEAD')
    P('LIB.begin').a(('LDI','zero',0),('OCUT','lib_payload','zero'),('LDI','lib_count',0),
                     ('INPUSHX','zero'),('SBCLR',),E.O('')).goto('LIB.line')
    # Explicit finite framing rules. Token reader leaves separators unread.
    rules(g,Path(__file__).parent,'librarysymbols',section='scan')
    P('LIB.first').a(('MARK','ls')).call('LIB.token').a(('MARK','le'),('INTERN','lid','ls','le'),
        ('SBCLR',),[('SBOUT',b) for b in b'@sym'],('SBINTERN','symword')).branch(
        {1:'LIB.dataws'},'LIB.label',[('CMP','lid','symword')])
    P('LIB.label').a(('ALUI','sub','lt','le',1),('JUMP','lt'),('BYTE','lc'),('JUMP','le')).branch(
        {1:'LIB.code'},'LIB.skip',[('CMPI','lc',58)])
    P('LIB.code').a(('COPYW','le','lt'),('INTERN','lid','ls','le'),('LDX','la','lid',LABD)).branch(
        {1:'LIB.fail'},'LIB.codeaddr',[('CMPI','la',0)])
    if direct_labels:
        P('LIB.codeaddr').a(('ALUI','sub','la','la',1)).goto('LIB.coderecord')
    else:
        P('LIB.codeaddr').a(('ALUI','sub','la','la',1)).branch({0:'LIB.codein'},'LIB.codeend',[('CMP','la','npc')])
        P('LIB.codein').a(('LDX','la','la',OFF)).goto('LIB.coderecord')
        P('LIB.codeend').a(('COPYW','la','endo')).goto('LIB.coderecord')
    P('LIB.coderecord').a(('A64','add','la','la','text_va'),('LDI','lib_kind',0)).call('LIB.record').goto('LIB.skip')
    P('LIB.datafirst').a(('MARK','ls')).call('LIB.token').a(('MARK','le'),('INTERN','lid','ls','le'),
         ('LDX','lp','lid',PRESENT)).branch({1:'LIB.dataaddr'},'LIB.fail',[('CMPI','lp',1)])
    P('LIB.dataaddr').a(('LDX','la','lid',SYM),('A64','add','la','la','data_shift'),('LDI','lib_kind',1)).call('LIB.record').goto('LIB.alias')
    # g_ aliases are a fixed lowering-name declaration, not host-side inference.
    prefix=(Path(__file__).with_name('librarysymbols-alias.tsv').read_text().strip().split('\t'))
    assert prefix==['data','g_','strip-prefix']
    P('LIB.alias').a(('JUMP','ls'),('BYTE','lc')).branch({1:'LIB.alias2'},'LIB.skip',[('CMPI','lc',ord('g'))])
    P('LIB.alias2').a(('ADV',),('BYTE','lc')).branch({1:'LIB.aliasname'},'LIB.skip',[('CMPI','lc',ord('_'))])
    P('LIB.aliasname').a(('ALUI','add','ls','ls',2),('JUMP','le'),('INTERN','lid','ls','le')).branch(
        {1:'LIB.fail'},'LIB.aliasrecord',[('CMP','ls','le')])
    P('LIB.aliasrecord').call('LIB.record').goto('LIB.skip')
    P('LIB.record').a(('LDX','lp','lid',SEEN)).branch({1:'LIB.recordok'},'LIB.fail',[('CMPI','lp',0)])
    p=P('LIB.recordok').a(('LDI','lp',1),('STX','lid',SEEN,'lp'),('OUTW','lib_kind'),
        ('ALU','sub','lb_v','le','ls'),('LDI','lb_n',8)).call('EI.bytes')
    p.a(('COPYW','lb_v','la'),('LDI','lb_n',8)).call('EI.bytes').a(('SPAN2','ls','le'),
        ('ALUI','add','lib_count','lib_count',1)).ret()
    p=P('LIB.done').a(('INPOP',),('OCUT','lib_records','zero')).o('UNILIB1\n').a(
        ('INPUSH','lib_payload'),('LDI','lt',8),('XLEN','le'),('SPAN2','lt','le'),('INPOP',)).o('SYMS1\n')
    p.a(('COPYW','lb_v','lib_count'),('LDI','lb_n',8)).call('EI.bytes').a(
        ('INPUSH','lib_records'),('XLEN','le'),('SPAN2','zero','le'),('INPOP',)).ret()
