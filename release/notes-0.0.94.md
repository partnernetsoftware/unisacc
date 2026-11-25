# unisacc 0.0.94 发布说明

状态: 候选预跑与阶段 A 定稿(S2 候选整链 d21f46dccf 已完链, S3 两段严格预跑 459 批已合并)

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | 04bda407373bcdc258243fbe825cd41869ace94f |
| 候选产品整链提交 (T = F3) | d21f46dccfa862b253a7579e9f9a008e2f6c881c (d21f46dccf) |
| S3 第一段全量 459 批预跑源提交 (F4) | 4a113b74cf028a0f34763cc17c6f51fef922415e (4a113b74cf) |
| 初始冻结行提交 (F0) | 9cf59a4cae438371ea165e42c1447cbcbb655c6c (9cf59a4cae) |
| 合并提交 (v0.0.94 -> main) | 544aa07b1651ad44d782db985514a97938363a0c (544aa07b16) |
| 中间冻结后提交 (F1, F2) | 473839db0b4cc4ff1431d6943e9b1bac1fe9c3cb (473839db0b), 6c4155af849082cbcfdaa978a35a2d9f120a56f6 (6c4155af84) |
| 发布 APE SHA256(unisacc.com) | e1408291e70767758f60a0a3598c11c3f47f2516cba0bc30739589e67085f1d1 |
| 源快照 SID | 14408789f1f0660dfd945e4f1f3f1ff14327da3258bf3ab0993fec8a7f2f42f9 |
| pkg.bin | 0f32c4449cc359b3926b5ccaefa5be0308928a734c1f7bf665b26b6493f7d77e |
| slice-win-arm64 | 089f5b275228ac6b64c329deb8e7183a68cee890879064c3b10fc70d7681ad4e |
| SHA256SUMS | 2d349a65a63a84ffb93597252cf375732ba9c0485f403f45efd113afc09c5ab3 |

