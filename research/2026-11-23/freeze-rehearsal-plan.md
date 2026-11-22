# v0.0.94 冻结前完整路径演练只读准备表 (S0 -> S8)

状态: 只读准备表(未提交,未执行写操作,未触发 CI)。
依据: `research/2026-11-23/freeze-runbook-draft.md` 第一节、各工具当前源码与 `release/evidence/0.0.93/INDEX.tsv`。
已知在途红灯(不在此处修改): `chain-ready` 项 `1-rev` 当前红(`deferred-gates` 30 行,由 `agy-flow` 处理 C6),全量 `chain-ready` 需待 C6 收口后再判定。

## 一、S0 -> S8 逐步演练准备表

| 编号 | 泳道 | 命令(精确形态) | 预期末行 | 预算 | 依赖工具当前实跑状态与证据 |
|---|---|---|---|---|---|
| S0 | K | `sh ./unisacc.com tools/premerge-union.cx -- --repo . --base main --branch v0.0.94 --shard all` (预演); 正式冻结: `git merge --no-ff v0.0.94` -> 切 `release/candidate.tsv` 为 `0.0.94` 行 -> `devsvc git commit` -> `git push`; 验收 `sh ./unisacc.com tools/chain-ready.cx -- --only 11-version-slot --version 0.0.94` | `PREMERGE_UNION_SUMMARY verdict=PASS ...` / `11-version-slot PASS` | 5 分 (并集 5 片实测 98s 顺序/约 60s 并行, 负载下 load 19-25) | **已实测 PASS**: `v0.0.94` 并集预演(`base=647e166860`, `branch=2735df3efe`, `branch_only=47`, `both=3`, `conflict=0`): `prep` 13.1s、`derive-a` 19.1s、`derive-c` 7.3s、`check-derive` 27.9s(`DERIVE_CHECK 59/0`)、`derive-b` 单独重跑 30.6s(`batch-packages-gen` 29.7s 贴 30s 线,并发下曾 `TIMEOUT 30.1s`) |
| S1 | K | `sh ./unisacc.com tools/chain-ready.cx -- --version 0.0.94 --only <item>` (空闲实测单条全量必超 55s,必须按 `--only` 分批跑) | `READY yes failed=none` (除 `7-recipe`/`9-ledger` 手工 `UNMEASURED` 外全 `PASS`) | 3 分 (空闲 `load<8` 分批实测成立: 串行 ~3.4 分, 2-3 批并行 ~2 分; 单条全量 `rc=124` >55s) | 空闲基准实测(`load 4.77..7.42`): `4-p1` PASS 19.1s、`5-candidate-chain` PASS 53.5s(贴 55s 线)、`15-gen-all` PASS 13.7s、`19-chain-preflight` PASS 30.9s; 全量单命令 `rc=124`(>55s) |
| S2 | K | `sh ./unisacc.com tools/bg-candidate.cx -- start --rev <T> --base ~/.cache/unisacc/chains/bg-b1` -> 监控终态 `sh ./unisacc.com tools/bg-candidate.cx -- status <T>` -> 核验 `sh ./unisacc.com tools/bg-candidate.cx -- promote --check --rev <T>` | `exit.tsv` 含 `rc=0 why=exited`, `bg.log` 含 `SUPERVISE STOP ... rc=0`, APE sha256 == `build.json product_digest` | 30 分 (已证实: 0.0.91..0.0.93 三版实测 27.1-29.3 分) | 三版实测均 PASS: 0.0.93 `1624s(27.1m)`、0.0.92 `1759s(29.3m)`、0.0.91 `1706s(28.4m)`; 占比前三 `g2-*`(20.1%)/`g3-*`(18.4%)/`gen1-*`(17.1%); **0.0.94 整链待跑** |
| S3 | K | `sh ./unisacc.com tools/prerun-drive.cx -- plan --ids-file <ABS_IDS> --times-file <ABS_TIMES> --out <ABS_DIR> --normal-jobs 8 --window-lanes 3` -> `sh ./unisacc.com tools/prerun-drive.cx -- run --root <ABS_ROOT> --candidate <ABS_APE> --out <ABS_DIR> --resume --max-jobs 8 --window-lanes 3` | 全部窗口完成, `0 FAIL`, `UNKNOWN` 仅限 `expected-unmeasured.tsv` 已列项 | **38 分 (推荐 `lanes=3, normal-jobs=8`, `load<8` 静默下预估 36-41 分; 0.0.93 `lanes=4,j=4` 去重实测 49.1 分)** | 0.0.93 实跑已用 `lanes=4,j=4`,按 `PRERUN_LANE_WALL` 去重后真实墙钟 `49.1m`(波次 8.0m + 串行首跑 33.7m + 重试 7.3m); `plan` 干跑 `lanes=3,j=8` 降至 `154 窗/83 组/wall_ms=1475909(24.6m 无争用)` |
| S3b | P | `sh ./unisacc.com tools/ci-early.cx -- --rev <T> --base ~/.cache/unisacc/chains/bg-b1 --out <ABS_OUT>` (与 S3 并行) | 6 行 `CI_EARLY_CELL ... PASS` + `CI_EARLY_SUMMARY verdict=PASS pass_cells=6 fail_cells=0` | 10 分 (0.0.93 实测 7m57s) | 工具在 0.0.93 已实跑 6/6 PASS(`run_id=45020659842`); **0.0.94 osx 四项 `surf-probe` 探针(`2f165f193d`) 样本 `UNMEASURED`** |
| S3c | P | 更新 `release/notes-0.0.94.md`、`release/rollback.txt`、`release/decision.txt` 填入真实冻结身份与 6 资产 sha256; 校验 `sh ./unisacc.com tools/check-release-prep.cx -- --part rollback --ver 0.0.94 --prev 0.0.93 --cand <PREV_CAND_TSV> --allow-pass` (与 S3 并行) | `CHECK_RELEASE_PREP pass=23 fail=0` | 10 分 | 工具已实跑并入库(`a227f6d4f3`, 模板态实跑 `pass=23 fail=0`); **0.0.94 填真实身份后的复核 `UNMEASURED`** |
| S4 | K | `sh ./unisacc.com tools/prerun-drive.cx -- merge --in-dir <ABS_DIR> --out <ABS_DIR> --version 0.0.94` -> `sh ./unisacc.com tools/prerun-drive.cx -- archive --in-dir <ABS_DIR> --repo . --version 0.0.94` | `PRERUN_MERGE ... FAIL=0 ...` + 归档收据落 `release/evidence/0.0.94/` | 3 分 | 工具在 0.0.93 已实跑通过(`release/evidence/0.0.93/INDEX.tsv`); **0.0.94 `UNMEASURED`** |
| S5 | K | `sh ./unisacc.com tools/release-run.cx -- --pack ... --snap <T> --commit <F> --cm-sha ...` (0a) -> `sh ./unisacc.com tools/release-exec.cx -- --warm <PACK_DIR>/candidate.tsv --layout <ABS_LAYOUT> --out <ABS_WARM>` (0b) | `PACK_OK` + `SOURCES_SID VERIFIED` + `PACK_APE SAME` / `WARM READY targets=5` | 5 分 (0.0.93 pack 与 warm 同一分钟内完成) | 工具在 0.0.93 已实跑通过(`release/evidence/0.0.93/pack-run.out`、`warm-57498eeb...tsv`); **0.0.94 `UNMEASURED`** |
| S6 | K | `sh ./unisacc.com tools/release-chain.cx -- run --inputs <ABS_INPUTS> --state <ABS_STATE> --layout <ABS_LAYOUT> --from 1` (覆盖 1 `prep` + 2 `stage-a`, 含 `scrub-home-paths`) | `CHAIN PASS 1 prep` + `CHAIN PASS 2 stage-a` (停在 3 等待操作者提交 X) | 3 分 | `check-release-chain` 64/0、`check-release-prep` 15/0+12/0 已实跑; **0.0.94 `UNMEASURED`** |
| S7 | K | 操作者用 `devsvc git commit` 提交 X(`release/candidate.tsv`、`release/evidence/0.0.94/*`、`release/notes-0.0.94.md`、`release/rollback.txt`、`release/decision.txt`)并 `git push`; 复核 `release-chain run ... --from 3` | `GIT_COMMIT rc=0` + `CHAIN PASS 3 stage-commit` | 3 分 (0.0.93 X 提后补文档用 11 分,本次因前置 8 已备模板预期 <=3 分) | 工具在 0.0.93 已实跑通过(`c1c68fbdc7`, `8b97587679`); **0.0.94 `UNMEASURED`** |
| S8 | K | 在已提交的树上重跑 `sh ./unisacc.com tools/release-chain.cx -- run --inputs <ABS_INPUTS> --state <ABS_STATE> --layout <ABS_LAYOUT> --from 4` (含 4 `stage-dry` + 5 `backup-evidence` 门禁校验) | `CHAIN PASS 4 stage-dry` + `CHAIN PASS 5 backup-evidence` + `CHAIN RESULT READY_TO_EXECUTE version=0.0.94` | 3 分 | `backup-evidence --verify` `~/` 展开已在 `6198e6e2e6` 修复并实跑 0.0.93 `PASS files=7`; **0.0.94 `stage-dry` 全链 `UNMEASURED`** |

