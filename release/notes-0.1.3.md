# unisacc 0.1.3

状态：同候选原生 CI run `<CI_RUN_ID>`（待 `release-exec` 回填）。整体裁决 `NOT_QUALIFIED`：全套 s9 本版未跑，严格证据只覆盖下列 549 批，不得称全批资格。

## 真实身份

| 对象 | 身份 | 来源 |
| --- | --- | --- |
| 冻结提交（构造输入、发布标签目标） | `f485ec9a2bf02a7204b0d40b4b048ebd8a652a7a` | `release/candidate.tsv` |
| 发布 APE (`unisacc.com`) | `sha256=eb6741afe2d54c4a7e867b229aea903f1650a797508623869f858c0b197d3575`, `bytes=9232894` | `~/.cache/unisacc/evidence-staging-0.1.3/release/evidence/0.1.3/notes-evidence.txt` |
| 源码身份 (`SOURCES_SID`) | `15305fd30620e1953b7479dfa3cc8e88f48278e6b0d9a2c1ff85a687627fc6e5` | `release/candidate.tsv` |
| 整链起止 | `T1=2026-11-26 12:10:24 +0800 (1795666224)` -> `T2=2026-11-26 12:39:48 +0800 (1795667988)` (`delta_s=1764`, `delta_min=29.4`) | `notes-evidence.txt` |
| 六目标不动点 | `lnx/x86_64=7aab61de...` (`2030366`B, `SAME`), `lnx/arm64=9f78aebd...` (`2185958`B, `SAME`), `osx/x86_64=235123d4...` (`3335650`B, `SAME`), `osx/arm64=87f7d5b3...` (`4508002`B, `SAME`), `win/x86_64=eff7c58f...` (`3656704`B, `SAME`), `win/arm64=dbfd71ec...` (`4899328`B, `SAME`) | `notes-evidence.txt` |
| CI run | `<CI_RUN_ID>` | 待 `release-exec` 回填 |

## 产品改动

(1) **NN 外部优先加载**：目录优先级 `--nn-dir DIR` / `--nn-dir=DIR` -> `UNISACC_NN_DIR` -> 可执行文件旁默认目录；找不到回退内置权重，存在但不可读、非普通文件、格式错误或属主/权限不符时拒绝；`--no-external-nn` 与正式构造链固定禁用外部权重；装载外部阶段时绕过运行缓存。经 C0f 修复后，覆盖真实 APE 默认旁置、显式目录、符号链接拒绝、损坏拒绝与正规链保持禁用；Darwin / Windows 仍标 `UNCOVERED`。
(2) **C3 进程所有权表 S1a-e 与 S2**：L0 增加进程所有权表，以槽位及代际绑定身份，明确脱离/放弃与退出超时处理；测试注入须显式开启并精确匹配 PID（未开启、缺 PID、错 PID 均零触发）。
(3) **Tape M1**：交付容器事实表、打包/往返检查、解释运行器与 Linux x86_64 冻结宿主槽工具；`tl_dce` 闭包通过静态准入，16/64 对象子集与原生逐字节一致，实测倍率 `94.0x` / `114.3x`（15 倍性能门未过）；102 对象全量等价与倍率未完成，不报全量倍率或等价通过。
(4) **批次容量 4096 与工具线子集**：批次登记容量提升至 4096；纳入 `dvreport` 超时单列 `stalled_phase`、开发服务共享总截止及已审核互斥安全子集。

## 严格证据

- 合并结果：`PRERUN_MERGE windows=298 batches=549 PASS=549 FAIL=0 UNKNOWN=0 SUPERSEDED=1`（来源：`~/.cache/unisacc/evidence-staging-0.1.3/release/evidence/0.1.3/strict.tsv`）。
- 补跑窗 `W9901-W9906` 实测原行（来源：`~/.cache/unisacc/evidence-staging-0.1.3/release/evidence/0.1.3/strict-W9901..W9906-*.tsv`）：
  - `W9901` (`tsv-lint-root`): `PASS rc=0 ms=11187`
  - `W9902` (`bpidecl`): `PASS rc=0 ms=2469`
  - `W9903` (`lane-key-probes`): `PASS rc=0 ms=20755`
  - `W9904` (`slow-split`): `FAIL rc=0 transport=-1 ms=55635`（冷态贴限超时）
  - `W9905` (`slow-split`): `PASS rc=0 ms=21690`
  - `W9906` (`slow-split`): `PASS rc=0 ms=20897`

## 已知缺口与未验证范围

(1) **推迟至 v0.1.4 的已知缺口**：s9 全套本版未跑；C3 S3、`--rebase-rev`、devsvc 提交互斥余项、CXIF 读探测续龄、S1 `deadline_at_ms` 字段复用留 v0.1.4；Tape 15 倍门未过且 102 对象全量未完成；Darwin / Windows 外部 NN 加载与 `sy_proc_finished` 仍为 `UNCOVERED`。
(2) **四次整链与三次预跑返工原因**：`C0c`/`C0d` 检查夹具缺陷、`C0f` A3-4 引导缺陷、`module-units-limits` 行级误报、`tests/env-semantics.tsv` 登记漏项。
(3) **冷态贴限与补跑**：`slow-split`、`tsv-lint-root`、`bpidecl`、`lane-key-probes` 在首次预跑中出现冷态贴限，已由 `W9901-W9906` 补跑验证（`W9904` 贴限 `55635ms` 失败后 `W9905`/`W9906` 均 `PASS`）。
(4) **证据丢失事件**：三个旧目录（`bg-b1.bak-pre-C0c`、`v0.1.3-bfdd1448ba.superseded-073824`、`v0.1.3-d07d9640ca.superseded-092413`）被 `moltbaby` 用户定时器误清，逐目录原行不可得，红停原件缺失仅保留监督输出重建件。