| 对象 | 身份与字节数 | 回执来源路径 |
|---|---|---|
| 最终冻结与 S3 第二段补跑基点 (F = F5) | `04bda407373bcdc258243fbe825cd41869ace94f` (`04bda40737`, C 类 harness: `tools/check-signal-targets.cx` 父批补 3 条作业控制断言对齐 `slow-split` 多重集 + `tools/check-precommit-derive.cx:740` `ln -s` 改 `ln -sn`; `T..F5` 产品输入零改动) | `git rev-parse 04bda40737` / `release/candidate.tsv` |
| S3 第一段全量 459 批预跑基点 (F4) | `4a113b74cf028a0f34763cc17c6f51fef922415e` (`4a113b74cf`, C 类 harness: `tools/check-gen-all-check.cx` `--hang` 对齐 `precommit` 树隔离与冻结门加固) | `git rev-parse 4a113b74cf` |
| 候选整链基点 (T = F3) | `d21f46dccfa862b253a7579e9f9a008e2f6c881c` (`d21f46dccf`, A 类产品输入: `CXM_SYMS 2048 -> 4096`) | `git rev-parse d21f46dccf` / `release/candidate.tsv` |
| 初始冻结行基点 (F0) 与合并基点 (M) | `F0=9cf59a4cae438371ea165e42c1447cbcbb655c6c`, `M=544aa07b1651ad44d782db985514a97938363a0c` | `git rev-parse 9cf59a4cae 544aa07b16` |
| 链产出 APE (`chain_bytes.ape` / `unisacc-next.com`) | `e1408291e70767758f60a0a3598c11c3f47f2516cba0bc30739589e67085f1d1` (`9003052` 字节) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` / `release/evidence/0.0.94/pack-run.out` |
| 源快照 SID (`source-packages-v3`) | `14408789f1f0660dfd945e4f1f3f1ff14327da3258bf3ab0993fec8a7f2f42f9` (`T=d21f46dccf`、`F4=4a113b74cf` 与 `F5=04bda40737` 源 SID 一致) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ctx/context.tsv` / `release/evidence/0.0.94/chain-promote.txt` |
| 链产出权重包 (`chain_bytes.p3` / `compiler.p4.pkg` / `pkg.bin`) | `0f32c4449cc359b3926b5ccaefa5be0308928a734c1f7bf665b26b6493f7d77e` (`2904652` 字节, 带 16 字节 `UNIPKG1` 尾) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` / `release/evidence/0.0.94/pack-run.out` |
| 链产出包体 (`chain_bytes.p3_body` / `p3-package.body`) | `f6934ac8cd9f976c25d262a324511afa29158e0d0ea8147f809d2c9277c227e5` (`2904636` 字节) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` / `release/evidence/0.0.94/pack-run.out` |
| 固定点执行切片 `lnx/x86_64` (`fixedpoint/g2/lnx-x86_64/entry.elf`) | `6e93dd31f690ec4d290848b5144dfb58dadb43ec29394ff139b26140512643fc` (`2383590` 字节) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` |
| 固定点执行切片 `lnx/arm64` (`fixedpoint/g2/lnx-arm64/entry.elf`) | `b312bc47b63ee467928d61ea020e42465d731c6494362712574d6f0b043af8b4` (`2576046` 字节) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` |
| 固定点执行切片 `osx/x86_64` (`fixedpoint/g2/osx-x86_64/slice-osx-x86_64`) | `203cdb4c76aab1f73ddfa67975ecbbee622c745e512de503640b5b4a1a450335` (`3104482` 字节) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` |
| 固定点执行切片 `osx/arm64` (`fixedpoint/g2/osx-arm64/slice-osx-arm64`) | `19d683065d16a4461cc9a2924b5bc9317217d49fe97432258330aac197689731` (`4210786` 字节) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` |
| 固定点执行切片 `win/x86_64` (`fixedpoint/g2/win-x86_64/slice-win-x86_64`) | `c6cdc85497d2fa7ca0e20cfa76c68b7d19b0ed278a50242899d8cf0b4c41c097` (`3418624` 字节) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` |
| 发布资产 `slice-win-arm64` (`fixedpoint/g2/win-arm64/slice-win-arm64`) | `089f5b275228ac6b64c329deb8e7183a68cee890879064c3b10fc70d7681ad4e` (`4578816` 字节) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` / `release/candidate.tsv` |
| `cache-manager/osx/{x86_64,arm64}` (`EXPLICIT_INPUTS`) | `x86_64`: `fa4320aa40428d9449a873bcd3e57b5ceb9ad4f33fac4469e5fa2b9d1da5fa1c` (`247906` 字节); `arm64`: `55c53ae5c674157f9293b7b5b622972afbb3802a39c7712df6f03e7878162d12` (`280930` 字节) | `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt` |
| 发布资产 `SHA256SUMS` | `2d349a65a63a84ffb93597252cf375732ba9c0485f403f45efd113afc09c5ab3` | `release/candidate.tsv` |
| 正式原生 CI (`candidate-native.yml`) | `<CI_RUN_ID>` | `release/evidence/0.0.94/ci-<CI_RUN_ID>.log` |

## 严格证据

459 批:PASS 457、FAIL 0、UNKNOWN 2、INVALID 0

- **候选整链 (`T = F3 = d21f46dccfa862b253a7579e9f9a008e2f6c881c`, 第一段预跑源 `F4 = 4a113b74cf028a0f34763cc17c6f51fef922415e`, 最终冻结与第二段补跑源 `F = F5 = 04bda407373bcdc258243fbe825cd41869ace94f`)**: 8 段监督构造完成 (`SUPERVISE STOP iterations=8 rc=0`, `EXIT 1795395966 pid=3986835 rc=0 why=exited`, 证据路径 `~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/exit.tsv`、`seal.tsv`、`ape-final/ape-receipt.txt`、`release/evidence/0.0.94/chain-promote.txt`)。
  - 启动根与源身份: `bootstrap-sha=c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843` (`0.0.93` 公开件), `source=14408789f1f0660dfd945e4f1f3f1ff14327da3258bf3ab0993fec8a7f2f42f9` (`~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ctx/context.tsv`)。
  - 两代不动点 (`g2 == g3`) 与打包对拍: `compare-recheck rc=0` (`13.381s`), `compare rc=0` (`16.669s`), `pack rc=0` (`46.318s`), 产出 `unisacc-next.com` `sha256=e1408291e70767758f60a0a3598c11c3f47f2516cba0bc30739589e67085f1d1` (`9003052` 字节)、`compiler.p4.pkg` `sha256=0f32c4449cc359b3926b5ccaefa5be0308928a734c1f7bf665b26b6493f7d77e` (`2904652` 字节)、`p3-package.body` `sha256=f6934ac8cd9f976c25d262a324511afa29158e0d0ea8147f809d2c9277c227e5` (`2904636` 字节) 及六目标切片 (`~/.cache/unisacc/chains/bg-b1/d21f46dccfa862b253a7579e9f9a008e2f6c881c/ape-final/ape-receipt.txt`)。
