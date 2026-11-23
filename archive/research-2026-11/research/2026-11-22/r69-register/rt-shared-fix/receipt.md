# rt-shared-gen 大块分配状态补登记

补丁 `rt-shared-registration.patch` 只改 `exec/facts/rt-shared.tsv`，属于 A 类（`build/lane-inputs.tsv:775`）。按派工留作私有补丁，等 0.0.90 起链后由集成者安排；未提交或应用到共享检出。

将 `_unisa_lbin`、`_unisa_lheld`、`_unisa_lev` 补进现有 `exempt_symbols`。它们与同表已有的 `_unisa_freel` 一样是分配器的单元级缓存状态。这笔保留实际持有量和轮转语义，64 MiB 限额仍作用于每份空闲链。全程序共享限额属于另一个行为改动，本次未做运行态多目标验证。

- `before.log` 退出 1，`RT_SHARED_GEN SAME groups=2 bytes=3191 unlisted=9`。
- `after.log` 的 `--write` 和 `--check` 均退出 0，两行都为 `RT_SHARED_GEN SAME groups=2 bytes=3191 unlisted=0`。生成模块 `cx/compiler/rtsharedname.cx` 与基线逐字节相同。
- 分别撤去三个登记，各退出 1，且只报被撤符号的三处命中。原始行见 `mutations/out/{lbin,lheld,lev}.out`。
- `git apply --check` 和私有补丁 diff 检查通过。基线见 `base.txt`，补丁摘要见 `SHA256SUMS`。
