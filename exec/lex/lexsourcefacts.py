"""E1 source provenance envelope; ordinary actions only, lexer payload unchanged."""
from pathlib import Path
from finite_rules import load


def install(d, lexer_start):
    # Losing these recognized constructs is observable only on the skip edge.
    for name, (mode, row) in list(d.states.items()):
        for key, (target, sid) in list(row.items()):
            actions = list(d.seqs[sid])
            if name.startswith('SKW:') and key == 40:
                assert target == 'ATT'
                actions.insert(0, ('LDI', 'sf_status', 0))
            if any(a[0] == 'ACCEPT' for a in actions):
                assert actions[-1] == ('ACCEPT',)
                actions.pop()
                target = 'SF.finish'
            row[key] = target, d.seq(actions)
    here = Path(__file__).resolve().parent
    for suffix, mode, domain in (('byte', 'b', range(257)),
                                 ('result', 'r', range(20))):
        for name, entries in load(here / ('sourcefacts-' + suffix + '.tsv'), {}, domain).items():
            row = d.state(name, mode)
            for key, (target, actions) in entries.items():
                # Table immediates are signed i64; retain identical u64 bits.
                actions = [(*a[:-1], a[-1] - (1 << 64))
                           if a[0] == 'A64I' and a[-1] > (1 << 63) - 1 else a
                           for a in actions]
                row[key] = (lexer_start if target == '@lexer' else target), d.seq(actions)
    return 'SF.start'
