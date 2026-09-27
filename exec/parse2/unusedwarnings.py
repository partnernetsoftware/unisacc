"""Reference block-local unused-variable warnings on generic model actions.
Binding records follow the existing shared-size scope undo records. Reads use
reference primary()'s token-context rule, not generated tape liveness.
"""
from tokenlocations import TOKEN_POS
ACTIVE=40<<40
PREVIOUS=41<<40
USED=42<<40
ELIGIBLE=43<<40
NAME=44<<40
POSITION=45<<40
NAME_TOKEN=46<<40
KIND=47<<40

def install(E,P,TIX,undo_size):
    # INDEX visits the full token stream before declarations are parsed.
    # Later reads/replays overwrite the same immutable token facts.
    P('WU.token').a(('LDX','wu_ix','tpos',TIX),('ALUI','mul','wu_unitbase','unit_epoch',1<<26),('ALU','add','wu_ix','wu_ix','wu_unitbase'),('STX','wu_ix',KIND,'tk')).tok({E.TK_ID:'WU.idtoken'},'RET')
    P('WU.idtoken').a(('STX','ps',NAME_TOKEN,'tpos')).ret()
    P('WU.bind').a(('LDX','wu_prev','v',ACTIVE),('STX','usp',PREVIOUS,'wu_prev'),
        ('ALUI','add','wu_active','usp',1),('STX','v',ACTIVE,'wu_active'),
        ('LDI','wu_zero',0),('STX','usp',USED,'wu_zero'),('STX','usp',ELIGIBLE,'wu_zero'),
        ('LDX','wu_tok','ps',NAME_TOKEN),('LDX','wu_pos','wu_tok',TOKEN_POS),('STX','usp',POSITION,'wu_pos')).goto('WU.name')
    # Multi-unit static isolation rewrites token names. Diagnostics use the
    # original preprocessed spelling at the retained source position.
    P('WU.name').a(('SBCLR',),('INPUSH','diag_source'),('JUMP','wu_pos'),('LDI','wu_nc',0)).goto('WU.namechar')
    E.g.on('WU.namechar',list(b'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789'),'WU.nameadd',[('ADV',),('ALUI','add','wu_nc','wu_nc',1)])
    E.g.els('WU.namechar','WU.namedone')
    P('WU.nameadd').branch({0:'WU.namechar'},'WU.namedone',[('CMPI','wu_nc',24)])
    P('WU.namedone').a(('MARK','wu_end'),('SBSPAN','wu_pos','wu_end'),('INPOP',),('SBSAVE','wu_name'),('STX','usp',NAME,'wu_name')).ret()
    P('WU.local').a(('ALUI','sub','wu_slot','usp',undo_size),('LDI','wu_one',1),('STX','wu_slot',ELIGIBLE,'wu_one')).ret()
    # UNWIND calls after decrementing usp and loading the bound id in v.
    P('WU.unbind').a(('LDX','wu_prev','usp',PREVIOUS),('STX','v',ACTIVE,'wu_prev')).ret()
    P('WU.use').a(('INTERN','wu_id','ips','ipe'),('LDX','wu_active','wu_id',ACTIVE)).branch(
        {1:'RET'},'WU.context',[('CMPI','wu_active',0)])
    P('WU.context').a(('LDX','wu_tok','ips',NAME_TOKEN),('LDX','wu_ix','wu_tok',TIX),('ALUI','mul','wu_unitbase','unit_epoch',1<<26),('ALU','add','wu_ix','wu_ix','wu_unitbase'),
        ('ALUI','add','wu_next','wu_ix',1),('LDX','wu_kind','wu_next',KIND)).branch(
        {1:'WU.previous'},'WU.mark',[('CMPI','wu_kind',E.TK['='])])
    P('WU.previous').a(('ALUI','sub','wu_previous','wu_ix',1),('LDX','wu_kind','wu_previous',KIND)).goto('WU.previous.test')
    p=P('WU.previous.test')
    # Previous token ; { } ) means a standalone write, just as primary().
    # A compound assignment or any other identifier occurrence is a use.
    for i,k in enumerate((';','{','}',')')):
        nxt='WU.prev'+str(i)
        p.branch({1:'RET'},nxt,[('CMPI','wu_kind',E.TK[k])]);p=P(nxt)
    p.goto('WU.mark')
    P('WU.mark').a(('ALUI','sub','wu_slot','wu_active',1),('LDI','wu_one',1),('STX','wu_slot',USED,'wu_one')).ret()
    P('WU.block').a(('COPYW','wu_i','wu_lo')).goto('WU.scan')
    P('WU.scan').branch({0:'WU.eligible'},'RET',[('CMP','wu_i','usp')])
    P('WU.eligible').a(('LDX','wu_yes','wu_i',ELIGIBLE)).branch({1:'WU.used'},'WU.next',[('CMPI','wu_yes',1)])
    P('WU.used').a(('LDX','wu_yes','wu_i',USED)).branch({1:'WU.report'},'WU.next',[('CMPI','wu_yes',0)])
    P('WU.report').a(('LDX','diag_pos','wu_i',POSITION),('LDX','wu_name','wu_i',NAME),('SBCLR',),
        *[('SBOUT',c) for c in b"unused variable '"],('SBBLOB','wu_name'),
        *[('SBOUT',c) for c in b"' [-Wunused-variable]"],('SBSAVE','diag_message'),('LDI','diag_warning',1)).call('DIAG.report').a(
        ('ALU','add','wr_count','wr_count','diag_reported')).goto('WU.next')
    P('WU.next').a(('ALUI','add','wu_i','wu_i',undo_size)).goto('WU.scan')
