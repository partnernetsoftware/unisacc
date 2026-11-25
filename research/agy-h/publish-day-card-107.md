# 0.0.107 发布日发布段(M2 到 Y2)时间线与读数卡

窗口: `agy-h` | 依据: `0.0.106` 实测(`INDEX.tsv`, `chain-to-x-ledger.md`) + `0.0.107` 第二轮(`F4=63354b4a2c`, `5d04a80a47`, `M1=516/517 UNKNOWN=1`)

## 1. M2 到 Y2 逐步骤分工、命令、基准耗时与红停处置

| 步骤 | 主做与复核 | 预计分(106实测/107估) | 命令与依据引用页 | 红时第一动作与升级条件 |
|---|---|---|---|---|
| `M2` (`pack`/`warm`/`dry`/`precommit` 预热) | `agy-flow` 主跑; `cdx` 核 `PACK_APE SAME` 与 `expected-unmeasured` `G1`; `agyneon` 备 `--post-chain-merge` 理由 | `106`: `16m46s` (含 `--src-dir` 组装); `107`: 估 `3-5m` (`P1..P6` 已前移; `M1` 已收 `516/517`) | `release-run -- --pack` -> `release-exec --dry` -> `release-exec --warm` (`tools/release-exec.cx:1673-1714`); 核对二/三(`car2-ci-dryrun.md:55-128`) | `PACK_APE` 不符或 `DRY_GATES` 红即停手查 `candidate.tsv`/`expected-unmeasured.tsv`; 不强进 `Step 1`; 同一门红 2 次升级 `cc` |
| `Step 1` (`X` 提交推送 + `核对四`) | `agy-flow` 主跑 `--from-step 1 --to-step 1`; `agyneon` 核三/四 `B-t` 提交与 `REASON`; `agy-h` 记 `X` 时刻 | `106`: `5m37s` (`00:45:18->00:50:55`); `107`: 估 `2-5m` (`T2+45` 与 `T1+120` 均已记 `OVER`) | `DEVSVC_OWNER=cc sh ./unisacc.com tools/release-exec.cx -- --execute ... --from-step 1 --to-step 1 --post-chain-merge "$REASON"`; 远端核对四(`car2-ci-dryrun.md:131-145`) | `X_ON_BRANCH`/`X_ON_MAIN`/`X_CI_TREE_FILES` 任一非 `OK` 禁建草稿; 检查 `git push` 或 `path-b-proof` 缺项并报 `cc` |
| `Step 2` (私有草稿 + `核对五`) | `agy-flow` 起草稿上传 6 资产; `cdx` 核草稿 6 件齐全; `ccogweco` 填草稿说明留 `<CI_RUN_ID>` | `106`: `2m22s` (`00:50:55->00:53:17`); `107`: 估 `2-3m` (含 `15-30s` 下载挡板) | `release-exec --from-step 2 --to-step 2`; `GUARD_107_DRAFT_SLICE_READY=PASS` (`car2-ci-dryrun.md:147-166`) | 若下载 `slice-win-arm64` / `pkg.bin` 报 404, 等 `15-30s` 重试 1 次至 `PASS`; 仍 404 停手查草稿资产, 禁带 404 派 CI |
| `Step 3` (派发 `CI` + 分诊 + 证据落盘) | `agy-flow` 派 CI 并抓日志; `cdx` 值守 6 格 `42/42` 与四类分诊; `agy-h` 记 `CI_RUN_ID` | `106`: 首轮 404 重派后二轮 `15.7m` (`01:14:30->01:30:12`); `107`: 估 `15-17m` | `CANDIDATE_QUALIFY verdict=QUALIFIED pass=42 fail=0 not_run=0 invalid=0` (`car2-ci-dryrun.md:170-192`) | 严禁取消运行中 run 与 `gh run rerun --failed`; 等自然结束后按节 `2` 四分类表判定, 类别 `3`/`4` 立即停手转 `cc` 走 `retry-ci` |
| `Step 4` (`Y1` 冻结证据与决定) | `agy-flow` 填 `<CI_RUN_ID>` 跑 `--from-step 4 --to-step 4`; `cdx` 核 `release/notes` 与 `decision.txt` | `106`: `4m32s` (`01:30:12->01:34:44`, 实提交 `<1m`); `107`: 估 `1-3m` | `exec-4-Y1` + `exec-4-Y1-frozen` (`release/evidence/0.0.106/INDEX.tsv:16-17`; `car2-run-timeline.md:10-12`) | `evidence.tsv` 或 `decision.txt` 校验不符、`notes` 漏写两硬线未满足与 1 项 `expected-unmeasured` 即停手修正后再进 `Y1` |
| `Step 5` (公开) + `Step 6` (读回) | `agy-flow` 执行 `--from-step 5 --to-step 6`; `cdx` 核 `c47-1-readback` 与 `INDEX`; `agy-h` 记公开 `T4` | `106`: 未预热回滚再跑共 `6m` (`01:36:31->01:42:35`), 预热后公开到读回 `14s` (`01:42:35->01:42:49`); `107`: 估 `1-2m` | `READBACK_PREWARM` 门(见节 `3` `RR_TIME` 原行); `post-publish-verify-106.md:128-230`; `tools/release-exec.cx:1456-1514,1673-1714` | `#14 v2` 拦截 `MISS`/`NOT_PREPARED` 禁公开; 若 `Step 6` 失败自动回滚写 `exec-6-rollback` (`Latest->v0.0.106`), 停手报 `cc` 查回读日志后重判 |
| `Step 7` (`Y2` 提交) + `release-post` | `agy-flow` 跑 `Y2` 与 `release-post`; `agy-h` 记 `Y2` 与全局账本收官; 全员核 `0.1.0` 起链线 | `106`: `Y2` `11s` (`01:42:49->01:43:00`), `post-publish` 记 `已公开`; 预计 `107` 合计 `3-6m` | `exec-7-Y2` / `exec-7-Y2-done` / `post-publish` (`post-publish-verify-106.md:228-245`; `release/evidence/0.0.106/INDEX.tsv:26-28`) | `version-seq` 密封或 `old-product` 更新非零、远端读回与 `build/version-seq.tsv` 不一致时立即停手报 `cc` 保留现场, 不重跳步 |

