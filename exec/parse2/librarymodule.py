"""K2 library/module manifest entrypoint (gen2 keeps the call site)."""
from pathlib import Path
import assemble


def install(E, P, start):
    named = E.results
    facts = assemble.load_facts('k2-librarymodule-map')
    env = dict(start=start, lm_hstate=named['lm_hstate'], lm_hnext=named['lm_hnext'],
               lm_header=list(named['lm_header']), lm_main=named['lm_main'],
               lm_initret=named['lm_initret'], lm_tailret=named['lm_tailret'],
               normal=named.get('lm_mainnext', 'DEAD'),
               reject=list(named['lm_mainreject']) if 'lm_mainreject' in named
                      else E.rej(facts['mainreason']),
               ret=E.O('  ret\n'))
    assemble.run(Path(__file__).with_name('librarymodule-manifest.tsv'), E, P, {}, env)
    return 'LMD.start'
