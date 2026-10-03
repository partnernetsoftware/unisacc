"""E1 source provenance envelope; ordinary actions only, lexer payload unchanged."""
from pathlib import Path
from finite_rules import load, install_template
from exec.facts.load import facts as fact_table


def install(d, lexer_start, facts):
    here = Path(__file__).resolve().parent
    # envelope edits over the lexer graph: sourcefacts-template.tsv (domain facts: skip keywords)
    install_template(d, here, 'sourcefacts', facts, None, mode='b')
    for filename, mode, domain in ((r['file'], r['mode'], range(r['domain']))
                                   for r in fact_table('lex-installs') if r['group'] == 'sourcefacts'):
        for name, entries in load(here / filename, {}, domain).items():
            row = d.state(name, mode)
            for key, (target, actions) in entries.items():
                # Table immediates are signed i64; retain identical u64 bits.
                actions = [(*a[:-1], a[-1] - (1 << 64))
                           if a[0] == 'A64I' and a[-1] > (1 << 63) - 1 else a
                           for a in actions]
                row[key] = (lexer_start if target == '@lexer' else target), d.seq(actions)
    return 'SF.start'
