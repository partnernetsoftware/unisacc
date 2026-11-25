# 第一车 0.0.106 链止(21:58:56)到 X(00:50:55)取证流水账与 0.0.107 并行压缩方案

- 数据来源:`release/evidence/0.0.106/INDEX.tsv`、`release/evidence/0.0.106/prerun-supervise.log`、`release/evidence/0.0.106/pack-run.out`、`release/evidence/0.0.106/path-b-proof.md`、`release/evidence/0.0.106/post-chain-merge-provenance.txt`、`release/evidence/0.0.106/fixedpoint-6targets.tsv`、`release/evidence/0.0.106/ape-receipt.txt`、`release/evidence/0.0.106/ratchets.txt`、`~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/done.W*` 与 `git log`(2026-11-24 21:58:56 至 2026-11-25 00:50:55 +0800)。
- 总体耗时概览:`0.0.106` 从链止(`2026-11-24 21:58:56`)到 Step 1 `X` 提交落盘(`2026-11-25 00:50:55`,提交 `07d767d1eb`)共耗时 **`171 分 59 秒`(`2 小时 51 分 59 秒`)**:
  - `(1)` 238 窗预跑有效执行墙钟:**`~78 分钟`**;
  - `(2)` 预跑途中 4 次停机(`W0080` 超时停 + `W0087`/`W0191`/`W0206` 三次 `PRERUN_PRODUCT_RED_STOP`)定因、落 5 笔 `R39/R40 B-t` 修复提交与人工续跑等待:**`~51 分钟`**;
  - `(3)` 预跑结束后(`00:08:15 -> 00:50:55`,共 **`42 分 40 秒`**)串行做 `R40 B-t` 20 批复跑 + `W0150` 三连跑(`19m00s`)、`strict` 折叠(`1m16s`)、`fixedpoint`/`ape-receipt`/`ratchets`/`path-b-proof` 证据拼装 + `--pack` + `release-exec --dry`(`16m45s`)、以及正式 Step 1 `X` 提交推送(`5m37s`)。

---

## (1) 0.0.106 从链止(21:58:56)到 X(00:50:55)逐动作时序流水账

