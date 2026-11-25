# V2 (0.1.2) 编号骨架、登记行草稿、写域不相交核对表与可逆提交切分

维护窗口: `agy-h` | 日期: 2026-11-25 | 上游依据: `three-versions.md` (三版整合令)、`plans/v0.1.1.md` (`b3e9731b6a`)、`v012-lifecycle-prep.md` (`agyneon`)、`plans/v0.0.109.md`、`plans/v0.0.125.md`、`plans/v0.0.126.md`
状态: 仅作草稿存档于研究页与私有目录；V1 链收尾期间不写共享 `plans/v0.1.2.md`，不触发 `git commit`。

---

## 1. 编号规划对齐说明

按三版整合令(`three-versions.md` 第 3、72 行):
(1) 原 `0.1.3`（进程生命周期）与 `0.1.4`（转发桩容量）独立版本号作废，三项同并入 `V2 = 0.1.2`；
(2) 现 `build/version-seq.tsv`（以 V1 头 `b3e9731b6a` 为准）已登记至槽位 `66` (`0.1.1`)，故 `V2 = 0.1.2` 顺排占用槽位 `67`；后续 `V3 = 0.1.3`（tape 产物形态车 M2 到 M4 或失败退路 -O2 档位）再顺排占用槽位 `68`；
(3) 遵守 `tools/verseq-gen.cx` 五条规则:
- `plans/v0.1.2.md` 首行第一版本号必须为 `0.1.2`（R1）；
- 首行 `"计划:"` 后到首个 `+`/`(`/`、`/`,`/`;` 的主题前缀必须为 `build/version-seq.tsv` 第 67 行 `class` 列的子串（R2）；
- 回滚段落与正文「置入」引用均须指向 `plans/v0.1.2.md` 而非已并入的历史计划号（R5）。

---

## 2. `plans/v0.1.2.md` 全文草稿

```markdown
# 0.1.2 计划:前端与系统层合装(B2 事实表化 + 转发桩容量 + 进程生命周期)

planned_units=TBD
done_units=UNMEASURED
unplanned_units=0
a_class=前端与系统层合装
a_files_cap=TBD
freeze_at=TBD

状态:草稿(编排者 2026-11-25 依据三版整合令立项 `0.1.2`; 在私有工作树 `.claude/worktrees/v0.1.2` 串联重放 B2、转发桩容量、进程生命周期三车，严格保证三车写域不相交、每车可整段 revert、每车前缀私有候选整链绿、至多一车改变既有生成字节)。
- 范围三车(按提交次序串联合入):
  (1) 车 1 前端 B2 硬上限事实表化(`agy-h`, 素材源 `0.0.109` 补丁 `b2-k2-caps-rebased.patch`):
      17 个硬上限迁入 `exec/facts/k2-caps.tsv`, 独立生成器 `tools/k2-caps-gen.cx` 产出 `exec/facts/k2-caps-consts.tsv` 并检查 17 处消费源表行及 5 项旧常量一版别名, `tools/cap-probe.cx` 全量读表验收(17 项值不变, 改表先红后绿), 补消费者遍历一致性验证与 0.1.1 APE 字节对照。
  (2) 车 2 转发桩容量修复(`ompgpt`, 素材源 `0.0.126` 根因定案):
      修复 `cx/compiler/fwdstub.cx` (`FWD_STUB_BYTES=131072`) 与相关边界/编码写入处(`cx/compiler/fwdmark.cx`、`cx/compiler/routepipe.cx`、`cx/compiler/fwdincrement.cx`、`src/driveroute.cx`), 统一容量事实源并做到 `N=354` 通过、`N=355` 起具名拒绝而非畸形或段错误(`N>=413`); 转正转发桩容量与 `forward-count` 合同批登记。
  (3) 车 3 进程生命周期产品部分(`agyneon`, 素材源 `src/v0.0.125` 第 8 源码清单):
      重放 `cx/sys/{posix,proc,time}.cx`、`ext/sysx/{process,processcore,reaper,run}.cx`、`cx/compiler/fwdmerge.cx` 共 8 个源码文件并通过 `layer-api --bump` 与 `derive --write` 刷新派生表(配套 `tools/` 与 `tests/` 工具消费者拆出另走工具线补丁, 不进 `0.1.2` 冻结提交)。
- V1 余项衔接(若 0.1.1 有遗留工具口径):
  - 若 `0.1.1` 余下纯工具基线调整(`diag-contract` 等), 优先按工具线落 `main` 不混入 `0.1.2` 产品提交; 仅当存在明确产品修补且通过三车前缀审查时另行登记。
- 已知约束:
  - B2 与进程生命周期车原则上不改变既有合法输入程序的生成字节(由各车前缀 APE 字节比较核实, 实测前标 `UNMEASURED`); 三车合计至多允许一车(转发桩容量边界与拒绝路径)改变对应输出或拒绝字节;
  - B2 的 5 个旧常量别名(`MAXISPL`/`ENUM_LIMIT`/`SBB`/`POSSPAN`/`STRUCT_LIMIT`)仅保留本版一版过渡, 清理任务明确记入后续候选。
- 依赖: 前置 `0.1.1` 已公开发布(最终头 `b3e9731b6a`, 公开 `Y2` 待链完成后回填); `V2` 开启前须以 `0.1.2` 跑通 `chain-ready`。
- 回滚: 按 `release/rollback.txt` 将 `v0.1.2` 退回草稿并恢复 `v0.1.1` 为 Latest; 车内可按 C3 -> C2 -> C1 分段 `git revert`, 整版回滚同时回退对应提交、`build/version-seq.tsv` 第 67 行与 `plans/v0.1.2.md`。
```

