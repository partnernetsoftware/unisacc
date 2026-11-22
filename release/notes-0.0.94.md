# unisacc 0.0.94 发布说明

状态: 模板(冻结前置 8,冻结与验证前未填身份与结果,不预填 PASS 或哈希)

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | TBD |
| 候选提交 | TBD |
| 合并提交 (v0.0.94 -> main) | TBD |
| 发布 APE SHA256(unisacc.com) | TBD |
| 源快照 SID | TBD |
| pkg.bin | TBD |
| slice-win-arm64 | TBD |
| SHA256SUMS | TBD |

| 对象 | 身份与字节数 | 回执来源路径 |
|---|---|---|
| 最终冻结基点 (F) | `TBD` | `git rev-parse TBD` / `release/candidate.tsv` |
| 候选整链与预跑基点 (T) | `TBD` | `git rev-parse TBD` / `release/candidate.tsv` |
| 链产出 APE (`chain_bytes.ape` / `unisacc-next.com`) | `TBD` | `release/evidence/0.0.94/pack-run.out` |
| 源快照 SID (`source-packages-v3`) | `TBD` | `release/evidence/0.0.94/chain-promote.txt` |
| 链产出权重包 (`chain_bytes.p3` / `compiler.p4.pkg` / `pkg.bin`) | `TBD` | `release/evidence/0.0.94/pack-run.out` |
| 链产出包体 (`chain_bytes.p3_body` / `p3-package.body`) | `TBD` | `release/evidence/0.0.94/pack-run.out` |
| 发布资产 `slice-win-arm64` (`asset_bytes.slice_win_arm64`) | `TBD` | `release/candidate.tsv` / `release/evidence/0.0.94/pack-run.out` |
| 发布资产 `SHA256SUMS` | `TBD` | `release/candidate.tsv` |
| 正式原生 CI (`candidate-native.yml`) | `<CI_RUN_ID>` | `release/evidence/0.0.94/ci-<CI_RUN_ID>.log` |

## 严格证据

TBD 批:PASS TBD、FAIL TBD、UNKNOWN TBD、INVALID TBD (待 S2 整链、S3 严格预跑、S3b ci-early 与 S9 正式 CI 实跑后据回执填齐,禁止预填)

- **候选整链 (`T = TBD`, `F = TBD`)**: `TBD` (`release/evidence/0.0.94/chain-promote.txt`)。
  - `chain_bytes` 与 `0.0.93` (`c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843`) 逐项对拍: `TBD` (六目标固定点执行切片、`unisacc-next.com`、`p3`、`p3-package.body`、`cache-manager/osx/{x86_64,arm64}`)。
- **冻结前门禁 (`T = TBD`)**:
  - `tools/derive.cx -- --check`: `TBD`
  - `tools/ratchets.cx`: `TBD` (`release/evidence/0.0.94/ratchets.txt`)
  - `tools/chain-ready.cx -- --version 0.0.94`: `TBD`
- **`ci-early` 原生样本 (含 Z2 `surf-probe` 四项探针 `sy_fchmod`/`sy_chown`/`sy_umask`/`sy_pipe2`)**: `TBD` (未拿到样本前不得声称 osx/win 经实机验证)。
- **严格预跑 (`PRERUN_MERGE`)**: `TBD` (`release/evidence/v0.0.94-strict-TBD.tsv`, `strict-final.tsv sha256=TBD`)。
  - `expected-fail`: `TBD` (逐条列出 `release/evidence/0.0.94/expected-fail.tsv` 全部行; 0 项须显式写 0 项)。
  - `expected-unmeasured` (`UNKNOWN`): `TBD` (逐条列出 `release/evidence/0.0.94/expected-unmeasured.tsv` 全部行及 `*-deferred.tsv` 绑定哈希)。
- **`l0-posix2` 候选产品重建补跑**: `TBD` (当前分支状态 `REGISTERED_NORUN`; 若到冻结时仍未在重建后产品上实跑, 须保持 `REGISTERED_NORUN` 并留作已知缺口, 不得记 PASS)。

## 产品改动

本版按 2026-11-23 fable 裁定 B 收缩范围, 仅含 `cx-L0` 库面四项必做与 `std-surface` 修补(Z3/Z3b/Z5/Z6 撤出由 `v0.0.96` 承接, 前端三项撤出由前端类候选编译覆盖缺口第二期承接):

