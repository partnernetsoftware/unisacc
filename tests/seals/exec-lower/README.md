# exec-lower stream leaf (tape.text -> target.text)

Measured lower cell for `tests/streamseal.cx` + `cell.tsv`. Input tapes are
`prune.net(ua_ref -O2 -S)` on hello/fib/fact/a_char (upstream e3/opt path is
fixture-only, not in the closure). pp is never a dependency.

Gate: `tests/gate.sh` job `lower-seal`.

Coverage: 4 accept, 0 refuse. Reference bytes frozen at capture (producer
lower.net on the same pruned tape).