## 二、全部 `UNMEASURED` 项汇总

1. **S0 (`v0.0.94` 并集预演)**：`premerge-union --base main --branch v0.0.94 --shard all` 尚未在最新 `main` + `v0.0.94` 上实跑。
2. **S1 (`chain-ready` 全量与耗时)**：除 `11-version-slot`(现 FAIL，S0 切) 与 `21-verseq-gen`(PASS) 外，其余项及全量墙钟耗时均未实测；且 `1-rev` 当前因 `deferred-gates` 30 行红(等 `agy-flow` C6)。
3. **S2 (`0.0.94` 整链耗时与 APE 身份)**：30 分钟为推断值，0.0.94 尚未起链。
4. **S3 (`0.0.94` 严格预跑)**：含 `l0-posix2`（现 `REGISTERED_NORUN`，须在 0.0.94 候选产品重建后首跑验证）。
5. **S3b (`0.0.94` `ci-early` osx 四项探针)**：`2f165f193d` 尚未推远端，osx/win 调用号与信号常量零实机样本。
6. **S3c/S4/S5/S6/S7/S8 (`0.0.94` 实装流水线)**：工具单测与 0.0.93 实跑均绿，但 0.0.94 端到端未跑。

## 三、最便宜的先测顺序(先廉价证伪,后重构造)

