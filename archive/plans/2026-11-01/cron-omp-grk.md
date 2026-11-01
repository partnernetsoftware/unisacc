# omp-grk 自巡检（接替 cc，2026-10-31 夜，主人裁定）

主人原话：「把你（cc）的定时器迁移到 omp-grk 上，让它自己安排算了，然后我想安排你下线休息了。」所以：cc 下线，**主刀 omp-grk 同时承担巡检、记账、范围取舍**。主人要结果；范围/取舍你来定，初心和需求有变主人会说。

定时器：你自己在你的环境里建一个每 10 分钟一次的自巡检（提示词就一句「按 plans/cron-omp-grk.md 执行一轮」）；也可以按工作节奏连续做、卡在等机器时再回来巡检。要改巡检内容就改这个文件。

禁区：闸、Draft、Latest、签名、公开发布，**一律不碰，等主人授权**。QUALIFIED 出来（`sh ./unisacc.com tests/run.cx -- --all`）时，在终端里直接告诉主人，并附 [release-0.0.43-known-gaps.md](release-0.0.43-known-gaps.md) 的豁免清单请主人过一眼。

## 路线（从现在到 QUALIFIED）

1. 候选整单真绿（私有候选，完整布局的私有根，见下）。
2. 补完 5 个剩格：`elf.pos.exact-image-independent`、`elf.pos.windows-args-template`、`lower.pos.darwin-carry|windows-gates|arm-narrowing`（对产品镜像解码；确实只有流容器才看得到的，写理由并入 R7）。
3. 打包安装冻结：`tools/release-build.cx` 五片 → 两代一致 → `release-ape.cx` → 装。
4. 装好后：用 `tools/matrix-flip-gen.cx` 把 STALE-RED 的格翻回（清单见下）；在已装产品上重跑 `tools/census.cx`；bootstrap（10 格）、candidate（6 格）走慢车道（context/PC/两代 GEN/kind2/安装/全批/矩阵换标，`plans/dev-methodology-2026-10-28.md`）。
5. 176 个声明了材料的批次备料（`tests/run.cx --list` 看 UNPREPARED），再跑完整 `tools/all-waves.cx` 会话。
6. 外平台 73 格：把冻结候选推到 CI（主人已批准推送），在 osx/win/lnx-arm64 实机上跑。
7. `tests/run.cx -- --all` 输出 QUALIFIED。

## 现在手头的发现（cc 交接，10-31 夜）

- 私有候选 456c7ee0（omp-grk 造的）整单：169 个批次，122 PASS，47 FAIL。其中 19 个 `full-*` 汇总是必需格没满足前本来就红。
- 真红且和候选有关：
  - `phase-timing-stamps`：`run.cold` 缺 `lookup_hit=0` 的语法（`stamp-grammar-one-lookup_hit=0-positive-ns`）；`run.miss.stored-one-host-entry-and-e3-receipt`；`obj.miss` 缺 `artifact_hit=0 mode=2`。
  - `cx-l0-sys-ffi`：已装产品 PASS，候选上 FAIL，日志 `reject: not covered: address or target declaration` / `unisacc: model driver: memory` / `route=lnx/x86_64/memory`，像是候选里某一刀影响了 memory 路线，**可能是真回归**，先查。日志在 `out/priv-full/out/census/log/cx-l0-sys-ffi.log`。
- 不是回归：`resources-limits` 的 `fd-no-leak-to-program` 在完整布局私有根里 PASS（`out/priv-full/out/census/results.tsv`）；它在别的环境红是调用者传下来的多余 fd 被产品原样交给 -run 程序。`snapshots*`、`core-compat`、`library-cells`、`cx-model-*`、`capsule-export`、`tape-gap-tool` 在完整布局私有根里 PASS。**造私有根要用完整布局**：符号链接顶层所有条目（除 `out`、`unisacc.com`、`.git`、`tmp`），复制候选为 `ROOT/unisacc.com`，再 `tools/census.cx -- ROOT ROOT/out/census --only …`。`out/priv-full` 就是这样的现成根（候选 456c7ee0，换新候选时只换 `unisacc.com`）。
- bdy（窗口 `1:omp-bdy`）：`cx0-unit`（`no host function sy_realpath`）、`cx-unit-graph`、`cx-host-run`、`cx-layers`、`cx-funit-host` 是 cx 车道的旧红，已发信；`cx-unit-graph` 在候选上已绿。bdy 的 `tests/cx-host-run.cx` 有 3 个 libc 调用让 cxlibc 棘轮红，已发信。
- csih 车道（窗口 `1:lz-unisacc`）：只知会过 `ujs/product/ape-pack.cx` 的 `--need` 改动（opt-in，csih.com/ujs.com 启动器文本不变）。`apps/csih/` 和 `ujs/` 不碰。

