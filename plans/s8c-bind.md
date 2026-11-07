# s8c 材料接线（一条命令）

**缺口**：`init` 只把 MAP 写入 `session/materials.tsv`。`pending` 的 `materials_registered` 看 session **MATERIAL 块**（`all-waves material`）。s8b 未跑后者，约 47 批图有路径仍 no。

**为何漏**：drive 只 `init` 后 `wave`；s7 曾手工 `material` 14 次。

**自动点**：init 成功后立刻对 MAP 每个名字 `material`，再 wave。

**一条命令**（`tools/s8c-bind.sh`，git log 无占用）：

```
sh tools/s8c-bind.sh ROOT SESSION ROOT/unisacc.com MAP
```

独立 scratch `/tmp/grk-s8c-dry/out/s8c14` 全图 14 名：

| | heavy registered=yes | heavy =no |
|---|---|---|
| init 后、material 前 | 0 | 162 |
| 14 名 material 后 | 84 | 78 |

余 78 为 map 未列名（ref/ri-cli/sha 绑定等），本命令不造伪材料。PREPARED≠PASS。s8b 终态后同命令起 s8c。

