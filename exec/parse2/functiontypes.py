"""Bind function-signature facts to the existing parser and conversion controls."""
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P, bindings, integers):
    from libraryexports import PARAMDEPTH, PARAMBASE, PARAMSHAPE, PARAMMARK, SIGEPOCH, VARIADIC, RETURNRANK, PARAMRANK, PREVRETURN, RETURNPENDING
    bindings = dict(bindings, PARAMDEPTH=PARAMDEPTH, PARAMBASE=PARAMBASE, PARAMSHAPE=PARAMSHAPE, VARIADIC=VARIADIC, RETURNRANK=RETURNRANK, PARAMRANK=PARAMRANK)
    root = Path(__file__).parent
    contexts = [line.split('\t') for line in (root / 'functiontypes-context.tsv').read_text().splitlines()[1:]]
    sequences = dict(reject=E.rej('not covered: function signature pool exhausted'))
    p = P('functiontypes.bindings')
    for group, slots in contexts:
        for name, method in (('save', 'vpush'), ('restore', 'vpop')):
            p.acts = []
            sequences[name if group == 'params' else group + '_' + name] = getattr(p, method)(*slots.split(',')).acts
    install_rules(E.g, root, 'functiontypes', bindings=bindings, sequences=sequences, classes=dict(narrow=[code for _,code,size,_,_ in integers if size < 8]), section='main')

    # Full signature queries use the same declaration-owned pool as exports.
    # Rule installation expands all observations; remove the old row first.
    E.g.st.pop('FS.query')
    P('FS.query').a(('LDX','fs_querycount','call_sig',bindings['FPS_COUNT'])).branch(
        {0:'FS.queryindex'},'FS.querydefault',[('CMP','na','fs_querycount')])
    P('FS.querydefault').a(('LDI','t',0),('CMPI','t',0)).goto('CL.signature')
    P('FS.queryindex').a(('ALUI','mul','fs_queryindex','call_sig',1024),
        ('ALU','add','fs_queryindex','fs_queryindex','na'),
        ('LDX','fs_querymark','fs_queryindex',PARAMMARK),('LDX','fs_queryepoch','call_sig',SIGEPOCH)).branch(
        {1:'FS.querytype'},'FS.querymissing',[('CMP','fs_querymark','fs_queryepoch')])
    P('FS.querymissing').a(E.rej('not covered: anonymous parameter facts missing')).goto('DEAD')
    P('FS.querytype').a(('LDX','t','fs_queryindex',PARAMDEPTH),('ALUI','mul','t','t',4096),
        ('LDX','fs_querybase','fs_queryindex',PARAMBASE),('ALU','add','t','t','fs_querybase'),('CMPI','t',0)).goto('CL.signature')
    P('FS.params.mode').branch({2:'FS.params.stacked'},'FS.params.modevar')
    P('FS.params.modevar').branch({1:'FS.params.result'},'FS.params.stacked',[('CMPI','fs_var',0)])
    P('FS.params.stacked').a(('LDI','fs_var',1),('STX','fs_code',bindings['FPS_VAR'],'fs_var')).goto('FS.params.result')
    P('FS.params.result').a(('LDI','td',1),('COPYW','tb','fs_code'),('LDI','type_shape',0),('LDI','type_rank',0)).ret()
    # Coinductive signature equality: bounded visits permit recursive edges.
    P('FS.TYPEEQ').branch({1:'FS.eq.root'},'FS.eq.enter',[('CMPI','fs_eqdepth',0)])
    P('FS.eq.root').a(('ALUI','add','fs_eqepoch','fs_eqepoch',1),('LDI','fs_eqnodes',0)).goto('FS.eq.enter')
    P('FS.eq.enter').branch({2:'FS.eq.exhausted'},'FS.eq.recurse',[('CMPI','fs_eqdepth',31)])
    E.g.labels.add('FS.eq.finish')
    P('FS.eq.recurse').a(('ALUI','add','fs_eqdepth','fs_eqdepth',1),('PUSH','FS.eq.finish')).goto('FS.typeeq.test')
    P('FS.eq.finish').a(('ALUI','sub','fs_eqdepth','fs_eqdepth',1),('RLD','fs_result')).ret()
    P('FS.eq.exhausted').a(E.rej('not covered: recursive signature equality limit')).goto('DEAD')
    P('FS.eq.visited').a(('ALUI','mul','fs_eqpair','fs_l',4096),('ALU','add','fs_eqpair','fs_eqpair','fs_r'),
        ('LDX','fs_eqmark','fs_eqpair',82 << 40)).branch({1:'FS.yes'},'FS.eq.visit',[('CMP','fs_eqmark','fs_eqepoch')])
    P('FS.eq.visit').branch({2:'FS.eq.exhausted'},'FS.eq.register',[('CMPI','fs_eqnodes',1023)])
    P('FS.eq.register').a(('ALUI','add','fs_eqnodes','fs_eqnodes',1),('STX','fs_eqpair',82 << 40,'fs_eqepoch')).goto('FS.eq.fields')

    # Compare retained semantic ranks before existing structural base equality.
    def rankhook(state,entry):
        old='FS.rank.original.'+state;E.g.st[old]=E.g.st.pop(state);E.g.labels.add(old)
        alias='FS.rank.hook.'+state;P(alias).goto(entry);E.g.st[state]=E.g.st[alias]
        return old
    old=rankhook('FS.eq.field2.test','FS.rank.return')
    P('FS.rank.return').branch({1:'FS.rank.returnread'},old,[('CMP','eq_x','eq_y')])
    P('FS.rank.returnread').a(('LDX','rank_x','fs_l',RETURNRANK),('LDX','rank_y','fs_r',RETURNRANK)).branch({1:old},'FS.no',[('CMP','rank_x','rank_y')])
    old=rankhook('FS.eq.param.base','FS.rank.param')
    P('FS.rank.param').a(('ALUI','mul','rank_key','fs_l',1024),('ALU','add','rank_key','rank_key','eq_i'),('LDX','rank_x','rank_key',PARAMRANK),('ALUI','mul','rank_key','fs_r',1024),('ALU','add','rank_key','rank_key','eq_i'),('LDX','rank_y','rank_key',PARAMRANK)).branch({1:old},'FS.no',[('CMP','rank_x','rank_y')])
    old=rankhook('FS.decl.test','FS.rank.decl')
    P('FS.rank.decl').branch({1:old},'FS.rank.declread',[('CMPI','fs_code',0)])
    P('FS.rank.declread').a(('LDX','rank_x','fs_code',RETURNRANK)).branch({1:'FS.rank.declbase'},'FS.rank.fail',[('CMP','rank_x','return_rank')])
    P('FS.rank.declbase').a(('LDX','rank_oldreturn','fs_code',bindings['FPS_RB']),('STX','fs_code',PREVRETURN,'rank_oldreturn'),('LDI','rank_one',1),('STX','fs_code',RETURNPENDING,'rank_one')).goto('FS.rank.declfinish')
    P('FS.rank.declfinish').a(('CMPI','fs_code',0)).goto(old)
    P('FS.rank.fail').a(E.rej('not covered: incompatible semantic declaration type')).goto('DEAD')

    # New semantic rank checks retain the historical primitive-base policy.
    # Only callback nodes need recursive structural comparison here.
    P('FS.rank.baseequal').branch({(1,2):'FS.rank.baselow'},'FS.rank.baseright',[('CMPI','fs_l',bindings['FPS_FIRST'])])
    P('FS.rank.baselow').branch({0:'FS.rank.baserecurse'},'FS.rank.baseright',[('CMPI','fs_l',bindings['SBB'])])
    P('FS.rank.baseright').branch({(1,2):'FS.rank.baserightupper'},'FS.rank.baseyes',[('CMPI','fs_r',bindings['FPS_FIRST'])])
    P('FS.rank.baserightupper').branch({0:'FS.rank.baserecurse'},'FS.rank.baseyes',[('CMPI','fs_r',bindings['SBB'])])
    P('FS.rank.baserecurse').call('FS.TYPEEQ').ret()
    P('FS.rank.baseyes').a(('LDI','fs_result',1)).ret()
