# 0.1.1 - 0.1.3 顺延十四项工具线与测试路线草案

编写窗口: `agy-h` | 依据: [car2-108-candidates.md](car2-108-candidates.md) (`14` 项全集) 与 [roadmap-0.1.x.md](../ccogweco/roadmap-0.1.x.md)
原则约束: 全 `14` 项均不触产品 `SOURCES_SID` 闭包; 严格遵守"一版一类大改" —— 产品车(`0.1.1` 诊断、`0.1.2` B2 上限表、`0.1.3` 进程生命周期)各自独立走版本分支, 下述工具/测试项以独立 `tool-only` 批按档期穿插落 `main`, 严禁混入产品车或同写域并发合入。

---

## 1. 已在 `0.1.0` 前消化或入手册的基线项 (`2` 项, 零新代码排期)

1. 第 `(9)` 项 `release-exec rollback 写 INDEX`: 已随 `#14 v2` (`readback-prewarm-gate-v2-on-0107.patch`) 于 `0.0.107` 落车验证 (`0m`, 零风险)。
2. 第 `(10)` 项 `门 5 (5-candidate-chain) 稳定办法入手册`: 现以起链前两次串行 `--prepare` (`~1.2s`, 全门稳在 `~51.5s`) 生效 (`0m`, 低风险); 仅当后续整链实测再度贴/超 `55s` 时触发并入 `0.1.3` 立分片。

---

## 2. 按 `0.1.1` 必做 / `0.1.2` / `0.1.3` 档口排序全表 (`12` 项)

| 档期 | 顺序与候选编号 | 内容与写域 | 预估耗时 | 风险等级与主风险点 | 前置与验收门槛 |
|---|---|---|---|---|---|
| `0.1.1` 必做 | `1.` `(7)+(12)` `prerun-watch` v3 合入登记 | `tools/prerun-watch.cx`, `tools/check-prerun-watch.cx`, `build/pkg-gaps.tsv`, `tests/` 登记表 | `10-15m` | 低: 需动 `tests/` 登记表与补跑 `table-index` | 已过 `cx-lint=0`、`12/0` (`4.3s`) 与 `107` 回放; `register.cx` 登记 `interfaces` 域 + 本地 `2a-derive` |
| `0.1.1` 必做 | `2.` `(11)` `prerun-inputs` 与 `prerun-drive:3067` 参数对齐 | `tools/prerun-inputs.cx:378`, `tools/prerun-drive.cx:3067` | `5-10m` | 极低: 改输出字段或别名兼容 | `PRERUN_CMD` 实测喂给 `prerun-drive -- plan`/`run` 无 `unknown-run-arg` |
| `0.1.1` 必做 | `3.` `(13)` `verify_version_seq_readback` 按三段整数序最大者判定 | `tools/release-chain.cx:1003-1036`, `tools/check-release-chain.cx` | `5-10m` | 中: 影响公开后 `version-seq` 读取门 | 共用 `releaseseq`; 须过乱序表(新版在前)通过与缺失公开行拒绝两夹具 |
| `0.1.1` 必做 | `4.` `(14)` `precommit` 新增批次"批次域已声明"提交点断言 | `tools/precommit.cx:2473`, `exec/facts/derive.tsv:3` | `10-15m` | 中: 触 `precommit` 热路径, 若超预算会拖慢日常提交 | 仅对触 `tests/manifests` 或 `tests/batches.tsv` 触发 `batch-domains` check; 先红后绿夹具 `<=55s` |
| `0.1.1` 必做 | `5.` `(4)` `#15` `notes` 正文写入与回读校验 (`release-exec-h`) | `tools/release-exec.cx`, `tests/batches.tsv` | `5-10m` | 中低: 改发布执行器并加真 `gh release edit --notes-file` 实测 | 与 `#16` 串行占用 `release-exec.cx`; 保留线下 `28.6s` 与线上回读验证 |
| `0.1.1` 必做 | `6.` `(2)` `#13 r39e` `runcache-units` 在 `env` 与候选间补 `sh` | `tests/runcache-units.cx` | `3-5m` | 低: 已有补丁, 仅缺显式 shell 启动差异正负夹具 | 补充正负夹具与单批回执(`<=55s`), `SOURCES_SID` 零触碰 |
| `0.1.2` 档口 | `7.` `(5)` 第 `16` 份 `CI run_attempt` 固定与按 `attempt` 封存 | `tools/release-exec.cx:1123-1293` | `20-35m` | 中高: 改 `gh` 重试追踪契约, 错判易漏抓失败 `attempt` | 须先实测 `gh 2.45.0` `run_attempt` 语义, 配 `7` 场景离线 `gh` 夹具; 接在 `(4)` 后串行 |
| `0.1.2` 档口 | `8.` `(6)` 第 `17` 份 `tlib bump` 内容漂移硬阻断 | `tools/precommit.cx`, `tools/check-tlib-frozen.cx`, `build/tlib-packages.tsv` | `25-45m` | 高: 须先洗清 `tlib-packages.tsv` 旧漂移至 `bad=0`, 否则大面积阻断提交 | 接在 `(14)` 后串行; 完成历史基线清洗 + 迁移 `precommit-tlibhint` + 3 变异全杀 |
| `0.1.2` 档口 | `9.` `(1)` `#8 v4` `-tool` 材料闭包 `__FILE__` 宏检查 | `tools/check-tool-material-macro.cx`, `tests/tool-material-macro-exempt.tsv`, `affected.cx`, `devsvc-core.cx` | `5-10m` | 中: 增免除表与登记 `tests/`, 若不先消两处字面收益减半 | 先处理 `affected.cx:1203` 与 `devsvc-core.cx:538` 字面再合补丁, `<=55s` |
| `0.1.3` 档口 | `10.` `(3)` `#13` 宿主编译器遮蔽三批支持显式 `CC` | `tools/check-chain-precheck.cx`, `tests/check-source-bom.cx`, `tools/check-realprog.cx` | `15-25m` | 中低: 仅查三批测试器; 当前已被干净 `PATH` 暂时规避 | 受控 `PATH` 假 `cc` 包装器正负实测均绿, 预算各 `<=55s` |
| `0.1.3` 档口 | `11.` `(8)` `read` 前预热门后续 (`pkg-gap #98`) | `release/tools/c47-1-readback.cx`, `tools/release-exec.cx` | `UNMEASURED` | 高: 涉及改固定下载路径, 易弱化 `c47-1-readback` 沙箱隔离 | 当前 `#14 v2` 预热后内层仅 `1.45-3.73s < 45s`, 仅当隔离方案经安全复审通过才推进 |
| `0.1.3` 条件 | `12.` `(10)` 门 `5` 分片化后备项 (条件触发) | `tools/check-candidate-chain.cx`, `tests/` | `UNMEASURED` | 中: 拆批改变整链前置门拓扑 | 仅当 `0.1.0..0.1.2` 实测两次 `--prepare` 后仍超 `55s` 时启用 |

