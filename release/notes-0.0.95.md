# unisacc 0.0.95 发布说明

状态: 模板(冻结准备, 冻结与验证前未填身份与结果, 不预填 PASS 或哈希; 接力顺序: 0.0.94 先公开后再冻结本版)

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | TBD |
| 候选提交 | TBD |
| 合并提交 (v0.0.95 -> main) | TBD |
| 发布 APE SHA256(unisacc.com) | TBD |
| 源快照 SID | TBD |
| pkg.bin | TBD |
| slice-win-arm64 | TBD |
| SHA256SUMS | TBD |

| 对象 | 身份与字节数 | 回执来源路径 |
|---|---|---|
| 最终冻结基点 (F) | `TBD` | `git rev-parse TBD` / `release/candidate.tsv` |
| 候选整链与预跑基点 (T) | `TBD` | `git rev-parse TBD` / `release/candidate.tsv` |
| 链产出 APE (`chain_bytes.ape` / `unisacc-next.com`) | `TBD` | `release/evidence/0.0.95/pack-run.out` |
| 源快照 SID (`source-packages-v3`) | `TBD` | `release/evidence/0.0.95/chain-promote.txt` |
| 链产出权重包 (`chain_bytes.p3` / `compiler.p4.pkg` / `pkg.bin`) | `TBD` | `release/evidence/0.0.95/pack-run.out` |
| 链产出包体 (`chain_bytes.p3_body` / `p3-package.body`) | `TBD` | `release/evidence/0.0.95/pack-run.out` |
| 发布资产 `slice-win-arm64` (`asset_bytes.slice_win_arm64`) | `TBD` | `release/candidate.tsv` / `release/evidence/0.0.95/pack-run.out` |
| 发布资产 `SHA256SUMS` | `TBD` | `release/candidate.tsv` |
| 正式原生 CI (`candidate-native.yml`) | `<CI_RUN_ID>` | `release/evidence/0.0.95/ci-<CI_RUN_ID>.log` |

## 严格证据

TBD 批:PASS TBD、FAIL TBD、UNKNOWN TBD、INVALID TBD (待 S2 整链、S3 严格预跑、S3b ci-early 与 S9 正式 CI 实跑后据回执填齐, 禁止预填)

- **候选整链 (`T = TBD`, `F = TBD`)**: `TBD` (`release/evidence/0.0.95/chain-promote.txt`)。
  - `chain_bytes` 与上一公开版逐项对拍: `TBD` (六目标固定点执行切片、`unisacc-next.com`、`p3`、`p3-package.body`、`cache-manager/osx/{x86_64,arm64}`)。
- **冻结前门禁 (`T = TBD`)**:
  - `tools/derive.cx -- --check`: `TBD`
  - `tools/ratchets.cx`: `TBD` (`release/evidence/0.0.95/ratchets.txt`)
  - `tools/chain-ready.cx -- --version 0.0.95`: `TBD`
- **0.0.95 等价对拍三条证据与阳性对照(冻结候选回执待入 `release/evidence/0.0.95/`)**:
  - (1) `-o` 1154 例字节对拍 (`diff=0`): `TBD` (分支开发期实测待绑定冻结候选回执 `release/evidence/0.0.95/pair-o.txt`);
  - (2) `pp-equiv` 5128 例完整 `UNIPP2` 对拍 (`diff=0`): `TBD` (分支开发期实测待绑定冻结候选回执 `release/evidence/0.0.95/pp-equiv.txt`);
  - (3) 单元 `113` 例 `unit-e2-compare` 对拍 (`diff=0`): `TBD` (分支开发期实测待绑定冻结候选回执 `release/evidence/0.0.95/unit-e2-compare.txt`);
  - (4) 阳性对照(证明对拍比较器可检出真实差异, 由 `omp-go` 补齐中): `TBD` (`release/evidence/0.0.95/positive-control.txt`)。
- **步数门与 `e2` 推理毫秒门(提交后树 `40df7913c5` 参考, 冻结候选须重测入回执)**:
  - `examples/hello.c` (`-run`, `HELLO_OBJ_CMP=IDENTICAL`): `e2` 步数 `1200813 -> 484709` (`-59.6%`), `e2` 推理耗时 `56.0 ms -> 42.7 ms` (见 `15cf2019d7611ff:plans/v0.0.95.md` 第七节; 冻结候选实测回执 `TBD`);
  - `tools/phase-budget.cx` (`-c`, `PB_OBJ_CMP=IDENTICAL`): `e2` 步数 `3960979 -> 2478603` (`-37.4%`), `e2` 推理耗时 `172.4 ms -> 123.7 ms` (见 `15cf2019d7611ff:plans/v0.0.95.md` 第七节; 冻结候选实测回执 `TBD`)。
