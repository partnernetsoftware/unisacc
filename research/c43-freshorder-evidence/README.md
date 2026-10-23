# Fresh allocation order: verified cause and retained raw evidence

The first F3 queue rejected modes 0–7 because their frozen allocation logs predated the bit-field promotion repair `0fc2428b`. It added `f16b` and `f21b` to each of ten compound operators: twenty `BC/b` allocations. The bare-enum fix only adds named states and does not allocate fresh labels.

Actual bounded measurements on main `b5727e2672`:

- `seed/gen.c` SHA256 is `c1be454cf86c7d4c01af985d437c21116e5f69b880bd763b370a1461ae426187`, identical to the constructor at baseline revision `b91fdffb`.
- The existing `tests/freshorderhook.h` recorded all eight modes, first with the paired `update-fresh.tsv`/`update-result.tsv` from `b91fdffb` privately overmounted read-only, then with current tables. Only the private mount view changed; the checked tree stayed fixed.
- Every old log exactly reproduces its frozen SHA256 and row count. Every current log differs only by twenty inserted `BC/b` allocations. Removing those insertions and renumbering restores the entire old sequence; no owner/kind reorder or other insertion/deletion is accepted by `compare.c`.
- `summary.tsv` pins both complete logs and row counts. The deterministic gzip files retain the actual raw logs; `insertions.tsv` records one valid subsequence alignment. Equal adjacent `BC/b` entries are indistinguishable, so those ordinals do not purport to identify individual binding names.
- Mode 8 (`parse2/units`) already passed the original F3 queue at 336 rows and SHA256 `8505b89155be5aec576d28c235313e9c8fe897850749051bcba43bf27db6425f`; it is not part of this twenty-insertion update.

`compare.c` is the actual native audit tool. Build it with `cc -O2 compare.c -o OUT` and run `OUT OLD_LOG NEW_LOG 20` after decompressing a pair into private scratch. It validates sequential numbering, exact retained owner/kind order and the identity/count of inserted rows. It compares the canonical generated logs; it is not a general TSV grammar validator. The old SHA equality is checked separately against the original baseline, not inferred from the subsequence check.

`inputs.tsv` pins the constructor, recorder and paired current/historical table files.

The baseline update uses the existing `freshordercheck.cx --write` interface. Its generated SHA/row pairs must match these independently retained current logs. Mutation testing must first establish the matching positive baseline, then detect the existing swapped-binding mutation and restore the original manifest. This record is scoped to allocation order; it does not replace complete CONTRACT or release acceptance.

Completed checks: all eight official `--write` outputs match the independent current-log SHA/row pairs; the units row is byte-identical to the original. A separate exported tree with the new baseline passed mode 0, detected the existing swapped-binding negative, and retained the exact original manifest SHA and baseline bytes afterward. The nine dependency declarations are updated to the current checker and its shared constructor/data closure. Complete F3 acceptance remains a separate queue on the final inputs.