| 序号 | 时段 (+0800) | 墙钟耗时 | 阶段 / 动作 | 执行命令原行(或日志原行) | 结果与损耗定性 |
|---|---|---|---|---|---|
| 1 | `21:58:56` | `0s` (起点) | 整链完成 (`CHAIN_STOP`) | `sh ./unisacc.com tools/bg-candidate.cx -- status 2e8a19a342251910d53010f51002b14a4ca7f916 --base ~/.cache/unisacc/chains/bg-b1` | `CHAIN fixedpoint objects=same count=102 / kernels=same count=2 / slices=same count=6 / product=same`;产出 `unisacc-next.com`(`5f83ac93d2...`, `8932113B`)。 |
| 2 | `21:58:56 -> 22:05:40` | `6m44s` | 链止核收与预跑 `plan` 生成 | `sh ./unisacc.com tools/prerun-supervise.cx -- plan . --candidate "$CHAIN/ape-final/unisacc-next.com" --out ~/.cache/unisacc/prerun/v0.0.106-2e8a19a342 --jobs 8 --segment-batches 10` | `22:05:40` 写出 `ids.txt`(`1996` 批)与 `prerun-windows.tsv`(`W0000..W0237` 共 238 窗)。**损耗 1**:链止到 `plan` 之间有约 `6 分钟` 人工确认空窗。 |
| 3 | `22:05:40 -> 22:15:11` | `9m31s` | 导出 `frozen-root` 与准备 `opt-frame4-material` | `sh ./unisacc.com tools/prerun-supervise.cx -- run . --candidate "$CHAIN/ape-final/unisacc-next.com" --out ~/.cache/unisacc/prerun/v0.0.106-2e8a19a342 --jobs 8` | `PRERUN_PREPARE_MATERIAL material=opt-frame4-material rc=0 wall_ms=43904`(`22:15:11` 完成)。**损耗 2**:`plan` 后到正式起 `run` 间有约 `8 分钟` 启动间隔。 |
| 4 | `22:15:11 -> 22:27:12` | `12m01s` | 预跑 `W0000`(warmup)至 `W0015`(含 2 个合法 `DEFERRED`) | `prerun-supervise.cx -- run` 自动推进 `W0000..W0015` | `W0000` attempt 1 冷超 `58035ms rc=124`,attempt 2 `36139ms rc=0`(`layout=ready kit=ready`);`W0001..W0013` 全绿;`W0014`(`cx-booter-ape`)与 `W0015`(`tools-default-regression`)按 `rc=3` 记 `PRERUN_KEEP_GOING`。 |
| 5 | `22:27:12 -> 22:51:47` | `24m35s` | 预跑 `W0016 -> W0079`(`heavy` 双车道 + `W0028` `prebuild-self`) | `prerun-supervise.cx -- run` 双车道推进 `W0016..W0079` | `W0016/17/18/29/55` 首次 attempt 触 `58s rc=124`,attempt 2 均热态通过(`31-56s`);`W0028` `prebuild-self` `43980ms rc=0`。 |
| 6 | `22:51:47 -> 22:59:38` | `7m51s` | **停机 1**:`W0080`(`check-split-parents`)连续两次 `58s rc=124` 触发 `PRERUN_STOP` 及续跑 | `sh ./unisacc.com tools/prerun-supervise.cx -- run . --candidate "$APE" --out "$PRERUN_OUT" --jobs 8 --keep-going product-red` | `W0080` attempt 1 (`58039ms`) + attempt 2 (`58042ms`) 超时停机;续跑后 `W0080` `wall_ms=57020 sum_batch_ms=54947 rc=0` 通过(`22:59:38`)。 |
| 7 | `22:59:38 -> 23:08:11` | `8m33s` | 预跑 `W0081 -> W0087` 并在 `W0087` 触发 **停机 2(`PRERUN_PRODUCT_RED_STOP`)** | `prerun-supervise.cx` 跑至 `W0087`(`check-gen-all-drift`) | `W0081..W0086` 全绿;`W0087` attempt 1 `58044ms rc=124`,attempt 2 `22689ms` 报 `check-gen-all-drift product-qualification FAIL rc=1` 停机(`23:08:11`)。 |
| 8 | `23:08:11 -> 23:20:04` | `11m53s` | **红窗修复 1(`W0087`)**:定因、`G50` 路径 B 复核与提交 `c698ef0091`,同时预跑从 `W0088` 续跑 | `DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- git commit -m "v0.0.106: R39 修复 W0087 check-gen-all-drift 最小仓缺 tests/ 触发 affected-canary (tools-only)" --whole tools/check-gen-all-check.cx --push` | 修复 `cgac_prep_minirepo` 清理 `tools/affected.cx`(`23:20:04` 落 `c698ef0091`);预跑在后台继续推进 `W0088..W0147`。 |
| 9 | `23:10:00 -> 23:42:09` | `32m09s` (与上步重叠 `10m`) | 预跑 `W0088 -> W0191`:途经 `W0148`、`W0150`、`W0189` 三个非阻断红窗,在 `W0191` 触发 **停机 3(`PRERUN_PRODUCT_RED_STOP`)** | `prerun-supervise.cx` 双车道推进 `W0088..W0191` | - `23:30:13` `W0148`(`check-ln-noderef` `tool-selfcheck FAIL rc=1`, `3763ms`, `KEEP_GOING`);<br>- `23:31:27` `W0150`(`release-exec-c` `tool-selfcheck FAIL rc=1`, `22216ms`, `KEEP_GOING`,长 `TMPDIR` 溢 200B 缓冲);<br>- `23:41:53` `W0189`(`devsvc-git-c-operations` `devsvc-selfcheck FAIL rc=1`, `6530ms`, `KEEP_GOING`,树内 `0.0.106-FREEZE` `23:00` 过期);<br>- `23:42:09` `W0191`(`fwdstub-replay` `product-qualification FAIL rc=2`, `2609ms`) -> **`PRERUN_PRODUCT_RED_STOP`**(直接执行候选 APE 撞 `ENOEXEC`)。 |
| 10 | `23:42:09 -> 23:53:50` | `11m41s` | **红窗修复 2/3/4(`W0148`/`W0191`/`W0189`)** 三笔提交 + 预跑推进 `W0192 -> W0206` 触发 **停机 4(`PRERUN_PRODUCT_RED_STOP`)** | - `23:43:05` 提交 `d12d2087bd`(`tools/check-chain-ready-freeze.cx`、`tools/check-prepush.cx` `ln -sT`);<br>- `23:51:56` 提交 `a46b98d713`(`tests/check-fwdstub-replay.cx` 经 `sh` 启动 APE,`G51`);<br>- `23:52:31` 提交 `879e446640`(`build/freeze.tsv` deadline 续期至 `1795546800=03:00`) | 预跑续跑 `W0192..W0206`,在 `23:53:50` 跑 `W0206`(`env-semantics`, `wall_ms=27986+15098`)报 `count=125/4 FAIL rc=1` 触发第 3 次 `PRERUN_PRODUCT_RED_STOP`。 |
| 11 | `23:53:50 -> 00:08:15` | `14m25s` | **红窗修复 5(`W0206`)** 提交 `8ca2f5e4b8` + 预跑收尾 `W0207 -> W0237` | - `00:04:37` 提交 `8ca2f5e4b8`(`tests/env-semantics.tsv` 登记 4 个 `fwdreplay` 变量,`G52`);<br>- `prerun-supervise.cx` 跑完 `W0207..W0237`(31 个 8 并发 `normal` 窗,`window_wall_ms=719019`) | `00:08:15.80` 写出 `strict-W0237.tsv` 与 `PRERUN_RUN status=DONE`。至此 238 窗全部跑完。 |
| 12 | `00:08:15 -> 00:27:16` | `19m01s` | **链后路径 B(`R40 B-t`)20 批同候选复跑 + `W0150` 短 `TMPDIR` 三连跑 + 静态取证(`fixedpoint` / `ape-receipt` / `ratchets` / `path-b-proof`)** | - `git -C "$W" archive 8ca2f5e4b8 \| tar -xf - -C ~/.cache/unisacc/prerun/v0.0.106-r40-rerun/frozen-root`<br>- `"$CHAIN/build/identity-helper" "$FROZEN_ROOT"` (`0.324s`, 核 `SOURCES_SID=c6eeca9f10...` 不变)<br>- `TMPDIR=/tmp sh ./unisacc.com tools/dvqualify.cx -- . --candidate "$APE" --ids ... --out strict-W9997-r40-bt1.tsv` (`10/10 PASS`, `00:16:57`)<br>- `strict-W9998a-r40-bt2.tsv` (`9/10 PASS`) + `strict-W9998b/c` (`devsvc-git-c-operations` 单批两次 PASS: `16748ms`, `6518ms`)<br>- `TMPDIR=/tmp sh ./unisacc.com tests/run-batch.cx -- ... release-exec-c` 连跑 3 次 (`strict-W9999a/b/c`: `23457ms`, `23205ms`, `23202ms`, `3/3 PASS`)<br>- `sh ./unisacc.com tools/ratchets.cx -- ... > release/evidence/0.0.106/ratchets.txt` (`26705ms`)<br>- 生成 `fixedpoint-6targets.tsv`、`ape-receipt.txt`、`post-chain-merge-provenance.txt`、`path-b-proof.md`(`00:27:00`) | 20 批 `R40 B-t` + 3 次 `W0150` 全部 `PASS`,6 个唯一失败批(7 条原始 FAIL 行)准备以 `superseded=7` 折叠。**损耗 3**:`fixedpoint-6targets.tsv`、`ape-receipt.txt`、`ratchets.txt` 与 `--src-dir` 原本在 `21:59` 就具备全部输入,却等到 `00:20+` 才串行做。 |
| 13 | `00:27:16 -> 00:28:32` | `1m16s` | `strict` 合并折叠(`prerun-supervise.cx -- merge`) | `sh ./unisacc.com tools/prerun-supervise.cx -- merge . --in ~/.cache/unisacc/prerun/v0.0.106-merge-in --out ~/.cache/unisacc/prerun/v0.0.106-merge-out` | `00:28:32.94` 产出 `strict-final.tsv` -> `release/evidence/0.0.106/strict.tsv`(`1989 PASS`, `2 DEFERRED`, `7 SUPERSEDED`, `0 FAIL`)。 |
| 14 | `00:28:32 -> 00:45:18` | `16m46s` | 组装 `--src-dir`、`release-run.cx -- --pack` 打包回填、`release-exec --dry` 预演与 `exec-1-post-chain-merge` 签发 | - `sh ./unisacc.com tools/release-run.cx -- --pack release/candidate.tsv --chain-dir "$CHAIN" --src-dir ~/.unisacc-release-evidence-backups/0.0.106/src-dir --strict release/evidence/0.0.106/strict.tsv --out ~/.cache/unisacc/release-v0106-state/pack`<br>- `sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv --layout ~/.cache/unisacc/release-v0106-state/pack/layout --strict release/evidence/0.0.106/strict.tsv --out /tmp/r106-dry --dry --post-chain-merge "$REASON"` | `--pack` 本身执行仅 `~15 秒`(`PACK_APE SAME sha256=5f83ac93d2...`, 回填 `release/candidate.tsv` 七哈希);`00:45:18` `INDEX.tsv` 写入 `exec-1-post-chain-merge WAIVED` 并通过 `DRY_GATES_OK`。**损耗 4**:组装 `--src-dir`、对齐 `--post-chain-merge` 理由文本与多窗口对账耗去约 `15 分钟`。 |
| 15 | `00:45:18 -> 00:50:55` | `5m37s` | 正式 Step 1 提交并推送 `X` (`07d767d1eb`) | `DEVSVC_OWNER=cc sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv --layout ~/.cache/unisacc/release-v0106-state/pack/layout --strict release/evidence/0.0.106/strict.tsv --out /tmp/r106-exec --from-step 1 --to-step 3 --post-chain-merge "$REASON"` | `00:50:40` 提交 `X = 07d767d1ebf83592f6538719cecf27963aac09b7`(`v0.0.106 X: candidate table`),`00:50:55` 写入 `exec-1-X-commit`,随即在 `00:53:17` 完成草稿回读并在 `00:53:27` 派发 CI。 |

