# unisacc 0.1.1 发布说明(产品车:诊断定位;2026-11-25)

状态:同一候选通过六格原生 CI,结论为 QUALIFIED(run 45578960112,attempt=1,status=completed、conclusion=success,6/6 候选目标全绿、42/0)。本版为产品车(唯一大改:诊断定位),未执行全套 s9,未取得全套 s9 资格;同候选六格原生 CI 与全套 s9 是两件事,不得混称。

## 1. 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(F6=起链提交) | `b3e9731b6ada2e5ffb34f5a74061e5e8cb1dfa64` (短 `b3e9731b6a`, `0.1.1-FREEZE base=f3793eb4e343a433a011f3d9b322a1fee915d1f0`, 基线 `main 864de15886`, 合并 `main 4869acd2ae`) |
| 发布 APE SHA256 (`unisacc.com`) | `d83537aef92eb55ed94d81081b0cff27d146a0ef0d83b2a544b989c15701c3eb` (`8,992,558` 字节; 本车为产品车, APE 与 `0.1.0` 有产品字节差异) |
| 源快照 `SID6` (`SOURCES_SID`) | `428897cabef7943a271c5642402466a57eaba227e2e34dbad30889539e568edb` (与 `0.1.0` 的 `37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238` 不同; 差异即本车诊断定位与 `S0` 表级改动闭包) |
| `X` 提交 | `adea1d7873f01c60dc9d086760230074101ed033` (短 `adea1d7873`, `2026-11-25 16:44:23 +0800`) |
| CI run | `45578960112` (`attempt=1`, `workflow_dispatch`, `headBranch=v0.1.1`, `headSha=adea1d7873f01c60dc9d086760230074101ed033`, `QUALIFIED 42/0`) |
| `pkg.bin` / `slice-win-arm64` / 两个 `build.json` / `SHA256SUMS` | `pkg.bin` `4d13b970d75732d61126e67f1f4574a70f48eb9675b95900453dfd370d3b4f1b`; `slice-win-arm64` `4af8e29e8cfa2c6122206c1002f00be1ec543100194b96b3e437f3e418f03e37`; `unisacc.com.build.json` 与 `build.json` 同为 `ff9df370e74c1fb842c925823bd15569ccf638ab9c6ac4a8db198dcd2f943e25`; `SHA256SUMS` `3cddbd09fe37324ce9e117d29143a524e6a45f6a396db67b4039258a7298ea88` |

## 2. 已落产品改动 (`main..b3e9731b6a`, 唯一大改:诊断定位)

(1) 任务 1.1 (`2fff21e634`):重放 `src/v0.0.108` 的 `S0 forward-result.tsv` 表级修复(`check-forward-limit-table ok=3 failed=0`)与 `c16` `cxmod`/`plan.cx` `no file` 候选路径(`searched` / `package-root`)。
(2) 任务 1.2 (`e5e2a225b4`):三源(`cx/compiler/cxmod.cx`、`src/cxmod.c`、`cx/cx0/plan.cx`)为 `does not export` 增加 `use` 行号定位(`cxm_uline[u][k]`, 对应 `c17`)。
(3) 任务 1.4 (`9ce7936228`):`cx/driver/output.cx` (`cxo_warn_src`)在 `-o` 目标为已有源文件(`.cx`/`.c`/`.h`/`.ts`/`.md`/`.tsv`)时向 `stderr` 输出警告,源文件字节不变。
(4) 编号与三源对齐 (`eedbb23506`、`f3793eb4e3`、`b6c9feb46d`):切 `plans/v0.1.1.md` 与 `build/version-seq.tsv` 槽位 `66`;恢复三源 `CXM_MAXU=256` (`3a-cxmod-tri passed=25 failed=0`、`3b-cx0-extract EXACT`);脱敏 `release/evidence/0.1.0/backup-evidence.tsv` (`no-home-path over=0`)。
(5) 未入本版与顺延项:
- 任务 1.3 (`e2`/`e1` 拒绝接定位):按 3 版整合令 `14:15` 截断,顺延 `0.1.2`。
- 任务 1.5 (`diag-contract` 基线):`DC_GAP_BASELINE=15` 保持实测值,与任务 1.3 一并在 `0.1.2` 收口。

## 3. 整链、pack、warm 与严格预跑实测

