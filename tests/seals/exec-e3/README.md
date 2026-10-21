# exec-e3 stream leaf (tokens.typed -> tape.text)

Measured e3 cell for `tests/streamseal.cx` + `cell.tsv`. Upstream **tokens** are frozen
fixtures (from `tests/seals/exec-lex-e1/` accept stdout on the same corpus sources);
the reference tape is `ua_ref -S -o -` on the matching `.c` source. The pp stage is
input-only, never a dependency.

Gate: `tests/gate.sh` job `e3-seal` (`streamseal.cx --gate`).

Coverage: 4 accept, 0 refuse (hello, fib, fact, a_char). Reseal: same chain as
`tests/reseal-lex-e1.sh` but `gen.py parse2`, fixtures = lex-e1 `.stdout`, reference
`-S` on sources (not committed as a script in this tip).
