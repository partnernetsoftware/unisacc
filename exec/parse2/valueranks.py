"""Semantic FP identity accompanies existing value descriptors/actions.
Storage remains unchanged. Pointer descriptors carry underlying identity only
internally; call-site serialization emits rank zero for non-FP descriptors.
"""
import sys as _s; from pathlib import Path as _P; _s.path.insert(0, str(_P(__file__).resolve().parent.parent / 'facts')); from load import facts
globals().update((r['name'],r['value']) for r in facts('valueranks') if r['kind']=='bank')
RANKOF={r['name']:r['value'] for r in facts('valueranks') if r['kind']=='rank'}
def slots(items):
    result=[]
    for item in items:
        result.append(item)
        companion=RANKOF.get(item)
        if companion and companion not in items:result.append(companion)
    return result

def install(E,P,b):
    import sys as _s,pathlib as _p
    _s.path.insert(0,str(_p.Path(__file__).resolve().parents[1]/"facts"))
    from load import facts as _f
    _LX={r["name"]:r["value"] for r in _f("libraryexports")}
    RETURNRANK,PARAMRANK,MEMBERRANK,TYPERANK=(_LX[k] for k in ("RETURNRANK","PARAMRANK","MEMBERRANK","TYPERANK"))
    import libraryexports,layoutfacts,libraryimports,librarycallables,types,assemble
    modelsignature=types.SimpleNamespace(**assemble.load_facts('top-modelsignature-banks')['top-modelsignature-banks!'])  # exec/facts/top-modelsignature-banks.tsv
    modelgraphequality=types.SimpleNamespace(**assemble.load_facts('top-modelgraphequality-banks')['top-modelgraphequality-banks!'])  # exec/facts/top-modelgraphequality-banks.tsv
    owned={VALUEBANK,NAMEDRETURN,LVSITERANK,LCSITERANK,COMMONBANK}
    for owner in (modelsignature,modelgraphequality,libraryexports,layoutfacts,libraryimports,librarycallables):
        assert not owned.intersection(v for k,v in vars(owner).items() if type(v) is int and v>=1<<40),owner.__name__
    g=E.g
    from pathlib import Path
    from finite_rules import install_template
    root=Path(__file__).parent
    holder=P('VR.fresh')  # unregistered: only names fresh labels
    def declare(section):
        install_template(g,root,'valueranks',{},holder.fresh,bindings=dict(FPS_FN=b['FPS_FN'],PARAMRANK=PARAMRANK),section=section)
    declare('common')
    poolrank={E.BASE:VALUEBANK,E.FRB:NAMEDRETURN,b['MBS']:MEMBERRANK,b['FPS_RB']:RETURNRANK}
    # Companions follow ordinary actions in one pass (valueranks-template.tsv,
    # ranks): facts are the slot->rank map, pool->rank map, FP type codes and
    # the common-rank table max(l,r) over four ranks (prepended at START).
    ranks=[dict(slot=k,rank=v) for k,v in RANKOF.items()]
    install_template(g,root,'valueranks',dict(m=[dict(key=l*4+r,value=max(l,r)) for l in range(4) for r in range(4)],
        cb=[COMMONBANK],s=ranks,t=ranks,p=[dict(pool=k,rank=v) for k,v in poolrank.items()],f=[b['DBL'],b['FLT']]),
        holder.fresh,section='ranks')
    # Hooks: each original moves to VR.original.*, an alias calls the rank
    # procedure and continues there (valueranks-template.tsv, in graph order).
    for section in ('h1','h2','h3','h4','h5','h6','h7'):declare(section)
    g.labels.update('VR.original.'+s for s in ('QT.original','QT.merge','CL.typed','CL.a2'))