## 2. Step 3 CI 失败四分类速查 (`car2-ci-dryrun.md:172-177`)

1. 类别 `1` (`fetch` 瞬态 404 漏 `slice-win-arm64` / `pkg.bin`, `win/arm64 version FAIL` `invalid=5`): 等 run 自然结束 -> 复跑 `核对五` 至 `PASS` -> 报 `cc` 后跑 1 次 `--from-step 3 --redispatch` (`exec-3b-ci-dispatch-2`); 若再红转 `cc` 查草稿上传。
2. 类别 `2` (`actions/checkout` / `download-artifact` 传输瞬态或 `host hash` 错, 且 `fetch:76` 已核 APE SHA 正确): 等 run 自然结束 -> 核 `核对四`/`五` -> 报 `cc` 批准后 `--redispatch` 1 次; 同错连红 2 次停手升级 `cc`/`fable`。
3. 类别 `3` (`cell-tools` 交叉编译 10 件 / `ffi-lnx` / 格内 `compile FAIL`): 确定性编译或测试树缺陷, 严禁 `--redispatch`, 立即停手保留 `ci-<RUN>.{json,log}` 报 `cc` 裁定 `release-chain.cx -- retry-ci` 修源重链或改测试。
4. 类别 `4` (`version`/`run`/`stream`/`smoke`/`ffi`/`surf-probe` 原生行为失败): 确定性跨平台产品缺陷, 严禁 `--redispatch`, 提取对应格 `evidence-<os>-<arch>` 原行报 `cc` 与 `cdx`, 保持草稿私有并按 `retry-ci` 归档转修复。

## 3. Step 5/6 预热门 (#14 v2) `RR_TIME` 原行判据 (`tools/release-exec.cx:1673-1714`)

1. `warm_probe` 以 `UNISACC_RR_TIME=1` 逐一执行 5 个工具预热探针并扫描 stderr 原始输出行:
   `RR_TIME	lookup_hit=<N>` (当 `<N>` 非 `'0'` 时判 `hit = 1` -> `HIT`; `lookup_hit=0` 或见 `RR_TIME	output	kind=exec-prepare` 时判 `hit = 0` -> `MISS`; 无前述行判 `-1` -> `NOT_PREPARED`)。
2. `warm_probes` 汇总输出 `WARM	TARGET	<tool>	<HIT|MISS|NOT_PREPARED>	<round>`, 仅当全部 `5` 项为 `HIT` (`miss == 0`) 时放行公开与免下载读回路径; 若 `miss > 0` 则执行 `warm_cold` 并复检, 复检仍未全 `HIT` 即阻断 Step 5 公开并升级 `cc` 排查 run-cache 键漂移。