---

## 3. 登记表新增行草稿 (均为 TAB 分隔，不直接写入主线)

### 3.1 `build/version-seq.tsv` 拟新增槽位行 (第 67 槽)

说明: 遵守 `tools/verseq-gen.cx` R2 规则(`class` 列起始片语与 `plans/v0.1.2.md` 首行 `计划:` 后首片语 `前端与系统层合装` 完全一致)，`state` 在开版合入后为 `进行中`，公开后改 `已公开`:

```tsv
67	0.1.2	前端与系统层合装(车1前端B2 17上限事实表化 + 车2转发桩容量131072与355界 + 车3进程生命周期8文件产品部分)	进行中	编排者 2026-11-25 依据三版整合令立项 0.1.2(合并原0.1.2/0.1.4/0.1.3三车);前置 0.1.1;计划 plans/v0.1.2.md
```

(注: `state` 列正式入库前保持 `进行中`, 待 `0.1.2` 公开后随 `release-post` 改写为 `已公开` 并填实际冻结 F SHA。)

### 3.2 `build/freeze.tsv` 冻结入场行模板 (仅在 `0.1.2` 冻结时写入，历史行改注释保留)

说明: 参照 `b3e9731b6a:build/freeze.tsv` 格式(`# 0.1.1-FREEZE ... (published ...)`，新行替换激活槽位)。正式冻结时由主调度填写 `BASE_COMMIT_V2_AFTER_011` 与 `DEADLINE_EPOCH`:

```tsv
0.1.2-FREEZE	<BASE_COMMIT_AFTER_V1_PUBLIC>	cc	<DEADLINE_EPOCH_S>	src cx ext exec include seed kernel tests release
```

### 3.3 `tests/batches.tsv` 与 `tests/coverage.tsv` 三车登记行草稿

(A) 车 1 (`B2` - `k2-caps` 生成器、探针、消费者遍历):

1. `tests/batches.tsv` (对应 `b2-k2-caps-rebased.patch` 已对齐两行):
```tsv
k2-caps-gen	IMPLEMENTED	contract	tools/check-k2-caps-gen.cx	SOURCE	lnx/x86_64	55	-	tools/check-k2-caps-gen.cx	-	contract.k2-caps-gen	rc0:CHECK_K2_CAPS_GEN	product	commit
cap-probe	IMPLEMENTED	contract	tools/check-cap-probe.cx	SOURCE	lnx/x86_64	55	-	tools/check-cap-probe.cx	-	contract.cap-probe	rc0:CHECK_CAP_PROBE	product	commit
```

