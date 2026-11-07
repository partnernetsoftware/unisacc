# source-job-wave 迁移到自身体系（只计划）

## 现状入口
- `tools/source-job-wave.cx`：`ROOT CTX BUILD host|list|wave|final`；当前仍是 W5 慢车道 107/131 job 的正规入口。
- 调用方：`../moltbaby/plans/omp-bdy-handoff-2026-11-02.md:60`、`../moltbaby/plans/omp-board.md`；检查方 `tools/check-source-job-count.cx`；表行 `tests/cxraw.tsv:370`、`tests/cxlibc.tsv:485`、`tests/cxplatform.tsv:107`。
- 当前计数权威：`nids=131`、`gen/tbl/net=42`、`tools=5`；先由 `source-package.cx` 的 recipes 派生。

## 旧 C / Python 债（逐路径）
- `tools/bound`、`tools/bound.c`：wave 编译 `source-package.cx` 的 C 启动编译器。
- `seed/ident.c`、`seed/gen.c`、`seed/tbl.c`、`seed/net.c`、`seed/compilerpack.c`、`seed/pack.c`：wave 的 5 个 tool constructor。
- `seed/facts.h`、`seed/json.h`、`seed/sha256.h`：上述 C constructor 的头文件。
- `exec/c/core.h`、`exec/c/compiler-routes.tsv`、`exec/facts/pp-gen.tsv`：native pack/declare 输入。
- `tools/source-package.cx:104`、`tools/cxlib/sourcepackage.cx:173`：用 `cc -std=c99` 编译 `seed/%s.c`，是旧 C 链核心。
- Python：无运行时依赖；`source-package.cx:12` 只把现存 `.py` 当 SID 输入字节，不执行、无固定路径。

## 步骤与验收
1. `freeze`：记录当前 `list.stdout`、131/42、job 目录；`sh ./unisacc.com tools/check-source-job-count.cx` 与 `tools/check-pins.cx` 绿后才动。
2. `controller`：用候选 `unisacc.com` 替代 `tools/bound` 编译 `tools/source-package.cx`；`... list` 仍输出 131/42，`check-source-job-count` 绿。
3. `seed-tool`：逐个把 `seed/{gen,tbl,net,compilerpack,pack}.c` 换成 CX（首选 `cx/compiler/bindstage.cx`/现模块）；每换一个，重跑对应 `wave` 相位并比对 job 输出字节。
4. `native-input`：把 `exec/c/core.h`、`compiler-routes.tsv`、`pp-gen.tsv` 的读取改为 `cx/runtime`/`cx/core` 接口；`pack`/`declare` 验收用现有 receipt 字节等价。
5. `identity`：预演只做 `seed/ident.c` 默认 identity 模式（同根字节比对已过）；`write`/`check`/`manifest` 三子命令与 SID 切换并入纪元，cutover 时 `seed/ident.c` 自身进入 closure 会使 SID 必变。
6. `count-cutover`：`check-source-job-count` 改读 `source-package.cx` recipes，删除 wave 的 `nids!=131`/`counts[1]!=42` 硬编码。
7. `archive`：旧 C 链移入 `archive/reference-c/`，删 `tools/bound`；更新 `AGENTS.md`/`CLAUDE.md`/pins.tsv 表行；`ratchets` 9/9、`toolsloc` 只降不升。
8. `cutover`：W5 结束前只做 1–3 的预演，不替换入口；W5 结束后才做 4–7，并跑完整 `wave` + `final` + 跨目标 smoke。

## 预演证据（仅 out/）
- predefines：最大样本与原 C 内容 sha `f82465ea…6f43c78`；空对象 rc0、非法 rc2。
- routes：最大样本 routes.tsv sha `69f9a583…b62d296`（308 行，cmp=0）；空/非法 rc1。
- compilerpack2 audit+kernel：audit 树相对内容 sha `bd463644…26ec1f`（cmp=0）；空模型/坏 kernel 双方 rc1。
- pack P1/P2：1 行 sha `cbb7fb6f…b93886`、20 行 sha `884937b3…740960`，与原 C cmp=0；全量 308 行超 60s，瓶颈 `tools/cxlib/netpack.cx:480/734` 的 `np_compact_q`，约 O(总 Q 动作 + 每行前缀重放)，大模型常数高，留纪元后优化。

## P3 压缩口径备选
- A 复刻 zlib9 比特流：与现 C 字节等价；代价是精确复刻 zlib 编码/CRC，维护面大且需锁定版本。
- B 改用自带 `cx/text/deflate.cx`：确定性、无宿主 zlib；代价是 P3 字节变，包身份/SID/旧 pin/receipt 全部换标，属纪元决策。
- 先做 P1/P2 uncompressed 片；P3 选 A 或 B 留纪元裁定，不动手实现。

## 必须等纪元
- `wave`/`final` 的替换、`tools/bound` 删除、旧 C 归档：等 W5 慢车道纪元结束。
- 每步验收：`sh ./unisacc.com tools/check-source-job-count.cx`；`sh ./unisacc.com tools/check-pins.cx`；`sh ./unisacc.com tools/ratchets.cx`；涉及批次再跑 `tools/affected.cx --files ... --run`。
- 迁移期间不缓存 PASS、不删断言、不放宽 60s/精确 PID/预算守卫。

## 裁定(omp-cc,2026-11-02)
- P3 压缩口径选 B:自带确定性 deflate(cx/text/deflate.cx),不复刻 zlib level9 比特流、不走 zlib FFI。理由:构造字节不得绑定宿主 zlib 版本;解码侧已是自带 inflate,只差压缩器;与"去宿主 C 依赖"方向一致。
- 代价:P3 压缩字节变化一次=换标,并入"arity 落地"那次纪元(同一身份变更窗口),不单开。
- seed/gen.c(9051 行)继续暂缓,待 B 落地后再评估是否值得移植。
