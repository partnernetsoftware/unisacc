# v0.1.3 编号后第一周工具线路线表

- 维护者: `agy-h` | 日期: `2026-11-25` | 范围: 今天 (`2026-11-25`) 累积的 9 项工具线待办排序与落库编排
- 交付入口: [toolline-roadmap.md](file://~/repos/unisacc/research/agy-h/toolline-roadmap.md)
- 既有私有前提: [short-batch-packing.patch](file://~/repos/unisacc/research/agy-h/short-batch-packing.patch) 保持私有未提交; 本页仅作路线编排, 不改源码、不碰共享 `main` 与 `v0.1.2` 工作树。

---

## (1) 红线总则: 哪些必须在 v0.1.2 公开前不碰 main

依据 [prd_mempalace.md](file://~/repos/unisacc/prd_mempalace.md) 第 39、47 条及 [chain-ready.cx](file://~/repos/unisacc/tools/chain-ready.cx#L1552-L1642)、[release-exec.cx](file://~/repos/unisacc/tools/release-exec.cx#L261) 现行逻辑:
- `release-exec gate_main` 与门 `23-freeze-armed` 当前均绑定 git 提交历史 (`origin/main` 必须为发布 `HEAD` 祖先 / `HEAD behind origin/main == 0`), 任何推送 `origin/main` 的提交都会立即触发 `MAIN_ADVANCED` / `FREEZE_BEHIND_MAIN`;
- 同时列车工具 (`chain-ready`、`bg-candidate`、`prerun-drive`、`dvqualify`、`release-exec`) 在冻结到公开期间冻结。

因此, **在门 23 改 `PIK1` 判据落地前, 全部 9 项对共享 `main` 的合入与推送都必须在 `v0.1.2` 公开 (`Y2` 与 `release-post` 解冻 `build/freeze.tsv`) 之后才能动 `main`**, 具体分为两级约束:

| 约束级别 | 涵盖项目 | `v0.1.2` 公开前允许动作 | 禁止动作 |
| --- | --- | --- | --- |
| 一级绝对禁碰 `main` (直触在途列车/发布门写域) | `(1) --rebase-rev`、`(2) 短批装箱`、`(3) dvqualify --out 与 prerun-drive retry W99xx`、`(4) chain-ready helper/prepare/launch-env`、`(5) 门 23 改 PIK1`、`(6) freeze-cut`、`(8) lane-isolation 最小件` | 仅限在 `$CLAUDE_JOB_DIR/tmp`、`/tmp`、私有归档或个人研究目录完成编码、补丁文本、私有回归与回放对拍 | 绝对禁止向共享 `main` 提交/推送, 禁止修改 `.claude/worktrees/v0.1.2` 源码 |
| 二级待解冻落库 (写域独立但不经 `main` 推进) | `(7) notes-evidence 入库` ([v012-release-unified.md](file://~/.local/share/unisacc/research/ompgpt/v012-release-unified.md#L24) 已明确属发布后另件)、`(9) check-prerun-start 与 admission P1` | 同上: 在私有副本补齐自含夹具、`cold-real` 阈值校准、走 `ratchets.cx` 干跑验证就绪 | `v0.1.2` 公开前同样不向共享 `main` 落库 (`git push origin main` 会触发 `MAIN_ADVANCED`); 待 `v0.1.2` 解冻后作为第一批无冲突提交率先合入 |

---

## (2) 工具线 9 项待办排序与明细表 (v0.1.3 编号后第一周)

按「写域互斥分为 3 条并行泳道 (`L1 预跑引擎`、`L2 起链与冻结门`、`L3 独立工具`), 泳道内先保高速迭代止血项与共享文件先后依赖」排序如下 (全部 **不触 `SOURCES_SID`**, 仅 `tests/batches.tsv` / `tests/coverage.tsv` 在合入登记行时经 `tools/commit-own.cx --keyed` 串行提交):

| 全局序 / 泳道 | 待办项 | 写域 | 先红后绿验收 | 触 SID | 预计小时 | 依赖 | 谁做 (主刀 / 复核) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `S1` (`L1-1`) | `--rebase-rev NEWREV` | `tools/prerun-drive.cx`, `tools/check-prerun-rebase.cx`, `tests/batches.tsv`, `tests/coverage.tsv` (`--keyed`) | 红: 改 `cx/` 使 `SID` 不等必 `REFUSE`, 非法路径越界必 `REFUSE`, 改单批测试若未删 `done.W*` 必 FAIL, 10% 抽样 FAIL 全局作废; 绿: `W0219` 停止快照(`prerun-v012-stop-W0118`) + `C9b` 回放保留未受影响窗且 `check-prerun-rebase` 全绿 | 否 | `2.5 - 3.5h` (私有实现约 1.5h + 复核落库约 1.5h) | 依赖 [prerun-rebase-design.md](file://~/.local/share/unisacc/research/agyneon/prerun-rebase-design.md); 必须排在 `L1-2` 之前串行落 `main` (`v0.1.2` 公开后) | `agyneon` / `ompgpt` |
| `S2` (`L3-1`) | `notes-evidence` 入库 | `tools/notes-evidence.cx`, `tools/check-notes-evidence.cx`, `tests/batches.tsv`, `tests/coverage.tsv` (`--keyed`) | 红: 原 [check-notes-evidence.cx](file://~/.local/share/unisacc/research/agyneon/check-notes-evidence.cx#L21) 默认读 `/tmp/v011-notes-ev.out` 硬编码产物且缺负控; 篡改 `ape-receipt` 1 字节或缺失 dir 必报错; 绿: 改用夹具内合成 `evidence-dir` (`ape-receipt.txt`, `fixedpoint-6targets.tsv`, `owner.tsv`, `exit.tsv`), 自检 PASS 且全量 `ratchets.cx` (`no-home-path` 等) 过线 | 否 | `1.0 - 1.5h` | 无前序技术阻塞; 与 `L1`/`L2` 写域完全不相交; `v0.1.2` 解冻即可首并发车 | `ccogweco` (自含夹具与登记) / `agyneon` (原代码作者) |
| `S3` (`L2-1`) | `chain-ready` 自带 helper 重建与 `--prepare` 与 `--launch-env` | `tools/chain-ready.cx`, `tools/check-chain-ready.cx`, `tools/bg-candidate.cx`, `tools/check-bg-candidate.cx` | 红: 缺 `UNISACC_IDENT_HELPER`、篡改 helper 尾字节 1B、带 `UNISACC_FREEZE_NO_FETCH=1` 时 `bg-candidate start` 建目录前必拒; 空缓存不跑 `--prepare` 则 `2a-derive` 落入 `cold`; 绿: 先跑 `chain-ready --prepare` (`PREPARED`) 再按 `--launch-env` 规范 `env -i` 跑通, `2a-derive` 6 片全 `identity-hit < 25s`, 门 10/20 PASS | 否 | `2.0 - 3.0h` | 补丁草稿已在 [launch-improve-123.md](file://~/.local/share/unisacc/research/agy-flow/launch-improve-123.md) 就绪; 须在 `L2-2` 之前落 `tools/chain-ready.cx` | `agy-flow` / `ompgpt` |
| `S4` (`L3-2`) | `check-prerun-start` 与 `admission` 的 P1 | `tools/prerun-start.cx`, `tools/check-prerun-start.cx`, `tests/batches.tsv`, `tests/coverage.tsv` (`--keyed`) | 红: `kill -9` 外层而子写手存活时次发 `prerun-start --nowait` 必须 `rc=2 already-running` 无并写; `cold-warmup-ms-out-of-range` 在非法超限区间拒入而按 [manifest.tsv](file://~/.local/share/unisacc/research/agy-flow/cold-real-20261125/manifest.tsv) 实测 (`cx-bounded 14.3s`) 校准范围后合法放行; 篡改收据 `ape_sha256` 续跑必拒; 绿: 子退即放锁、`--allow-migrate` 先写后换连续保留 `.orig/.orig.1/.orig.2`、`check-prerun-start` 全绿过 `ratchets` | 否 | `3.0 - 4.0h` | 已有 `prerun-start-d1.patch` 与 `prerun-start-p1-full.patch`; 与 `L1`/`L2` 写域独立, 可在 `v0.1.2` 解冻后立即并行落地 | `agy-flow` / `ompgpt` |
| `S5` (`L1-2`) | 短批装箱 (`short-batch-packing.patch`) | `tools/prerun-drive.cx`, `tools/check-prerun-drive.cx` | 红: 补丁前 `--main` 跑第 18 项 `short-batch-out-of-long` 得 `ok=54 failed=1`; 绿: 5 轮尾窗 (`PD_TIMES_WINDOW=5`) 消除历史单刺, 安全短批并 `normal jobs=8`, 7 域独占/定时批留 `long`, `diff -u` 证 17 批与 527 批全集 100% 守恒 (`ok=95 failed=0`), `long 138->78` | 否 | `0.5 - 1.0h` (仅做基于 `S1` 的 rebase 与全套件复测) | 补丁已完备并通过 `apply --check`; 必须严格串行在 `S1 (--rebase-rev)` 合入 `main` 之后 | `agy-h` / `agyneon` |
| `S6` (`L1-3`) | `lane-isolation` 最小件 127 行 | `tools/prerun-drive.cx`, `tools/check-prerun-drive.cx` | 红: 传入 `--cpuset 0-999` / 非法列表 / 超允许核掩码无具名拒绝, 或无钉核时子进程核掩码漂移; 绿: `--cpuset 0-5` 非法返回 `rc=2 invalid-cpuset`, 合法输出 `PRERUN_CPUSET verified=1`, 子进程/重试均继承 `taskset -c`, 并行编译不致 `rc=124` | 否 | `1.5 - 2.0h` (补 `check-prerun-drive` 夹具段 + 基于 `S5` rebase 实跑验证) | 127 行主补丁 [lane-isolation-min.patch](file://~/.local/share/unisacc/research/agy-flow/lane-isolation-min.patch) 已在案; 须排在 `S5` 之后串行合入 `tools/prerun-drive.cx` | `agy-flow` / `agy-h` |
| `S7` (`L1-4`) | `dvqualify --out` 唯一性护栏与 `prerun-drive` retry 不 `unlink` `W99xx` | `tools/dvqualify.cx`, `tools/check-dvqualify.cx`, `tools/prerun-drive.cx`, `tools/check-prerun-drive.cx` | 红: 现行 [dvqualify.cx](file://~/repos/unisacc/tools/dvqualify.cx#L705-L718) 用 `O_WRONLY\|O_CREAT` 直接覆写已有 `--out`, 现行 `prerun-drive.cx` 在启动/retry 时 `sy_unlink(outp)`; 绿: 非 `--resume` 下 `--out` 目标已存在必 `rc=2` 拒绝且原文件 sha 不变, `prerun-drive` 重试不 `unlink` 已有 `strict-W99*` 及前轮证据 (保留旧记录/后缀隔离), 两套自检全绿 | 否 | `1.5 - 2.0h` | `dvqualify.cx` 侧可先在私有完成测试; `prerun-drive.cx` 侧紧接 `S6` 串行合入 | `ompgpt` / `agy-flow` |
| `S8` (`L2-2`) | 门 23 (`23-freeze-armed`) 改 `PIK1` 判据 | `tools/cxlib/prodkey.cx` (或 `tools/product-input-key.cx`), `tools/chain-ready.cx`, `tools/check-chain-ready.cx`, `tools/release-exec.cx`, `tools/check-release-exec.cx`, `tests/batches.tsv` (`--keyed`) | 红: 先跑 `fast-iteration F3` 跨版 (`0.0.107`/`0.1.0`/`0.1.2`) 解包对拍验证 `PIK1` 闭包无漏项; 夹具中在冻结 `F` 后向 `origin/main` 推纯文档/工具提交, 现行门 23 报 `FREEZE_BEHIND_MAIN`; 再推 1 笔 `cx/` 改动 (`PIK1` 变) 未拒绝必 FAIL; 绿: `PIK1` 相等时工具推 `main` 门 23 与 `gate_main` PASS, `PIK1` 变化时仍硬拒绝 | 否 | `4.0 - 6.0h` (`F3` 对拍 0.5h + `PIK1` 计算与 8 条夹具 2.5-3.5h + 门 23/`release-exec` 对接 1.5-2h) | 前置依赖: `S3` 先落 `chain-ready.cx`, 且须通过 [product-input-key.md](file://~/repos/unisacc/research/agy-h/product-input-key.md) 第 5.2 节与 `F3` 证伪对拍; 此项完成后正式消除工具/文档线对 `main` 冻结阻塞 | `agy-h` (`PIK1`/`F3`) + `agy-flow` (门 23 / `release-exec`) / `ompgpt` |
| `S9` (`L2-3`) | `freeze-cut` 事务化冻结命令 | `tools/freeze-cut.cx` (或 `tools/freeze-day.cx` 子命令), `tools/check-freeze-cut.cx`, `tests/batches.tsv`, `tests/coverage.tsv` (`--keyed`) | 红: 三源之一留 `128 != 256` 必拒绝并点名文件行号, 编号步混 `src/`/`cx/` 必 `REFUSE numbering-step-touched-product-source`, 武装 `main` 后外部文档推 `main` 被拒; 绿: 一条命令自动武装 `main` -> 合入 `v<V>` -> 产出 40 位 `F` 且门 23 (`S8` 口径) 一次 `PASS` | 否 | `2.5 - 3.5h` | 依赖 `S3` (`--launch-env` 口径) 与 `S8` (门 23 终态判据); 若新立 `tools/freeze-cut.cx` 前再次核 `git log` 无冲突, 推荐收束入 `freeze-day.cx` 子命令或按 [launch-improve-123.md](file://~/.local/share/unisacc/research/agy-flow/launch-improve-123.md) 独立成件登记 | `ccogweco` / `agy-flow` / `ompgpt` |

注:
- 上述 9 项总预计工作量约 `20.5 - 28.5` 人时; 经 3 条无冲突泳道并行推进, 在 `v0.1.3` 编号后第一周 (前 3-5 个工作日波次内) 可全部收口。
- `tests/batches.tsv` 与 `tests/coverage.tsv` 仅在各泳道提交末尾由落库者使用 `tools/commit-own.cx --keyed` 串行合入自己的行; 若两项同时就绪, 按 `S2 -> S1 -> S4 -> S7 -> S8 -> S9` 顺序轮流过门入 `main`。

---

## (3) 一周内的先后与并行关系图 (文本)

```text
[阶段 0: v0.1.2 公开与解冻前 -- 全部 9 项严禁推共享 main, 仅做私有构建/测试/回放]
  |-- agyneon:  在 $CLAUDE_JOB_DIR/tmp/rebase-impl-agyneon 编码 S1 (--rebase-rev) + W0219/C9b 回放
  |-- agy-h:    保持 S5 (short-batch-packing.patch) 私有 + 执行 PIK1 F3 跨版字节对拍准备
  |-- agy-flow: 补齐 S6 (127 行 --cpuset) 夹具 + 校准 S4 (cold-real 14.3s) P1 阈值 + 备好 S3 草稿
  |-- ompgpt:   私有复核 S1 / S4 / S3, 起草 S7 (dvqualify --out 与 W99xx 护栏)
  +-- ccogweco: 改写 S2 (check-notes-evidence.cx 自含夹具) 并在私有跑通 ratchets
  |
  v  <-- v0.1.2 Y2 公开 + release-post 解冻 build/freeze.tsv (同时分出 v0.1.3 产品车工作树)
  |
[v0.1.3 第一周落库编排 -- 三条写域隔离泳道并行, 共享表 tests/batches.tsv 用 --keyed 错峰]

  泳道 L1 (tools/prerun-drive.cx + dvqualify.cx, 严格串行)
  ================================================================================
  [S1: --rebase-rev] ---> [S5: 短批装箱] ---> [S6: lane-isolation 127行] ---> [S7: dvqualify --out + W99xx护栏]
   (agyneon / ompgpt)      (agy-h / agyneon)   (agy-flow / agy-h)              (ompgpt / agy-flow)
   2.5-3.5h                0.5-1.0h            1.5-2.0h                        1.5-2.0h

  泳道 L2 (tools/chain-ready.cx + bg-candidate.cx + release-exec.cx + freeze-cut)
  ================================================================================
  [S3: chain-ready helper/prepare/launch-env] ---> [S8: 门 23 改 PIK1 判据] ---> [S9: freeze-cut 事务切口]
   (agy-flow / ompgpt)                              (agy-h+agy-flow / ompgpt)   (ccogweco+agy-flow / ompgpt)
   2.0-3.0h                                         4.0-6.0h                    2.5-3.5h
        ^                                                |
        +--- (依赖 F3 跨版对拍就绪, 与 S1/S5 并行推进) ----+--- [里程碑: 工具/文档推 main 不再阻塞冻结列车]

  泳道 L3 (独立工具写域: notes-evidence / prerun-start, 可与 L1/L2 完全并行)
  ================================================================================
  [S2: notes-evidence 入库]     [S4: check-prerun-start + admission P1]
   (ccogweco / agyneon)          (agy-flow / ompgpt)
   1.0-1.5h                      3.0-4.0h
   |                             |
   +--> 经 commit-own --keyed 错峰登记 tests/{batches,coverage}.tsv (S2 -> S1 -> S4 -> S7 -> S8 -> S9)

[与 v0.1.3 产品车的关系]
  - 上述 S1..S9 全数不触 SOURCES_SID; 在 v0.1.3 分支进行 C3 进程修复 + M1 tape 编码期间于 main 推进。
  - 优先保证 S1、S5、S6、S3 在 v0.1.3 正式起链冻结之前合入 main, 使 v0.1.3 冻结切口即享 "--rebase-rev + 短批出 long + 核隔离 + prepare/launch-env" 提速收益。
  - S8 (门 23 改 PIK1) 与 S9 (freeze-cut) 完成后, 下一次冻结切口将正式实现工具/文档线推 main 与版本发车解耦。
```

---

## (4) 职责与负载均衡汇总 (5 人窗口)

| 窗口 | 主刀职责 (按时间顺序) | 复核职责 | 协同避让说明 |
| --- | --- | --- | --- |
| `agyneon` | `S1` (`--rebase-rev` 编码 + 回放 + 合入) | `S5` (短批装箱)、`S2` (`notes-evidence` 原作者确认) | `S1` 合完并交接 `L1` 后即转回 `v0.1.3` 产品车 `C3` 主线, 不在工具线尾部滞留 |
| `agy-h` | `S5` (短批装箱 rebase 合入)、`S8` (`PIK1` 与 `F3` 实现) | `S6` (`lane-isolation` 127 行) | `S5` 在 `S1` 后 1 小时内收口; 主精力接手 `F3` 证伪与 `PIK1` 落地 |
| `agy-flow` | `S3` (`chain-ready` 三件)、`S4` (`prerun-start` P1)、`S6` (`lane-isolation` 127 行)、`S8` (门 23/`release-exec` 对接) | `S7` (`dvqualify` 护栏) | 按 `S3 (L2)` 与 `S4 (L3)` 错开启动, 等 `S5` 腾出 `prerun-drive.cx` 写锁后即落 `S6` |
| `ompgpt` | `S7` (`dvqualify --out` 与 `W99xx` 护栏) | `S1`、`S3`、`S4`、`S8`、`S9` | 承担主要独立复核角色, 确保关键路径门工具件按双人复核规则过线 |
| `ccogweco` | `S2` (`notes-evidence` 自含夹具与登记)、`S9` (`freeze-cut` 对齐与登记) | 全局 `batches.tsv` / `coverage.tsv` 登记稽核 | 负责 `v0.1.2` 解冻首件 `S2` 落地及协调 `--keyed` 登记顺序, 与 `S8` 收尾对接 `freeze-cut` |
