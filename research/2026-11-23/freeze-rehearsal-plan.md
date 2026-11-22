# v0.0.94 冻结前完整路径演练只读准备表 (S0 -> S8)

状态: 只读准备表(未提交,未执行写操作,未触发 CI)。
依据: `research/2026-11-23/freeze-runbook-draft.md` 第一节、各工具当前源码与 `release/evidence/0.0.93/INDEX.tsv`。
已知在途红灯(不在此处修改): `chain-ready` 项 `1-rev` 当前红(`deferred-gates` 30 行,由 `agy-flow` 处理 C6),全量 `chain-ready` 需待 C6 收口后再判定。

## 一、S0 -> S8 逐步演练准备表

| 编号 | 泳道 | 命令(精确形态) | 预期末行 | 预算 | 依赖工具当前实跑状态与证据 |
|---|---|---|---|---|---|
| S0 | K | `sh ./unisacc.com tools/premerge-union.cx -- --repo . --base main --branch v0.0.94 --shard all` (预演); 正式冻结: `git merge --no-ff v0.0.94` -> 切 `release/candidate.tsv` 为 `0.0.94` 行 -> `devsvc git commit` -> `git push`; 验收 `sh ./unisacc.com tools/chain-ready.cx -- --only 11-version-slot --version 0.0.94` | `PREMERGE_UNION_SUMMARY verdict=PASS ...` / `11-version-slot PASS` | 5 分 (并集 5 片实测 98s 顺序/约 60s 并行, 负载下 load 19-25) | **已实测 PASS**: `v0.0.94` 并集预演(`base=647e166860`, `branch=2735df3efe`, `branch_only=47`, `both=3`, `conflict=0`): `prep` 13.1s、`derive-a` 19.1s、`derive-c` 7.3s、`check-derive` 27.9s(`DERIVE_CHECK 59/0`)、`derive-b` 单独重跑 30.6s(`batch-packages-gen` 29.7s 贴 30s 线,并发下曾 `TIMEOUT 30.1s`) |
| S1 | K | `sh ./unisacc.com tools/chain-ready.cx -- --version 0.0.94` (若单次超 55s 则按 `--only` 分项并行; 演练期暂跳过 C6 在修的 `1-rev`) | `READY yes failed=none` (除 `7-recipe`/`9-ledger` 手工 `UNMEASURED` 外全 `PASS`) | 3 分 (负载下 load 26-29 实测 4 项超 55s, 须拆子步) | 分项摸底实测(跳过 `1-rev`): PASS 8 项(`10/12/12p/13/14/16/17/21`, 合计 1.9s); FAIL 6 项(`11-version-slot` 8.0s、`20-ident-helper` 0.2s 未设 env、`18-parse2-gen` 16.7-24.4s、`2b-ratchets` 22.6s、`6-affected` 30.3s、`2a-derive` 45.3s); **超 55s (`rc=124`) 4 项**: `4-p1`、`5-candidate-chain`、`15-gen-all`、`19-chain-preflight` |
| S2 | K | `sh ./unisacc.com tools/bg-candidate.cx -- start --rev <T> --base ~/.cache/unisacc/chains/bg-b1` -> 监控终态 `sh ./unisacc.com tools/bg-candidate.cx -- status <T>` -> 核验 `sh ./unisacc.com tools/bg-candidate.cx -- promote --check --rev <T>` | `exit.tsv` 含 `rc=0 why=exited`, `bg.log` 含 `SUPERVISE STOP ... rc=0`, APE sha256 == `build.json product_digest` | 30 分 (推断) | 工具在 0.0.93 已实跑通过(`a206d09fa1`, `iterations=8 rc=0`, 证据 `~/.cache/unisacc/chains/bg-b1/a206d09fa1.../exit.tsv`); **0.0.94 整链与真实耗时 `UNMEASURED`** |
| S3 | K | `sh ./unisacc.com tools/prerun-drive.cx -- plan --ids-file <ABS_IDS> --out <ABS_DIR> --times-file <ABS_TIMES> --priority-file <ABS_PRIO>` -> `sh ./unisacc.com tools/prerun-drive.cx -- run --root <ABS_ROOT> --candidate <ABS_APE> --out <ABS_DIR>` | 全部窗口完成, `0 FAIL`, `UNKNOWN` 仅限 `expected-unmeasured.tsv` 已列项 | 30 分 (0.0.93 `wall_ms=1689560` 约 28 分) | 工具在 0.0.93 已实跑通过(156 窗 459 批: 457 PASS / 0 FAIL / 2 UNKNOWN, 证据 `release/evidence/0.0.93/prerun-windows.tsv`); **0.0.94 严格预跑 `UNMEASURED`** (含 `l0-posix2` 现 `REGISTERED_NORUN`) |
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
- **待办(空闲基准)**: 等 `csr` 0.0.95 复核结束且 `uptime` 1 分钟负载 `< 8` 后,重测 4 项超时项(`4-p1`/`5-candidate-chain`/`15-gen-all`/`19-chain-preflight`)与 S1 全量耗时,定真实 S1 预算。