## STALE-RED 的格（批次转绿后用 matrix-flip-gen 翻回）

ri：`ri.frame.hash-mismatch`、`ri.id.sealed-live-header`、`ri.id.e3-companion-states`、`ri.class.source.MISSING`、`ri.class.inc-quote.PRESENT`、`ri.route.image`、`ri.route.multi-tu`（批次 ri-image-single/multi/pem，候选上已绿）；negative：`loader.footer`、`loader.d-row`（negative-model-loader，候选上已绿）、`diag.error-limit`（negative-diagnostics，候选上已绿）。其余未降格的 MISSING 格见 `tools/release-board.cx`。`ri.class.source.EMPTY-ABSENT`、`ri.id.e3-companion-file-faults`、`ri.id.front-blob-mutation`、`ri.id.lower-stage-cli`、`ri.class.attributes.cli` 要等候选装好后做（image 路线现在能出账本了）；`res.mem.compile-as-limit`、`res.mem.run-reserve`、`res.mem.recovery`（要用 `--amend` 把『~2GB 预留』改成现状）、`res.recovery.after-budget` 等启动器预检进候选后做；`res.runner.timeout-is-unknown`、`res.runner.bound-rc142` 未做。`loader.terminal-elf-stage` 应随 negative-model-loader-terminal 转绿后翻（它是 BLOCKED，要手改状态）。

## 工具（都在 tools/，prd.md 能力速查有行）

- `tools/census.cx`：已装/私有产品上跑全部无材料批次（`--only/--redo/--report`，可续跑）。
- `tools/matrix-flip-gen.cx`：MISSING→COVERED_LOCAL；`--amend` 改要求文字并写依据；`--migrate` 换候选标。
- `tools/matrix-stale.cx`：COVERED_LOCAL→MISSING（STALE-RED，证据是红批次时用）。
- `tools/matrix-waive.cx` + `tests/waivers-0.0.43.tsv`：已知缺口豁免（R1–R12，写理由和 0.0.44 编号；`--check` 必须 `disagree=0`）。豁免只用于确实做不到的；每次豁免同步 `plans/v0.0.44.md` 和 `plans/release-0.0.43-known-gaps.md`。
- `tools/commit-own.cx --keyed`：共享表只提交自己的行。`tools/ratchets.cx`：七个棘轮必须 7/7。`tools/release-board.cx`、`tests/full-domain.cx ROOT DOMAIN`：看板。
- 退役批次的做法：删 `tests/batches.tsv` 和 `tests/coverage.tsv` 的行，豁免对应格，`tests/run.cx -- --list` 要 `metadata_valid=1`（见 negative-model-qprefix、formats-tapebin 的例子）。已知缺口的断言改成 `KNOWN_GAP` 行（不删断言，产品以后变好检查自动恢复；见 pp-cli-domain.cx 的 `known_gap`）。

## 每轮

- 看自己的进展，再看全局：`git log`、`tools/release-board.cx`、`tools/ratchets.cx`（7/7）、census 结果。偏题就拉回，卡住就换动作，不重复同样命令。
- 维护计划：`plans/release-0.0.43.md`（STATE43）、`plans/v0.0.44.md`（STATE44）、`prd.md` §2（STATEPRD）三块「当前状态」原位更新，打勾要有提交号。
- 提交：只提交自己的路径；一律 main；不用 stash/reset/checkout/clean；每条命令 ≤60 秒，等待要有总 deadline 和次数上限；单机同时只做一组重测试。
- 不要写 `cat > /dev/null`（会等 stdin 卡 120 秒）。
- 不通过删断言、缩覆盖、放宽时限制造绿。

## 停止条件

QUALIFIED 并已告诉主人，或主人叫停：停掉你的定时器，短报一次。发布（签名/Draft/Latest/公开）等主人授权后再做。
