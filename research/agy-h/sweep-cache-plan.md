# 0.1.0 公开后缓存与 `/tmp` 清扫计划 (只出清单待批, 不直接删除)

编写窗口: `agy-h` | 基准日期: 2026-11-25 13:00 | 授权原则: 先核无占用/出清单/获批后再删; `symindex`、`tlib`、`e3cache` 重建昂贵不删; `prerun` 两份留至 `0.1.1` 公开。

---

## 1. `$HOME/.cache/unisacc/` 清扫分类总表

实测工具:
- `sh ./unisacc.com tools/bg-candidate.cx -- prune --keep 2 --dry-run`:
  `PRUNE KEEP a11d28fc0a61939ee71345d732a7dcf205b261f1 state=DONE reason=newest`
  `PRUNE KEEP 63354b4a2cb707ecc923a89f0df6030149fa697f state=DONE reason=newest`
  `PRUNE SKIP 28b6a518fe9fac6b117f31adeb25e1c8ab51701f state=DONE reason=no-trash-dir`
  `BG_PRUNE candidates=3 moved=0 skipped=1 deleted=0 trash=-`

| 目录 (`$HOME/.cache/unisacc/...`) | 实测占用 | 处置分类 | 说明与依据 |
|---|---:|---|---|
| `chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1` | 1.1G | 保留 | `0.1.0` 公开候选 (`bg-candidate prune --keep 2` 标 `KEEP`) |
| `chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f` | 1.1G | 保留 | `0.0.107` 公开候选 (`bg-candidate prune --keep 2` 标 `KEEP`) |
| `chains/bg-b1/28b6a518fe9fac6b117f31adeb25e1c8ab51701f` | 1.1G | 建议迁出/清理 | 旧候选 `DONE` 项, 可加 `--trash` 迁往恢复目录确认无用后清 |
| `chains/snap/a11d28fc0a-c7790079` | 1.1G | 保留 | `0.1.0` 对应快照 |
| `chains/snap/{3eca4049b4-57df4fd4,5a36b8c283-a5a25ed3,5a44072f58-c7790079,a24762a6ae-c7790079,b427b3316c-a5a25ed3}` (及空 `.partial.lock`) | ~1.3G (5 份各 ~249-250M) | 建议清旧留新 | 非 `0.1.0` 旧快照及 0 字节残锁, 待主调度核准后清 (共省 ~1.3G) |
| `prerun/v0.0.107-63354b4a2c` 与 `prerun/v0.1.0-a11d28fc0a` | 1.5G + 1.5G | 保留 | 裁定保留两份至 `0.1.1` 公开 |
| `symindex` | 13G | 保留 (禁删) | 重建昂贵缓存 |
| `tlib` | 7.3G | 保留 (禁删) | 重建昂贵缓存 |
| `e3cache` | 4.3G | 保留 (禁删) | 重建昂贵缓存 |
| `devsvc` (`pinned` 555M / `native` 72M / `hook-warn.tsv` 6.1M / 会话作业槽) | 646M | 保留运行态 / 可择机清过期 job 槽 | 常驻服务运行态 (`pinned`/`native` 不删); 旧 `jobs` 空槽/历史作业日志可按 `ttl` 回收 |
| `prep-cache` | 171M | 保留 | 材料准备缓存 (`old-product` 另占 8.6M, `release-v010-state-a11d28fc0a` 占 105M 建议随 `0.1.0` 收尾留存至 `0.1.1` 后) |
| `check-devsvc-core` | 579M | 建议清理 | 旧单批自检残留目录 (先 `fuser` 确认无占用) |
| `prerun-start-check-` 前缀 (14 个 0 字节空目录) | 0 | 可随清 | 先确认无活跃 `check-prerun-start` 进程后再清 |

注: `chain-disk-ledger.cx` 用于单链写 `<chain base>/KIND/ID/disk.tsv` 峰值行, 读阶段本身允许已清缓存缺省回退。

---

## 2. `/tmp` 超 3 小时私有残留目录清单 (已排除 `cdx` 三目录, 待批再删)

筛选口径: `-maxdepth 1 -mmin +180`, 前缀匹配 `unisacc-qualify-`, `unisacc-affected-`, `cdx-`, `vshape-`, `g1wt`; 显式排除 `cdx` 指定的:
- 保留: `/tmp/cdx-v010-reseal.pwAFG9` (复验页引用)
- 避重(交 `cdx` 自理): `/tmp/cdx-v010-freeze.Wfi8dU`, `/tmp/cdx-releaseseq-domain.po9TKP`

实测汇总 (已跑 `fuser`, 输出 `FUSER_FREE` 无活跃进程占用):
- 命中总目录数: `47` 个
- 总体积: `301M`
- 主要构成:
  1. `/tmp/unisacc-qualify-b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f-a11d28fc0a61939ee71345d732a7dcf205b261f1`: `298M` (`0.1.0` 候选 `a11d28fc0a` 旧 qualify 工作区; `0.1.0` 已公开且 evidence 已归档)
  2. `/tmp/unisacc-affected-` 系列 `46` 个残留临时目录 (按体积倒序首 9 项为 `876975` 524K、`881717` 400K、`992714` 284K、`973604` 248K、`964810` 248K、`886930` 204K、`883590` 72K、`972871` 40K、`964805` 40K 等, 合计约 `3M`)

以上全部仅作只读核查记录, 等 `1:cc` 批示确认后执行移动/清理并逐项报实际删除前后的 `df`/`du` 原行。
