# v0.1.3 发布说明:已知缺口段草稿(最终状态)

口径:每条给「缺口 / 状态 / 来源与证据路径 / 可复算数字」。来源为调度信(本树无文件)的项明确标「来源:cc 派工信,证据路径待补」,不写成已验证。

## 一、C3 进程生命周期

- **C3 S3 顺延 v0.1.4。** 本版不含 S3 的消费者收口;发布说明按「S3 推 v0.1.4」写。来源:`research/cdx/notes-0.1.3-skeleton.md:49`(S1 至 S3 范围与实际合入范围待 `<C3_SCOPE_COMMITS>`/`<C3_CHECK_RESULTS>`)。
- **cdx 25 格待定。** 口径与清单来源:cc 派工信(2026-11-26),证据路径待补(本树未命中 25 格清单)。
- **gatekeeper B1-B5 未闭。** 来源:cc 派工信;证据路径待补(本树只有 `research/ccogweco/after-seal-roadmap.md` 的项目 B 条目,非 C3 的 B1-B5)。
- **I9 进程树收集上限场景未验(S4)。** 来源:`research/cdx/c3-invariants.md:23`。
- **裸 fork/waitpid 棘轮未入库。** 存量 40 调用/12 文件(`research/ccogweco/bare-proc-calls.tsv`,列计数可复算);棘轮补丁 `research/ccogweco/s5-bare-proc-ratchet-v3.patch`(私有树夹具 13 断言、`RATCHETS ok=20 fail=0`)。
- **macOS/Windows `sy_proc_finished` UNCOVERED。** 来源:`research/cdx/notes-0.1.3-skeleton.md:51`。
- **27 格 UNMEASURED(未计 PASS)。** 清单 `tools/proc-own-unmeasured.tsv`(28 行 = 头 + 27,可 `wc -l` 复算);检查器 `tools/check-proc-own-table.cx`;登记提交 `d51481c6b`(夹具 90 格 63 PASS、27 具名 UNMEASURED)。

## 二、tape

- **15 倍速度门未达。** 正式测量:16 对象 94.0 倍、64 对象 114.3 倍、102 对象未完成。来源:`research/agy-flow/v013-release-plan.md:22`;量化与步数上限:`research/agy-flow/tape-t14c-speed-baseline.md:71`(15 倍门 29370 ms,至多 ~16.9-18.5 亿步)。
- **T1.6 未做;对象不可复现。** T1.6 预解码为候选来源同上条;「对象不可复现」证据路径待补(候选 `research/agy-flow/tape-m0-bets.md` §7 复现命令)。

## 三、nn

- **平台 UNCOVERED:Darwin/Windows。** 四格精确分量 UNMEASURED;来源:`research/agy-h/slice-nn-progress.md:113`。
- **同尺寸原地写 UNSUPPORTED-DOCUMENTED,非缺陷。** 来源待补(本树未命中原文;A3-4 v4 状态以 agy-h 为准)。

## 四、工具线未落项

以下四项未入 v0.1.3,来源:`research/agy-h/toolline-series/README.md:13,17,151,155`:
- `--rebase-rev`(agyneon;`tools/prerun-drive.cx`、`tools/check-prerun-rebase.cx`);
- 短批出 `long` 窗装箱(`research/agy-h/short-batch-packing.patch`,mk 已 `apply --check` 通过);
- `chain-ready` `--launch-env`(`research/agy-h/toolline-series/0001-chain-ready-launch-env.patch`,sha256=e8c210c1…);
- 门 23(`gate23-pik1`,与 launch-env 同改 `tools/chain-ready.cx`,须串行)。

## 使用说明

- 可复算数字:`bare-proc-calls.tsv`(调用/文件数按列统计)、`proc-own-unmeasured.tsv`(27 行)、S5 补丁夹具断言数(`ck(` 计数 13)。
- 发布说明引用 RATCHETS/夹具数字时注明「私有树验证」;「证据路径待补」条目发布前必须补齐或删除。
