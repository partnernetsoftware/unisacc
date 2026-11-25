# unisacc 0.0.107 全程时间线与证据入口

## (1) 候选与关键提交身份

| 项目 | 值 | 证据入口 |
|---|---|---|
| 版本与标签 | `0.0.107` / `v0.0.107` | `release/candidate.tsv` |
| 起链封口提交 (`F4`) | `63354b4a2cb707ecc923a89f0df6030149fa697f` | `release/evidence/0.0.107/fixedpoint-6targets.tsv` |
| 源树身份 (`SOURCES_SID`) | `37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238` | `release/evidence/0.0.107/pack-run.out` |
| 候选 `unisacc.com` (`APE2`) | `b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63` (`8991230` B) | `release/evidence/0.0.107/ape-receipt.txt` |
| `pkg.bin` (`p3_sha`) | `4d13b970d75732d61126e67f1f4574a70f48eb9675b95900453dfd370d3b4f1b` | `release/candidate.tsv` |
| `slice-win-arm64` | `ba133d3ce42d603438b12fdfd8f0b3544cc7199f3214c5c2d101ce202da114b5` | `release/candidate.tsv` |
| `build.json` / `unisacc.com.build.json` | `d66f4ac3811acecd532917162be0fe9f9c8b08f6ce1c84290166d955d92461fc` | `release/candidate.tsv` |
| `SHA256SUMS` | `643a77e285820d541d7aaa886ff657462067fce51b5281664116233dad193f33` | `release/candidate.tsv` |
| 链后路径 B / B-t 区间 | `63354b4a2c..2d387e524d` (4 笔提交, 5 文件, 零 SID 触碰) | `research/cdx/path-b-proof-107.md` |
| Step 1 `X` 提交 | `8a0b2939730222c2c2581399bf962c59201e0be9` | `release/evidence/0.0.107/INDEX.tsv` (`exec-1-X-commit`) |
| Step 3 CI 运行号 | `45512387135` (`verdict=QUALIFIED fail=0 invalid=0`) | `release/evidence/0.0.107/ci-45512387135.json`, `ci-45512387135.log` |
| Step 4 `Y1` 提交 | `72187d1e489008617d80ef8e74ebf1e12180d647` | `release/evidence/0.0.107/INDEX.tsv` (`exec-4-Y1`) |
| Step 7 `Y2` 提交 | `6f55d3c1b583332322078c072b8efe3c6c273937` | `release/evidence/0.0.107/INDEX.tsv` (`exec-7-Y2-done`) |
| `release-post` 收尾提交 | `68b15c4028132de75e3431a6c424f3b2550d0c39`, `521797c3a882855dca29c44e6b6e8bd7cae61a5b` | `build/version-seq.tsv`, `tools/old-product-assets.tsv`, `build/freeze.tsv` |

## (2) 全程时间线 (`2026-11-25 +0800`)

| 阶段 | 时刻 / 区间 | 耗时 | 终态与证据原行 |
|---|---|---|---|
| `T1` Round 2 起链 (`F4=63354b4a2c`) | `05:08:30 -> 05:34:55` | `26.4m` (`1585s`) | `s1=239s s2=32s s3=104s s4=538s s5=457s s6=215s`, `slices=same count=6 objects=same count=102 kernels=same count=2 product=same` |
| `T_prerun2` 241 窗严格预跑 | `05:39:02 -> 06:59:53` | `80.8m` | 前 22 优先窗与 68 个 `netregen-*` 重窗及 `W0123 cx-booter-ape` 全过; `W0213` 补登记后 `PRERUN_RUN status=DONE ran=28 skipped=213` |
| 链后单跑与 `W9994` 分片补跑 | `07:03:00 -> 07:35:30` | `32.5m` | `W0000`、`W9916`、`W9917c`、`W9931`、`W9993b`、`W9994` 全 `PASS`; `strict.tsv` 汇总 `batches=519 PASS=518 FAIL=0 UNKNOWN=1 INVALID=0` |
| `M2` (`--pack` / `--warm` / `--dry`) | `07:31:00 -> 07:36:30` | `5.5m` | `PACK_APE SAME`, `RELEASE_RUN RESULT PACK_OK`, `WARM RESULT READY targets=5`, `--dry` Steps 1-3 全 `PASS` |
| Step 1 `X-commit` | `07:39:14` | `2302ms` | `exec-1-X-commit 8a0b2939730222c2c2581399bf962c59201e0be9` (`GATE 1 X_PUSHED PASS`) |
| Step 2 `draft` + Check 5 | `07:43:44` / `07:44:28` | `9578ms` | `exec-2-draft-readback PASS draft-readback.txt`, Check 5 `GUARD_107_DRAFT_SLICE_READY=PASS` |
| Step 3 `ci-dispatch` -> `ci` | `07:43:53 -> 07:52:48` | `543801ms` (`9.06m`) | `exec-3-ci PASS 45512387135 six targets success, CANDIDATE_QUALIFY no fail/invalid` (`verdict=QUALIFIED fail=0 invalid=0`) |
| 草稿公开说明定稿更新 | `07:54:40` | `<10s` | `gh release edit v0.0.107 --notes-file` (`notes-0.0.107-v2.md sha256=d30a2fa172a0f7ce03af9a85bedf910175244eb62b49f2ad3690b2eecfb2c7d9`) |
| Step 4 `Y1` | `07:56:07` | `13337ms` | `exec-4-Y1 72187d1e489008617d80ef8e74ebf1e12180d647`, `exec-4-Y1-frozen 9d4db05f...:4a6551d2...` |
| Step 5 `publish` (`T4`) | `07:57:29` | `5548ms` | `GATE 5 READBACK_PREWARM PASS smoke passed; chain-step and release-ape hit`, `exec-5-publish v0.0.107 isDraft=false, Latest` |
| Step 6 `readback` | `07:57:44` | `14664ms` | `WARM 6 READY`, 7 相位全部 `attempt=1 rc=0`, `exec-6-public-readback PASS public-readback.txt` |
| Step 7 `Y2` | `07:57:53` | `20192ms` | `exec-7-Y2-done 6f55d3c1b583332322078c072b8efe3c6c273937`, `RELEASE_EXEC RESULT EXECUTED steps=7 deferred=0` |
| `release-post` + `R33` | `08:14:05 -> 08:14:10` | `5s` | `RELEASE_POST status=DONE`, `OLD_PRODUCT PREPARED version=0.0.107`, 提交 `68b15c4028` 与 `521797c3a8` |