- **冻结后五笔 F 提交 (`F1`..`F5`) 门禁与回执证据**:
  - `F1 = 473839db0b` (`build/lane-inputs.tsv` 移除已删除的 `ext/sysx/e2stream.cx` 陈旧行): 消除 `tools/chain-ready.cx` `15-gen-all DIFF`, `tools/derive.cx -- --check` 归零 (`rc=0`);
  - `F2 = 6c4155af84` (`tools/sixtarget-precompile.cx` `-cx-plan` 按每批 8 根分批并去重合并): `sh ./unisacc.com tools/check-sixtarget-precompile.cx` `PASS` (`rc=0`), `tools/chain-ready.cx` `19-chain-preflight` `sys-six-target` 转 `PASS` (`rc=0`);
  - `F3 = T = d21f46dccf` (`cx/cx0/plan.cx`、`cx/compiler/cxmod.cx`、`src/cxmod.c` 将 `CXM_SYMS` 从 `2048` 提升至 `4096`): `sh ./unisacc.com tools/cx0-extract.cx -- --check` `PASS` (`rc=0`), 重起 S2 候选整链 `gen1/g2/g3` 全六目标编译完成 (`rc=0`);
  - `F4 = 4a113b74cf` (`tools/check-gen-all-check.cx` `--hang` 对齐 11-22/23 `precommit` 门加固): `sh ./unisacc.com tools/check-gen-all-check.cx -- --hang` `2/0` (`33s`, `rc=0`)、`--drift` `3/0` (`rc=0`)、`--pass-gates` `1/0` (`rc=0`);
  - `F5 = F = 04bda40737` (`tools/check-signal-targets.cx` 父批补 3 条作业控制断言对齐 `slow-split` 多重集 `11 == 11`, 以及 `tools/check-precommit-derive.cx:740` `ln -s` 改 `ln -sn` 消除 `check-ln-noderef` 棘轮拒绝): 两处均为 C 类 harness 修复; `T (d21f46dccf) -> F5 (04bda40737)` 之间仅改 `tools/` 测试与冻结日辅助工具, 产品输入零改动, 源快照 SID 保持 `14408789f1f0660dfd945e4f1f3f1ff14327da3258bf3ab0993fec8a7f2f42f9`。
- **严格预跑 (`PRERUN_MERGE`, 候选 APE `e1408291e70767758f60a0a3598c11c3f47f2516cba0bc30739589e67085f1d1`, 两段源 rev `F4=4a113b74cf` + `F5=04bda40737`)**:
  - **两段式 `strict` 表构成与合并回执**:
    1. 段一 `~/.cache/unisacc/prerun/0.0.94-r2/strict-final.tsv` (`sha256=6aefbb0ec0e641f272b9ec6e8b91b44cd821079780526f79335c5a8de091c436`, 源 `F4=4a113b74cf`): `SUMMARY batches=459 PASS=455 FAIL=2 UNKNOWN=2 INVALID=0` (其中 `slow-split` 与 `check-ln-noderef` 为 `FAIL`);
    2. 段二 `~/.cache/unisacc/prerun/0.0.94-r3/strict-final.tsv` (`sha256=32eb58655967847ae852a4fccc13f51f3a4f1d62307eee0eb33646fd9b19eff5`, 源 `F5=04bda40737`): `SUMMARY batches=2 PASS=2 FAIL=0 UNKNOWN=0 INVALID=0` (同一候选 APE 字节补跑 `check-ln-noderef` `PASS` 与 `slow-split` `PASS`);
    3. 拼接表 `release/evidence/v0.0.94-strict-2026-11-23.tsv` (`sha256=0c5ebcc471886e555ac9c55f46e7dcbfd312e396507a80549fb7014d978b3b5e`): 第一段的两条 `FAIL` 记录按 `release-exec` 的记录顺序折叠为 `SUPERSEDED`, 终态为 `459 批:PASS 457、FAIL 0、UNKNOWN 2、INVALID 0` (`sh ./unisacc.com tools/release-flow.cx -- check --coverage tests/split-parents.tsv` 得 `COVERAGE gaps=0`, `rc=0`)。
  - `expected-fail`: `0 项` (`release/evidence/0.0.94/expected-fail.tsv` 仅表头)。
  - `expected-unmeasured` (`UNKNOWN`, 共 `2 项`, 见 `release/evidence/0.0.94/expected-unmeasured.tsv`):
    1. `cx-booter-ape` (`reason=no-producer`, `DEFERRED cx-booter-ape prepare-failed=no-producer=booter-raw-elf`, 证据 `release/evidence/0.0.94/cx-booter-ape-deferred.tsv`);
    2. `tools-default-regression` (`reason=no-producer`, `DEFERRED tools-default-regression prepare-failed=no-producer=old-checker`, 证据 `release/evidence/0.0.94/tools-default-regression-deferred.tsv`)。