按单步耗时与写副作用从低到高排序(前 4 步纯只读、零写 `main`、合计 < 3 分钟，可先打掉大部分确定性红灯)：

1. **[~5 秒, 只读] 模板与回滚对拍(S3c 静态基线)**：
   - `sh ./unisacc.com tools/check-release-prep.cx -- --part rollback --ver 0.0.94 --prev 0.0.93` (已绿 23/0)
   - `sh ./unisacc.com tools/verseq-gen.cx -- --check` (已绿 165/0)
   - `sh ./unisacc.com tools/install-ape.cx -- . --rollback` (已绿 `ROLLBACK_DRYRUN c561e7e0...`，确保 S10 回退槽就绪)
2. **[~20-40 秒, 只读] S0 并集树预合并演练(私有目录,不动 `main`)**：
   - `sh ./unisacc.com tools/premerge-union.cx -- --repo . --base main --branch v0.0.94 --shard all`
   - 若单次超 55s，拆 `--shard prep` -> `--shard derive-a/b/c` -> `--shard check-derive`；同步核对 `scrub-home-paths` 与 `no-home-path`。
3. **[~30-60 秒, 只读, 避开 C6 `1-rev`] S1 `chain-ready` 分项摸底计时**：
   - 按 `--only` 分批并发测 `19-chain-preflight`、`15-gen-all`、`5-candidate-chain`、`14-root-record`、`17-root-public`、`20-ident-helper`、`21-verseq-gen`、`22-premerge-union`，拿到真实单步耗时，证伪或修正 S1 3 分钟预算；待 `agy-flow` 修完 C6 后再补测 `1-rev`。
