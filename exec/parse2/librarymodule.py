"""Explicit library/module=LE64(1): no process startup, fixed __init role.
Missing/zero preserves program output. __init is generated, zero-argument,
soft-stack ABI, called once per mapping by the host's declared adapter.
"""
from pathlib import Path
from modelinput import u64
from finite_rules import install as rules

def install(E,P,start):
    g=E.g
    u64(E,'LMD.read',b'\0library/module','library_module','lmd_present','LX.fail')
    rules(g,Path(__file__).parent,'librarymodule',section='read',bindings=dict(start=start))
    P('LMD.start').a(('PUSH','LMD.readback')).goto('LMD.read')
    g.labels.add('LMD.readback')
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
        g.st[state][1][k]=('LMD.header',g.seq(before))
    rules(g,Path(__file__).parent,'librarymodule',section='header',bindings=dict(next=n),sequences=dict(header=header,tail=after))
    # Main requirement and init-tail are existing finite control points.
    candidates=[name for name,(mode,row) in g.st.items() if any(n=='END.ok' for n,q in row.values())]
    assert len(candidates)==1,candidates
    state=candidates[0];mode,row=g.st[state]
    reject=None;normal=None
    for k,(n,q) in list(row.items()):
        if n!='END.ok':
            actions=list(g.seqs[q])
            assert normal is None or (normal,reject)==(n,actions)
            normal,reject=n,actions;row[k]=('LMD.main',g.seq([]))
    assert normal is not None
    seq=list(g.seqs[next(iter(g.st['END.ok'][1].values()))[1]])
    state=next(a[1] for a in reversed(seq) if a[0]=='PUSH')
    g.st['LMD.original.initend']=g.st.pop(state);g.labels.add('LMD.original.initend')
    P(state).goto('LMD.initend')
    seq=list(g.seqs[next(iter(g.st['END.x2'][1].values()))[1]])
    tail=next(a[1] for a in reversed(seq) if a[0]=='PUSH')
    rules(g,Path(__file__).parent,'librarymodule',section='finish',bindings=dict(tail=tail,normal=normal),sequences=dict(ret=E.O('  ret\n'),reject=reject))
    # __init is reserved only for explicit modules; source cannot counterfeit role.
    original=g.st.pop('LX.definition');g.st['LMD.original.definition']=original;g.labels.add('LMD.original.definition')
    actions=[('SBCLR',),*[('SBOUT',c) for c in b'__init'],('SBINTERN','lmd_initid'),('INTERN','lmd_fnid','fns','fne')]
    g.st['LX.definition']=('r',{k:('LMD.definition',g.seq(actions)) for k in range(257)})
    rules(g,Path(__file__).parent,'librarymodule',section='reserved')
    return 'LMD.start'
