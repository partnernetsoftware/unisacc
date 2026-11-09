# Eighteen-net native contracts

Prepare material and run each command separately from the repository root:

```sh
tools/bound 30 sh ./unisacc.com tools/net-material.cx -- "$PWD" "$PWD/out/nets-fresh"
tools/bound 25 sh ./unisacc.com tools/net-material.cx -- --verify "$PWD" "$PWD/out/nets-fresh"
tools/bound 55 sh ./unisacc.com out/nets-fresh/nets.cx -- "$PWD"
tools/bound 55 sh ./unisacc.com tests/net-controls.cx -- "$PWD" "$PWD/out/nets-fresh" "$PWD/out/net-controls-fresh"
```

Output directories must be new. All handwritten CX uses the implicit runtime and
module interfaces. No Python, host C compiler, subprocess-per-key, weight rebuild
or test-result cache is used. The existing native watchdog has its separately
documented host C cold bootstrap.

The material tool expands the existing kernel/unisa_core.c and its quoted model
includes, makes implementation state private, and adds checked scalar functions.
The infer body and model literals are retained. DENSE is carried privately as part
of the source but never read by the contract. Invalid stage/head checks belong to
the adapter; field-domain refusal is tested in the original infer implementation.
This single-instance kernel has mutable workspaces and is not concurrent/reentrant.

Semantic index mappings use the existing model vocabularies and canonical CHARC/
DEFINED tuples read as data from unisa/gold.py. Composite heads reuse the declared
isel/abi vocabularies. Gold labels come from the actual eighteen gold TSV files,
with strict schema, vocabulary, column, duplicate-key and full-domain checks.
They are independent expected-label input to inference; they are not an external
C-language semantics oracle. The additional lexcls/lexword files are not among
the eighteen declared nets.

One default-run batch executes 8,770 keys and 22,694 key/head inferences. Every
winner must match gold and be the unique maximum logit. For each field, both -1
and its upper boundary are tested in every legal context of the other fields,
across all heads, followed by a valid recovery query. This produced 28,868 field
refusals and recoveries, plus 72 adapter refusals, locally. It does not enumerate
infinitely many invalid integers. Optional NAME selects one net for debugging.

Material receipts bind the recursive kernel source, gold, canonical mapping data,
tool, batch and bootstrap product, plus generated module and batch hashes.
Preparation is PREPARED only; --verify regenerates in memory and compares bytes.
The batch itself also checks each gold file's before/after hash. Source inputs
must remain fixed while running; perform material verification before and after
the batch when using it as evidence.

The separate controls intentionally change a label in an isolated gold fixture,
change one model-literal byte in the private prepared copy, and restore the copy.
They require the specific label mismatch, material-identity refusal and successful
restoration; three controls passed. They do not mutate repository gold or kernel
sources. A forced kill during the control can leave the private material invalid;
--verify then refuses it and a new material directory can be prepared.

These results cover the local classic integer-kernel contract. They do not prove
the complete P3 route, stream interfaces, resources, all six native hosts or
release qualification. Internal timing phases remain UNMEASURED. The main --all
entry reports these implemented but unscheduled nets as NOT_RUN until integration.

## 增量演练执行缓存（source-packages-v2）

`tools/net-deps.cx` 的 `shared_gen` 数组只描述全42网共享生成器闭包上界，
不是持久缓存。实际执行缓存由 `tools/net-incremental-rehearsal.cx` 的
`cache` 写入，`build`、`validate`、`compare`、`report` 路径通过 `cached` 消费。
新文件名为 `profile-source-packages-v2.exe`、`checker-source-packages-v2.exe`
及各自 `*-source-packages-v2-cache.tsv`；旧命名空间不查找、不回退。

必需字段包含 schema、规范物理仓库根、kind、权威十二包表摘要、代码输入
profile、源码/产品/实际ELF/实际编译参数摘要及 execution_key。普通CX编译不假定
额外生成 `.build.json`；本缓存只声明实际执行过的固定编译命令与观察到的输入/ELF。
包表通过已有 `seed/ident.c ROOT packages` 权威入口观测，必须是完整且有序的
`source-packages-v2` 表。profile保守包含实际入口源码与tools/cxlib、cx、include、ext
完整物理目录文件（包括gitignore忽略的私有头）；不是最小运行时读取闭包证明。
创建前后重读包表与代码输入；消费时重算。
物理根参与执行键，因此相同内容搬家不能沿用执行缓存；内容型NET_DIGEST不改，
编译器物理 `#line` 不改。字段缺失、为空、重复、旧schema或根不匹配均拒绝。

独立行为边界检查（本次修改未运行）：

```sh
tools/bound 55 sh ./unisacc.com tools/check-net-incremental-rehearsal.cx
```

该检查通过真实 `compare -> window -> cached` 消费链覆盖旧schema、v2缺字段、
不同物理根和重复字段拒绝；不授予42网全量构造或功能PASS。

有效v2重建及真实采用另分阶段执行，每条均单独受55秒watchdog约束。
先选新的绝对路径 `CHECK`、`CONTROLLER` 和尚不存在的绝对目录 `RUN`，例如：

```sh
CHECK=/tmp/speed049-cache-check
CONTROLLER=/tmp/speed049-cache-controller
RUN=/tmp/speed049-cache-run
tools/bound 55 sh ./unisacc.com tools/check-net-incremental-rehearsal.cx -o "$CHECK"
tools/bound 55 sh ./unisacc.com tools/net-incremental-rehearsal.cx -o "$CONTROLLER"
tools/bound 55 "$CHECK" --cache-create "$CONTROLLER" "$RUN"
tools/bound 55 "$CHECK" --cache-adopt "$CONTROLLER" "$RUN"
```

创建阶段调用真实 `cache RUN profile` 重建当前v2可执行缓存。采用阶段调用
`cache-list RUN`：先执行与build共用的 `cached`，再实际运行缓存profile的
`--list`，同时要求RECIPE输出与CACHE_ADOPTED标记；不是单独receipt validator。
采用阶段还创建两个物理根及逐字节相同的输入文件，使用同一份真实receipt全部字段，
仅替换物理根，调用生产执行键编码 `cache_key_at`，断言键不同。
`cache-key RUN ROOT` 只是该编码的诊断入口，不验证或授予缓存采用资格；
异根真实消费拒绝由前述独立边界检查覆盖。上述命令本次均未运行。
