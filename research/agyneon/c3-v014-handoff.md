# C3 S3 推迟至 `v0.1.4` 交接单

- 维护者: `agyneon` | 裁定来源: `1:cc` (`v0.1.3` C3 收口为 `S1a-e` + `S1b-2/3` + `cdx` 表夹具 v2)

## 1. 现有材料索引

- 规格与转移表第五版: `~/.local/share/unisacc/research/agyneon/c3-s3-spec.md` (37 格)
- 第二轮产品补丁 (未入库): `~/.local/share/unisacc/research/agyneon/c3-s3.patch` (201 行, `ext/sysx/reaper.cx` + `ext/sysx/run.cx` + `cx/sys/proc.cx`)
- 第二轮夹具补丁 (未入库): `~/.local/share/unisacc/research/agyneon/c3-s3-fixture.patch` (301 行, `tools/proc-s3-transitions.tsv` + `tools/check-proc-s3.cx`, 15/15 PASS)

## 2. `gatekeeper` 与 `cdx` 二轮阻断清单

1. **B1 (`cx/sys/proc.cx`)**: `sy_proc_wait` 有界轮询不得施于 `deadline_ms=0`; 且 `syp_reap` 内层吞系统 `EINTR` 重试绕开外层截止, 需将截止时间传入 `syp_reap`。
2. **B2 (`cx/sys/proc.cx`)**: `sy_proc_end` `SIGTERM` 后 `grace_ms` 循环必须每轮轮询 `sy_proc_try_wait` 提前退出, 不得白睡满 `grace_ms`。
3. **B3 / `cdx` 表夹具覆盖**: 规格 37 格按 `event/outcome` 对账, 第二轮 15 行仅覆盖 12 个键, 缺 25 格具名 `UNMEASURED` 与只降基线; 需补齐 `fatal_poll`、`read_err`、`OOM`、`st` 置零哨兵、阻塞 `reap` 恢复等变异必红。
4. **B5 (`build/layer-api/ext.sysx.tsv`)**: `rp_kill_tree_keep_root` 公开接口扩展需随 `--derive` 同步接口表。
5. **`reaper.cx:84-94` (`kt_stat`)**: 当前 `kt_stat` 把 `ENOENT` 及其他错误折为 `-1`, 导致 `keep_root` 新增的 `krc == -ENOENT` 分支不可达; 需先让 `kt_stat` 精确返回 `-errno`。

## 3. `v0.1.4` 起步建议

**不要**在旧补丁上逐点打补丁: 直接基于 `v0.1.4` 新起点, 先修 `reaper.cx` `kt_stat` 精确错误码与 `syp_reap` 外层截止传递, 再按 `c3-s3-spec.md` v5 全部 37 格建表驱动夹具 (可观测格 + 具名 `UNMEASURED` 只降基线), 确保 `gatekeeper` 与 `cdx` 全变异必红后一次性落地。
