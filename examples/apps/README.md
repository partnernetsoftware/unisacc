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
| `procview.c` | a process-tree analyser: real process snapshots via the launcher's `ps`, or a bounded `/proc/N/status` scan in direct Linux use. Parent lookup, subtree sums by walking ancestors with a depth cap (so parent cycles cannot loop), `qsort` on index arrays with three comparators, orphan / self-parent / cycle detection |
| `winlayout.c` | window-stack analysis: exact visible area per window by coordinate compression and a topmost-owner grid, off-screen clipping, overlap pairs, the largest empty rectangle, a text minimap. No automatic real data: window-server access (X11/Wayland/Win32/CoreGraphics) needs bindings this compiler doesn't have; `tools/wingeom.c` is a small **system-cc** helper (macOS, CoreGraphics) that supplies real geometry over a pipe -- proof that real data plus this analysis works end to end, and the reference `winlayout.c` itself should eventually match if unisacc gains that access |
| `memmap.c` | address-space analysis of a `/proc/PID/maps` listing: hand-written unsigned 64-bit hex parsing (kernel-half addresses), region classification, image grouping, W+X / overlap / hole audit. Linux defaults to its own `/proc/self/maps`; macOS launcher collects real Mach regions. Identical snapshots are compared against cc; live self maps are checked structurally |
| `exeinfo.c` | dissects ELF64, Mach-O (thin and fat), PE32+ and the compiler's own polyglot `unisacc.com`; every field goes through a bounds-checked reader; reads explicit real files; launcher defaults to the actual compiler container |
| `colorpack.c` | bit-field packed pixel formats (RGB565/555, RGBA4444): quantisation, round-trip error, per-channel histograms. Every field value matches host `cc`; `sizeof` currently does not (a filed, unfixed defect -- see the file comment) |

The four system tools consume **real input**, never a fabricated fallback.
Use the bounded launcher (55 seconds including compilation and collectors):

```sh
./examples/apps/run.sh procview     # actual ps snapshot on this host
./examples/apps/run.sh memmap       # Linux: analyser's own maps; macOS: live collector's maps
./examples/apps/run.sh winlayout    # macOS: actual CoreGraphics window geometry
./examples/apps/run.sh exeinfo      # actual unisacc.com bytes
```

`APP_COM=/absolute/path/to/compiler` selects another compiler. Explicit files
or `-` are passed through unchanged. Direct C use also accepts captured input:

```sh
ps -axo pid=,ppid=,rss=,comm= | ./unisacc.com -run examples/apps/procview.c -
./unisacc.com -run examples/apps/memmap.c /proc/PID/maps
./unisacc.com -run examples/apps/winlayout.c real-layout.txt
./unisacc.com -run examples/apps/exeinfo.c unisacc.com /bin/ls
```

On Linux, direct `memmap.c` with no argument reads `/proc/self/maps`.
Direct `procview.c` still has the bounded Linux PID scan; the launcher uses
`ps` instead, avoiding its PID-range limit. On other hosts these analysers
require input. `winlayout.c` and `exeinfo.c` require input on all targets.
Missing or empty input fails rather than inventing a desktop or process list.

The bundled libc implements its own functions; it does **not** default to
forwarding system libc. The launcher explicitly uses system `ps` and, on
macOS, builds two small **system-cc-only** collectors: `tools/selfmaps.c`
uses Mach to enumerate its own actual mappings, and `tools/wingeom.c`
uses CoreGraphics to collect real bottom-to-top windows. Neither collector
is compiled by unisacc. The memory snapshot belongs to the collector, not
the analyser; regions are real but file names are not recovered by this helper.
Window access may be restricted by OS permissions; unavailable data is an
error. Other hosts can provide an explicit geometry file; no window bindings
are implied.

`python3 tests/appsrealcheck.py` captures live processes and mappings once,
then compares each analyser's host-cc, model `-run`, and O2-native output on
identical input. Its window geometry is an analytic **test fixture**, kept out
of application defaults. `--live-windows` additionally uses the real macOS
collector. Missing/empty inputs and unsupported defaults must fail. A Linux
analyser's own live mappings get structural checks: different binaries and
ASLR need not produce identical address spaces.

2026-09-28 local macOS arm64 verification: all four analysers matched host cc
through model `-run` and O2 native execution on one real process/maps/window
snapshot and the actual container; missing and empty input checks passed.
This does not claim Linux/Windows testing of the changed defaults.

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
