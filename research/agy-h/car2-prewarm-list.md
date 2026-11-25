# 第二车 (0.0.107) 公开前预热清单

编写窗口: `agy-h` (只读分析写作, 未执行重命令)
核心依据:
- `ompgpt` 第 14 份 `readback-prewarm-gate` (`~/.local/share/unisacc/research/ompgpt/readback-prewarm-gate.patch`、`readback-prewarm-gate-evidence.md`)
- `tools/release-exec.cx:1601-1746` (`--warm`、`warm_tool[5]`、`warm_probe`、`warm_cold`)
- `tools/chain-step.cx:1891-1975` 与 `tools/release-ape.cx:140-171` (`smoke` 探针)
- `src/runcache.cx:223-260,351-425`、`cx/runtime/rcident.cx:82-98,278-290`、`cx/runtime/modelcache.cx:10-45,213-249`
- `tools/affected.cx:1308-1315,1383-1472,1506-1513,1979-2043` 与 `cdx` 材料实测 (`~/.local/share/unisacc/research/cdx/car1-unstage/material.log:1-3`、`~/.local/share/unisacc/research/cdx/c1-devsvc-a-material.log:1-5`)
- `0.0.106` 实测现场 (`release/evidence/0.0.106/INDEX.tsv:20-25`、`warm-bf42706b4baa52a0cb704d7b4f00cf93bfa771a4449146ca6f8b6bc1cdcbad8d.tsv`、`/tmp/r106-exec/execute-step5-7.log`、`/tmp/r106-exec/execute-step5-7-retry.log`、`/tmp/r106-smoke-exact.M5CcO0/{cold,hot}.log`、`/tmp/r106-smoke-live.ka3U25/live.log`)

---

## 1. 0.0.106 实测对照: 为什么必须公开前预热与过门

在 `0.0.106` 正式发布时, `INDEX.tsv:20-24` 记录了因首次 `smoke` 冷超时触发公开后回滚再重跑的 `6.1` 分钟事故:

- 未命中全冷态实测:
  - 现场 `exec-5-publish` (`01:36:31`) 后 `step_readback` 的 `smoke` 阶段 (`c47-1-readback.cx:166` 硬限 `45` 秒) 直接超时拒绝: `/tmp/r106-exec/execute-step5-7.log:77-78` 记录 `C47_READBACK REFUSED ... timed out after 45 seconds`、`RUN	6	readback	phase=smoke	attempt=1	ms=45269	rc=1`, 随即触发 `exec-6-rollback` (`01:37:39`, `INDEX.tsv:21`)。
  - 同条件无缓存冷态隔离复现(`/tmp/r106-smoke-exact.M5CcO0/cold.log:1,315,382`): 外层两次查找均 `RR_TIME	lookup_hit=0`, 完整墙钟耗时 `55.319` 秒 (`real	0m55.319s`; 口径上跨全冷 `51-55` 秒区间, 确定超过 `45` 秒硬限)。
- 双层命中热态实测:
  - 复跑通过时 (`/tmp/r106-exec/execute-step5-7-retry.log:75`): `RUN	6	readback	phase=smoke	attempt=1	ms=1452	rc=0` (`1.452` 秒)。
  - 隔离热态与现场验证日志 (`/tmp/r106-smoke-exact.M5CcO0/hot.log:1,10,14` 与 `/tmp/r106-smoke-live.ka3U25/live.log:1,10,14`): 两层工具分别打出 `RR_TIME	lookup_hit=2` (`ns=25480478` + `ns=17891693`, 共 `43.4ms`) 与 `lookup_hit=2` (`24.1ms` + `17.1ms`, 共 `41.2ms`), 整步 `smoke` 分别仅耗 `3.157` 秒 (`hot.log:14`) 与 `3.732` 秒 (`live.log:14`), 落于 `1.2-3.7` 秒热态区。
- 核心结论: 两层工具 (`chain-step.cx` 与 `release-ape.cx`) 的热命中将耗时从超 45 秒限(`~51-55s`)压缩至 `1.45-3.73s`。`ompgpt` 第 14 份 `readback-prewarm-gate.patch` 将这一验证从"公开后第 6 步读回才暴露"前移至"公开动作第 5 步 (`gh release edit --draft=false`) 之前"的硬门 (`READBACK_PREWARM`)。

---

## 2. 预热对象全清单 (四类边界与架构约束)