---

## (2) 可压缩的串行等待分析:哪些动作只依赖链产物(`$CHAIN` + `F=REV`),可在预跑期间并行预做?

把上述 15 个动作按**真实数据依赖**拆解,可以发现除 `prerun-supervise.cx -- merge` 与最终带 `--strict` 的 `release-run.cx -- --pack` 必须等 238 窗结束外,**其余所有取证、核验、源目录组装、CI 预演与文档草稿均只依赖 `$CHAIN` 与 `F=REV`,对 `strict.tsv` 零依赖**:

| 动作项 | 在 `0.0.106` 的实际执行位置(串行阻塞时长) | 真实输入依赖 | 在 `0.0.107` 可前移到的并行时段 | 在预跑结束后的关键路径上省下时间 |
|---|---|---|---|---|
| **A. `prerun-supervise.cx -- plan` 与启动 `run`** | 链止后人工等待 `6m44s` 才 `plan`,又隔 `8m` 才起 `run`(共耗时 `15m`) | 仅依赖 `$CHAIN/ape-final/unisacc-next.com` 与 `$W`(`F=REV`) | **链止后 `60 秒` 内自动衔接**(提前写好 `CHAIN_STOP` 接力脚本,核 `fixedpoint product=same` 后立即 `plan` + `run`) | 省去 **`~13 分钟`** 链止空窗 |
| **B. `fixedpoint-6targets.tsv` 与 `ape-receipt.txt` 生成** | 预跑结束后 `00:25 -> 00:27` 才生成(占后置关键路径 `~2m`) | 仅依赖 `$CHAIN/fixedpoint/g{2,3}/*`、`$CHAIN/compiler.p4.pkg`、`$CHAIN/cache-manager/*`、`$CHAIN/ape-final/*` 与 `chain-run-*.stdout` | **预跑启动后第 1 分钟(`T_chain_stop + 2m`)并行生成**(纯文件 `sha256sum` 与日志提取,耗时 `< 5 秒`,不抢编译 CPU) | 省去 **`~2 分钟`** 后置关键路径 |
| **C. `--src-dir` 打包源目录组装(`p3.pkg`、六目标 `g2` 切片、`cache-manager-{x64,arm64}`、`sources.tsv`、`commit.txt`)** | 预跑合并后 `00:29 -> 00:35` 才手工拷入 `~/.unisacc-release-evidence-backups/0.0.106/src-dir` | 仅依赖 `$CHAIN` 与 `F=REV` 的 `SOURCES_SID` | **预跑启动后第 2 分钟(`T_chain_stop + 3m`)一次拷齐并校验 SHA256**(纯文件复制,`< 3 秒`) | 省去 **`~5 分钟`** 打包前准备 |
| **D. 2 个合法 `DEFERRED` 证据文件(`cx-booter-ape-deferred.tsv`、`tools-default-regression-deferred.tsv`)与 `expected-fail.tsv` / `expected-unmeasured.tsv`** | 预跑结束后才整理写入 `release/evidence/0.0.106/` | `W0014` 与 `W0015` 在预跑启动后 **`~12 分钟`** 即完成 | **`W0015` 完成后(`T_chain_stop + 15m`)立即提取落盘** | 省去 **`~2 分钟`** 合并后整理 |
| **E. `ratchets.txt`(`tools/ratchets.cx`)** | 预跑结束后 `00:25` 才跑(`26.7 秒`) | 仅依赖 `F=REV` 的 `frozen-root` 静态源码树 | **预跑进入尾段或单车道间隙并行跑一次(`26.7 秒`)**,直接落入 `release/evidence/0.0.107/ratchets.txt` | 省去 **`~30 秒`** 后置关键路径 |
| **F. CI 起跑前“核对三”(候选 APE 本机交叉编译 10 个 CI 二进制 + `ci-candidate-check`)** | 之前规划在 `--pack` 后、Step 1 `X` 前串行跑(`~45-88 秒`) | 仅依赖候选 APE(`$CHAIN/ape-final/unisacc-next.com`,与 `--pack` 后的 `layout/unisacc.com` 字节 100% 相同 `PACK_APE SAME`)与 `$W` | **预跑 `W0203..W0237` 收尾前或 `merge` 并行执行**(直接用 `$CHAIN/ape-final/unisacc-next.com` 在 `/tmp/ci-dryrun-107` 预演;还能顺带为候选 APE 的 `# UPI1` 预热 run-cache!) | 省去 `--pack` 后 **`~1.5 分钟`** 串行等待 |
| **G. Step 6 公开回读预热(`release-exec.cx -- --warm`)与 `release/notes-0.0.107.md` / `rollback.txt` 预填** | `106` 在 Step 5 首次公开时未预热候选 APE 的 `hostc.cx` + `srcshape.cx`,撞 45s 超时触发回滚重来(损耗 **`~8 分钟`**) | `--warm` 仅依赖候选 APE 与 `$W`;`notes-0.0.107.md` 除 `<CI_RUN_ID>` 外的所有哈希(`F=REV`、`SID`、`APE_SHA`、大小、变更条目)在链止时即已全部确定 | **在预跑尾段或 Step 3 CI 运行期间(`20 分钟` 空窗)完成 `--warm` 与 `notes-0.0.107.md` 终稿预填**(CI 一绿只填 1 个 `<CI_RUN_ID>` 即可秒进 Step 4 `Y1`) | 省去 Step 3->4 **`~5 分钟`** 与 Step 5->6 回滚 **`~8 分钟`** |
| **H. 6 个已知红批前置清零(消除 `R40 B-t` 链后修补与 20 批复跑)** | `106` 在预跑中停机 3 次 + 链后复跑 20 批 + 写 `path-b-proof.md`,共耗 **`~71 分钟`** | `W0087`(`1122ace564`)、`W0148`(`d12d2087bd`)、`W0150`(`5b4f77f33e`)、`W0189`(`deadline=06:30`)、`W0191`(`a46b98d713`)、`W0206`(`8ca2f5e4b8`) 均已合入 `v0.0.107`,加干净 `PATH` 与短 `TMPDIR=/tmp` | **起链前(阶段 B/C)已全部前置消除** | 省去 **`~71 分钟`** 红窗停机与链后路径 B 税 |