2. `tests/coverage.tsv` (两行已与 `tests/manifests/k2-caps-gen.tsv` 文本完全一致):
```tsv
contract.k2-caps-gen	contract	IMPLEMENTED_SEPARATE_BATCH	k2-caps 生成器与事实表一致性: 17 个硬上限实表驱动产出 k2-caps-consts.tsv (含 5 旧常量别名与 =CAPS), --check 核对 17 处消费源表行、5 张旧常量表及改表变更	tools/k2-caps-gen.cx;tools/check-k2-caps-gen.cx;exec/facts/k2-caps.tsv;exec/facts/k2-caps-consts.tsv
contract.cap-probe	contract	IMPLEMENTED_SEPARATE_BATCH	cap-probe 17 项硬上限全量读表验收: 读表后各阈值严格等于改前原值, 改表任一值先红后恢复转绿, 覆盖所有消费者遍历点	tools/cap-probe.cx;tools/check-cap-probe.cx;exec/facts/k2-caps.tsv
```

(B) 车 2 (转发桩容量合同批转正):

现状实证(`b3e9731b6a`):
- `fwd-cap` 批已存在(`tests/batches.tsv`, `tests/coverage.tsv` `contract.fwd-cap`: 修复前构造的产品记 SKIP 且非零退出, 修复后新候选上 N=354 rc0/rc127、N=413/1000 具名拒绝 `forward stub buffer exceeded`)。
- `tests/coverage.tsv` 中 `interfaces.forward-count` 当前为 `PLANNED` (`UNREACHABLE at product level`)；且 `b3e9731b6a` 已合入 `tests/check-forward-count.cx` 与 `tests/fixtures/forward-count/gen_forward_count.cx`，尚无独立 `forward-count` 批行。

车 2 合入后待补/更新登记草稿(`ompgpt` 实跑确认后按 `--keyed` 更新):
```tsv
# tests/batches.tsv 新增独立批(若由 check-forward-count.cx 作为合同验收批):
forward-count	IMPLEMENTED	interfaces	tests/check-forward-count.cx	SOURCE	lnx/x86_64	55	-	tests/check-forward-count.cx	{candidate}	interfaces.forward-count	rc0:CHECK_FORWARD_COUNT	product	commit
```
```tsv
# tests/coverage.tsv 原 PLANNED 行原地更新为 IMPLEMENTED_SEPARATE_BATCH:
interfaces.forward-count	interfaces	IMPLEMENTED_SEPARATE_BATCH	转发调用边界实机验收(统一 FWD_STUB_BYTES 131072 事实源): 354 符号通过(rc127 no host function), 355 起具名拒绝(不产生畸形 /tmp/unisacc-forward-entry.c 或 >=413 段错误), 450 声明/少调用保持不变	tests/check-forward-count.cx tests/check-forward-limit-table.cx tests/fixtures/forward-count/gen_forward_count.cx cx/compiler/fwdstub.cx cx/compiler/fwdmark.cx cx/compiler/routepipe.cx src/driveroute.cx
```

(C) 车 3 (进程生命周期产品部分):

现状实证(`b3e9731b6a` + `v012-lifecycle-prep.md`):
- `tests/batches.tsv` 已有 `cx-reaper` (`tools/check-reaper.cx`) 与 `l0-subreaper` (`tools/check-l0-subreaper.cx`) 两位既有门；
- 按照 `agyneon` 拆出原则，`src/v0.0.125` 新增的 15 个 `tools/` 检查脚本(含 `check-proc-lifecycle.cx`, `check-process-deadline.cx`, `check-cx-sys-spawn.cx`, `check-l0-capture.cx` 等)及对应新批次(不进 V2 产品冻结提交)，另走工具线提交登记 `tests/tools-compile-sources.tsv` 与 `tests/batches.tsv`；
- V2 车 3 提交内仅同步 `layer-api --bump` 与 `derive --write` 所更新的产品派生表与现有门验证，若需在 `tests/coverage.tsv` 增加生命周期产品覆盖行，待产品核心重放并工具线 check 就绪后由 `--keyed` 登记。