- 整链实测:`T1 = 2026-11-25 14:36:35 +0800` (`1795588595`, `owner.tsv`),`T2 = 2026-11-25 15:01:27 +0800` (`1795590087`, `exit.tsv rc=0 why=exited`),用时 `24.9` 分钟(`1492s = 24 分 52 秒`, `499/499 DONE`)。
- 不动点 (`compare_attempt=243`):`objects=same count=102`、`kernels=same count=2`、`slices=same count=6`、`product=same`、`package=present sha=4d13b970d75732d61126e67f1f4574a70f48eb9675b95900453dfd370d3b4f1b`;六目标 `g2/g3` 逐对 SHA256 相同(从 `release/evidence/0.1.1/fixedpoint-6targets.tsv` `sha256=9b76071245d814bc20d419630f23e0bd69a50c9bd5ad35d9442b4a55688a66c2` 机器提取, `ape-receipt.txt sha256=cabaac622b5c749cf8d982f5d8515d6630207063c42dfa3262ee2edd15dfa67d`):
  - `lnx/x86_64` (`fixedpoint/g2/lnx-x86_64/entry.elf`): `4542533b550c284189f91de4000f434be74baec4790128aee218c7a9b85402ed` (`1954382` bytes, `SAME`)
  - `lnx/arm64` (`fixedpoint/g2/lnx-arm64/entry.elf`): `f7ab55f6b8cb1466c517e0f3b25c819630317d572202e5fab224593721df055c` (`2105878` bytes, `SAME`)
  - `osx/x86_64` (`fixedpoint/g2/osx-x86_64/slice-osx-x86_64`): `cc9a98e17f2c15f582daebcaa1bd88b3d7099c83ad50aac4291ea10678f81114` (`3203554` bytes, `SAME`)
  - `osx/arm64` (`fixedpoint/g2/osx-arm64/slice-osx-arm64`): `b10fc08cc1436fd57653f245e99547eb87453bff46093b8a1fbaf43d4e0cf3a5` (`4326370` bytes, `SAME`)
  - `win/x86_64` (`fixedpoint/g2/win-x86_64/slice-win-x86_64`): `ff49983e97b4f3f8a58a5235f717704a2792d0ce4310f530395a541e37dcbc46` (`3514368` bytes, `SAME`)
  - `win/arm64` (`fixedpoint/g2/win-arm64/slice-win-arm64`): `7f29174a5232c63a0906496d03bc844b4b2e41bc3944b77ad06bbb6f96f72ee1` (`4708352` bytes, `SAME`)
- pack 与 warm:`RELEASE_RUN RESULT PACK_OK`, `PACK_APE SAME sha256=d83537aef92eb55ed94d81081b0cff27d146a0ef0d83b2a544b989c15701c3eb`;`WARM RESULT READY key=724591f94ed13900544d968b66275d10ac18ad63316d201d098b2d88f2a595bb targets=5`;`draft-readback.txt sha256=de6070fc18eeb8893e359f0c69183acc38e55d2cce565d9c813acc11341d13c6`。
- 严格预跑 (`release/evidence/0.1.1/strict.tsv`, `sha256=749ff13b6594726176d231fb5397b6285a82480fb6fb2dc5b3a7f534959c0b59`):
  - 预跑 `246` 窗 (`15:08` -> `16:18`, `522` 批, `split_parents_excluded=9`):原合并为 `519 PASS / 1 FAIL (slow-split W0087 55456ms) / 2 UNKNOWN (precommit-list-cap W0014 55531ms, tsv-lint-root W0241.z06 55583ms) / 0 INVALID`。
  - 同候选(`d83537aef92eb55ed94d81081b0cff27d146a0ef0d83b2a544b989c15701c3eb`)同布局热编译补跑两窗:`W9941 tsv-lint-root PASS 11085ms load1=3.48 assertions=2`、`W9987 slow-split PASS 21760ms load1=9.51 assertions=5`。
  - 合并折叠后:`rows=524, expected_fail=0, superseded=2; SUMMARY 522/521/0/1/0` (`521 PASS / 0 FAIL / 1 UNKNOWN / 0 INVALID`);唯一 `UNKNOWN` 为已登记贴限批 `precommit-list-cap` (`55531ms load1=5.68`, `release/evidence/0.1.1/expected-unmeasured.tsv` 与 `release/evidence/0.1.1/list-cap-margin.md`);`ratchets 19/0`。

## 4. 准入七条(产品车口径)

| # | 条件 | 本版状态 |
|---|---|---|
| 1 | 六目标不动点连续 3 版 | `0.0.106`、`0.0.107`、`0.1.0` 已归档;`0.1.1` 归档第四套(产品字节变化仍四同且六目标 `g2/g3` `6/6` 全同) |
| 2 | CI 六格 QUALIFIED 且 strict FAIL/UNKNOWN=0 | CI `run 45578960112` (`attempt=1`) `6/6` 全绿 `QUALIFIED 42/0`;strict `521 PASS / 0 FAIL / 1 UNKNOWN / 0 INVALID`,唯一 `UNKNOWN` 为已登记且证据可核项 `precommit-list-cap` (`window-timeout`) |
| 3 | 零已知 P0 | P0 账本已清零或按 3 版整合令降级 P1(`D1 admission`、`T11` 甲件、候选 `17`、`R1-H1`、候选 `19` 均入 `0.1.2`/`0.1.3`) |
| 4 | `state-unverified` 声称类 `<=3` | 未满足,按 R38 口径如实披露 |
| 5 | 前端覆盖三期 | 未满足,按 R37 豁免;`B2` 前端事实表化顺延 `0.1.2` |
| 6 | 起链到公开 `<=120` 分钟 (`0.1.1` fable 线 `130` 分钟) | 未满足;`T1 = 14:36:35 +0800` (`1795588595`) 到 `T4 = 16:54:30 +0800` (`1795596870`, `exec-5-publish`) = `137.9` 分钟 (`8275s`);`T2 = 15:01:27 +0800` (`1795590087`, `exit.tsv`) 到 `T4 = 113.1` 分钟 (`6783s`);`T2` 到 `X` (`16:44:23 +0800`) = `102.9` 分钟 (`6176s`),按 fable 口径如实记 |
| 7 | 能用清单 30 过 25 | 清单 `29` 项就绪;唯一保留缺口 `release-chain` 批登记 `0.1.x` |