---

## 3. 并行与写域串行约束

1. 工具线总量估时: `0.1.1` 必做 `6` 项合计约 `38-65m`, `0.1.2` `3` 项合计约 `50-90m`, `0.1.3` `1` 项确定项约 `15-25m` (另 `2` 项留 `UNMEASURED`/条件触发)。
2. 同写域严格串行链(严禁同一档期内并行提交冲突):
   - `tools/release-exec.cx` 链: `(4) #15` (`0.1.1`) -> `(5) 第 16 份` (`0.1.2`) -> `(8) pkg-gap #98` (`0.1.3`);
   - `tools/precommit.cx` 链: `(14) batch-domains` (`0.1.1`) -> `(6) 第 17 份 tlib bump` (`0.1.2`);
   - `tests/batches.tsv` / `coverage.tsv` / `manifests/` 登记锁: 解冻窗口内逐批调用 `register.cx` + `commit-own.cx --keyed`, 且每次新增批次落 `main` 前强制先跑本地 `chain-ready` `2a-derive` 门。
3. 与产品车的一版一类协同关系:
   - `0.1.1` 产品车(诊断与拒绝文案 + `precommit-list-cap` 拆批)独立于 `v0.1.1` 分支执行; 工具线 `1..6` 项在 `0.1.0` 公开解冻后立即作为独立 `tool-only` 提交先落 `main`, 确保 `0.1.1` 起链前 `prerun-watch v3`、`batch-domains` 预检与 `version-seq` 读回均已就绪。
   - `0.1.2` (B2 上限表 `17` 项) 与 `0.1.3` (0.0.125 进程生命周期) 同理保持工具批先于对应产品车冻结窗口合入 `main`, 互不混车。
