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