4. **[~2 分钟, 只读, 私有副本] S5/S6/S8 干跑脚本与参数对拍**：
   - 用 0.0.93 已有 `bg-b1/a206d09fa1` 产物只读试跑 `release-chain plan --inputs ... --state ... --layout ...`，确认无 `REFUSE CHAIN_USAGE` 与路径口径漂移。
5. **[~30 分钟, 重构造] S2 `bg-candidate` 0.0.94 整链**：
   - 仅当前 4 步全绿且 S0 合并就绪后启动，拿到 0.0.94 真实 APE 与 `build.json`，立刻解除 S3/S3b 阻塞。
6. **[~8 分钟 与 ~28 分钟并行] S3b `ci-early` 与 S3 `prerun-drive` 并行**：
   - S2 DONE 后第一时间先起 S3b(8 分钟即可暴露 osx 四项探针真红，比等 S9 省 ~40 分钟)，同时并行起 S3 严格预跑(重点盯 `l0-posix2` 首跑)与 S3c 文档填哈希。
## 四、2026-11-23 负载下 (1)-(4) 纯只读实测记录(未提交)

- **(1) 静态基线三项** (`uptime: load average: 19.57, 16.33, 12.25`, 负载下):
  - `check-release-prep --part rollback --ver 0.0.94 --prev 0.0.93`: `CHECK_RELEASE_PREP pass=23 fail=0` (`20301 ms`)
  - `verseq-gen --check`: `VERSEQ_SUMMARY checks=165 fail=0` (`5384 ms`)
  - `install-ape -- . --rollback`: `INSTALL_APE ROLLBACK_DRYRUN c561e7e0f24d9b82e71b40c86920bc92c12b7035751c8d6e958930bbfe1dd843` (`16217 ms`)
- **(2) `premerge-union` on `v0.0.94`** (`base=647e166860`, `branch=2735df3efe`, `merge_base=b7b3b67760`, `branch_only=47`, `both=3`, `conflict=0`):
  - `prep` (`load 19.48`): `PREMERGE_UNION_SUMMARY verdict=PASS shard=prep conflict=0 gen_fail=0 derive_drift=0 derive_refused=0 check_derive_fail=0 ms=13125` (墙钟 `13148 ms`)
  - `derive-a` (`load 19.44`): `PREMERGE_UNION_SUMMARY verdict=PASS shard=derive-a ... ms=19063` (`ok=11 drift=0`, 墙钟 `19082 ms`)
  - `derive-c` (`load 19.44`): `PREMERGE_UNION_SUMMARY verdict=PASS shard=derive-c ... ms=7178` (`ok=1 drift=0`, 墙钟 `7287 ms`)
  - `check-derive` (`load 19.44`): `PREMERGE_UNION_SUMMARY verdict=PASS shard=check-derive ... ms=27720` (`DERIVE_CHECK ok=59 failed=0`, 墙钟 `27898 ms`)
  - `derive-b`: 4 路并发 (`load 19.44`) `batch-packages-gen TIMEOUT 30114 rc=142` (`verdict=FAIL derive_refused=1 ms=30986`); 单独重跑 (`load 24.61`) `PREMERGE_UNION_SUMMARY verdict=PASS shard=derive-b ... ms=30485` (`batch-packages-gen OK 29737ms`, 墙钟 `30584 ms`, 紧贴 30s 上限)
- **(3) `chain-ready` 分项摸底** (跳过 `1-rev`, `load 26.40..28.65`, 负载下):
  - PASS (8): `10-envhygiene`(303ms)、`12-root-files`(498ms)、`12p-preflight-entry`(152ms)、`13-snap-cache`(214ms)、`14-root-record`(219ms)、`16-disk-free`(123ms)、`17-root-public`(61ms)、`21-verseq-gen`(328ms)
  - FAIL (6): `11-version-slot`(7967ms, S0 切)、`20-ident-helper`(162ms, `UNISACC_IDENT_HELPER unset`)、`18-parse2-gen`(24352ms/16671ms, `READY=no build rc=0 rev=647e166860`)、`2b-ratchets`(22639ms)、`6-affected`(30297ms)、`2a-derive`(45296ms)
  - **超 55s 超时 (`rc=124`, 4 项, 供 `agy-flow` 拆子步)**: `4-p1`(55012ms)、`5-candidate-chain`(55006ms)、`15-gen-all`(55004ms)、`19-chain-preflight`(55005ms)
