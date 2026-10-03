"""Explicit library/module=LE64(1): no process startup, fixed __init role.
Missing/zero preserves program output. __init is generated, zero-argument,
soft-stack ABI, called once per mapping by the host's declared adapter.
"""
from pathlib import Path
from modelinput import u64
import json
from finite_rules import install as rules, install_template

def _template(E,name,facts={}):
    install_template(E.g,Path(__file__).parent,'librarymodule',facts,None,section=name)

def install(E,P,start):
    g=E.g
    u64(E,'LMD.read',b'\0library/module','library_module','lmd_present','LX.fail')
    rules(g,Path(__file__).parent,'librarymodule',section='read',bindings=dict(start=start))
    _template(E,'start')
    # Split only the declared startup output, preserving positioning/continuation.
    header=list(E.O(E.HEADER)); hits=[]
    for state,(mode,row) in list(g.st.items()):
        for k,(n,q) in list(row.items()):
            seq=list(g.seqs[q])
            for i in range(len(seq)-len(header)+1):
                if seq[i:i+len(header)]==header:hits.append((state,k,n,seq[:i],seq[i+len(header):]))
    assert len({h[0] for h in hits})==1 and hits
    state,k,n,before,after=hits[0]
    for state,k,nn,bb,aa in hits:
        assert (nn,bb,aa)==(n,before,after)
    _template(E,'split',{'H':[dict(s=state,k=k,before=json.dumps([list(x) for x in bb])) for state,k,nn,bb,aa in hits]})
    rules(g,Path(__file__).parent,'librarymodule',section='header',bindings=dict(next=n),sequences=dict(header=header,tail=after))
    # Main requirement and init-tail are existing finite control points.
    candidates=[name for name,(mode,row) in g.st.items() if any(n=='END.ok' for n,q in row.values())]
    assert len(candidates)==1,candidates
    state=candidates[0];mode,row=g.st[state]
    reject=None;normal=None;keys=[]
    for k,(n,q) in list(row.items()):
        if n!='END.ok':
            actions=list(g.seqs[q])
            assert normal is None or (normal,reject)==(n,actions)
            normal,reject=n,actions;keys.append(k)
    assert normal is not None
    _template(E,'main',{'state':[state],'key':keys})
    seq=list(g.seqs[next(iter(g.st['END.ok'][1].values()))[1]])
    state=next(a[1] for a in reversed(seq) if a[0]=='PUSH')
    _template(E,'initmove',{'state':[state]});_template(E,'initend',{'state':[state]})
    seq=list(g.seqs[next(iter(g.st['END.x2'][1].values()))[1]])
    tail=next(a[1] for a in reversed(seq) if a[0]=='PUSH')
    rules(g,Path(__file__).parent,'librarymodule',section='finish',bindings=dict(tail=tail,normal=normal),sequences=dict(ret=E.O('  ret\n'),reject=reject))
    # __init is reserved only for explicit modules; source cannot counterfeit role.
    _template(E,'defmove');_template(E,'definition')
    rules(g,Path(__file__).parent,'librarymodule',section='reserved')
    return 'LMD.start'