## (3) 四笔链后路径 B / B-t 提交 (`63354b4a2c..2d387e524d`)

四笔提交共改 5 个测试/工具/基线文件 (`tools/check-ratchets.cx`、`tests/daily-compile-smoke.tsv`、`build/retcheck-baseline.tsv`、`tools/release-exec.cx`、`tools/check-release-exec.cx`), 零产品输入改动 (`SOURCES_SID = 37ff46cd...` 恒等不变, 证明见 `research/cdx/path-b-proof-107.md`):

- (1) `309e2e1ab8`: `tools/check-ratchets.cx` 19 门测试夹具同步, `W9916 ratchets-tool-full` 同候选重跑 `PASS 0 (42680ms, assertions=4)`。
- (2) `a63bc7bece`: `tests/daily-compile-smoke.tsv` 补 `tests/check-precommit-list-cap.cx` 登记行, `W0213 check-daily-compile-smoke` 同候选重跑 `PASS 0 (5073ms, assertions=8)`。
- (3) `5d04a80a47`: `build/retcheck-baseline.tsv` 补两行测试夹具基线, `W9931 retcheck` 同候选重跑 `PASS 0 (3862ms, assertions=15)`。
- (4) `2d387e524d`: `tools/release-exec.cx` 与 `tools/check-release-exec.cx` 实现 `G1 v2` `window-timeout` 原因分类, 强制校核 `strict.tsv` 中 `RAW` 行真实超时证据 (`transport-failed-raw-neg1`、`elapsed_ms >= 53000`、`assertions=UNMEASURED`) 与说明文件存在性。

## (4) `G1 v2 window-timeout` 与 `precommit-list-cap` 证据

- `precommit-list-cap` (`headers=tree`, 4 次串行真实 `precommit.cx` 子进程调用) 在同候选 `APE2` 与同布局下连跑四次: `W9915` (`55557ms`)、`W9915b` (`55530ms, load1=1.82`)、`W9915c` (`55486ms, load1=1.93`)、`W9915d` (`55492ms, load1=2.21`), 均触 `../../tests/run-batch.cx:264` 的 53 秒子进程硬限 (`UNKNOWN 0`, `transport-failed-raw-neg1`, `assertions=UNMEASURED`)。
- 按 `G1 v2` 登记 `release/evidence/0.0.107/expected-unmeasured.tsv` (`precommit-list-cap<TAB>window-timeout<TAB>list-cap-margin.md`) 与 `release/evidence/0.0.107/list-cap-margin.md` (同载于 `INDEX.tsv` `precommit-list-cap-window-timeout WAIVED`), 并在公开说明披露三批贴限批次 (`precommit-list-cap 55.5s`、`slow-split 51.1s`、`premerge-selfcheck-main/rd 50.3s/49.9s`), 排入 `0.1.1` 拆批。

## (5) `W9994` (`premerge-selfcheck` 两分片) 补跑

