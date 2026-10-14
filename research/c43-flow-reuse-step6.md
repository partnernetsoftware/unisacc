# 0.0.43 step 6, batch 1 of the outside-name-list audit (strace per job, <=55 s, serial)

| job | strace | reads outside the name list | result
|---|---|---|---|
| tools-1..3 | rc0 | corpus/crypto-algorithms/* (gitignored, fetched at a pinned commit in tests/tools.sh) | external pinned input, no declaration |
| decision-ledger | rc0 | research/decision-ledger.json | declared (files) |
| exittable | rc0 | archive/plans/v0.0.38.md, plans/v0.0.42.md | declared (plans/ tree: version-keyed) |
| k2b | rc124 at 55 s (partial) | apps/*, archive/*, ujs/*, docs/*, plans/*, research/*, bench/*, .github/* | declared (trees: it walks every tracked file, tests/k2bcheck.py:10) |
| h1source | rc0 | none | static false positive |
| precommithook | rc0 | none | static false positive |
| rowcov-lex | rc0 | none | static false positive |
| rowcov-pp-locations | rc124 at 55 s (partial) | none in the part read | incomplete, not declared |
| rowcov-pp-shared | rc124 at 55 s (partial) | none in the part read | incomplete, not declared |
| rowcov-parse2-build | rc124 at 55 s (partial) | none (exec/parse2/../ paths normalise into exec/) | incomplete, not declared |

## batch 2 (strace, rc as shown)
| job | strace | reads outside the name list | result |
|---|---|---|---|
| seedpy, ccinterop, c99-ledger, exec-bridge-linux, rowcov-lower-build | rc0 | none | none |
| realprog | rc0 | corpus/cJSON, corpus/jsmn, corpus/lua... (pinned external, realprog.sh) | external pinned, no declaration |
| com-tools | rc0 | corpus/crypto-algorithms (pinned external, tools.sh) | external pinned, no declaration |
| gate-infra | rc1 | unisacc.com, unisacc.com.build.json (root product) | declared (files) |
| com-tapedomain, com-latebind | rc1 | unisacc.com (root product) | declared (files + executable_inputs) |

Root-product jobs without MODEL_COM: 95. 17 undeclared (no product identity). Not fixed here; gatequeue change needed (product file is gitignored).
