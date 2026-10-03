"""Optional raw label/data map, computed by delta after final image layout.
UNILIB1 is not a host ABI. Code entries include internal labels. Data aliases
are declared here and collision-checked in the delta, never guessed by C.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from exec.facts.load import facts
SEEN = {r['name']: r['bank'] << 40 for r in facts('enc-librarysymbols-regions')}['SEEN']


def install(E, OFF, LABD, direct_labels, completion):
    from finite_rules import install as rules, install_template
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
    install_template(g,Path(__file__).parent,'librarysymbols',{},None,section='move')
    # Stage control lives in librarysymbols-result.tsv (sections d0s1/d0s2 for offset-table
    # labels, d1s1/d1s2 for direct labels) around the finite scan section; fresh labels are
    # declared in librarysymbols-fresh.tsv and allocated in recorded order via E.P.fresh on
    # an unregistered scope. Python binds only dynamic facts: OFF, LABD, SYM, PRESENT, SEEN.
    # Residue: the completion-row RET rewrite, and the u64 resource reader; the move of ELF.begin
    # into LIB.imagebegin is librarysymbols-template.tsv (move-state).
    prefix=(Path(__file__).with_name('librarysymbols-alias.tsv').read_text().strip().split('\t'))
    assert prefix==['data','g_','strip-prefix']
    class _Scope:
        def __init__(self,cur):self.cur=cur
    root=Path(__file__).parent;cfg='d1' if direct_labels else 'd0'
    bindings=dict(OFF=OFF,LABD=LABD,SYM=SYM,PRESENT=PRESENT,SEEN=SEEN)
    fresh=[l.split('\t') for l in (root/'librarysymbols-fresh.tsv').read_text().splitlines()[1:]]
    def section(name):
        for part,key,kind,pre in fresh:
            if part==name:bindings[key]=P.fresh(_Scope(pre),kind)
        rules(g,root,'librarysymbols',bindings,section=name)
    section(cfg+'s1')
    # Explicit finite framing rules. Token reader leaves separators unread.
    rules(g,root,'librarysymbols',section='scan')
    section(cfg+'s2')