- **Z1a (`sy_child_subreaper` 下沉核心)**: `exec/facts/sys-syscalls.tsv`、`cx/sys/sysnums.cx`、`cx/sys/posix.cx` 下沉 `prctl(PR_SET_CHILD_SUBREAPER)` 实现; `ext/sysx/reaper.cx` 移除平台分支与裸 `__syscall6`; `tools/check-l0-subreaper.cx` 登记批次 `l0-subreaper`; osx/win 为具名拒绝 `-ENOSYS`。
- **Z1b (停止信号常量生成模块)**: `exec/facts/signal-targets.tsv` 经 `tools/signal-targets-gen.cx` 生成 `include/signal.h` 与 CX 常量(`lnx SIGSTOP=19`, `osx SIGSTOP=17`), 去掉 `reaper.cx` 停止信号平台 `#if`。
- **Z2 (`chown`/`fchmod`/`umask`/`pipe2` 核心 POSIX 接口补齐)**: 调用号入 `exec/facts/sys-syscalls.tsv` 生成 `cx/sys/sysnums.cx`, 实现落 `cx/sys/posix.cx` 与 `cx/sys/proc.cx` (`sy_pipe2`); 补齐 `surf-probe` 四项 L0 探针供 `ci-early`/`candidate-native` 采集 osx 样本; `tools/check-l0-posix2.cx` 40 项检查登记批次 `l0-posix2` (待产品重建后补跑)。
- **Z4 (`strtol` 家族回迁 `cx/text` + `sy_now_ms` 注释 + `std-surface` 修补)**: `cx/text/num.cx` 转为实体并持 `dock`, `ext/textx/num.cx` 去 `dock`; `cx/sys/time.cx` 补记 `sy_now_ms` 仅作单调差值时钟注释; `build/std-surface.tsv` 将 `tx_*` 从 `ext.textx` 迁至 `cx.text` 并修正 `sy_syscall6`/`sy_random` 的 win 口径为 `refuse:ENOSYS`。

## 已知缺口

### 未跑范围(`UNMEASURED`)
- (1) 全套 `s9` `daily`/`release` 长跑未运行, 整体裁决保持 `NOT_QUALIFIED`, 不声称全批资格;
- (2) `l0-posix2` 批次当前为 `REGISTERED_NORUN` (需在 0.0.94 候选产品重建后实跑; 未实跑前不得记 PASS);
- (3) 严格预跑中的 `UNKNOWN` 与 `expected-fail` 项: 待 S3 严格预跑完成后逐条列出批次名、原因与 `release/evidence/0.0.94/{expected-unmeasured.tsv,expected-fail.tsv}` 绑定证据(不得省略或合并为总数);
- (4) osx/win 调用号与信号常量实机样本: 仅在 `ci-early` 或正式 `candidate-native.yml` 拿到真实样本并实测通过后才可记 CI 样本; 未拿到样本的项必须如实写「仅读码」, 不得声称经实机运行验证。

### 已知产品与工具缺口
- (1) `U31`/`U32` 已随 Z3/Z3b 迁入 `v0.0.96` 承接; `U33` (`sy_proc_kill` 回收后 pid 复用竞态, `build/state-unverified.tsv`) 随 Z5 撤出改回 `OPEN`, 由 `v0.0.96` 承接;
- (2) Z3/Z3b (`sy_proc_capture` 与 `np_exec` 薄封装)、Z5 (`sy_child_t` `LIVE`/`REAPED` 状态位)、Z6 (`rp_kill_tree_after_wait` 跳过已回收原 PID) 及前端三项(`A/X`、`G-redecl`、`D`)已从本版撤出(`3970fc6d5a`), 不在本版交付;
- (3) `win` 停止信号表达法待裁; `osx`/`win` 的 `sy_child_subreaper` 为具名拒绝 `-ENOSYS`;
- (4) `pipe2` 在 `osx` 上为 `pipe` 加 `fcntl` 的非原子实现(不可声称原子 `CLOEXEC`);
- (5) 0.0.95 若同期冻结, 其未判定项不并入本版发布说明。

## 回滚方案

公开前已备好回滚方案, 详见 `release/rollback.txt` (草稿撤回 `v0.0.94`、Latest 移回 `v0.0.93`、前向撤销 `Y2`/`Y1`/`X` 与本机 Root 回退至 `0.0.93` `c561e7e0...`)。