- **`ci-early` 原生样本**: `TBD` (`release/evidence/0.0.95/ci-early.txt`, 未拿到样本前不得声称 osx/win 经实机验证)。
- **严格预跑 (`PRERUN_MERGE`)**: `TBD` (`release/evidence/v0.0.95-strict-TBD.tsv`, `strict-final.tsv sha256=TBD`)。
  - `expected-fail`: `TBD` (逐条列出 `release/evidence/0.0.95/expected-fail.tsv` 全部行; 0 项须显式写 0 项)。
  - `expected-unmeasured` (`UNKNOWN`): `TBD` (逐条列出 `release/evidence/0.0.95/expected-unmeasured.tsv` 全部行及 `*-deferred.tsv` 绑定哈希)。

## 产品改动

本版按 2026-11-22 fable 裁定 A 收口范围(`a_class=e2 预处理步数(模型与网)`, `a_files_cap=4`), 唯一产品输入改动为 `e2 #include` 前缀重扫修复(`40df7913c5`, A 类 `3/4`):

- **`e2 #include` 前缀重扫修复 (`INC_SUB`/`INPUSHXE`/`SPAN2`)**: 仅修改 3 张手写事实表 `exec/pp/directive-body-byte.tsv`、`exec/pp/text-byte.tsv`、`exec/pp/text-result.tsv`, 消除 `#include` 展开路径上的重复前缀重扫与冗余步数, 保持 `e2` 输出字节等价。
- **范围裁定与明确不做**:
  - `L5(a)` 前导 `e2` 预制: 实测净值为负(带实质头前导命中 `0/11`, 现场索引 53-74 ms), 已移出本版回 `prd.md` 候选池;
  - 可选项「`e2` 逐字节拷贝改段拷贝」(含阶段 1 `LOC.text`/`ACP`): 在提交后树 `40df7913c5` 经 `sim.cx` 复测剩余可省上界仅约 4-5 ms (低于 8 ms 复测门槛) 且改动 5+ 张表将超出 `a_files_cap=4`, 裁定不做。

## 已知缺口

### 未跑范围(`UNMEASURED`)
- (1) 全套 `s9` `daily`/`release` 长跑未运行, 整体裁决保持 `NOT_QUALIFIED`, 不声称全批资格;
- (2) 等价对拍未覆盖 `-o` 之外的编译与输出模式(例如 `-E` 多输出、`-S`、直接 `-run` 全量语料等未做同规模逐例对拍, 仅覆盖 `-o` 1154、`pp-equiv` 5128、单元 113 三类范围);
- (3) `tools -E` 绝大多数样本与 `4000` 次引入上限因 `multiple preprocessing outputs not migrated` 仅核对拒绝合同, 真实帧容量溢出未测; 带嵌套 `#include` 的强制头两边返回 `rc=2` 未判定;
- (4) 阳性对照(由 `omp-go` 补齐中)与冻结候选树上的 S2/S3/S9 正式回执尚未落盘前, 不得声称冻结候选已完成全量等价验证;
- (5) 严格预跑中的 `UNKNOWN` 与 `expected-fail` 项: 待 S3 严格预跑完成后逐条列出批次名、原因与 `release/evidence/0.0.95/{expected-unmeasured.tsv,expected-fail.tsv}` 绑定证据(不得省略或合并为总数)。

### 已知产品与工具缺口
- (1) `REFUSE_SAME` 的 8 个样本为产品既有拒绝行为(修改前后均按相同拒绝合同返回, 非本版引入回归, 亦非本版修复目标);
- (2) 端到端墙钟收益在 `examples/hello.c` ABAB 5 轮实测中受系统负载噪声影响(`-243` 到 `+125 ms`, R2 出现噪声反转), 本版仅以确定性 `e2` 步数门与 `e2` 推理毫秒门作为度量口径, 不承诺固定端到端墙钟降幅;
- (3) `L5(a)` 前导 `e2` 预制与 `e2` 段拷贝均不在本版交付;
- (4) 与 `0.0.94` 的接力与说明边界: `0.0.94` 先公开后再合并 `v0.0.95` 并冻结本版; `0.0.94` 的产品改动、证据与未判定项不并入本版发布说明, 两版各自独立绑定冻结提交 `F`、候选 APE 与 `release/evidence/0.0.NN/` 目录。

## 回滚方案

公开前已备好回滚方案, 正式冻结接力后写入 `release/rollback.txt` (当前预备模板见 `archive/release-pre0100/rollback-0.0.95.txt`, 含草稿撤回 `v0.0.95`、Latest 移回上一公开版、前向撤销 `Y2`/`Y1`/`X` 与本机 Root 回退流程)。
