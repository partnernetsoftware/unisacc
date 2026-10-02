# R20-3b: first differing bytes, reference vs network chain (2026-10-02)

Tree: HEAD f91aa86 **plus uncommitted edits by another agent** (tapebin*, exec/parse2/unitmode.py, unisa/tape*.py, prd.md). E3 is generated from that dirty tree, so the result describes the working tree, not a commit.
All results below were **run**, except where marked *inferred*.

## Method (run)

Same as the gate `exec/c/chain.sh` (that is what reads `exec/c/chain.knownfail`; also in tests/gatedeps.json, tests/tapebin*.py), but every product goes to a scratch dir and nothing in exec/build or the tree is rebuilt:

    S=<scratch>; B() { python3 tests/bound.py 55 "$@"; }
    B sh tests/build_ref.sh $S/ref.c $S/ua                       # private reference
    B cc -O2 -std=c99 -w -o $S/run exec/c/run.c                    # generic executor
    B python3 exec/pp/gen.py $S/e2.json; B python3 exec/lex/gen.py --typed $S/e1.json; B python3 exec/parse2/gen2.py $S/e3.json
    for s in e2 e1 e3; do B python3 exec/c/tbl.py $S/$s.json $S/$s.tbl; B python3 exec/c/net.py $S/$s.tbl $S/$s.net; B $S/run --check-net $S/$s.tbl $S/$s.net; done
    # per file f: reference tape, then e2 -> e1 -> e3 through threshold networks (NETWORK=1, the gate default)
    $S/ua f -S -o - > ref
    $S/run $S/e2.net f f include > e2; $S/run $S/e1.net e2 f > e1; $S/run $S/e3.net e1 f > e3
    cmp ref e3; diff ref e3

All stages exit 0 for all three files; reference rc 0.

## 1. tests/c/b_compound.c -- differs at byte 124033, line 7384

Source: `struct S *gs = &(struct S){1, 2};` (l.6), `struct T gt = {9, &(struct S){3, 4}};` (l.7).

Reference 7381-7389:

    7381:   .frame -8
    7382:   ret
    7383: .bss g_gs 8
    7384: .bss __cl3857 8        <-- first diff
    7385: .bss g_gt 16
    7386: .bss __cl3875 8
    7387: sum:

Network 7381-7387:

    7381:   .frame -8
    7382:   ret
    7383: .bss g_gs 8
    7384: .bss g_gt 16
    7385: sum:

and later, inside `__init`, the network emits the records the reference omitted there (ref 7652-7655 vs net 7650-7654):

    ref:  .st [r1+0], r0, 4 / .lea r1, __cl3857 / .zero r1, 0, 8
    net:  .st [r1+0], r0, 4 / .bss __cl3857 8 / .lea r1, __cl3857 / .zero r1, 0, 8
    (same again for __cl3875: ref 7673, net 7672-7673)

The whole diff is only those 4 moved lines. Names (`__cl3857`, `__cl3875`), sizes and the init code are identical.
**Diagnosis:** this is about *where the data record goes*. The reference puts a file-scope compound literal's `.bss` record in the global-definition list, right after the object that refers to it (`g_gs`, then `__cl3857`), before the first function. The network emits it lazily, where the literal's initialiser code is generated inside `__init`, so a `.bss` directive lands between instruction records. Both sides name the literal the same way and initialise it the same way.

## 2. tests/c/b_pp2.c -- differs at byte 126620, line 7541

Source l.18: `struct S s = ((struct S){7, 8});`.

Reference 7538-7551:

    7538:   imm r2, 12
    7539:   add64 r1, r1, r2
    7540:   .st [r1+0], r0, 4
    7541:   .lea r1, g_s            <-- first diff
    7542:   .zero r1, 0, 8
    7543:   imm r0, 7
    7544:   .lea r1, g_s
    ...
    7550:   .st [r1+0], r0, 4
    7551:   .lea r1, g_braced

Network 7538-7563:

    7538:   imm r2, 12
    7539:   add64 r1, r1, r2
    7540:   .st [r1+0], r0, 4
    7541: .bss __cl3816 8
    7542:   .lea r1, __cl3816
    7543:   .zero r1, 0, 8
    7544:   imm r0, 7
    ...
    7552:   .lea r0, __cl3816       # then an 8-byte copy __cl3816 -> g_s
    7553:   mov r3, r0 / .lea r0, g_s / .frame 8 / store64 / ... / load64 r2,[r1+0] / store64 [r0+0], r2
    7563:   .lea r1, g_braced

**Diagnosis:** this is about *whether the literal exists*, not what it is called. When a whole (parenthesised) compound literal is the initialiser of an object of the same struct type, the reference drops the literal and initialises `g_s` in place: there is no `__cl` symbol in the reference tape at all. The network makes an anonymous object `__cl3816`, initialises it, and copies it into `g_s` (11 extra lines). The knownfail note ("distinct compound literal symbol address") describes this. On top of that, the network's `.bss __cl3816` sits inside `__init`, the same placement pattern as case 1 (*inferred*: if the reference did keep the literal, case 1's rule would place it with the globals).

## 3. tests/c/fb12-13-extern-incomplete-array.c -- NO difference

`cmp` is silent: the tapes are byte-identical (7483 lines). The extern-then-define ordering split named in r20-3-link-semantics.md section 5 does **not** reproduce through chain.sh's E2/E1/E3 networks on this tree. It is not in chain.knownfail. If it diverges elsewhere (the product .com, the -funit path, or the tree before db93239/00da7aa), that path was not exercised here (*inferred*, not run).

## One root cause? (bytes only)

No. The diffs that reproduce are two different mechanisms, and both involve file-scope compound literals:
- b_compound: same symbols and code; only the `.bss` record placement differs (global list vs lazy, inside `__init`).
- b_pp2: the reference elides the literal (direct init of g_s); the network materialises it and copies. Its placement inside `__init` matches b_compound, but fixing placement alone would not make b_pp2 equal.
- fb12-13: no difference, so it shares nothing at the byte level.

One unified "`__cl` order/renumbering" rule would therefore close b_compound only. b_pp2 also needs the network to elide the literal when it is a whole same-type initialiser (or the reference to stop eliding it).
