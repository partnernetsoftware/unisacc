# 第二车 0.0.107 与 0.1.0 24 小时目标实况倒计时与关键路径(04:22 W0201 路径 C 重起链版)

- 实况基准时刻:`2026-11-25 04:22 +0800`(按 `04:19` `W0201` 产品红停与 `04:21` 编排者**路径 C 重起链**裁定更新)
- **路径 C 重起链触发事实与裁定记录(`2026-11-25 04:19 -> 04:22 +0800`)**:
  - `(1)` 触发事件:第一轮 `0.0.107` 预跑(`F1=28b6a518fe9fac6b117f31adeb25e1c8ab51701f`, `APE1=9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544`)在 `04:19` 推进至 `W0201`(`202/247` 窗)时触发 `PRERUN_PRODUCT_RED_STOP`,批次 `c99-array-param-typedef`(`product-qualification`)两次重试均 `FAIL rc=1`(`3610ms`, `marker=ARRPARAM_SUMMARY`, `passed=9 failed=1 total=10`)。
  - `(2)` 具名根因(`cdx` 私有直证与本窗复现):唯一失败用例为 `ARRPARAM_CASE N2-param-unclosed-bracket FAIL want_rc0=0 got_rc=0 err=...:2:15: error: expected ']'`——候选编译器对语法错误 `typedef int row[3]; void f(row r[2);` 打印了 `error: expected ']'`,但退出码仍返回 `rc=0` 并写出 `N2.o`(`2144B`),属确定性候选产品缺陷,不得改测试迁就。
  - `(3)` 既有正面成果(已验证):`F1` 转发桩批 **`W0199`(`fwd-cap`, `PASS rc=0`, `19846ms`, `assertions=28`)** 及 `W0198`(`c99-global-redecl-conflict` `PASS`)、`W0200`(`c99-tentative-array-incomplete` `PASS`)、`W0015`(`tools-default-regression` `PASS`)、`W0080`(`check-split-parents` `PASS`)、`W0087`(`check-gen-all-drift` `PASS`)、`W0148`(`check-ln-noderef` `PASS`)、`W0150`(`release-exec-c` `PASS`)均已在 `APE1` 上通过;同时 `cdx` 正在对 `APE1` 单批扫完剩余 `W0202..W0246`(45 窗)以一次性摸清是否还有同类红批。
  - `(4)` **路径 C 执行方案**:
    - 原链目录 `~/.cache/unisacc/chains/bg-b1/28b6a518fe...` 与原预跑目录 `~/.cache/unisacc/prerun/v0.0.107-28b6a518fe`(`STOPPED` 于 `W0201`)原样保留作历史审计证据,不清理、不复用其 202 窗结果(严格表绑定候选 APE 字节);
    - `ompgpt` 修复 `N2` 错误退出码(`research/ompgpt/c99-n2-exitcode-fix.patch`) -> `cdx` 对抗复核 + 剩余 45 窗摸底 -> 阶段 C 重封口 `F2`(刷新冻结截止期至 `10:00`) -> 重起第二轮 `0.0.107` 整链(`~27m`) -> 新候选 `APE2` 全量重预跑(`~70m`) -> 补跑与 Step 1-7 公开发布(`~60m`) -> 直接起 `0.1.0` 封版车。
- 24 小时总目标截止:`2026-11-25 17:50 +0800`(当前剩余总窗口 **`13 小时 28 分钟`**,即 **`808 分钟`**)

---

## (1) 路径 C 重起链从现在(04:22)到 0.0.107 公开的两档时刻重估(基准 187m vs 保守 235m)

