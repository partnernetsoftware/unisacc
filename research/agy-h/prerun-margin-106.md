# 0.0.106 预跑真实计时余量清单 (为 0.1.x 余量包提供实测基线)

日期: 2026-11-25
执行窗口: agy-h (只读统计)
数据源: `~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/strict-W0000..W0237.tsv`、补跑 `~/.cache/unisacc/prerun/v0.0.106-r40-rerun/` (`strict-W9997..W9999`)、合并输出 `~/.cache/unisacc/prerun/v0.0.106-merge-out/strict-final.tsv`、[prerun-margin-106.tsv](prerun-margin-106.tsv)

## 1. 核心统计摘要

(1) 总量与分布:
- 主预跑 (`v0.0.106-2e8a19a342`) 共 `40` 批单批耗时 `> 27500ms` (全部 `PASS`), 加上补跑 (`v0.0.106-r40-rerun`) 新进入的 `4` 批 (`devsvc-git-c-operations` `55555ms FAIL`、`check-gen-all-drift` `44275ms PASS`、`premerge-selfcheck` `41623ms PASS`、`check-gen-all-part2` `28996ms PASS`), 合计 `44` 个去重批次 `> 27500ms`。
- 按档位: `commit` 层 `30` 批、`daily` 层 `14` 批 (`0` 个仅属 `release` 层)。

(2) 超过 `50000ms` 贴 `55` 秒限的高危 5 批 (按 ms 降序):
- `notcovered`: `55865ms` (`strict-W0086.tsv`, `PASS`, 无 WP 补丁)
- `module-units`: `55557ms` (`strict-W0085.tsv`, `PASS`, `WP5` `r35-module-units.patch` 已合入 `107` 待补冷测)
- `devsvc-git-c-operations`: `55555ms` (`v0.0.106-r40-rerun` `W9998-r40-bt2` 与 `W9998-cold-attempt` 均 `FAIL rc=0 transport=-1`, 实际已超 `55s` 限; 另有 `run1/run2` 实测 `16748ms` / `6518ms PASS`)
- `check-split-parents`: `54947ms` (`strict-W0080.tsv`, `PASS`, 前两轮 `58.0s rc=124` 停机; 较 `0.0.105` 的 `38490ms` 慢 `16.5s`, 已列 `0.1.x` 余量包 `C4`)
- `chain-ready-freeze`: `50288ms` (`strict-W0210.tsv`, `PASS`, 本次新晋贴限, 暂无减时拆批/材料计划)

(3) 3 个重点冷失败行 (`W0087`、`W0150`、`W0189`) 对照:
- `W0087` `check-gen-all-drift`: 主跑冷失败 `FAIL rc=1 ms=21104` (`load1=3.85`, 迷你仓受 `tools/affected.cx` canary 影响), 补跑 `W9997-r40-bt1` 转 `PASS ms=44275` (`>27.5s`, 已有 `gen-all-drift-fixture-tests.patch` 修漂移但无减时拆批计划)。
- `W0150` `release-exec-c`: 主跑冷失败 `FAIL rc=1 ms=22216` (`load1=5.15`, `assertions=15`), 补跑 `W9999-w0150-run1/2/3` 连续 `PASS`: `23457ms / 23205ms / 23202ms` (均 `<= 27500ms`, 已有 `r39c-release-exec-sha.patch`)。
- `W0189` `devsvc-git-c-operations`: 主跑冷失败 `FAIL rc=1 ms=6530` (`load1=4.89`), 补跑 `W9998-r40-bt2` 与 `W9998-cold-attempt` 呈现 `55555ms FAIL (transport=-1)` 严重超时、另两次 `devsvc-git-c-run1/2` 为 `16748ms / 6518ms PASS`, 需在 `WP1` 按现行 `c2` 批适配器补齐 `{devsvc-core-tool}`/`{commit-own-tool}`。

## 2. 对 `0.1.x` 余量包队列的直接含义

- 已有计划/补丁覆盖 (`7` 批):
  - `WP5`: `module-units` (`55557ms`, `107` 已合待复测)
  - `WP1` (须适配 `d1a1ca1924` 新拆片): `devsvc-git-c-operations` (`55555ms` 超时 / `W0189` `6530ms` 失败)、`devsvc-git-c-background` (`49930ms`)
  - `WP3`: `premerge-selfcheck` (`41623ms`, 已列入 `split-parents`, 待抽材料与再拆)
  - `WP4`: `layer-api-real-c` (`41554ms`) 与 `layer-api-real-b` (`39965ms`) 在 `0.0.106` 实测双破 `27.5s`, 满足 `wp2-wp4-plan.md` 触发条件, 应直接启用 `{layer-api-tool}` / `{iface-hash-tool}` 材料相
  - `0.1.x C4`: `check-split-parents` (`54947ms`)
- 暂无拆批/材料化计划的 `37` 批里, 优先排期前 `6` 名高险项 (`>39s` 且占 `commit` 核心门):
  (1) `notcovered` (`55865ms`) — 首号贴限无补丁 `commit` 批;
  (2) `chain-ready-freeze` (`50288ms`) — 新晋 `>50s` 贴限 `commit` 批;
  (3) `optimization-closure-0-4` (`49980ms`, `daily`) 与 `optimization-corpus-6/4/1/2` (`31.0-35.7s`);
  (4) `release-flow` (`47246ms`) 与 `release-exec-r` (`40009ms`);
  (5) `stream-pipe-parts` (`45518ms`)、`check-run-split` (`42512ms`)、`affected-gitnode` (`42060ms`)、`slow-split` (`40093ms`);
  (6) `check-gen-all-` 系列 (`drift` `44275ms`、`hang` `39341ms`、`pass` `37074ms`、`check` `35116ms`, 外加 `part2` `28996ms`), 因 `precommit-run-tier-design` 已将其标为重批, 最适合做 `{gen-all-check-tool}` 预编材料或合并冷编。
