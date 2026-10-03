"""Borrowed writable scalar data: declaration facts and address selection in δ.
Only resource-enabled externs get deferred addresses; real definitions win later.
"""
EXTERN, DEPTH, BASE, SHAPE = (i << 40 for i in range(193,197))
def global_address(E,P,entry,classic,done):
    E.g.labels.add(entry+'.emit')
    P(entry).branch({1:classic},entry+'.library',[('CMPI','li_len',0)])
    P(entry+'.library').a(('INTERN','ld_v','ips','ipe'),('LDX','ld_ext','ld_v',EXTERN)).branch({1:entry+'.import'},classic,[('CMPI','ld_ext',1)])
    P(entry+'.import').a(('LDI','ld_one',1),('STX','ld_v',197 << 40,'ld_one'),('LDX','ld_depth','ld_v',DEPTH),('LDX','ld_base','ld_v',BASE),('LDI','ld_class',0),('PUSH',entry+'.emit')).goto('LD.classify')
    p=P(entry+'.emit').o('  .libraryaddr r0, g_').a(('SPAN2','ips','ipe')).o(', ').a(('COPYW','n','ld_depth')).call('PRN').o(', ').a(('COPYW','n','ld_class')).call('PRN').o(', ').a(('COPYW','n','ld_width')).call('PRN').o(', ').a(('COPYW','n','ld_uns')).call('PRN').o('\n').goto(done)

USED = 197 << 40

def install(E,P,b,integers):
    """Stage control lives in librarydata-result.tsv / -byte.tsv (sections storage, record,
    rest); fresh labels are declared in librarydata-fresh.tsv. Python binds only dynamic
    facts: graph constants (this module's and libraryimports'), the parent's banks and tape
    codes (b, E.TK, E.LOC, E.GMARK). It keeps two kinds of residue: the parent-row edits
    (prefix: actions prepended to TOP.st and FN; hook: GV.storage/GV.record moved aside),
    and the integer-type branch (keys and state names come from the integers list)."""
    class _Scope:
        a=E.P.a
        def __init__(self,cur):self.cur=cur;self.acts=[]
    from pathlib import Path
    from finite_rules import install as rules
    import libraryimports
    from libraryimports import STRIDE
    g=E.g;root=Path(__file__).parent
    g.labels.add('LD.match')
    bindings={'STRIDE':STRIDE,'E.LOC':E.LOC,'E.GMARK':E.GMARK,'TK.=':E.TK['='],'TK.type=extern':E.TK['type=extern']}
    for k,v in list(globals().items())+[('libraryimports.'+k,v) for k,v in vars(libraryimports).items()]:
        if type(v) is int and v>=1<<40:bindings[k]=v
    classes={}
    for k,v in b.items():
        if type(v) is int:bindings['b.'+k]=v;classes['b.'+k]=[v]
    fresh=[l.split('\t') for l in (root/'librarydata-fresh.tsv').read_text().splitlines()[1:]]
    def section(name):
        for part,key,kind in fresh:
            if part==name:bindings[key]=E.P.fresh(_Scope(key.split('.')[0]),kind)
        rules(g,root,'librarydata',bindings,None,classes,name)
    def prefix(state,acts):
        mode,row=g.st[state]
        for key,(n,q) in list(row.items()):row[key]=(n,g.seq(acts+list(g.seqs[q])))
    def hook(state):
        orig='LD.original.'+state;g.st[orig]=g.st.pop(state);g.labels.add(orig)
    # FN is the shared entry to function and global declarations.
    prefix('TOP.st',[('COPYW','ld_storage','tk')])
    prefix('FN',[('COPYW','ld_decl','ld_storage'),('LDI','ld_storage',0)])
    hook('GV.storage');section('storage')
    hook('GV.record');section('record')
    P('LD.scalar').branch({code:'LD.int.'+str(code) for _,code,_,_,_ in integers},'LD.bool',[('RLD','ld_base')])
    for _,code,width,uns,_ in integers:P('LD.int.'+str(code)).a(('LDI','ld_class',1),('LDI','ld_width',width),('LDI','ld_uns',uns)).ret()
    section('rest')
