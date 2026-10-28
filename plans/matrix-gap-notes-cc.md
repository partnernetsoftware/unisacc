# 五张矩阵剩余本机格的闭合计划（cc，2026-10-28，只读审计；fable 裁定的无算力待办）

对象：stream-e4、stream-prune、optimization、formats（properties 本机列已清零）。只列本机 lnx/x86_64 的 MISSING / NEEDS_PRODUCT_CHANGE / BLOCKED；外宿主格（NEEDS_FOREIGN_HOST）需宿主，不在此。
可复现校验：`for f in stream-e4 stream-prune optimization formats properties; do echo $f $(cut -f4 tests/$f-domain-matrix.tsv | sort | uniq -c | grep -v "#" | tr '\n' ' '); done`。
机器可读队列：plans/matrix-gap-cc-queue.tsv（cell、batch/tool、oracle、material、budget、state）。

## 1. 已有登记批覆盖、只差换标（0 算力，需该批主人或下一纪元）

| cell | 覆盖批 | 说明 |
|---|---|---|
| elf.x86_64.segments, elf.arm64.segments, ape.container-structure, object.lnx-arm64.unit-link | formats-elf-ape（IMPLEMENTED，tests/formats-elf-ape.cx 283 行） | 该批已登记但这四格仍 MISSING；按 §7 格式贴 `formats-elf-ape@<候选>:N/0;日志` 即可，须先在当前候选实跑一次取 N。 |

## 2. 只扩现有生成器、一个窗口槽（每格 ≤40 s）

| cell | 工具扩展 | oracle | 材料 |
|---|---|---|---|
| e4.det.file-vs-stdin / prune.det.file-vs-stdin | o2-cases-gen / prune-cases-gen 加 `-` 路线：同输入经文件与 stdin 两路 | 两路输出字节相等 | stream4 / stream5 |
| e4.ref.non-regular-input / prune.ref.non-regular-input | 同上加拒绝段：INPUT 为目录、无读权限、不存在 | 具名拒绝且 ENOTDIR/EACCES 与 ENOENT 文本不同、stdout 空（CLAUDE.md 规则） | 容器 |
| e4.rec.after-refusal / prune.rec.after-refusal | 同上：每个拒绝后立刻跑一次干净输入 | 输出 == 该输入的既有 oracle | 容器 |
| e4.ref.step-budget | o2-cases-gen 加 `UNISA_MAXSTEPS=1` 一例 | 退出 3、stderr 含 `step budget exhausted (budget 1 steps)`、stdout 空；与 resources 行 `res.steps.stream-exhaust` 同一观测，一处实现两处引用 | 容器 |
| e4.id.same-candidate-model / prune.id.same-candidate-model | 新小工具：读候选包成员与 streams.json 的 model 记录 SHA 比对 | 相等（纯哈希比对，无推理） | 候选包 + streams.json；注意 candidateprofile/identity 是 cdx 窗，只读数据不碰其代码 |

## 3. 需要新的小生成器或新材料（每格一个窗口槽）

| cell | 方案 | 阻塞 |
|---|---|---|
| e4.in.o1-model, e4.pos.o1-save-restore-to-mov | O1 作为独立流：参考 `src/opt.c` O1 对软件栈 save/restore→mov 的四轮界；oracle = REF `-O1 -S` | streams.json 无 O1 容器（material packer 只出 image/O2 路线）→ 先记 NEEDS_MATERIAL，由 packer 主人加 O1 目标 |
| e4.sem.stream-boundary / prune.sem.execution-preserved | E4/prune 输出再经 pinned lower(stream6)+elf(stream7) 得镜像并执行，与输入 tape 同路径执行结果相等 | 需 stream7 直跑入口（tests/stream-elf.cx 已有）；复用 properties `tu` 组的执行比对骨架 |
| e4.sem.corpus-breadth | opt-corpus-gen 加 corpus-7..9：float、varargs、switch 表、指针算术、深递归 O0==O2 执行 | struct 值返回要 ace3 候选（d65 上记 UNREALIZABLE，不假装覆盖） |
| prune.route.placement | 固定死函数夹具：`-S`/`-c` 输出保留死函数（未剪），镜像/-run 路线剪除 | 产品 `-S` 与镜像路线各一次编译 |
| prune.pos.library-exports, prune.ref.library-signatures | 库模式（导出根、签名拒绝） | 材料无库路线；先记 NEEDS_MATERIAL |
| tapebin.roundtrip | `.tape → .tapebin → .tape` 字节稳定 | §-probe：`-S -o x.tapebin` 写的是文本且回读拒绝"origin target mismatch"，真二进制路线的拼写待探（exec/c/compiler.c:60 tapebin_target） |
| archive.product-ar | `unisacc ar rcs|t|x` 与系统 ar 互认，链接只拉需要的成员 | 行声明宿主 ar 为 oracle：需裁定改为自写 ar 解析器才能 cx-only |