---

## 4. 三车写域不相交核对表 (V2 合装硬条件验证)

| 目录 / 模块归属 | 车 1: B2 事实表化 (`agy-h`) | 车 2: 转发桩容量 (`ompgpt`) | 车 3: 生命周期产品 (`agyneon`) | 交集判断 |
|---|---|---|---|---|
| `exec/facts/` | `k2-caps.tsv`, `k2-caps-consts.tsv` | `limits.tsv` (或 `exec/parse2/forward-result.tsv`, 如需调整) | 无 | 不相交 |
| `cx/compiler/` | 无 | `fwdstub.cx`, `fwdmark.cx`, `routepipe.cx`, `fwdincrement.cx` | `fwdmerge.cx` (+2) | 严格隔离(车 2 禁碰 `fwdmerge.cx`) |
| `src/` | 无 | `driveroute.cx` | 无 | 不相交 |
| `cx/sys/` | 无 | 无 | `posix.cx`, `proc.cx`, `time.cx` | 不相交 |
| `ext/sysx/` | 无 | 无 | `process.cx`, `processcore.cx`, `reaper.cx`, `run.cx` | 不相交 |
| `tools/` 与 `tests/manifests/` | `k2-caps-gen.cx`, `check-k2-caps-gen.cx`, `cap-probe.cx`, `check-cap-probe.cx`, `tests/manifests/k2-caps-gen.tsv` | `tests/check-fwd-cap.cx`, `tests/check-forward-count.cx`, `tests/fixtures/forward-count/gen_forward_count.cx` | 0 文件入 V2 冻结(15 个工具与新清单全部拆出走工具线) | 不相交 |
| `build/` 派生表 | 无(独立 `k2-caps-gen`) | 无 | `layer-api/{cx.os,ext.sysx}.tsv`, `iface-hash.tsv`, `std-surface.tsv`, `tlib-packages.tsv`, `table-index.tsv`, `state-unverified.tsv`, `deferred-gates.tsv` | 仅车 3 重算派生表；收尾顺序排最后 |
| 公共台账与登记表 | `batches.tsv` / `coverage.tsv` (各自新增 `k2-caps` 行) | `batches.tsv` / `coverage.tsv` (`forward-count` 行) | 不新增登记行(工具线另行按 `--keyed` 登记) | 公共文件须用 `commit-own.cx --keyed` 隔离各自行 |

边界守则(防跨界碰撞):
(1) `cx/compiler/fwdmerge.cx` 明确划归车 3 (`agyneon`)；车 2 (`ompgpt`) 修改转发桩缓冲与截断错误回传时禁止改动 `fwdmerge.cx`。
(2) `tests/batches.tsv`、`tests/coverage.tsv`、`tests/tools-compile-sources.tsv` 在版本树 `.claude/worktrees/v0.1.2` 上严格按车 1 -> 车 2 -> 车 3 串行合入；若任意车辆退回工具线，通过 `--keyed` 键精确撤出该车行。
(3) `tools/register.cx` 对新增工具的 `tests/tools-compile-sources.tsv` 与 `tools-compile-NNN` 分片登记放在每车自身最后一步执行，避免相邻两车同抢分片索引尾行。

---

## 5. 可整段 revert 的提交切分方案

目标工作树: `.claude/worktrees/v0.1.2`（分支 `v0.1.2`，自 V1 公开后融合的 `main` 切出）。提交严格分层，保证任一前缀集合私有候选均具备可测性，且任一车可随时整段 `git revert` 而不连带破坏前后两车:

| 提交序号 | 归属与名称 | 包含文件路径 (精确写域) | 前缀验收条件 | 整段 Revert 命令范围 |
|---|---|---|---|---|
| `C0` | 开版骨架 (`cc` / 集成者) | `plans/v0.1.2.md`, `build/version-seq.tsv` (第 67 槽) | `verseq-gen.cx -- --check` 0 fail | `git revert C0` (退回整版开版) |
| `C1a` | 车 1 B2 表、生成器与探针 (`agy-h`) | `exec/facts/k2-caps.tsv`, `exec/facts/k2-caps-consts.tsv`, `tools/k2-caps-gen.cx`, `tools/check-k2-caps-gen.cx`, `tools/cap-probe.cx`, `tools/check-cap-probe.cx`, `tests/manifests/k2-caps-gen.tsv`, `tests/{batches,coverage,tools-compile-sources}.tsv` (仅 B2 行) | `k2-caps-gen -- --check`, `check-k2-caps-gen` (8/0), `check-cap-probe` (18/0), `derive -- --check` 其余 0 漂移 | `git revert C1a` |
| `C1b` | 车 1 B2 消费者一致性与字节比对 (`agy-h` / `cdx`, 若新增检查/表消费) | 若 2.4 只读全绿无产品改则仅记证据不占提交；若接入消费者读表则为对应产品表及一致性检查 | 每上限消费点数匹配来源行数，前缀私有候选与 `0.1.1` APE 字节对照(预期相等，待实测) | `git revert C1b C1a` (车 1 整段清除) |
| `C2a` | 车 2 转发桩容量产品修复 (`ompgpt`) | `cx/compiler/{fwdstub,fwdmark,routepipe,fwdincrement}.cx`, `src/driveroute.cx`, `exec/facts/limits.tsv` | 编译前缀私有候选：在候选上实跑 `N=354` 通过、`N=355/413/1000` 具名拒绝 `forward stub buffer exceeded` | 可与 `C2b` 联合 `git revert C2b C2a` |
| `C2b` | 车 2 合同批登记转正 (`ompgpt`) | `tests/check-forward-count.cx`, `tests/check-fwd-cap.cx`, `tests/{batches,coverage}.tsv` (仅 `forward-count` 行) | `fwd-cap` + `forward-count` 在车 2 前缀候选上双绿；`check-forward-limit-table` 维持绿 | `git revert C2b C2a` (车 2 整段清除，退回 `C1` 状态仍全绿) |
| `C3a` | 车 3 生命周期 8 产品文件重放 (`agyneon`) | `cx/sys/{posix,proc,time}.cx`, `ext/sysx/{process,processcore,reaper,run}.cx`, `cx/compiler/fwdmerge.cx` | 车 3 前缀全部 L0/生命周期/reaper 既有门(`cx-reaper`, `l0-subreaper`)及重放自检绿 | 须与 `C3b` 成对 `git revert C3b C3a` |
| `C3b` | 车 3 接口层位与派生表刷新 (`agyneon`) | `build/iface-hash.tsv`, `build/layer-api/{cx.os,ext.sysx}.tsv`, `build/std-surface.tsv`, `build/tlib-packages.tsv`, `build/table-index.tsv`, `build/state-unverified.tsv`, `build/deferred-gates.tsv` | `layer-api -- --check`, `derive -- --check`, `tsv-lint` 0 error; 车 3 前缀私有候选整链(含产物字节核对)绿 | `git revert C3b C3a` (车 3 整段清除，退回 `C2` 状态仍全绿) |
| `C4` | V2 冻结提交 (`cc` / `agy-flow`) | `build/freeze.tsv`, `release/candidate.tsv` (`0.1.2`) | `chain-ready -- --root` 全门 `READY=yes`、`release-exec -- --dry` 彩排绿、暂存区空 | `git revert C4` (解除冻结而不损 `C1..C3`) |

回滚顺序规则:
- 因三车写域互不相交(仅公共派生表与登记表由车序串行)，`git revert` 任意一车的两笔提交(`C3b C3a`、`C2b C2a`、`C1b C1a`)均可无代码冲突直接还原(若 `tests/batches.tsv` / `coverage.tsv` 末尾相邻行因先后追加出现上下文三行耦合，按行 key 删除对应车行即可完成语义整段撤出)。
- 严禁把车 3 工具线 15 文件并入 `C3a`/`C3b`，工具线单独在 `main` 按常规工具提交，与版本分支冻结零耦合。