- `tests/split-parents.tsv` 中 `premerge-selfcheck -> premerge-selfcheck-main, premerge-selfcheck-rd` 原不在 `ids.txt` (517 批) 内, 触发 `--dry` 的 `GATE 1 STRICT_SPLIT_INCOMPLETE FAIL premerge-selfcheck`。
- 经编排者放行, 在同候选 `APE2`、同 `frozen-root` (`2d387e524d`) 单跑 `W9994` (`release/evidence/0.0.107/strict-W9994-premerge-selfcheck-shards.tsv`): `premerge-selfcheck-main PASS 0 (50251ms, assertions=46)`、`premerge-selfcheck-rd PASS 0 (49898ms, assertions=25)`。
- 经 `prerun-drive.cx -- merge` 合入 `release/evidence/0.0.107/strict.tsv`, 终态 `SUMMARY batches=519 PASS=518 FAIL=0 UNKNOWN=1 INVALID=0` (`GATE 1 STRICT PASS rows=529 expected_fail=0 superseded=6`)。

## (6) `release-post` 行位互换处置与 `R33`

- `tools/release-chain.cx:1025-1035` (`verify_version_seq_readback`) 自顶向下取表内最后一个 `已公开` 行作为 `last_public`; 因 `build/version-seq.tsv` 中 `0.0.106` 原位于槽位 47 (第 43 行)、`0.0.107` 原位于槽位 45 (第 41 行), 首次运行 Step 2 报 `REFUSE VERSION_SEQ_MARK readback-verify-failed version=0.0.107 last_public=0.0.106`。
- 按编排者裁示互换 `0.0.106` 与 `0.0.107` 所处行位并互换槽位号 (`0.0.106` 取槽位 45、`0.0.107` 取槽位 47, 保持槽位号严格递增与 `已公开` 标记), 实测 `VERSEQ_SUMMARY checks=719 fail=0`、`CHECK_VERSEQ_GEN checks=36 failed=0`、`DERIVE verseq-gen OK 30`。
- 因 `tools/precommit.cx:664-669` 禁止同一提交内同时修改 `build/freeze.tsv` 与受冻结路径 `release/`, 分两笔净提交完成收尾 (均 `precommit blocked=0`):
  - (1) `68b15c4028132de75e3431a6c424f3b2550d0c39`: `build/freeze.tsv` (`0.0.107-FREEZE` 注释留痕) + `tools/old-product-assets.tsv` (登记 `0.0.107 8991230 b773c7e2...`)
  - (2) `521797c3a882855dca29c44e6b6e8bd7cae61a5b`: `build/version-seq.tsv` + `release/evidence/0.0.107/INDEX.tsv` (`post-publish PASS`)
- `release-post.cx --execute` 全 7 步一次通过:
  - `POST_STEP 1 index-seal SKIP already-DONE`
  - `POST_STEP 2 mark-version-seq OK mark-version-seq-commit-no-push`
  - `POST_STEP 3 backup-evidence OK BACKUP_VERIFY-PASS`
  - `POST_STEP 4 metrics OK metrics-written`
  - `POST_STEP 5 root-upgrade SKIP no-root-upgrade-flag`
  - `POST_STEP 6 worktree-cleanup-print OK printed-cleanup-cmds`
  - `POST_STEP 7 rearrange-patch-print OK printed-rearrange-cmds`
  - `RELEASE_POST status=DONE`
- `R33` 正式入口 `tools/old-product.cx` 实跑通过: `OLD_PRODUCT PREPARED version=0.0.107 sha256=b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63 bytes=8991230 source=download` (缓存目录 `0700`)。

## (7) 条件 4 / 5 / 6 核定

- **条件 4 (严格证据与靶标批次)**: **满足** — `519` 批 `PASS=518 FAIL=0 UNKNOWN=1 INVALID=0`; 前端 B1 三项靶标批次 (`c99-array-param-typedef`、`c99-global-redecl-conflict`、`c99-tentative-array-incomplete`) 与 `F1` (`fwd-cap` 28 断言、`fwdstub-replay` 25 断言、`cxmod-tri-exports`、`module-units-forward`、`fixedpoint-evidence`、`cx-booter-ape`) 同候选全绿; 唯一 `UNKNOWN` (`precommit-list-cap`) 经 `G1 v2` `window-timeout` 绑定 `RAW` 实测证据入账。
- **条件 5 (跨平台 CI 与公开回读)**: **满足** — Step 3 CI (`run_id=45512387135`) 6 目标 42 格 `QUALIFIED` (`fail=0 invalid=0`, `543801ms`); Step 5 预热门 `#14 v2` 双命中 (`GATE 5 READBACK_PREWARM PASS`); Step 6 公开回读全部 7 相位 `attempt=1 rc=0` (`14664ms`)。
- **条件 6 (起链至公开 120 分钟目标)**: **未满足 (如实记未满足)** — `T1 (05:08:30)` 至 `T4 (07:57:29)` 合计 `168.98` 分钟 (超 120 分钟上限; 其中整链 `26.4m`、预跑 `80.8m`、链后 4 笔 B/B-t 修复与分片补跑及 `G1 v2` 门升级 `57.3m`、`X` 至公开 `18.2m`), `R6` 留待 `0.1.0` 封板车 (`T3_R6`) 验证。