## 4. 需裁定或产品改动，不排跑

| cell | 原因 |
|---|---|
| corpus.cc-differential | 宿主 cc 为 oracle，cx-only 不可达；同语料已对经典参考覆盖（optimization-corpus-0..6），需裁定改 oracle |
| closure.o1-round-bound | 内部轮数不可观测，需产品侧计数器 |
| object.system-linker | 宿主 ld 为 oracle |
| ape.fat-macho-universal, object.macho-coff-emission | NEEDS_PRODUCT_CHANGE，拒绝是现行合同 |
| corpus.c-testsuite | 语料不在树内 |
| e4.host.lnx-x86_64 / prune.host.lnx-x86_64 | 汇总格，随本机列清零自动闭合 |
| *.host.win-arm64 | BLOCKED，需原生 ARM64 PE 产品 |

## 估算

第一个 10 分钟窗口（只扩现有工具）：§2 的 9 格；再四个小生成器：§3 中无材料阻塞的 5 格；其余 12 格需材料、裁定或产品改动，在 d65 上保持开放是诚实状态。

## 附：stream-semantics 工具的接口事实（只读，2026-10-28）

- stream6（lower）输入 tape.text，输出 target.text（`@target/@data/@sym/@src_os/@data_len/@bss/@relocs` 头 + 目标指令行，见 §14 lower 组）；stream7（elf）输入 target.text，输出 image.bytes，pin 2f063f38（tests/stream-elf.cx:6），调用形同 stream4/5：`tools/bound 12 unisacc-stream7.com INPUT INPUT INC`，env `UNISA_ATTRIBUTES=`、`UNISA_MAXSTEPS=200000000`。
- 因此 e4.sem.stream-boundary / prune.sem.execution-preserved 的链：tape → stream4|5 → stream6 → stream7 → 镜像 → `tools/bound 5 镜像` 执行；对照链：原 tape → stream6 → stream7 → 执行。断言：两镜像的 stdout/退出码相等，且与手推字面相等（非空洞）；镜像字节可不同（E4/prune 改写了指令）。夹具可复用 properties 的循环程序 tape（经产品 `-S` 得 E3 tape）与 prune 的 dead 夹具。

## 附：两份草稿的接口桌面核对（只读，2026-10-28；fable 裁定：等窗期不再写新草稿）

- 容器 CLI：`tools/bound 12 unisacc-streamN.com INPUT INPUT INC`，INPUT 为 `-` 时读 stdin（tools/stream-runtime.c:12-19）✓。
- 拒绝文本：stream-runtime.c:5 `#include "../exec/c/run.c"`，故 `readfile()` 的 `run: cannot open PATH: ENOENT|ENOTDIR|EACCES` + `cannot open input` 同产品（run.c:155-178）✓；目录作为 INPUT 可能 open 成功而在 read 失败，草稿按 INFO 记录 ✓。
- 步数预算：容器是非产品构建，`UNISA_MAXSTEPS` 经 strtol 生效，status 3 打印 `step budget exhausted (budget N steps)`（run.c:1061-1069），execute() 返回 status 即进程 rc ✓；草稿已改为不传重复变量（91f6e33791）。
- stream6/7 链：输入输出格式与 pin 见上节 ✓。未核：`UNISA_MAXSTEPS=abc` 被 strtol 变 0（已知产品外分支缺陷，queue 中 res.steps 行），草稿不触碰。

## 附：窗口到来时的执行清单（更新 2026-10-28 15:5x，csr 转 cdx：ace3 冻结根 f3705dfa 已装，共享树仍 d65）

1. 两份草稿首跑（stream-io-cases-gen ×2 容器、stream-semantics ×2）≈3 min，只修工具不登记。
2. stream-e4-peep-0..7 重跑 ≈6 min（TABLE_SUSPECT 16→12）。
3. B2 等价核 ≈3 min。
4. 迁移准备：待共享树产品换到 ace3 根后，在冻结导出重跑 35 批到 out/c43-cc-reg-02，`tools/matrix-flip-gen.cx … --migrate d65c1474 <ace3 候选短 SHA> out/c43-cc-reg-02` 生成并 `--check`，再覆盖五张矩阵（≈12 min，可另开窗）。
不授跨平台、不换标到未装候选。