- **`l0-posix2` 与 `l0-subreaper` 树源首跑与预跑覆盖边界**:
  - 树源首跑已在 `0.0.93` 编译器 (`candidate=c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843`, `SOURCE` 型读树源) 上实跑: `l0-posix2` 得 `CHECK_L0_POSIX2 passed=40 failed=0` (`6.3s`, `BATCH_RUN status=PASS`, `sh ./unisacc.com tools/check-l0-posix2.cx`), `l0-subreaper` 得 `checks=4 failed=0` (`4.9s`, `sh ./unisacc.com tools/check-l0-subreaper.cx`)。
  - 两批次未进入 `0.0.94` S3 预跑的 `459` 批窗口清单(`l0-posix2` 在 `tests/batches.tsv` 仍为 `REGISTERED_NORUN`, 且两批次未列入 `release/evidence/0.0.94/prerun-add-ids.tsv`), 故在 `0.0.94` 候选 APE 下未作为预跑批次运行(如实列于下方已知缺口)。
- **跨平台原生 CI (`candidate-native.yml`)**:
  - `<!-- S3B_S9_CI_PLACEHOLDER -->` 待 S3b `ci-early` 与 S9 正式 `candidate-native.yml` 完成后填入 `run_id` 与六目标格结果 (`release/evidence/0.0.94/ci-<CI_RUN_ID>.log`; 未拿到样本前 `osx`/`win` 不得声称经实机验证)。

## 产品改动

本版按 2026-11-23 fable 裁定 B (`3970fc6d5a`) 收缩范围, 仅含 `cx-L0` 库面四项必做 (`Z1a`/`Z1b`/`Z2`/`Z4`)、`std-surface` 修补及冻结后 `CXM_SYMS 2048 -> 4096` 扩容 (`Z3`/`Z3b`/`Z5`/`Z6` 撤出由 `0.0.96` 承接, 前端三项撤出由 `0.0.98`/`0.0.99` 前端分支承接):

- **Z1a (`sy_child_subreaper` 下沉核心)**: `exec/facts/sys-syscalls.tsv`、`cx/sys/sysnums.cx`、`cx/sys/posix.cx` 下沉 `prctl(PR_SET_CHILD_SUBREAPER)` 实现(`lnx x86_64=157`、`lnx arm64=167`、`PR_SET_CHILD_SUBREAPER=36`); `ext/sysx/reaper.cx` 移除平台分支与裸 `__syscall6`; `tools/check-l0-subreaper.cx` 登记批次 `l0-subreaper` (已在 `0.0.93` 编译器 `candidate=c561e7e0...` 加树源上首跑 `checks=4 failed=0`, `4.9s`, `sh ./unisacc.com tools/check-l0-subreaper.cx`); **`osx`/`win` 四目标均为 `-ENOSYS` 具名拒绝** (`osx`/`win` 调用号表项为 `0`, 没有 CI 样本, 验证级别为**仅读码**)。
- **Z1b (停止信号常量生成模块)**: `exec/facts/signal-targets.tsv` 经 `tools/signal-targets-gen.cx` 生成 `include/signal.h` 与 CX 常量(`lnx SIGSTOP=19`, `osx SIGSTOP=17`; `osx` 信号常量没有 CI 样本, 验证级别为**仅读码**), 去掉 `ext/sysx/reaper.cx` 停止信号平台 `#if`; `win` 仅保留进程内兼容数值且停止信号/树回收路径为 `-ENOSYS` 具名拒绝; **有样本无批次(仅有 `tools/check-signal-targets.cx`、`tools/check-signal-targets-part.cx` 与编译样本, 在 `0.0.94` 无专属测试批次, 覆盖为 0 个专属批次, 不得声称已获批次覆盖)**。
- **Z2 (`chown`/`fchmod`/`umask`/`pipe2` 核心 POSIX 接口补齐)**: 调用号入 `exec/facts/sys-syscalls.tsv` 生成 `cx/sys/sysnums.cx`, 实现落 `cx/sys/posix.cx` 与 `cx/sys/proc.cx`; **`sy_pipe2` (`pipe2`) 在 `osx` 上为 `pipe` 加 `fcntl(F_SETFD/F_SETFL)` 的非原子实现** (`cx/sys/proc.cx:225-240`, 不可声称原子 `CLOEXEC`), 在 `win` 上为 `-ENOSYS` 具名拒绝; **`osx`/`win` 调用号没有 CI 样本, 验证级别为仅读码**; `tools/check-l0-posix2.cx` 40 项检查登记批次 `l0-posix2` (`pipe2` 10、`umask` 8、`fchmod` 8、`chown` 8、表 5、`open` 1; 已在 `0.0.93` 编译器 `candidate=c561e7e0...` 加树源上首跑 `CHECK_L0_POSIX2 passed=40 failed=0`, `6.3s`, `BATCH_RUN status=PASS`)。
- **Z4 (`strtol` 家族回迁 `cx/text` + `sy_now_ms` 注释 + `std-surface` 修补)**: `cx/text/num.cx` 转为实体并持 `dock`, `ext/textx/num.cx` 去 `dock`; `cx/sys/time.cx` 补记 `sy_now_ms` 仅作单调差值时钟注释; `build/std-surface.tsv` 将 `tx_*` 从 `ext.textx` 迁至 `cx.text` 并修正 `sy_syscall6`/`sy_random` 的 `win` 口径为 `refuse:ENOSYS`; **有样本无批次(仅有编译与零散引用样本, 在 `0.0.94` 无专属测试批次, 覆盖为 0 个专属批次, 不得声称已获批次覆盖)**。

