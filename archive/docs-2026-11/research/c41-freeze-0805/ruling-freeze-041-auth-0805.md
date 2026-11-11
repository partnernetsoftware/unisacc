# 代裁：开 0.0.41 freeze（机房主任 0805）— 缺口/Stage残表顺延 0.0.42

**墙钟**：2026-10-12 ≈08:05 SGT / Asia/Singapore  
**授源**：政委跳过收口问；机房主任代裁执行。  
**对照 tip（开冻瞬间）**：`2c909b26`；`src/version.h`=**0.0.40**；Latest=**v0.0.40**；产品叶 `e36b5962`（K6-29）；收口材料 `c41-closeout-checklist-0749.md` + `c41-closeout-ready-ruling-0749.md`。

## 裁定（一句话）

**收口可冻**——金标 `*_label` 战役（K6-18…29）已尽；**全 unisa 缺口与 gold Stage 残表书面顺延 → 0.0.42+**；本拍**开** 0.0.41 freeze：`freezecheck` → `version.h` bump **0.0.41** → 刷戳（`refresh_gatedeps`）。**禁** Draft / Latest / 签 / 公开，除非另授。

## 书面顺延（钉死；非另授不得回灌本版义务）

| 项 | 处置 |
|---|---|
| 全 unisa 无专用 seed C 模块（assemble / ckernel / construct / control/* / docgen / driver / emit_x86 / exec_target / front/* / intnet / ir / lower / opt / oracle / prune / tsvgold / uns2 / `__main__` 等） | **顺延 0.0.42+** |
| gold Stage 残表：tyinfo / pfconv / enc / regmap / isel / abi / combo + build() 机械/元数据 | **顺延 0.0.42+**（非本战役已钉队列；另授窄叶才开工） |
| 删 Python | **仍禁**（计划明文；删档另授且须 C 参考逐字节绿之后） |
| K6-30+ / 新实现叶 | **本拍不授** |
| H1 INHERIT/新建 | **仍 OPEN** —— 挡后续 `build_candidate`/preflight/queue；**不**挡本拍 version+刷戳。须另裁写入 plans |

## freeze 本拍授执行（仅此三步 + 收口页/paste）

1. `python3 tests/freezecheck.py`（版本前提基线；rc0 期望：「v0.0.40 already released / not in freeze window」）
2. 单独提交 `src/version.h` → `0.0.41`（经 `commitgate.sh --suite exec-unitparse`）
3. `python3 tests/refresh_gatedeps.py` → 单独 chore 提交 `tests/gatedeps.json`
4. 版本后 / 刷戳后再各跑 freezecheck（期望 closure commits without fix: 0）
5. 落 research 收口页 + paste 三窗；推送快进（无 force）

## 明确不授（硬禁）

| 项 | |
|---|---|
| Draft / `gh release create` / draftfetch | **禁** |
| 动 Latest v0.0.40 / publish / `--draft=false` | **禁** |
| 签（Apple / Win / company approve） | **禁** |
| seal_candidate / rc_tag / 全量 queue / build_candidate 实跑 | **本拍不授**（另授） |
| 删 py / 改产品验收 / 并刀 CX 升产品门 | **禁** |
| 预填产品 PASS / 冒称 K6 全树闭包已齐 | **禁** |

## 对 0749 门闩对照

| # | 0749 前提 | 本拍 |
|---|---|---|
| F1 收口可冻 | 缺冻令 | **已授**（本文件） |
| F2 H1 | 缺 | **仍缺**（卡点；不挡 version+刷戳） |
| F3 缺口处置 | 缺 | **书面顺延 0.0.42** |
| F4 Stage 残表 | 缺 | **书面顺延 0.0.42** |
| F5 删 py | 禁 | **保持禁** |
| F6 version bump | 未授 | **本拍授** |
| F7 freezecheck→刷戳 | 未授 | **本拍授至刷戳**；queue/seal 另授 |
| F8 Draft/签/Latest | 禁 | **保持禁** |

## 产出路径

- 本裁定；`research/c41-closeout-freeze-0805.md`（收口页）
- `research/c41-freeze-receipt.md`（事实回执）
- `research/c41-freeze-0805/paste-{cc,cdx,cdx2}-0805.txt`
- 既有 0749 清单/裁定一并入仓（材料 only 轨迹）

*机房主任代裁 2026-10-12 ≈08:05 SGT*