- **(4) `release-chain plan` 参数口径试跑** (`0.0.93 inputs`, `load 35.21`, 负载下):
  - 缺 `date` 行拒: `release-chain: REFUSE CHAIN_USAGE inputs need version and date` (`rc=2`, 首次冷编 `26157 ms`)
  - 补 `date 2026-11-22` 后过: `CHAIN RESULT PLAN version=0.0.93` (`rc=0`, 热跑 `38 ms`)
- **6 项 FAIL 源码定因**:
  - 确定性 (5): `11-version-slot`(S0 切)、`20-ident-helper`(`chain-ready.cx:1609` env 未设)、`2b-ratchets`(`dup-census FAIL new=9`)、`6-affected`(`precommit-coverage-src`/`src2` FAIL)、`2a-derive`(`table-index DRIFT line=72`)
  - 负载相关 (1): `18-parse2-gen`(`chain-ready.cx:686` `build` 步 16s 上限, `load=26.4` 超时 -> `load=10.3` 复跑 PASS `16217ms`)
- **前置 2 残余(私有并集副本 `v0.0.94` 脱敏与无家目录路径检查)**:
  - `tools/no-home-path.cx -- --check`: `no-home-path: ok: files=133 lines=3501 over=0 (fix: tools/scrub-home-paths.cx --write FILE)` (`rc=0`, `v0.0.94` 0 新增越线)
  - `tools/scrub-home-paths.cx -- --check --tracked-docs`: `SCRUB files=7 lines=120` (`rc=1`, 7 份均为 `build/no-home-path-baseline.tsv` 既有历史基线文件: `build/preflight-rules.tsv`、`tests/check-guard-edit.cx`、`tests/real-inputs.tsv`、`tests/stream-{e1,e2}-domain-matrix.tsv`、`tools/check-preflight.cx`、`tools/libneed-material.cx`, `v0.0.94` 新增/修改文件 0 命中)
- **空闲基准实测(2026-11-23, `load < 8`, 单项串行不抢 CPU)**:
  - `4-p1` (`uptime: load average: 4.77, 7.48, 11.26`): `PASS` `ms=19060` (`2654 files planned by cx0_plan ... 19019`)
  - `5-candidate-chain` (`uptime: load average: 6.40, 7.84, 11.12`): `PASS` `ms=53495` (`rc=0 pass=139 fail=0 ... 52630`, 紧贴 55s 上限)
  - `15-gen-all` (`uptime: load average: 5.61, 7.38, 10.73`): `PASS` `ms=13701` (`GEN_ALL_SUMMARY total=45 pass=45 diff=0 error=0 ... 13666`)
  - `19-chain-preflight` (`uptime: load average: 6.57, 7.48, 10.67`): `PASS` `ms=30853` (`CHAIN_PREFLIGHT rev=HEAD verdict=PASS nodes=8 ms=30442 ... 30816`)
  - S1 全量单命令 (`uptime: load average: 7.42, 7.67, 10.57`): `TIMEOUT rc=124` (`ms=55004`)
  - **S1 真实预算结论**: 单条全量命令即使空闲也必超 55s; 空闲下按 `--only` 分批串行合计 ~204s (~3.4 分), 分 2-3 批并行约 110-120s (~2 分), 3 分钟预算在「`load < 8` + `--only` 分批」下成立, 其中 `5-candidate-chain` 须独占一批(53.5s)。