### 冻结后产品输入变更与 F 提交

1. `473839db0b` (`build/lane-inputs.tsv`, 冻结后 C 类 harness 派生表修正):
   - 触发原因: `chain-ready 15-gen-all` 检出 `build/lane-inputs.tsv` 残留已删除源文件 `ext/sysx/e2stream.cx` 的陈旧行(`15-gen-all DIFF`);
   - 修复动作: 移除该陈旧行, 使派生表与源树一致, 不改动任何产品源文件。
2. `6c4155af84` (`tools/sixtarget-precompile.cx`, 冻结后 C 类 harness 修正):
   - 触发原因: `0.0.94` 合入 Z1a/Z1b/Z2/Z4 新增导出符号后, `chain-ready 19-chain-preflight` 的 `sys-six-target` 步使用已装 `0.0.93` 根编译器(`CXM_SYMS=2048`)对 44 个产品根一次性执行 `-cx-plan` 时触发符号表上限拒绝(`too many names for the module check`);
   - 修复动作: 将 `tools/sixtarget-precompile.cx` 的 `-cx-plan` 调用改为按每批 8 根分批执行并去重合并编译计划(`tools/check-sixtarget-precompile.cx` `PASS`, `chain-ready 19-chain-preflight` 转 `PASS`), 不改动已装 `0.0.93` 根编译器与产品源文件。
3. `d21f46dccf` (`cx/cx0/plan.cx`、`cx/compiler/cxmod.cx`、`src/cxmod.c`, 冻结后 A 类产品输入变更, 重起 S2 整链):
   - 触发原因: S2 候选整链在 `gen1-osx-x86_64` 相位调用 `tools/whole-front.cx` 时, 对全部 44 个产品根一次性执行 `-cx-merge`: 合并闭包共 101 个 `.cx` 单元, 去重后跨模块符号数 `nsym=2104`, 超出原 `CXM_SYMS=2048` 上限 56 个符号, 导致 `osx`/`win` whole-front 确定性溢出(`too many names for the module check`);
   - 修复动作: 将 `cx/cx0/plan.cx` 与 `cx/compiler/cxmod.cx` 中的 `CXM_SYMS` 从 `2048` 提升至 `4096`, 并经 `sh ./unisacc.com tools/cx0-extract.cx -- --all` 同步派生 `src/cxmod.c`(`tools/cx0-extract.cx -- --check` `PASS`), 基于 `T = d21f46dccf` 重起 S2 候选整链并完链(`rc=0`)。