| 阶段 | 动作与责任窗口 | 基准耗时 | 保守耗时 | 基准起止时刻 | 保守起止时刻 | 数字出处与损耗挡板 |
|---|---|---|---|---|---|---|
| C1:产品修复 + 对抗复核 + 剩余 45 窗摸底 + 重封口 `F4` | (1) `ompgpt` 出 `c99-n2-exitcode-fix.patch`;<br>(2) `cdx` 变异复核 + 扫完 `APE1` 剩余 `W0202..W0246`;<br>(3) `agyneon`/`cc` 合入修复、刷新冻结截止期至 `10:00`(`1795572000`)、封口 `F4=63354b4a2cb707ecc923a89f0df6030149fa697f` 并过全量 `chain-ready` | `46m (实测)` | `46m (已完)` | `04:22 -> 05:08:30 (已完)` | `04:22 -> 05:08:30 (已完)` | `05:08:30` (`T1=1795554510`)正式起第二轮链(`PID/PGID=2968889`) |
| C2:`0.0.107` 第二轮整链(`F4=63354b4a2c`) | `bg-candidate start --rev 63354b4a2cb707ecc923a89f0df6030149fa697f --parallel 4` 跑 `499/499` + `promote --check` | `27m` | `32m` | `05:08:30 -> 05:35` | `05:08:30 -> 05:40` | 第一轮 `28b6a518fe` 实测 `26.2 分钟`,预计 `05:34 -> 05:37` 链止 |
| C3:`P1-P3` 覆写(`F4/APE2`) + `kit 17/0` + 247 窗全量重预跑 | (1) `P1-P3` 以 `F4/APE2` 生成 `release/evidence/0.0.107/*` 与 `src-dir`(核零 `28b6a518fe/9a1e2630` 残留);<br>(2) `ompgpt` 跑两轮 `tlib-kit` 达 `17/0`(`cx.plan@0.1.14`);<br>(3) `agy-flow` 以 `timeout 5400s`、`--window-lanes 2 --max-jobs 8`(无 `--normal-jobs`)全量跑 247 窗(14 个新批置顶前 20 窗) | `70m` | `85m` | `05:35 -> 06:45` | `05:40 -> 07:05` | `old-product-cache` `0700` 已修好,`W0019..W0034` 16 窗首跑直过 |
| C4:尾段单批补跑 + `P5/P6` + `merge/archive` + `--pack/--dry` + Step 1-7 公开与收尾 | (1) 预跑完空闲单批补跑 `W0014`(`UNISACC_BOOTER_CHAIN_OUT=$CHAIN2`)与 `W0088`(`slow-split`)(`~3m`);<br>(2) `P5 ratchets` + `P6` CI 核对三/读回 `--warm` + `merge/archive` + `--pack` + `--dry`(`~10m`);<br>(3) Step 1 `X` -> Step 2 草稿 + **核对五(`slice-win-arm64` 独立下载)** -> Step 3 CI -> Step 4 `Y1` -> Step 5 `publish`(#14 v2 预热门) -> Step 6 `readback` -> Step 7 `Y2` + `release-post`/`R33`(`~47m`) | `60m` | `75m` | `06:45 -> 07:45` | `07:05 -> 08:20` | 单轮 CI `~18-20m`(保守含 1 次 `G4 --redispatch` `+18m`),距 `13:40` 余量 `+5h20m -> +5h55m` |

- **`0.0.107` 重起链后公开时刻**:
  - **基准档**:**`2026-11-25 07:25 -> 07:35 +0800`**(从 `04:22` 起算总计 `187 分钟` = `3 小时 07 分钟`)。
  - **保守档**:**`2026-11-25 08:10 -> 08:25 +0800`**(从 `04:22` 起算总计 `235 分钟` = `3 小时 55 分钟`)。

---

## (2) 0.0.107 公开时刻与 0.1.0 最迟起链线(13:40)及总截止(17:50)的余量核算表

| 档位 | `0.0.107` 公开 / 解冻时刻 | 距 `0.1.0` 最迟起链硬线 (`13:40`) 的余量 | `0.1.0` 计划起链时刻 | `0.1.0` 预计公开时刻(零代码变化封版车 `160-220m`) | 距 24 小时总截止 (`17:50`) 的余量 |
|---|---|---|---|---|---|
| **路径 C 基准档** | **`07:29`** (`07:25 -> 07:35`) | **`+6 小时 11 分钟` (`+371 分钟`)** | `07:40` | **`10:25 -> 10:45`** | **`+7 小时 05 分钟` (`+425 分钟`)** |
| **路径 C 保守档** | **`08:17`** (`08:10 -> 08:25`) | **`+5 小时 23 分钟` (`+323 分钟`)** | `08:30` | **`12:00 -> 12:15`** | **`+5 小时 35 分钟` (`+335 分钟`)** |
| **路径 C 极端兜底档** (假定 `107` 或 `0.1.0` 再遇一次 90m 波折) | `09:30` | **`+4 小时 10 分钟` (`+250 分钟`)** | `09:45` | **`13:30 -> 14:00`** | **`+3 小时 50 分钟` (`+230 分钟`)** |

- **核心结论**:即便走路径 C 重起链(`+187m -> +235m`),`0.0.107` 在 **`07:29`(基准)至 `08:17`(保守)** 即可完成公开解冻,距离 `0.1.0` 最迟起链硬线 **`13:40` 仍留有 `5 小时 23 分钟` 至 `6 小时 11 分钟` 的充裕余量**(`0.1.0` 在 `10:45 -> 12:15` 即可公开,距离 `17:50` 总截止留有 `5.5 - 7 小时` 余量)。

---

## (3) 路径 C 重起链新增的四条防损耗挡板(增补 loss-guards-107,杜绝第一轮四类损耗重演)

在 `research/ccogweco/loss-guards-107.md` 原有五条挡板(草稿后独立下载 `slice-win-arm64`、Step 1 `X` 前预热 `precommit`、候选表七哈希与 `PACK_APE SAME`、Step 5 前 `--warm` + `#14 v2` `READBACK_PREWARM`、`notes` 预审仅留 `<CI_RUN_ID>`)基础上,针对第一轮预跑暴露的四处损耗新增 **挡板六至挡板九**(同步写入 `research/agy-flow/car2-parallel-prep.md`):

| 挡板编号 | 第一轮(`F1=28b6a518fe`)暴露的损耗 | 第二轮(`F2`)重起链强制挡板与验收命令原行 | 节省时间 / 规避风险 |
|---|---|---|---|
| **挡板六:旧候选 `F1/APE1` 证据隔离与新候选 `F2/APE2` 覆写核验** | 第一轮 `P1-P3` 已在 `release/evidence/0.0.107/` 与 `~/.unisacc-release-evidence-backups/0.0.107/src-dir` 写入 `F1`(`28b6a518fe`)与 `APE1`(`9a1e2630`)文件;若第二轮漏覆写任一文件会导致 `release-run --pack` 或 `release-exec --dry` 报身份不符。 | 1) 旧链 `~/.cache/unisacc/chains/bg-b1/28b6a518fe...` 与旧预跑 `~/.cache/unisacc/prerun/v0.0.107-28b6a518fe` 原样保留只读;<br>2) 第二轮链止 `T2_new + 1m` 重跑 `P1-P3` 覆写 `release/evidence/0.0.107/*` 与 `src-dir/*` 后,强制执行:<br>`test "$(grep -cE '28b6a518fe\|9a1e263010' "$W"/release/evidence/0.0.107/* 2>/dev/null \| awk -F: '{s+=$2} END{print s+0}')" -eq 0 && echo "GUARD6_NO_STALE_F1_APE1=PASS"` | 100% 杜绝新旧候选证据串味 |
| **挡板七:`old-product-cache` `0700` 权限核验 + 新候选 `kit 17/0` 双轮预备** | 第一轮因 `old-product-cache/5f83ac93...` 目录 `0775` 导致 `W0019..W0034` 16 个 `netregen` 窗 `UNKNOWN 3`,且 `tlib-kit` 首轮 `16/1` 缺 `cx.plan@0.1.14`。 | 1) 起预跑前必核 `old-product-cache` 权限为 `700`:<br>`test "$(stat -c '%a' ~/.cache/unisacc/prep-cache/old-product-cache/5f83ac93*)" = 700 && echo "GUARD7_OLDPROD_0700=PASS"`<br>2) 新候选 `APE2` 封包后,固定跑**两轮** `tlib.cx -- kit` 直至输出 `PREPARED tlib-kit libs=17 skipped=0` 且 `kit.tsv` 含 `cx.plan@0.1.14.tlib` 才放行预跑。 | 让 `W0019..W0034` 16 个 `netregen` 窗在第二轮预跑首轮全部直过(直接省去尾段 16 窗补跑的 `~5 分钟`) |
| **挡板八:预跑守护外层 `timeout 5400s` + 剔除 `--normal-jobs` + 预跑前清理旧 `prerun-windows.tsv`** | 第一轮 (a) `prerun-inputs.cx` 模板带 `--normal-jobs 8` 触发 `prerun-drive.cx:3067 unknown-run-arg`;(b) 外层 `timeout 3600s`(60 分钟)低于 247 窗实测时长(`~70m`)。 | 1) 第二轮预跑守护固定使用 **`timeout -k 10s 5400s`**(90 分钟),且 `prerun-drive.cx -- run` 只传 `--window-lanes 2 --resume --max-jobs 8 --keep-going --min-free 2147483648`(严禁传 `--normal-jobs`);<br>2) `prerun-drive.cx -- archive` 前先记 `sha256sum` 并 `rm -f release/evidence/0.0.107/prerun-windows.tsv`,避开 `archive-destination-exists`。 | 消除 60 分钟中途被外层 `timeout` 误杀与启动参数拒绝 |
| **挡板九:阶段 C 重封口 `F2` 时将 `build/freeze.tsv` 截止期续期至 `10:00`(`1795572000`)** | 当前 `main` 上 `0.0.107-FREEZE` 截止为 `07:30`、车树 `F1` 上为 `08:00`;重起链后保守公开时刻为 `08:10 -> 08:25`,若不续期会在预跑尾段(`W0189`)或 `release-exec` 时撞冻结过期(重演 `0.0.106` `W0189` 事故)。 | 在 `F2` 阶段 C 封口前,由 `cc` 将 `main` 与 `v0.0.107` 车树的 `build/freeze.tsv` `0.0.107-FREEZE` `deadline_epoch_s` 刷新至 **`1795572000`(`2026-11-25 10:00:00 +0800`)**。 | 彻底封堵第二轮预跑 `W0189` 与发布阶段冻结行过期 |