- **S2 整链与 S3 预跑日志证伪实测(2026-11-23)**:
  - S2 三版真实墙钟(`EXIT - SEGMENT1`): 0.0.93(`a206d09fa1`) `1624s(27.1m)`、`attempt_sum=4297s` (`g2-*` 20.1% / `g3-*` 18.4% / `gen1-*` 17.1% / `jobs-*` 4.0%); 0.0.92(`96d0c6dd2e`) `1759s(29.3m)`; 0.0.91(`7be3cc5d4b`) `1706s(28.4m)` -> **S2 30 分钟预算成立**。
  - S3 0.0.93 `prerun-supervise.log` 全部 `PRERUN_WINDOW` 求和: `windows=187`, `tot_wall_ms=4,040,746 ms(67.3m)`, 其中 `attempt=1` 首跑 `3,602,024 ms(60.0m)`, 重试 24 次 `438,722 ms(7.3m)`, `rc=124` 超时 8 次 `464,407 ms(7.7m)`; 分 phase: `normal` 1492s + `long` 1268s + `heavy` 1042s + `warmup` 131s + `prebuild` 106s -> **S3 原 30 分钟预算与「125 分钟主因 F/F2」推断被证伪**(真实 20:03 -> 21:28 墙钟 85 分, 首跑纯窗口已占 60.0 分)。
  - **T0 到公开总预算重估(`--window-lanes` 源码审计与 `plan` 干跑修正)**:
    - 0.0.93 实跑实际已开 `window-lanes=4, normal-jobs=4`(22 波 `PRERUN_LANES` 共 88 窗), 按 `PRERUN_LANE_WALL` 去重后真实累计墙钟为 **`49.1 分钟`**(`2,946,522 ms` = 22 波 8.0m + 75 串行首跑窗 33.7m + 24 次重试 7.3m); 简单加总 67.3m 含波内并发重复计数。
    - 离线装箱(0.0.93 实测窗, 下界 `max_window_ms=58075`): `lanes=1` 60.0m/67.3m、`lanes=2` 48.6m/55.9m、`lanes=3` 44.4m/51.8m、`lanes=4` 41.7m/49.1m。
    - `prerun-drive plan` 干跑(459 批, `v0.0.93-strict`): `lanes=2,j=4` `groups=118 wall_ms=2127589`; `lanes=3,j=4` `groups=101 wall_ms=1834765`; **`lanes=3,j=8` `windows=154 groups=83 wall_ms=1475909(24.6m 无争用, 实际含开销约 36-41m)`**; `lanes=4,j=8` `groups=74 wall_ms=1330704(22.2m 无争用, 实际约 33-38m)`。
    - 16 核推荐与护栏: 共享波次内均为 `jobs=1`(3-4 核), `normal` 串行独占运行(`normal-jobs=8` 占 8/16 核), 两者不叠加; 推荐 `load < 8` 静默下用 **`--window-lanes 3 --normal-jobs 8`**(兼顾提速与防边缘 58s 窗 `rc=124`), S0..S10 总预算 **111 分钟**(T0..S9 公开 **103 分钟 <= 120 分钟红线**)。
- **`--normal-jobs 8` 小规模试点实测(`PARTIAL`, 2026-11-23, 定稿依据)**:
  - 样本: 0.0.93 最慢 20 个 `normal` 窗共 136 批(基线 `sum_wall_ms=765545`, `max_win_ms=63216`, `rc124=1`), 重排为 13 个 `normal` `jobs=8` 窗(`W0018..W0030`) + 17 `long` + 1 `warmup`。
  - 已测最重 7/13 窗(`W0018..W0024`, 58 批, `load 7.70..9.49` + `omp-go --jobs 2`): `sum_wall_ms=161421(2.69m)`, `sum_batch_ms=823661`(5.10x 并发加速), `max_win_ms=48671`(<58s 留 9.3s 余量), **`rc=124` = 0** (7/7 全 `rc=0`)。
  - 第 5 段前 `uptime load average: 18.86 > 10` 按护栏停手; 剩余 `W0025..W0030` 6 窗断点保存在 `~/.cache/unisacc/prerun-pilot-j8`(仅在 `load < 8` 空窗可选补跑,不阻塞冻结)。

