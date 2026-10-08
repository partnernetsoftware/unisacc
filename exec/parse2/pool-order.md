# String pool traversal order

E59: E3 emits a for loop body before its step. The final pool scan must use
that same order; source-order labels silently associate loads with wrong data.

The loops table records three boundaries keyed by the step token prefix offset:

| Sparse bank base | Stored value |
| --- | --- |
| 1125899906842624 (2^50) | closing parenthesis offset |
| 1126999418470400 (2^50 + 2^40) | body start offset |
| 1128098930098176 (2^50 + 2*2^40) | first token after body |

These banks are disjoint from existing parse2 banks; offsets must stay below
2^40, and keys stay exactly representable below 2^53. At PO.l the pool consumes
the recorded body range, then the step range using jumps on the original token stream, then resumes after the body.
The mapping is consumed before recursion so the step cannot recursively defer
itself. Five range registers are saved on the existing value stack. Nested
loops therefore use the same rule. PO.l checks the exclusive token boundary
before consuming a token, without slicing away token-dump terminators. Empty steps and unevaluated/skipped strings
retain the existing pool filtering rules. No tape or executor opcode changes.

Regression: tests/c/b_forstep_strings.c exercises strtok/sprintf, nested loops,
formatted calls, adjacent literals, __func__, sizeof, and an empty step.
