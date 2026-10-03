"""Semantic FP identity accompanies existing value descriptors/actions.
Storage remains unchanged. Pointer descriptors carry underlying identity only
internally; call-site serialization emits rank zero for non-FP descriptors.
"""
VALUEBANK,NAMEDRETURN,LVSITERANK,LCSITERANK,COMMONBANK=(i<<40 for i in range(647,652))
RANKOF={'tb':'type_rank','fp_tb':'fp_rank','rb':'return_rank','vb':'value_rank','lb':'left_rank','rvb':'right_rank','svb':'sv_rank','cast_tb':'cast_rank','ivb':'iv_rank','qn_base':'qn_rank','bc_targetvb':'bc_target_rank'}
def slots(items):
    result=[]
    for item in items:
        result.append(item)
        companion=RANKOF.get(item)
        if companion and companion not in items:result.append(companion)
    return result

def install(E,P,b):
    from libraryexports import RETURNRANK,PARAMRANK,MEMBERRANK,TYPERANK
    import modelsignature,modelgraphequality,libraryexports,layoutfacts,libraryimports,librarycallables
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
