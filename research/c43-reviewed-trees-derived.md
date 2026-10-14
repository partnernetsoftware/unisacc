# reviewed_trees 作为派生文件（0.0.43，如实说明）

- `compilercheck` 族的 `reviewed_trees` 是 exec/、src/、unisa/（及 include/ 等）三棵树的内容摘要，由 `tests/refresh_gatedeps.py` 从 HEAD 算出（同一规则在 `tests/reviewedtreescheck.py` 中只读复算）。
- 它现在被当作派生文件：`release/derived-map.tsv` 把 exec/*、src/*、unisa/* 映射到 `reviewed-trees` 套件；改这些树的提交必须同笔带上刷新后的 `tests/gatedeps.json`，否则派生步骤拒绝提交。
- 如实的边界（请读者知悉）：
  1. k2b 白名单为空（`tests/k2b-whitelist.tsv` 无数据行），因此 `reviewed_trees` 不参与任何作业的指纹选择；它只影响 gate-infra 中 README 夹具的选择断言。
  2. 这一刷新是派生一致性，**不是逐提交的代码审阅**：检查只保证快照与树一致，不保证树里的改动经过了审阅或测试。
  3. 本次刷新在干净 HEAD 导出上完成；干净导出中 gate-infra、script-inventory、reviewed-trees、gate-layers 均 rc0。
