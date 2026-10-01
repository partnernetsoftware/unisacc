# Design report: table-driven C99 compiler, problems 1-6

All claims below are **design only**. Nothing was run; no file other than the brief was read.

## 1. Diagnosis (design only)

Root cause: **the unit of verification (one stage table, one finite key domain) is not the unit of failure (a combination of constructs crossing stages, units, or the library boundary).** Three symptoms of that one cause:

- **Data encoded as tables.** The system-call catalog is open-ended *data* (op -> number per OS/arch, arg shape), but it is baked into gold tables and weights. Every addition re-derives all tables (2 calls -> 60 files / 6.5k lines). That is problem 1, and part of 5 (each rebuild re-invalidates the candidate and queue).
- **Closure is unverified.** Each table is exhaustive over its keys; the glue between stages (what key a stage sees given upstream state such as "callee returns struct", "prototype came from another unit", "symbol already defined") is not. Missing keys turn into refusals far downstream. That is problems 2, 4 and 6 (symbol order / compound-literal offsets are glue state, not table keys, so tapes agree while objects differ).
- **Whole-program facts have no stage.** Cross-unit duplicate definitions, two `main`s, cross-unit prototypes are link-level facts. With no stage owning them, they either merge silently (3) or fall into an unrelated table and refuse with a wrong label (2c, 4).

Problem 5 is mostly process, but shares the first symptom: any change to data-in-tables forces a full regenerate/rebuild, so cycle cost scales with catalog churn.

Shared-cause grouping: {1, 5-partly} = data-in-tables; {2, 4, 6} = unverified closure/glue state; {3, 2c} = missing link stage.

## 2. Improvement ideas (design only)

### I1. Move the syscall catalog out of the tables into a data section
- Idea: tables decide only *shape class* (e.g. `syscall/N-args/ret-kind`), never per-op numbers. The op->number map becomes a versioned data file (TSV) consumed by the image writer / intrinsic lowering as a lookup, identical in both routes. Adding a call = one TSV row + header body; no weight rebuild unless a new shape class appears.
- Precedent: Linux `syscall.tbl` -> generated `unistd.h`; Go `zsysnum_*.go`; musl `bits/syscall.h`.
- Smallest experiment: pick the 2 most recent additions, re-express as rows, regenerate; count changed files (target: <=3) and confirm byte-equal outputs on the 186-program corpus.
- Pressures: exhaustive verification (the lookup is outside tables; verify it by a separate trivially exhaustive check: every row, every target, both routes) and executor size (one lookup primitive).