---

## (3) 第二车 0.0.107 从链止到 X 的并行执行方案草表(目标:链止 -> X 压缩至 80 分钟内,预跑完 -> X 压缩至 4 分钟内)

```text
[T_chain_stop + 0m] 整链完成 (CHAIN_STOP: REV=F, APE=unisacc-next.com)
  |
  +---> [主车道: 预跑执行 (T+0m -> T+76m, 预计 ~76 分钟)]
  |       (1) T+0m..T+1m: 设干净 PATH(/usr/bin/cc) 与短 TMPDIR=/tmp, 立即执行:
  |           prerun-supervise.cx -- plan (10s) -> prerun-supervise.cx -- run --keep-going product-red
  |       (2) T+1m..T+76m: 连续跑完 W0000..W0237 (6 个 106 红窗已前置修好, 零 PRODUCT_RED_STOP)
  |
  +---> [并行预做车道 (T+1m -> T+30m, 仅读 $CHAIN 与 frozen-root, 零主车道等待)]
          (P1) T+1m (5s):  生成 release/evidence/0.0.107/fixedpoint-6targets.tsv 与 ape-receipt.txt,
                           在 INDEX.tsv 预登记 fixedpoint-6targets 与 ape-receipt 两行
          (P2) T+2m (5s):  组装 ~/.unisacc-release-evidence-backups/0.0.107/src-dir
                           (拷入 compiler.p4.pkg -> p3.pkg, 六目标 g2 切片, cache-manager-{x64,arm64}, sources.tsv, commit.txt)
          (P3) T+3m (10s): 预填 release/notes-0.0.107.md 与 release/evidence/0.0.107/rollback.txt
                           (填入真实 F=REV、SOURCES_SID、APE_SHA、8.9MB 字节数, 仅留 <CI_RUN_ID> 占位)
          (P4) T+15m (5s): W0014/W0015 跑过即写出 cx-booter-ape-deferred.tsv、tools-default-regression-deferred.tsv、
                           expected-fail.tsv、expected-unmeasured.tsv
          (P5) T+20m (27s): 在 frozen-root 上跑 tools/ratchets.cx > release/evidence/0.0.107/ratchets.txt
          (P6) T+74m (45s): 用 $CHAIN/ape-final/unisacc-next.com 在 /tmp/ci-dryrun-107 跑完 CI 核对二 + 核对三(10 个二进制)
                            加 release-exec.cx -- --warm (预热候选 APE 的 hostc.cx 与 srcshape.cx run-cache)
  |
[T_chain_stop + 76m] 预跑 W0237 完成 (PRERUN_RUN status=DONE)
  |
  +---> [T+76m -> T+77m (1m): strict 合并折叠]
  |       sh ./unisacc.com tools/prerun-supervise.cx -- merge . --in ... --out ...
  |       拷入 release/evidence/0.0.107/strict.tsv, 核 unexpected_fail=0
  |
  +---> [T+77m -> T+78m (1m): --pack 打包回填与核对二校验]
  |       sh ./unisacc.com tools/release-run.cx -- --pack release/candidate.tsv \
  |         --chain-dir "$CHAIN" --src-dir "$SRCDIR" \
  |         --strict release/evidence/0.0.107/strict.tsv --out "$S/pack"
  |       (因 --src-dir 已在 P2 备好, --pack 仅需 ~15 秒; 核 PACK_APE SAME 与 layout/SHA256SUMS OK)
  |
  +---> [T+78m -> T+79m (1m): release-exec --dry 全门预演]
  |       sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
  |         --layout "$S/pack/layout" --strict release/evidence/0.0.107/strict.tsv --out "$S/dry" --dry
  |       核 DRY_GATES_OK
  |
  +---> [T+79m -> T+80m (1m): Step 1 提交并推送 X (随后直连 Step 2 草稿 + 核对五 + Step 3 CI)]
          DEVSVC_OWNER=cc sh ./unisacc.com tools/release-exec.cx -- --execute release/candidate.tsv \
            --layout "$S/pack/layout" --strict release/evidence/0.0.107/strict.tsv --out "$S/exec" \
            --from-step 1 --to-step 2
```