4. `4a113b74cf` (`tools/check-gen-all-check.cx`, 冻结后 C 类 harness 修正, 重起 S3 第一段全量 459 批严格预跑):
   - 触发原因: S3 严格预跑窗口 `W0074` 中批次 `gen-all-check` 连续两次 `FAIL`(`tools/check-gen-all-check.cx -- --hang` 子测试 H1/H2 失败);
   - 根因与修复动作: 2026-11-22/23 `tools/precommit.cx` 门加固后新增 `build/freeze.tsv` 冻结门与暂存区树隔离 (`tree_isolate_prepare`)。`check-gen-all-check.cx -- --hang` 用 `cp -a` 构造 minirepo 时继承了主仓活跃的 `build/freeze.tsv` (在进 `gen-all-check` 前先被冻结门拦截), 且写入的 H1/H2 替身脚本未 `git add` 进暂存区(导致隔离树仍跑真 `gen-all-check.cx`), 同时 H2 超时断言未对齐 `GATE_TIMEOUT gate=genall`。修复动作在 `--hang` 夹具中去掉继承的 `build/freeze.tsv`、将替身 `git add` 进暂存区、H2 断言对齐 `GATE_TIMEOUT gate=genall`(不动 `precommit.cx`、不删断言、不放宽时限; `sh ./unisacc.com tools/check-gen-all-check.cx -- --hang` `2/0` `33s`、`--drift` `3/0`、`--pass-gates` `1/0`, `rc=0`); `T (d21f46dccf) -> F4 (4a113b74cf)` 产品输入零改动, 源 SID 不变, 重起 S3 第一段全量 459 批严格预跑。
5. `04bda40737` (`tools/check-signal-targets.cx`、`tools/check-precommit-derive.cx`, 冻结后第五笔 C 类 harness 修正, S3 第二段 2 批补跑源):
   - 触发原因: `F4 = 4a113b74cf` 上 S3 第一段全量 `459` 批严格预跑跑出两批 harness `FAIL`: (a) `slow-split`: `c2f1f65883` (`Z1b`) 仅在分片 `tools/check-signal-targets-part.cx` 新增了 3 条作业控制信号断言而漏同步父批 `tools/check-signal-targets.cx`, 导致 `slow-split` 多重集比对 `8` 对 `11` 不一致; (b) `check-ln-noderef`: `411c12824b` 在 `tools/check-precommit-derive.cx:740` 引入裸 `ln -s` 而未带 `-n`, 触发 `check-ln-noderef` 变红;
   - 修复动作: 在 `tools/check-signal-targets.cx` 父批补齐 3 条作业控制断言使父子多重集对齐(`11` 对 `11`), 并将 `tools/check-precommit-derive.cx:740` 的 `ln -s` 改为 `ln -sn`; 两处均为 C 类 harness 修复, 产品输入零改动, 在 `F5 = 04bda40737` 上以同一候选 APE 字节补跑 `slow-split` 与 `check-ln-noderef` 两批 (`2/2 PASS`), 第一段的两条 `FAIL` 记录按 `release-exec` 的记录顺序折叠为 `SUPERSEDED`。

## 已知缺口

### 冻结时限与整体资格裁决披露
- (1) **超出 120 分钟冻结红线及原因**: 本版自合并冻结基点 `544aa07b16` / 初始冻结行 `9cf59a4cae` (`2026-11-23 08:10 +0800`) 起算, 冻结至公开整体耗时超出 120 分钟分治时限红线。超限原因为:
  - 冻结后发生 5 笔 F 提交: `F1=473839db0b` (`build/lane-inputs.tsv` 移除已删除的 `ext/sysx/e2stream.cx` 陈旧行)、`F2=6c4155af84` (`tools/sixtarget-precompile.cx` `-cx-plan` 按每批 8 根分批)、`F3=T=d21f46dccf` (`CXM_SYMS 2048 -> 4096` 产品输入变更)、`F4=4a113b74cf` (`tools/check-gen-all-check.cx` `--hang` 对齐 `precommit` 树隔离与冻结门加固)、`F5=04bda40737` (`tools/check-signal-targets.cx` 父批补 3 条作业控制断言 + `tools/check-precommit-derive.cx:740` `ln -s` 改 `ln -sn`);
  - S2 候选整链重起一次: 首轮 S2 (`6c4155af84`) 在 `gen1-osx-x86_64` 相位因 44 个产品根合并闭包 101 单元 `nsym=2104 > CXM_SYMS=2048` 溢出失败, 合入 `F3=d21f46dccf` 后重起 S2 整链(耗时约 `26` 分钟完链);
  - S3 严格预跑全量重起一次加第二段 2 批补跑一次: 首轮 S3 在窗口 `W0074` (`gen-all-check`) 因 `check-gen-all-check.cx -- --hang` 未同步 `precommit` 树隔离与 `build/freeze.tsv` 继承而两次 `FAIL`, 合入 `F4=4a113b74cf` 后重起 S3 第一段全量 `459` 批预跑; 第一段跑出 `slow-split` 与 `check-ln-noderef` 两批 harness `FAIL` 后, 合入 `F5=04bda40737` 并执行第二段 2 批补跑。
