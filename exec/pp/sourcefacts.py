"""Opt-in E2 provenance envelope; all work uses existing generic actions."""
from pathlib import Path
from finite_rules import install as install_rules

def install(g):
    # START is entered once; P3/include rescan paths cannot reset sticky facts.
    assert 'SF.original.START' not in g.st
    g.st['SF.original.START']=g.st.pop('START')
    g.labels.add('SF.original.START')
    # Final acceptance only: located output is already complete at this point.
    for state,(mode,row) in list(g.st.items()):
        for key,(target,seq) in list(row.items()):
            actions=list(g.seqs[seq])
            if ('ACCEPT',) in actions:
                assert actions[-1]==('ACCEPT',),(state,actions)
                assert actions.count(('ACCEPT',))==1
                row[key]=('SF.finish',g.seq(actions[:-1]))
    install_rules(g,Path(__file__).parent,'sourcefacts')
    g.st['START']=g.st['SF.start']
