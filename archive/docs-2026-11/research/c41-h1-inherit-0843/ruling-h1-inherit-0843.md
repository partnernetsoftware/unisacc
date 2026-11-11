# 代裁：H1=INHERIT + 开 candidate/queue 备 Draft 材料（机房主任 0843）

**墙钟**：2026-10-12 ≈08:43 SGT / Asia/Singapore  
**授源**：政委跳过收口问；机房主任代裁。  
**对照 tip（落裁瞬间）**：`dce7d5ce`；`src/version.h`=**0.0.41**；Latest=**v0.0.40**；0805 freeze 已落（`c41-freeze-0805/ruling-freeze-041-auth-0805.md`）；H1 此前 OPEN（挡 build_candidate）。

## 裁定（一句话）

**H1=INHERIT**——本版不新建 H1 行、不声明 EMPTY；显式沿用 `archive/plans/v0.0.38.md` 的 H1 行（跨版进行）。**残表/未端口模块书面顺延 → 0.0.42+**；**归档≠删除**（顺延/归档不删 py、不删 Stage 表、不删 seed 缺口模块）。**允许**开 candidate/queue **准备 Draft 材料**。**仍禁** Latest / 签 / 公开，除非另授。

## H1 机读声明（写入 archive/plans/v0.0.41.md）

```
H1-INHERIT: archive/plans/v0.0.38.md sha256=d1f05f7ddbec20556334630cc0b58bd16a05cad729b0d2661221158e60e35bc9 host=linux-x86_64-8core-xeon-cloud
```

- 目标 sha 与 0.0.40 声明一致；落裁前本机复算全长吻合。
- 宿主形态：云机 Linux x86_64 8 核 Xeon 基线（与本机 `uname -sm`=`Linux x86_64`、`nproc`=8 一致）。
- **禁止**用 `--h1` 指旧计划绕过本版结算。

## 书面顺延钉死（非另授不得回灌本版义务）

| 项 | 处置 |
|---|---|
| gold Stage 残表：tyinfo / pfconv / enc / regmap / isel / abi / combo + build() 机械/元数据 | **顺延 0.0.42+** |
| 全 unisa 无专用 seed C 模块（assemble / ckernel / construct / control/* / docgen / driver / emit_x86 / exec_target / front/* / intnet / ir / lower / opt / oracle / prune / tsvgold / uns2 / `__main__` 等） | **顺延 0.0.42+** |
| 删 Python / 删 Stage 表 / 删缺口模块 | **仍禁**；**归档≠删除** |
| K6-30+ / 新实现叶 / 自动开 0.0.42 K7 | **本拍不授**（顺延≠自动开工） |

## 本拍授执行

1. 落本裁定 + paste 三窗；更新 `archive/plans/v0.0.41.md`（H1-INHERIT + 卡点清 H1 OPEN）与收口页 H1 态。
2. `python3 release/tools/h1source.py .` → 期望 `INHERIT archive/plans/v0.0.38.md (host linux-x86_64-8core-xeon-cloud)`。
3. **允许** `build_ref` / `build_candidate` / comboot / **queue** 以准备 Draft 材料（不预填产品 PASS）。
4. seal / rc_tag 可作为 Draft 材料链一步；**创建 Draft release / Latest / 签 / 公开仍另授**。

## 明确不授（硬禁）

| 项 | |
|---|---|
| 动 Latest v0.0.40 / `publish.sh` / `--draft=false` | **禁** |
| 签（Apple / Win / company approve） | **禁** |
| 未另授的 `gh release create` / 公开 Draft | **禁**（本拍只备材料） |
| 删 py / 改产品验收 / 并刀 CX 升产品门 | **禁** |
| 预填产品 PASS / 冒称 K6 全树闭包已齐 | **禁** |
| 冒称 H1 新建或 EMPTY | **禁**（本裁=INHERIT） |

## 对 0805 / 0749 门闩对照

| # | 前提 | 本拍 |
|---|---|---|
| F2 H1 | 0805 仍 OPEN | **已裁 INHERIT**（本文件） |
| F3/F4 缺口/残表 | 0805 已顺延 0.0.42 | **维持**；补钉 **归档≠删除** |
| F8 Latest/签/公开 | 禁 | **保持禁** |
| candidate/queue | 0805 另授 | **本拍授**（备 Draft 材料） |

## 产出路径

- 本裁定；`research/c41-h1-inherit-0843/paste-{cc,cdx,cdx2}-0843.txt`
- `archive/plans/v0.0.41.md`（H1-INHERIT 行）
- `research/c41-closeout-freeze-0805.md`（H1 态刷新）
- `research/c41-h1-inherit-0843/receipt.md`（事实回执）

*机房主任代裁 2026-10-12 ≈08:43 SGT*
