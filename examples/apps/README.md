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
| `procview.c` | a process-tree analyser: on Linux, with no argument, real data straight from `/proc/N/status` for every `N` (no subprocess -- this compiler's syscall catalog has no directory listing or `fork`/`exec`, so it scans instead of reading the directory); elsewhere, or off `ps -axo pid=,ppid=,rss=,comm=`. Parent lookup, subtree sums by walking ancestors with a depth cap (so parent cycles cannot loop), `qsort` on index arrays with three comparators, orphan / self-parent / cycle detection |
| `winlayout.c` | window-stack analysis: exact visible area per window by coordinate compression and a topmost-owner grid, off-screen clipping, overlap pairs, the largest empty rectangle, a text minimap |
| `memmap.c` | address-space analysis of a Linux `/proc/PID/maps` listing: hand-written unsigned 64-bit hex parsing (kernel-half addresses), region classification, image grouping, W+X / overlap / hole audit |
| `exeinfo.c` | dissects ELF64, Mach-O (thin and fat), PE32+ and the compiler's own polyglot `unisacc.com`; every field goes through a bounds-checked reader; with no argument it builds one sample of each format in memory |

Each program runs with no input and has deterministic output. The four system
tools also take real input, and their default output does not depend on the
machine:

```
ps -axo pid=,ppid=,rss=,comm= | unisacc -run examples/apps/procview.c -
cat /proc/self/maps           | unisacc -run examples/apps/memmap.c -
unisacc -run examples/apps/winlayout.c layout.txt     # "screen W H", then "X Y W H title", bottom to top
unisacc -run examples/apps/exeinfo.c unisacc.com /bin/ls
```

`procview.c` was also run on a live 685-process table (piped `ps`, on macOS)
and, after adding the `/proc` scan, on a live 171-process Linux VM with no
argument at all (0.21 s wall, real kernel data, no `ps`); `exeinfo.c` was run
on the compiler's own output (`unisacc.com`, and a Mach-O it had just
written). Host `cc` and `unisacc.com -run` gave identical output each time.
The output was
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

Evidence provenance: product C source `7d50875`, delta generators/runtime
`1775315`; `exec/pipeline/elf.sh` produced
`/tmp/unisacc-self-route/final-pipeline/unisacc.elf` (671,404 B), SHA256
`d8f7586e1d0250064c248cfbb1bfe8ad8dc8021abf6f9f6e86da5d50b58d1979`.
In Lima `minicon-lnx-x86_64`, this image was named `n1`; each application ran
as `timeout 10 ./n1 -run examples/apps/NAME.c`, with stdout compared to the
host cc result. The same image rebuilt unisacc.c twice with
`-O2 -b lnx/x86_64`; N1, N2 and N3 have that same hash. These runtime checks
do not claim that the application sources themselves passed through all six
deltas; they were compiled by the resulting C compiler.