### (1) `run-cache` 缓存根与 `TMPDIR` 环境约束

| 约束项 | 源码规则与出处 | 公开前执行要求 |
|---|---|---|
| 缓存根目录优先级 | `cx/runtime/modelcache.cx:214-248`、`src/runcache.cx:370`: 依次取 `$UNISACC_CACHE_DIR` -> `$UNISACC_E3CACHE` -> `$XDG_CACHE_HOME/unisacc/e3cache` -> `$HOME/.cache/unisacc/e3cache` -> 无 HOME 时回退 `$TMPDIR/unisacc-e3cache-<uid>` | 起链本体按 `prd_mempalace.md:65` 必须 `unset UNISACC_RUN_CACHE UNISACC_CACHE_DIR`; 链后 `--warm` 与 `release-exec --execute` 全程保持统一默认 `$HOME/.cache/unisacc/e3cache` (或固定同一个 `$UNISACC_CACHE_DIR`) 与同一 `$HOME`, 禁止设 `UNISACC_RUN_CACHE=0` / `""` (`src/runcache.cx:223-229`) |
| `TMPDIR` 稳定性约束 | `cx/runtime/rcident.cx:278-280`: `.plan` 备忘密钥绑定 running exe、当前目录 `cwd`、临时目录 `TMPDIR`、`-funit` 及源路径 | 从 `--warm`、第 5 步 `READBACK_PREWARM` 到第 6 步 `c47-1-readback`, 必须继承同一个 `TMPDIR` (不得一处 `/tmp`、另一处换短/长自定义 `$TMPDIR`), 且路径总长受 `cx_fmt` / `sha_file` 约束, 推荐短路径如 `/tmp` (`0.0.106` 经验) |
| 工作目录与调用产品路径 | `tools/release-exec.cx:1602-1604`, `src/runcache.cx:245-260`, `cx/runtime/rcident.cx:82-98`: `-run` 按工具入口路径形式、产品容器路径(`UPI1` pid + 容器 PATH)与 `cwd` 建键 | 必须固定在 `v0.0.107` 工作树根 (`cwd=<here>`) 以同款产品启动器 (`./unisacc.com` vs 绝对 `<here>/unisacc.com`) 调用; 不可拿已装 Root 的热缓存冒充版本工作树产品热缓存 |
| 嵌套 `devsvc` 环境清理 | `tools/release-exec.cx:1755-1762`: 清除外层 `DEVSVC_REPO_TABLE` 与 `DEVSVC_REPO_TABLE_PIN` | 若通过 `devsvc x` 调 `release-exec`, 工具自清两变量防 key 漂移引起的内层 `devsvc-core` 冷编 |

### (2) 五工具 `warm_tool[5]` (`release-exec --warm` 预热对象)

依据 `tools/release-exec.cx:1612-1614`, `--warm` 按读回第 6 步实际传入的精确路径拼写顺序绑定以下 5 个脚本:

| 顺序 | 数组索引 | `warm_tool` 文件 | 路径形式 (`warm_abs`) | `--warm` 参数 (`warm_arg`) | 并发说明 (`tools/release-exec.cx:1690-1704`) |
|---|---|---|---|---|---|
| 1 | `0` | `release/tools/c47-1-readback.cx` | 相对路径 (`0`) | `--list-steps` | 必须首个单独跑完 (防新建缓存根并发首次写丢失 `2/5` 条目, `:1690-1692`) |
| 2 | `1` | `<here>/tools/chain-step.cx` | 绝对路径 (`1`) | `warm` | 项 `1..4` 在项 `0` 结束后并行冷编 (`55s` 限) |
| 3 | `2` | `tools/release-ape.cx` | 相对路径 (`0`) | `warm` | 同上 (注: 内层被 `chain-step.cx` 以 `./unisacc.com tools/release-ape.cx` 调起时亦走相对路径!) |
| 4 | `3` | `<here>/tools/check-run-comm.cx` | 绝对路径 (`1`) | `warm` | 同上 |
| 5 | `4` | `<here>/tools/release-layout.cx` | 绝对路径 (`1`) | `warm` | 同上 |

身份收据键 (`tools/release-exec.cx:1633-1657`): 对 `here` + 5 个工具字节 + `git ls-files -- cxroot.tsv cx ext tools/cxlib unisa` 全闭包 + `checkout_product` sha256 + `cand.sha[0]` 求 sha256 生成 `warm-<key>.tsv` (注意 `0.0.106` `warm-bf42706...tsv:1` 显示 `closure_files=462`)。

