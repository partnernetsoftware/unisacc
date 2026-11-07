# s8c 材料接线（一条命令）

**缺口**：`init` 只把 MAP 写入 `session/materials.tsv`。`pending` 的 `materials_registered` 看 session **MATERIAL 块**（`all-waves material`）。s8b 未跑后者，约 47 批图有路径仍 no。

**为何漏**：drive 只 `init` 后 `wave`；s7 曾手工 `material` 14 次。

**自动点**：init 成功后立刻对 MAP 每个名字 `material`，再 wave。

**一条命令**（独立空跑已过）：

```
sh out/s8c-bind.sh ROOT SESSION ROOT/unisacc.com ROOT/out/s8c.map
```

`s8c-bind.sh`：`init` 后 `awk` MAP 第一列，逐名 `tools/bound 55 … -- material ROOT SESSION NAME`。

空跑 `/tmp/grk-s8c-dry`：`INIT materials_mapped=1`；`MATERIAL hostcc … PREPARED-identity-only`；`BIND_DONE`。全 14 名同命令换 MAP（s8b 的 `out/s8b.map`）。PREPARED≠PASS。不挡 0.0.46 发布。