## 5. 已知缺口与未跑范围

(1) `precommit-list-cap` 设计性贴限:本版 `W0014` 两次 `rc=124` (`55531ms load1=5.68`),按 `expected-unmeasured.tsv` (`window-timeout`) 与 `list-cap-margin.md` 披露;不放宽 `55` 秒、不缩覆盖,已在 `0.1.2` (V2 `C1`) 拆为四片(`precommit-list-cap-a/b/c/d`)。
(2) V2 并行编译期间 `W0241` 8 并行窗超时、串行恢复通过:预跑 `W0241` (`normal`, 8 并行) `attempt=1 rc=124 58044ms` 时 `load1` 达 `14..17` (含 V2 并行编译负载),经 `PRERUN_RECOVER` 自动降为串行恢复窗 `W0241.z01..z08`;其中 `W0241.z06` (`tsv-lint-root`) 冷编贴限 `55583ms`,同布局热编译补跑 `W9941` `PASS 11085ms load1=3.48 assertions=2`,恢复批全部 `PASS`。
(3) 预跑贴限与重试披露:
- `W0000` (`bpidecl`):首试 `rc=124` (`58047ms`, 与同期 `T0.1` 实跑约 `18s` 重叠),次试 `PASS` (`54306ms`, 超余量线 `11.405s`);
- `W0079` (`heavy`):首试 `rc=0` 但 `wall_ms=57612` `sum_batch_ms=55521` 贴 `55s` 线,次试 `PASS` (`wall_ms=40996` `sum_batch_ms=38851`);
- `W0087` (`slow-split`):原窗冷编子工具链 `55456ms`,同布局热编译补跑 `W9987` `PASS 21760ms load1=9.51 assertions=5`。
(4) `D1 admission` 冷态合同:`0 < ms < 12000` 在 `UNISACC_RUN_CACHE=0` 真批下不可达(`tlib.cx -- kit` 冷编固定开销 `11770ms + 491ms = 12261ms > 12000ms`,最小真批 `cx-bounded` 实测 `strict_ms = 14185..14295ms`),按 3 版整合令降为 P1 在后续工具车收口。
(5) 转发调用上限:`8192` 仅表级证据(`check-forward-limit-table ok=3 failed=0`);实测约 `355` 个以上不同被调用转发符号即生成物畸形失败、`>=413` 段错误,产品级上限在 `0.1.3` (V3) 收口,未测写 `UNMEASURED`。
(6) 未跑范围:`U05` (`provenance`)未测;全套 `s9` 未跑;严格证据所列 `522` 批之外的范围 `UNMEASURED`。

## 6. 证据入口与撤回方案

- 证据目录:`release/evidence/0.1.1/INDEX.tsv`、`release/evidence/0.1.1/strict.tsv`、`release/evidence/0.1.1/expected-unmeasured.tsv`、`release/evidence/0.1.1/list-cap-margin.md`、`release/evidence/0.1.1/fixedpoint-6targets.tsv`、`release/evidence/0.1.1/draft-readback.txt`、`release/evidence/0.1.1/ci-45578960112.json`、`release/evidence/0.1.1/ci-45578960112.log`、`release/evidence/0.1.1/public-readback.txt`、`release/evidence/0.1.1/backup-evidence.tsv`。
- 撤回方案:`release/rollback.txt` (回退 Latest 到 `v0.1.0`,将 `v0.1.1` 转回 draft,永不删已公开 tag 与 release)。

## 7. 公开后更正 (2026-11-25)

- 六目标 `g2/g3` 切片 SHA256 改为从提交内 `release/evidence/0.1.1/fixedpoint-6targets.tsv` 机器提取,消除原手抄偏差。
- `T2` 终态时刻按链产物 `exit.tsv` (`EXIT 1795590087 pid=2994845 rc=0 why=exited`) 对齐为 `2026-11-25 15:01:27 +0800` (`T1 -> T2 = 1492s = 24.9` 分钟;`T2 -> T4 = 6783s = 113.1` 分钟)。
- 更正不触资产、`SOURCES_SID`、候选字节与 strict 计数。