### (3) 两层工具调用链 (`chain-step` + `release-ape`, 公开前第 5 步硬门 `READBACK_PREWARM`)

依据 `readback-prewarm-gate.patch:158-200` 与 `readback-prewarm-gate-evidence.md:5`, 公开 (`gh release edit --draft=false --latest`) 之前, 必须用与第 6 步正式读回完全相同的外层调用链跑通一次 `smoke`:
- 第 1 层: 车树启动器 `<here>/unisacc.com` 驱动 `<here>/tools/chain-step.cx -- smoke <layout>/unisacc.com` (`UNISACC_RR_TIME=1`)。
- 第 2 层: `tools/chain-step.cx:1891-1896` 调起 `sh ./unisacc.com tools/release-ape.cx -- smoke <ape>` (继承 `UNISACC_RR_TIME=1` 与工作树根)。

### (4) 候选 APE 内层探针 (`tools/release-ape.cx:145-171`, 5 项功能实跑验证)

| 序号 | 探针名 | 执行方式与超时 (`release-ape.cx:159-168`) | 期望输出 / 退出码 | 缓存行为真实边界 (`release-exec.cx:1610-1611`) |
|---|---|---|---|---|
| 1 | `candidate-version` | `./tools/bound 30 sh <ape> --version` | `unisacc 0.0.107` (`rc=0`) | 直接启动, 无编译缓存 |
| 2 | `run-c-exit-7` | `./tools/bound 30 sh <ape> -run <tmp>/seven.c` | `rc=7` | 短 C 源执行 |
| 3 | `S-prints-tape` | `./tools/bound 30 sh <ape> -O0 -b lnx/x86_64 -S <tmp>/seven.c -o -` | `rc=0` 含 `_start:` | 汇编输出探针 |
| 4 | `cx-host-c-library` | `./tools/bound 50 sh <ape> <tmp>/hostc.cx` | `rc=0` 含 `release-ape-hostc` | 不被 `--warm` 跨轮命中: `<tmp>` 绑定 `PID`, 公开读回下载目录新建且容器 path 键不同 |
| 5 | `cx-tool-srcshape` | `./tools/bound 50 sh <ape> tests/srcshape.cx -- --check` | `rc=0` | `tests/srcshape.cx` 路径固定但在公开读回侧候选 APE 下载路径为新随机目录(`readback<rand>`), 本身不复用 `--warm` 命中 |

说明: 尽管第 4/5 项内层探针不跨目录复用候选 APE 的 `run-cache`, 实测显示外层两层 (`chain-step` + `release-ape`) 命中后的全部 5 条内层探针总计仅需 `1.45-3.73` 秒 (`execute-step5-7-retry.log:75`, `live.log:14`), 完全在 `45` 秒窗口安全区之内。

---

## 3. 公开前预热标准操作顺序、命令原行、命中判据与失败处置