- (2) **严格预跑 `strict` 表为两段、两个源 rev 的如实披露**: 本版严格预跑 `strict` 表 (`release/evidence/v0.0.94-strict-2026-11-23.tsv`, `sha256=0c5ebcc471886e555ac9c55f46e7dcbfd312e396507a80549fb7014d978b3b5e`) 由两段组成、跨越两个源 rev(均仅差 C 类 harness, 产品输入与源快照 SID `14408789f1f0660dfd945e4f1f3f1ff14327da3258bf3ab0993fec8a7f2f42f9` 零改动): 第一段 (`~/.cache/unisacc/prerun/0.0.94-r2/strict-final.tsv`, `sha256=6aefbb0ec0e641f272b9ec6e8b91b44cd821079780526f79335c5a8de091c436`) 在 `F4 = 4a113b74cf` 上跑完被选全量 `459` 批(`455 PASS`、`2 FAIL`、`2 UNKNOWN`), 其中 `slow-split` (`c2f1f65883` 漏向父批同步 3 条作业控制断言, 多重集 `8` 对 `11`) 与 `check-ln-noderef` (`411c12824b` 在 `tools/check-precommit-derive.cx:740` 引入裸 `ln -s`) 两批 `FAIL`; 第二段 (`~/.cache/unisacc/prerun/0.0.94-r3/strict-final.tsv`, `sha256=32eb58655967847ae852a4fccc13f51f3a4f1d62307eee0eb33646fd9b19eff5`) 在 `F5 = 04bda40737` 上以同一候选 APE (`e1408291e70767758f60a0a3598c11c3f47f2516cba0bc30739589e67085f1d1`) 字节补跑这两批 (`2 PASS`), 第一段的两条 `FAIL` 记录按 `release-exec` 的记录顺序折叠为 `SUPERSEDED`。
- (3) **不得宣称全批发布资格 (保持 `NOT_QUALIFIED`)**: 全套 `s9` `daily`/`release` 长跑未运行, 整体裁决保持 `NOT_QUALIFIED`, 不得宣称全批资格。

### 未跑范围(`UNMEASURED`)
- (1) **`osx`/`win` 调用号与信号常量没有 CI 样本 (仅读码)**: `exec/facts/sys-syscalls.tsv` -> `cx/sys/sysnums.cx` 中的 `osx`/`win` 调用号(`prctl`/`chown`/`fchownat`/`fchmod`/`umask`/`pipe2`)与 `exec/facts/signal-targets.tsv` -> `include/signal.h` 中的 `osx`/`win` 信号常量(`SIGCHLD`/`SIGCONT`/`SIGSTOP`/`SIGTSTP`)没有 CI 实机样本, 验证级别为**仅读码** (`UNMEASURED`), 不得声称经实机运行验证;
- (2) **`Z1b` 与 `Z4` 有样本无批次**: `Z1b` (停止信号常量生成模块 `include/signal.h` 与 `reaper.cx` 去平台 `#if`) 与 `Z4` (`strtol` 家族回迁 `cx/text/num.cx` 实体、`sy_now_ms` 单调时钟注释、`build/std-surface.tsv` 修补) 在 `0.0.94` **仅有探针/编译样本, 无专属测试批次**(专属批次覆盖为 0 个批次, 只有零散引用, 不得声称已获批次覆盖);
- (3) **严格预跑 `strict` 表的 `expected-fail` (`0 项`) 与 `UNKNOWN` (`expected-unmeasured`, `2 项`)**:
  - `expected-fail`: `0 项` (`release/evidence/0.0.94/expected-fail.tsv` 无失败豁免项);
  - `expected-unmeasured` (`UNKNOWN`, 共 `2 项`, 见 `release/evidence/0.0.94/expected-unmeasured.tsv`):
    1. `cx-booter-ape`: `reason=no-producer` (`DEFERRED cx-booter-ape prepare-failed=no-producer=booter-raw-elf`, 证据 `release/evidence/0.0.94/cx-booter-ape-deferred.tsv`);
    2. `tools-default-regression`: `reason=no-producer` (`DEFERRED tools-default-regression prepare-failed=no-producer=old-checker`, 证据 `release/evidence/0.0.94/tools-default-regression-deferred.tsv`);