### I2. POSIX surface via a libc shim layer per OS, not raw syscalls on macOS
- Idea: on macOS (no stable syscall ABI; two-register returns; no getcwd call), implement fork/pipe/getcwd etc. in header C over a small set of per-OS *primitive* intrinsics with declared return shapes (`ret2`), and getcwd via `open(".")+fcntl(F_GETPATH)`. Batch the backlog (fork, getpid, getcwd, waitpid, pipe, strftime, localtime_r, sigaction, setjmp, pwd.h) as one release item driven by a POSIX-subset list, not per user report.
- Precedent: Cosmopolitan libc (per-OS dispatch in C), Go runtime on darwin (moved to libSystem for stability), Zig's per-OS std.
- Smallest experiment: implement fork+pipe+waitpid on macOS arm64 using a `ret2` intrinsic; a test that forks, pipes 1 byte, waits; compare both routes.
- Pressures: self-hosting no-libc goal (Go's experience says macOS raw syscalls break across OS versions; option: allow an *optional* libSystem dynamic import on macOS only, see section 4).

### I3. Combinatorial closure generator as a gate, with refusal-guided keys
- Idea: promote the construct x type x position generator to a gated suite with a fixed seed and pairwise (covering-array) selection, so coverage is measured, not sampled. Each refused program is minimised (delta reduction) and its refusal record (I4) gives the missing key directly; that key is added to the gold table.
- Precedent: Csmith + C-Reduce; YARPGen; pairwise testing (PICT).
- Smallest experiment: pairwise over {struct-return, FILE*, float fmt, static, extern} x {return, init, assign, arg} positions; count product refusals vs reference successes; each must map to one table key.
- Pressures: 60 s per run (shard by seed range; each shard <=50 s).

### I4. Structured refusal records at the point of first miss
- Idea: when a stage cannot match, emit a record instead of letting garbage flow downstream:
  `{stage, table, key: [field=value...], construct (AST kind + source span), upstream_facts: [e.g. callee.ret=struct], route: product, reference_ok: unknown}`.
  Message names the construct: "not covered: call returning struct in return position (stage E3, key ret=struct,pos=return)". Downstream stages must never receive a "maybe" key; a stage that receives an out-of-domain key refuses with "upstream miss" plus the originating record.
- Precedent: rustc's ICE / `span_bug!` with query stack; LLVM `report_fatal_error` with pass name; Cranelift's verifier errors naming the instruction.
- Smallest experiment: wire records into the stage that produced "unknown identifier" for struct return; check the 10 generator failures all report a construct, not a downstream symptom.
- Pressures: executor size (one record writer), determinism (record fields must be stable, no addresses).

### I5. Add an explicit link/whole-program stage
- Idea: a stage whose keys are symbol facts across units: `(binding, defined?, initialised?, count, kind)`. Its table refuses/diagnoses duplicate initialised definitions, duplicate `main`, tentative-vs-initialised merging (C99 6.9.2), and checks cross-unit prototype compatibility. The same stage owns symbol-table ordering (sorted by a declared key), fixing the order part of problem 6 by construction.
- Precedent: every system linker's symbol resolution (lld `SymbolTable::insert`, ELF COMMON rules); tinycc's `tcc_add_symbol` duplicate check.
- Smallest experiment: two units both `int x = 1;` and two `main`s; product must refuse with "duplicate definition of x" in both routes; then run the 3 divergent corpus programs and see whether symbol order matches.
- Pressures: byte equality (reference must adopt the same ordering rule first).

### I6. Make glue state part of the verified key (tape-level invariants)
- Idea: the facts that flow between stages (callee return class, storage of compound literals, prototype origin) become explicit fields in the tape, and each table's key domain includes them. Then "exhaustive per table" also covers the combinations because the combination is in the key. Add a tape checker: every tape field value must lie in the next stage's key domain (closure check, static, finite).
- Precedent: typed IRs with verifiers (MLIR op verifiers, LLVM `Verifier`); attribute grammars.
- Smallest experiment: add `ret_class` to the call tape record; regenerate; static closure check lists all (producer value, consumer domain) gaps; compare gap list to generator findings.
- Pressures: table size / executor size (key domains grow multiplicatively; keep fields orthogonal and few).

### I7. Release pipeline: budget precheck and option-meaning versioning
- Idea: (a) a dry "budget pass" that runs every step against its 55 s bound on the previous candidate before freeze (already partially done); (b) options get immutable meaning: changing semantics means a new option name, old one kept with old meaning until internal suites migrate; (c) catalog/data changes (after I1) no longer rebuild weights, removing the main rebuild trigger.
- Precedent: Bazel's input hashing; Chromium's flag deprecation policy; Rust's edition model.
- Smallest experiment: replay the last release's 4 rebuild causes against the precheck; count how many it would have caught.
- Pressures: 60 s limit (precheck itself sharded).

### I8. Compound-literal storage plan as a declared, shared allocator
- Idea: the storage offset of compound literals is chosen by a single specified rule (order of first appearance, alignment-first) written as data used by both routes; the object writer consumes a plan record, not ad hoc state.
- Precedent: GCC/Clang `.compoundliteral` naming; reproducible-builds work on deterministic section layout.
- Smallest experiment: the 3 divergent programs; diff the plan records of both routes.
- Pressures: byte equality (reference changes too; re-baseline).

## 3. Recommendation (design only)

1. **I4 + I6 together first (refusal records, then glue state in keys).** They fix problems 2, 4 and most of 6 at the root, turn the generator (I3) into a key-discovery machine, and keep every hard constraint. Do I4 first (cheap, immediately better diagnostics), then I6 driven by the records.
2. **I1 (catalog as data) second, then I2 on top.** It stops the 60-file churn, removes the dominant rebuild trigger in releases (5), and makes the POSIX backlog (1) a matter of header bodies and rows.

I5 (link stage) is the natural third; it is small and fixes problem 3 for good.

## 4. Tempting ideas that conflict with constraints (design only)

- **Use system headers and link host libc (tinycc/gcc model).** Breaks byte equality across hosts (headers differ per SDK/glibc version), determinism, and the no-host-libc six-target cross build. A narrow variant (optional libSystem dynamic import on macOS only, behind a declared ABI table) may be acceptable but must be a deliberate owner decision.
- **Fall back to the reference route when the product refuses.** Hides coverage gaps, makes the shipped product effectively the reference, and voids the "tables are the compiler" claim; also two code paths inside one binary weakens byte-equality evidence.
- **Learn/train the tables (SGD) to generalise to unseen combinations.** Cannot be exhaustively verified to be exact; risks silent wrong code, which the constraints forbid.
- **Make tables "default" unknown keys to the nearest known key.** Silent wrong code.
- **One giant joint table over all stages.** Key domain explodes; exhaustive verification and executor/table size become infeasible; per-run 60 s broken.
- **Relax the 60 s bound for the candidate build or queue.** Violates the hard rule; the fix is sharding and caching, not longer runs.
- **Sort/normalise objects post hoc to hide the 3 divergences.** Masks a real plan divergence; equality must come from a shared rule (I5/I8), not a normaliser.

## 5. 核对（cc-unisacc，2026-10-01，实际跑过）

- I1 的前提成立：系统调用名是 abi/enc/combo/isel 四张 gold 表 `op` 字段词表的一部分（abi.tsv 表头 op 列含 ftruncate、gettimeofday 等），每加一个调用，四个阶段的键域都变大，必须全量重建——88cca5e 两个调用改 60 文件约 6500 行即此。
- 问题 2 的实例已被生成器复现：tests/combo.py 首跑产品 10 处误导性 unknown identifier（结构体返回 × 5 种位置）；R19-6② 实测只在产品（参考输出 `C p=0.8300`）。
- I5 的参考侧已部分落地（f5a394c、fc597d6：重复定义按名报错，tapelink 报重复导出名）；产品同单元由 cdx c000c2b 对齐，跨单元一步编译仍缺。
- I7 已部分落地（R19-0：工作树隔离、树哈希判据、precheck）。
- 报告第 4 节把“链接宿主 libc”列为与约束冲突，只留“仅 macOS 可选 libSystem 导入”作主人决定项；与 R19-9 中期方案一致。