| 顺序 | 阶段与时点 | 命令原行与出处 | 命中判据 (`RR_TIME lookup_hit` 与日志行) | 实测参考耗时 (`0.0.106`) | 失败处置 |
|---|---|---|---|---|---|
| 1 | 链止打包出 `layout` 后、冻结/`X` 提交前 (或随 `release-chain` 阶段 `0b`) | `sh ./unisacc.com tools/release-exec.cx -- --warm release/candidate.tsv --layout "<OUT>/layout" --out "<OUT>"` (`tools/release-chain.cx:15`; `tools/release-exec.cx:1751,1803`) | `warm_probe` 校验 5 工具全部打出 `RR_TIME\tlookup_hit=<非0>` 且无缺备忘; 输出 `WARM\tTARGET\t<tool>\tHIT\t...` x5 + `WARM\tRESULT\tREADY\tkey=<64hex>\ttargets=5` (`release-exec.cx:1672-1738`), 并写 `<OUT>/warm-<key>.tsv` | 首次全冷分 2 波(首工具单跑 + 后 4 工具并行, 各 `<=55s`) 约 `45-65s`; 热验证复跑约 `4-6s` | 任一工具超时(`>55s`)或复探 `miss` 报 `REFUSE RELEASE_EXEC_GATE_WARM` (`:1728,1735`), 检查是否 `UNISACC_RUN_CACHE=0` / 路径拼写改动, 禁止手写伪造 `warm-<key>.tsv` (`:1722` 会比字节拒绝 `REFUSE RELEASE_EXEC_GATE_WARM_RECEIPT`) |
| 2 | 将 `warm-<key>.tsv` 随 Y1 证据带入 `release/evidence/0.0.107/` (可选/按流程, 可 `scrub_home` 脱敏 root) | `cp "<OUT>/warm-<KEY>.tsv" release/evidence/0.0.107/` (对应 `readback-prewarm-gate.patch:211-235` `warm_receipt_at` 同时认 `out` 与 `evdir` 且支持脱敏后的 `~/...` 核验) | `warm_note()` (`release-exec.cx:1741-1746`) 打印 `WARM\t6\tREADY\tkey=<64hex>` | `<1s` | 若报 `RECEIPT_DIFFERS`, 重跑顺序 1 重新生成收据 (`:1722`) |
| 3 | 步骤 5 公开动作 (`exec-5-publish`) 正前方 (`readback-prewarm-gate.patch:158-200`) | 若已打入第 14 份补丁, `release-exec --execute` 自动调用: `UNISACC_RR_TIME=1 sh "<here>/unisacc.com" "<here>/tools/chain-step.cx" -- smoke "<layout>/unisacc.com"` (若 107 未打该补丁, 须人工在 `--from-step 5` 前必跑一次此命令验证!) | `smoke` 返回 `rc=0` 含 `RELEASE_APE\tSMOKE\tok=5\tfailed=0`, 且 stderr 中严格包含 `2` 条正数 `RR_TIME\tlookup_hit=` 且 `0` 条 miss (`smoke_ok=1 hits=2 misses=0`), 打印 `GATE\t5\tREADBACK_PREWARM\tPASS` (`readback-prewarm-gate.patch:171-190`) | `106` 冷未过(`45.27s` 超时 / 隔离冷测 `55.32s`); 命中双 `lookup_hit=2` 后实测 `1.45s - 3.73s` (`execute-step5-7-retry.log:75`, `hot.log:14`, `live.log:14`) | 若命中 `<2` 或 `misses>0` 或超时(`>45s`), 门禁阻断在公开前(`GATE\t5\tREADBACK_PREWARM\tFAIL`), 不执行 `gh release edit --draft=false` (`readback-prewarm-gate-evidence.md:5`); 处置为在同一工作树/相同 `TMPDIR`/缓存环境重跑顺序 1 (`--warm`) 后恢复 `--from-step 5` |
| 4 | (补充: 如在未打第 14 份补丁下发生 `exec-6` 超时) | 同上 `UNISACC_RR_TIME=1 sh "<here>/unisacc.com" "<here>/tools/chain-step.cx" -- smoke "<layout>/unisacc.com"` 再重跑 `--from-step 6` | `READBACK_STEP smoke PASS`, `lookup_hit=2` | `1.45-3.73s` (`0.0.106` 复跑 `1452ms`) | 若已公开后才失败则会触发 `exec-6-rollback` (`+6.1m`); 因此顺序 3 必须在公开前执行完备 |

注: 确认 `--warm` 虽预热 5 个工具, 但若 `c47-1-readback` 实际调用环境与 `--warm` 存在 `TMPDIR`/`HOME`/容器路径差异, 或中间经历了缓存失效动作, 顺序 1 的静态收据无法替顺序 3 的实时 `lookup_hit=2` 探针担保 (`readback-prewarm-gate-evidence.md:7`: "收据仅诊断, 不能替代实际 smoke 缓存命中")。另按 `cc` 勘误与 `cdx/car2-chain-ready-gates.md:14,17`, 本页仅覆盖链后公开前预热; 起链前阶段 C 门 5 (`5-candidate-chain`) 的正确预备命令为同 `TMPDIR` 下分别执行两次 `sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$TMPDIR/unisacc-cr-prep-cache" tool` 与 `generator` (非 `-- OUT CACHE --prepare`), 且门 22 `stamp` 命中(`premerge-union.cx:659-672`)仅省预合树生成而仍重跑 `prep` 阶段生成器检查, 不得把重复 `prep` 算零。

---

## 4. "预热后不得清缓存" 禁令清单 (自顺序 1 `--warm` 起到顺序 `Y2` 结束)

在 `0.0.107` 从执行 `--warm` 开始直至公开并完成 `exec-7-Y2-done` (`INDEX.tsv`) 之前, 所有窗口严格遵守以下禁令:

(1) 禁止运行或手工删除 `~/.cache/unisacc/e3cache/` 及默认用户缓存目录 (`tools/sweep-plan.cx:11,184,235`):
- 禁止对 `~/.cache/unisacc/e3cache`、`$UNISACC_CACHE_DIR`、`$TMPDIR/unisacc-e3cache-<uid>` 执行 `rm -rf` 或按 `sweep-plan` 执行 `DELETE` 项(直接删空 `USE33`/`USE34`/`.plan` 与 `mc-<hash>` 条目, 使外层两层直接退回 `51-55s` 全冷态)。

(2) 禁止切换、清空或改写 `TMPDIR` 与 `HOME` (`cx/runtime/rcident.cx:278-280`, `cx/runtime/modelcache.cx:214-248`):
- 预热、`step_publish` 与 `step_readback` 必须共用完全相同的 `HOME`、`UNISACC_CACHE_DIR` (均未设或同绝对路径) 及 `TMPDIR`; 更换 `TMPDIR` 会直接击穿 `.plan` 缓存与两层工具路径环境一致假设。

(3) 禁止在未授权情况下运行 `tools/cache-retain.cx` 清理窗口正用目录, 或运行未经审计的全局磁盘清理脚本 (`tools/cache-retain.cx:49-67`, `prd_mempalace.md:108`):
- 任何按时间戳淘汰缓存文件或 `TMPDIR` 的外部脚本不得在发布执行窗口触发; 同时禁止跳过首工具单跑而对全新空缓存根并发执行冷编(`tools/release-exec.cx:1690-1692`)。

(4) 禁止在 `--warm` 之后改动工作树产品启动器 (`./unisacc.com`) 或五工具闭包含文件 (`tools/release-exec.cx:1607-1609, 1635`):
- 不得对 `.claude/worktrees/v0.0.107` 执行 `git worktree remove`、`git clean`、`git reset`, 不得重新替换 `unisacc.com` 字节或修改 `cxroot.tsv`/`cx/`/`ext/`/`tools/cxlib/`/`unisa/` 与 `warm_tool[5]` 源文件(会立刻改变 `warm_key` 与 `USE33` `rc_prod`/`rc_src` 哈希使预热失效)。

(5) 禁止在发布执行命令或子脚本环境中注入禁用/旁路缓存环境变量:
- 严禁设置 `UNISACC_RUN_CACHE=0` (`src/runcache.cx:227`, `cx/runtime/modelcache.cx:38`)、`UNISACC_UNIT_CACHE=0` (`cx/runtime/unitcache.cx:47`)、`UNISACC_CACHE_DIR=""` (`modelcache.cx:229`) 或 `UNISACC_RUN_CACHE_KEY_OLD=1` (`src/runcache.cx:253-258`)。
- 若通过 `devsvc` 派发 `release-exec`, 保留 `release-exec.cx:1759-1762` 对 `DEVSVC_REPO_TABLE` / `DEVSVC_REPO_TABLE_PIN` 的剥离, 不得在外层强行覆盖注入错误 `PIN` 造成内部冷编译偏移。

---

## 5. 材料预热判据 (`cdx` 实测补录): 必核每个 `MATERIAL` 实有路径与 `PREPARED`, 不可只看 `PREBUILD failed=0`

(1) 为什么只看 `PREBUILD ... failed=0` 不成立 (`tools/affected.cx:1308-1315,1388-1441,2005-2043`):
- `PREBUILD` 把 `failed` 与 `deferred` 分列(`tools/affected.cx:1439`: `PREBUILD\tmaterials=%d\thit=%d\tbuilt=%d\tfailed=%d\tdeferred=%d\telapsed_ms=%ld`), 实际汇总失败计数是 `pb_failed = failed + deferred` (`tools/affected.cx:1440`); 若批无声明材料或未选中, 仍输出 `PREBUILD\tmaterials=0\thit=0\tbuilt=0\tfailed=0\tdeferred=0`。
- `--materials-dump --prepare-materials` 模式下(`tools/affected.cx:2005-2043`), `PREPARED`/`PREP_FAILED`/`PREBUILD` 走 `stderr` (`prep_quiet=1`, `:1315,2008`), `stdout` 输出每个材料解析结果 `MATERIAL`, 且函数末尾固定 `return 0;` (`:2043`): 即使某项未备好而打印 `MATERIAL\t<BATCH>\t<NAME>\t-\tREFUSED=<why>` (`:2025-2026,2029`), 退出码仍可为 `0`。

