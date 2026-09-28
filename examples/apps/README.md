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
| `procview.c` | a process-tree analyser: macOS libproc queried by the application itself, or a bounded `/proc/N/status` scan on Linux. Parent lookup, subtree sums by walking ancestors with a depth cap (so parent cycles cannot loop), `qsort` on index arrays with three comparators, orphan / self-parent / cycle detection |
| `winlayout.c` | queries live CoreGraphics/CF windows on macOS through explicit FFI, then computes visibility, overlaps, largest empty rectangle and minimap; also accepts captured geometry |
| `winlist.c` | queries the real macOS window list through `unisacc_ffi`, reports IDs, applications/PIDs, layers, visibility, alpha, bounds and titles; groups by application/layer and ranks visible windows by area. Other platforms explicitly reject |
| `memmap.c` | address-space analysis of a `/proc/PID/maps` listing: hand-written unsigned 64-bit hex parsing (kernel-half addresses), region classification, image grouping, W+X / overlap / hole audit. Linux defaults to its own `/proc/self/maps`; macOS queries this application's own regions through libproc. Identical snapshots are compared against cc; live self maps are checked structurally |
| `exeinfo.c` | dissects ELF64, Mach-O (thin and fat), PE32+ and the compiler's own polyglot `unisacc.com`; every field goes through a bounds-checked reader; reads the files named on the command line; with none, dissects this host's own system executables (`/bin/ls` and `/usr/lib/dyld` on macOS, the running image `/proc/self/exe` and `/bin/ls` on Linux, `cmd.exe`/`kernel32.dll` on Windows) |
| `colorpack.c` | bit-field packed pixel formats (RGB565/555, RGBA4444): quantisation, round-trip error, per-channel histograms. Every field value matches host `cc`; `sizeof` currently does not (a filed, unfixed defect -- see the file comment) |

The four system tools consume real input. On macOS, procview, memmap and
winlayout query system APIs themselves through `unisacc_ffi.h`. No system-cc
collector participates in these commands.

```sh
./unisacc.com -run examples/apps/procview.c
./unisacc.com -run examples/apps/memmap.c
./unisacc.com -run examples/apps/winlayout.c
./unisacc.com -run examples/apps/winlist.c
./unisacc.com -run examples/apps/winlist.c -- --all
./unisacc.com -run examples/apps/exeinfo.c
./unisacc.com -run examples/apps/exeinfo.c unisacc.com /bin/ls
./unisacc.com -run examples/apps/memmap.c -- --capture > maps.txt
```

The former collector launcher has been removed. Explicit files or `-` remain available
for reproducible analysis tests. Linux procview and memmap retain their
`/proc` paths; live window collection currently requires macOS. Permission
restrictions, vanished processes and collection limits are reported: on macOS
procview lists every process (other users' via the unprivileged short BSD
record) and shows their resident size as `?`, because only the setuid-root
`ps` may read other users' task information.
There is no synthetic fallback.

The macOS bridge resolves system symbols using dlopen/dlsym and calls them
through libffi with explicit native types. This adds real foreign calls;
it does not mean all bundled libc functions have been replaced. Bundled
FILE and va_list objects never cross into system libc. The former system-cc
collectors have been removed. `--capture` exports the live raw snapshot
from each application for reproducible analysis tests.

`tests/appsrealcheck.py` uses cc only as a test reference. Captured inputs
are compared byte for byte; live defaults are checked structurally because
processes, windows and each executable's own maps vary between runs.


The older six application checks below are historical evidence:

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
