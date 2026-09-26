# Application examples

Larger programs than `examples/*.c`. They live in a subdirectory on purpose:
many suites glob `examples/*.c`, and these are not meant to change those
suites' inputs.

| file | what it exercises |
|---|---|
| `calc.c` | recursive descent, mutual recursion, static state, `ctype.h`, `%-24s` / `%ld` |
| `life.c` | 2-D `char` arrays, modular arithmetic on a torus, `memcpy` of a whole array |
| `wordfreq.c` | a chained hash table (FNV-1a), `malloc`/`free`, `qsort` with a comparator through `const struct entry *const *` |
| `queens.c` | backtracking with unsigned bit masks (`x & -x`, shifts), recursion |
| `bf.c` | a Brainfuck interpreter: `switch`, a bracket jump table, `unsigned char` wraparound |
| `dijkstra.c` | a binary-heap priority queue, an adjacency list in arrays, recursive path printing |

Each program runs with no input and has deterministic output. The output was
checked byte for byte against the host `cc` build, using
`unisacc.com -run FILE` and `unisacc.com -O2 FILE -o OUT`:

```
cc -std=c99 -o /tmp/x examples/apps/calc.c && /tmp/x > want
./unisacc.com -run examples/apps/calc.c > got && cmp want got
```

Independent takeover check (2026-09-26): all six compile warning-free under
host `cc -std=c99 -Wall -Wextra`. The Linux x86-64 compiler image produced by
the six-delta source-to-ELF route runs every example with `-run`; each exits 0
and its stdout equals host cc byte for byte. Execution was in the local Lima
x86-64 VM, emulated on an arm64 host. The compiler image is the existing C
compiler built through the new route, not an executor-based product switch.
The VM was stopped afterwards. These examples are not yet fixed E3 keep items.
