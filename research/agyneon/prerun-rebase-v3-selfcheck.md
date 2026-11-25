# `prerun-drive --rebase-rev` v3 自查表 (`main` `c500c0db0`)

- 维护者: `agyneon` | 基线: `c500c0db0` (`main`) | 补丁: `~/.local/share/unisacc/research/agyneon/prerun-rebase-rev-v3.patch`

## 1. 相对 v2 的关键修复

1. **规范 40 位 SHA**: 入口 `git rev-parse --verify <new_rev>^{commit}` 校验 40 位 SHA, 绝不将 `"HEAD"` 写入 `frozen-source.rev`。
2. **禁止截断 diff**: `pd_rebase_run_git` 检测 `r.truncated`, 截断即返回 `-EOVERFLOW` 并拒绝。
3. **`argv` 直调 SID**: `PRERUN_REBASE_SID_BIN` 经 `argv` 直调 (去掉 `sh -c`)。
4. **采样真 `sy_unlink` + 错误检查**: 检查 `sy_dir_next < 0` 与 `sy_unlink != 0` 报错中止; 采样按 `(i + 1) % 10 == 0` 真正 `sy_unlink` 对应 `done.W*` (修掉了 `retained` 停滞 bug); 支持 `--plan`/`--dry-run` 只读计划。
5. **实物核验 + `cxtrunc` 清零**: 夹具核验 `frozen-source.rev` 为 40 位 SHA 且磁盘剩余 `done.W*` 数量严格等于 `retained`; `memcpy` 改逐字节拷贝, `RATCHETS` 19/0。

## 2. 实测原行

- 表驱动夹具 (`tools/check-prerun-rebase.cx` + `tools/prerun-rebase-cases.tsv`, 冷跑 `12.932s < 25s`):
  ```text
  PASS	non_ancestor_refused
  PASS	disallowed_path_refused
  PASS	sid_mismatch_refused
  PASS	single_batch_changed_reruns
  PASS	sampling_10pct_floor
  PASS	replay_c7_to_c10_disallowed_posix
  PASS	replay_c9b_to_c10_scale_20
  CHECK_PRERUN_REBASE	rows=7	asserted=7	failed=0
  ```
- Lint (`tools/cx-lint.cx -- tools/check-prerun-rebase.cx`): `CX_LINT files=1 findings=0 elapsed_ms=6`
- 棘轮 (`tools/ratchets.cx`): `RATCHETS ok=19 fail=0 elapsed_ms=26493`