- (4) **`l0-posix2` 与 `l0-subreaper` 候选预跑未纳入**: 两批次已在 `0.0.93` 编译器 (`candidate=c561e7e0...`, `SOURCE` 型读树源) 上完成树源首跑 (`l0-posix2` `40/40`, `l0-subreaper` `4/4`), 因 `l0-posix2` 在 `tests/batches.tsv` 仍为 `REGISTERED_NORUN` 且两批次未列入 `release/evidence/0.0.94/prerun-add-ids.tsv`, 本轮 S3 `459` 批严格预跑未包含这两批; 且 `l0-posix2` 40 项仅覆盖 `Z2` (`pipe2` 10、`umask` 8、`fchmod` 8、`chown` 8、表 5、`open` 1, 其中 3 条表断言断的是 `arm64` 与 `osx` 的缺席)。

### 已知产品与工具缺口
- (1) **`Z3`/`Z3b`/`Z5`/`Z6` 已撤出**: `Z3`/`Z3b` (`sy_proc_capture` 双路捕获与 `np_exec` 薄封装)、`Z5` (`sy_child_t` `LIVE`/`REAPED` 状态位与 `sy_proc_kill` 回收后 `-ESRCH`)、`Z6` (`rp_kill_tree_after_wait` 跳过已回收原 PID) 已按范围裁定 B (`3970fc6d5a`) 从 `0.0.94` 撤出, 移交 `0.0.96` (`v0.0.96` 分支) 承接; 未验证项台账 `U31`/`U32` (`sy_proc_capture` `osx`/`win`) 与 `U33` (`sy_proc_kill` 回收后 pid 复用竞态, 改回 `OPEN`) 一并由 `0.0.96` 承接; 前端三项 (`A/X`、`G-redecl`、`D`) 同批撤出由 `0.0.98`/`0.0.99` 前端分支承接, 均不在 `0.0.94` 交付;
- (2) **`pipe2` (`sy_pipe2`) 在 `osx` 上非原子**: `osx` 无原生 `pipe2` 系统调用, `cx/sys/proc.cx:225-240` 在 `osx` 上用 `pipe` 加 `fcntl(F_SETFD/F_SETFL)` 非原子模拟(多线程并发 `fork+exec` 下存在描述子泄漏竞态窗口, 不可声称原子 `CLOEXEC`); 在 `win` 上为 `-ENOSYS` 具名拒绝 (`cx/sys/proc.cx:221-223`);
- (3) **`sy_child_subreaper` 在 `osx`/`win` 为 `-ENOSYS` 具名拒绝**: `sy_child_subreaper` (`Z1a`, `cx/sys/posix.cx`) 仅在 `lnx` (`x86_64=157`, `arm64=167`) 经 `prctl(PR_SET_CHILD_SUBREAPER)` 实现, 在 `osx` 与 `win` 四目标上调用号表项为 `0`, 均为 `-ENOSYS` 具名拒绝;
- (4) **`win` 停止信号为 `-ENOSYS` 具名拒绝**: `exec/facts/signal-targets.tsv` 在 `win` 仅保留进程内兼容数值(非原生 POSIX 信号投递), `ext/sysx/reaper.cx` 的 `sy_kill_tree` 在 `win` 及无 `/proc` 的 `osx` 均为 `-ENOSYS` 具名拒绝;
- (5) **`0.0.95` 独立版本边界**: `0.0.95` (分支 `v0.0.95`, `e2` 重扫修复主线, `../archive/plans-skipped/v0.0.95.md`) 为独立候选与独立版本槽, 其产品改动、严格证据、未判定项与回滚方案均不并入本版 `0.0.94`。

## 回滚方案

公开前已备好回滚方案, 详见 `release/rollback.txt` (草稿撤回 `v0.0.94`、Latest 移回 `v0.0.93`、前向撤销 `Y2`/`Y1`/`X`、冻结后 F 提交 `F5`/`F4`/`F3`/`F2`/`F1` 逆序回退清单与本机 Root 回退至 `0.0.93` `c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843`)。
