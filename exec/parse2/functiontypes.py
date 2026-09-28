"""Bind function-signature facts to the existing parser and conversion controls."""
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P, bindings, integers):
    from libraryexports import PARAMDEPTH, PARAMBASE, PARAMSHAPE, PARAMMARK, SIGEPOCH, VARIADIC
    bindings = dict(bindings, PARAMDEPTH=PARAMDEPTH, PARAMBASE=PARAMBASE, PARAMSHAPE=PARAMSHAPE, VARIADIC=VARIADIC)
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
    P('FS.params.result').a(('LDI','td',1),('COPYW','tb','fs_code'),('LDI','type_shape',0)).ret()
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
