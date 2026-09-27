"""_Bool conversion uses the shared declared scalar classifier and output templates."""
from truth import classify, outputs, rules


def install(E, P, BOOL, DBL, FLT):
    classify(E, P, DBL, FLT, 'TO.b', 'TB.scalar', 'TB.single', 'TB.double', 'TB.float', 'TB.integer', 'TB.integer')
    outputs(E, 'bool')
    p = P('BOOLCV')
    save = p.vpush('vt', 'vb').acts
    p.acts = []
    restore = p.vpop('vt', 'vb').acts
    rules(E, 'bool-save', dict(resume=p.fresh('r')), dict(save=save, restore=restore))
    rules(E, 'bool-target', dict(BOOL=BOOL, target_test=P('BOOLTARGET').fresh('b'),
                               bool_test=P('BT.scalar').fresh('b')))
