# c43 seedmemory 重钥（2）：opt-gen.tsv 变更（2026-10-14）

来源：补交 `exec/facts/opt-gen.tsv`（1d1bf3a1 派生补交）。读集中 **e4 与 o1** 读取该文件（`tests/seedmemory-reads.tsv`，gen 族；`tests/seedmemory.py` 的 `C_RSS` 对应这两路线）。按规则：读取的输入变了，须重测该路线的 RSS，并更新键。

## 键

- gen 族参考键：HEAD（697a0b05）上为 `3ee14c19342648a7`（与 pinned 一致，即 c4d45200 之后的值）。补交后为 **`a0f1253495dbefa6`**，在补交后的干净导出上计算。
- parse2 族键 `1292f970a84806f1` 不变（parse2 路线不读 opt-gen.tsv）。

## RSS 重测（C 生产路线，`seed/gen.c` 以 `cc -std=c99 -O2 -Iseed` 编译，补交后的导出）

| 路线 | 参数 | 重测 RSS | 既有 C_RSS（MiB） | 结果 |
|---|---|---:|---:|---|
| o1 | `opt` | 15096 KiB → 14 MiB | 14 | 不变 |
| e4 | `opt --o2` | 41228 KiB → 40 MiB | 40 | 不变 |

测量：每个子进程用 `os.wait4` 取 `ru_maxrss`（进程最大驻留集）。宿主没有 `/usr/bin/time`，因此没有使用 `time -v`；数值与既有记录逐一相同，可作为一致性证据。

结论：两路线的 RSS 未变，只更新键。`C_RSS` 不改动。

## 其余

- `tests/seedmemory.py`：`REFERENCE_KEYS['gen']` 改为 `a0f1253495dbefa6`。
- `tests/seedmemorycheck.py` 的 `test_pinned_keys_match_read_files` 以真树比对键，本笔之后应通过。