### 并行预做方案的五条量化收益汇总

(1) **链止 -> 预跑启动(`plan` + `run`)**:从 `0.0.106` 的 **`15 分 15 秒`**(`21:58:56 -> 22:14:11`)压缩至 **`<= 1 分钟`**(省 `14 分钟`)。
(2) **预跑执行期间(`W0000 -> W0237`)**:6 个 `0.0.106` 红窗已在阶段 B/C 前置修好 + 干净 `PATH` + 短 `TMPDIR=/tmp`,消除 3 次 `PRERUN_PRODUCT_RED_STOP` 停机等待,从 `0.0.106` 的 **`113 分钟`**(`22:15 -> 00:08`)压缩至 **`~75 分钟`**(省 `38 分钟`)。
(3) **预跑期间并行预做 `P1..P6`**:把 `fixedpoint-6targets.tsv`、`ape-receipt.txt`、`--src-dir` 组装、2 个 `*-deferred.tsv`、`ratchets.txt`、CI 核对三(10 个二进制预演)、`release-exec --warm` 与 `notes-0.0.107.md` 草稿全部移入预跑并行的空闲片完成,后置关键路径耗时降为 **`0 秒`**。
(4) **预跑结束(`W0237 DONE`) -> 提交 `X`**:从 `0.0.106` 的 **`42 分 40 秒`**(`00:08:15 -> 00:50:55`)压缩至 **`<= 4 分钟`**(`merge 1m` + `pack 1m` + `dry 1m` + `Step 1 X 1m`,省 `38 分钟`)。
(5) **总计(`CHAIN_STOP` -> `X`)**:从 `0.0.106` 实测的 **`171 分 59 秒`(`2h52m`)** 压缩至 **`~80 分钟`(`1h20m`)**,整体缩短 **`~92 分钟`**。