(2) 必须同时留存并核对的四类判据原行 (基于 `tools/affected.cx:1436-1439,1465-1471,1508-1511,2025-2030` 与 `cdx` 实测 `~/.local/share/unisacc/research/cdx/car1-unstage/material.log:1-3`、`~/.local/share/unisacc/research/cdx/c1-devsvc-a-material.log:1-5`):

| 判据类别 | 合格原行格式与出处 | 拒绝/失效原行与 `cdx` 实测示例 |
|---|---|---|
| 1. `MATERIAL` 实有路径行 (`stdout`) | `MATERIAL\t<BATCH>\t<NAME>\t<REAL_ABS_PATH>` (`tools/affected.cx:2027`; 第 4 列为磁盘实有绝对路径或经生产者校验的值, 无第 5 列 `REFUSED=`) | 若出现 `MATERIAL\t<BATCH>\t<NAME>\t-\tREFUSED=<why>` (`tools/affected.cx:2025-2026,2029`) 或第 4 列路径不存在即拒。`cdx` 实测原行 (`car1-unstage/material.log:3`): `MATERIAL\tdevsvc-git-unstage\tdevsvc-core-tool\t/tmp/unisacc-cdx-prep-dTb6jFdw/prep/devsvc-core-tool-6b5c0cf67413f520.bin` |
| 2. `PREPARED` 材料行 (`--prep-only` 写 `stdout`; `--materials-dump --prepare-materials` 写 `stderr`) | 构造材料(`:1437`): `PREPARED\t<NAME>\tid=<16hex>\tsha256=<64hex>\tcache=<hit\|built>\tms=<N>`; 宿主材料(`PK_HOST`, `:1436`): `PREPARED\t<NAME>\tid=<16hex>\tsha256=<64hex>\tcache=hit\tms=0\tpath=<ABS>\tversion=<VER>\tvia=<SPEC>`; `tlib-kit` (`:1465-1471`): `PREPARED\ttlib-kit\t...rebuilt=<0\|1>`; `--prep-only` 身份绑定(`:1508`): `PREP_IDENTITY\t<NAME>\tid=<16hex>\tsha256=<64hex>\tproduct=<64hex>\tpath=<REAL_ABS_PATH>` | 出现任何 `PREP_FAILED\t<NAME>\t<why>` (`:1438`) 或 `PREPARED\ttlib-kit\trebuilt=UNKNOWN\trc=<N>` (`:1471`) 即失败。`cdx` 实测原行 (`c1-devsvc-a-material.log:1-2`): `PREPARED\tdevsvc-core-tool\tid=e3eb649e883c2ed6\tsha256=0c03a19820b5c354bdeedfdeb2d496e5138f2a2133946874211eebd8a71d1bde\tcache=built\tms=41837` 与 `PREPARED\tdevsvc-tool\tid=5b15a31be7afe8a5\tsha256=29a0d22cf39579f919318989991d87adb3df8edf214558ec21a46365ba99bff6\tcache=built\tms=10430` |
| 3. `PREBUILD` 与 `AFFECTED_PREP` 汇总行 | `PREBUILD\tmaterials=<N>\thit=<H>\tbuilt=<B>\tfailed=0\tdeferred=0\telapsed_ms=<MS>` (`:1439`, 要求 `N > 0` 且 `H + B == N`, `failed=0` 且 `deferred=0`); 若走 `--prep-only`, 还需 `AFFECTED_PREP\tkit=...\tmaterials=<N>\t...\tfailed=0\t...\tcompile_prewarmed=1` (`:1510`) | 禁止只匹配 `failed=0` 而放行 `materials=0` 或 `deferred!=0`。`cdx` 实测原行 (`c1-devsvc-a-material.log:3`): `PREBUILD\tmaterials=2\thit=0\tbuilt=2\tfailed=0\tdeferred=0\telapsed_ms=42778` |
| 4. 资格边界 (禁止代替 `PASS`) | 材料预热完毕后仍须由同候选 APE + 同材料路径实跑对应批次, 留存 `BATCH_RUN\tid=<ID>\tstatus=PASS\trc=0` (`research/cdx/car2-add-acceptance.md:16,24`) | 只有 `PREPARED`/`MATERIAL` 无 `BATCH_RUN` 一律只记 `PREPARED`, 不计批次 `PASS` |
