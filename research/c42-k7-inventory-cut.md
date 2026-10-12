# c42 K7-0 inventory cut (draft; 政委授 1424)

**状态**：盘点草稿；**归档 ≠ 删除**；不授默删 py；实现切口另批。
**tip**：`458fd724`; `src/version.h` = **0.0.42**; Latest 仍 v0.0.41 直至本刀切。

## 范围

| 树 | 角色 |
|---|---|
| `ujs/` | JS/WASM/ seed-construct 旁路；K7 清点对象 |
| `archive/` | 已退役材料落点（含 plans/prd/research 历史） |
| 活动链 py/sh | `exec/c`、`tests`、`release/tools` 仍驱动构建/queue |

## ujs/ 文件数：**201**（节选前 40）

```
ujs/.gitignore
ujs/ARCHITECTURE.md
ujs/README.md
ujs/__init__.py
ujs/__main__.py
ujs/archive/DOCS-pre-v1.3.md
ujs/archive/FUTURE-pre-v1.2.md
ujs/archive/FUTURE-stub-pre-v1.3.md
ujs/archive/PLATFORM-pre-v1.3.md
ujs/archive/README.md
ujs/archive/TOOLS-pre-v1.3.md
ujs/archive/prd-v1.0.md
ujs/compile.mjs
ujs/iterate/README.md
ujs/iterate/compile.mjs
ujs/iterate/compiler.ujs
ujs/iterate/compiler.wasm
ujs/iterate/compiler_core.meta.json
ujs/iterate/compiler_core.wasm
ujs/iterate/compiler_rt_stub.json
ujs/iterate/compiler_rt_stub.wasm
ujs/iterate/rebuild-main.mjs
ujs/iterate/run-compiler-core.mjs
ujs/native/README.md
ujs/native/compiler_min.c
ujs/native/ujs_vm.c
ujs/native/uxe_asteroid.c
ujs/package.json
ujs/practice/README.md
ujs/practice/cloudflare/uxe-live/.gitignore
ujs/practice/cloudflare/uxe-live/package-lock.json
ujs/practice/cloudflare/uxe-live/package.json
ujs/practice/cloudflare/uxe-live/src/index.js
ujs/practice/cloudflare/uxe-live/wrangler.jsonc
ujs/practice/core/BUILD.json
ujs/practice/core/README.md
ujs/practice/core/compiler.gen.js
ujs/practice/core/compiler.js
ujs/practice/core/compiler.ujs
ujs/practice/core/compiler.wasm
… +161 more
```

## archive/ 顶层（26）

`docs`, `exec`, `iterate`, `paper-a-history.md`, `plans`, `prd-findings-20260929.md`, `prd-history-20260929.md`, `prd-history-20260930.md`, `prd-history-20260930b.md`, `prd-history-20261009.md`, `prd-history.md`, `prd-notes-20261002.md`, `prd-r17-notes-20261001.md`, `prd-r9-r10-receipts-20260929.md`, `prd-release-and-development-history-20261001.md`, `prd.map-20260930.md`, `prd.tree-20260930.md`, `prd.v1.md`, `prd.v2.1.md`, `prd.v2.2.md`, `r9-integration-history-20260928.md`, `release`, `research`, `s17-migration-log-20260928.md`, `spec-training-20260930.md`, `tests`

## 活动链入口计数（非 archive）

| 路径 | 数 |
|---|---|
| `exec/c/*.py` | 34 |
| `exec/c/*.sh` | 18 |
| `tests/*.py` | 187 |
| `tests/*.sh` | 159 |
| `release/tools/*.py` | 14 |
| `release/tools/*.sh` | 19 |

## 具名表：活动链仍依赖 py 的关键入口（本刀只标；不迁）

| 入口 | 角色 | K7 态 |
|---|---|---|
| `exec/c/comboot.py` | comboot 记录；C 等价 `comboot.sh` 已有 | 保留对照 |
| `exec/c/compilerpack.py` | pack-models；seed/compilerpack.c 并行 | 保留对照 |
| `tests/gatequeue.py` | release queue 调度 | 待 C/无 py 全链 |
| `release/tools/queuestart.py` | fresh/resume + env-pin | 待等价 |
| `tests/env_pin.py` | env-pin 合同 | 待等价 |
| `ujs/seed/construct/**/*.py` | ujs 构造旁路 | 盘点→归档路径另授 |

## 1 条可批窄草案（K7-0a）

> 产出本文件之具名表 + `ujs/` 文件清单；**不**迁移、**不**删除、**不**改验收。下一切口须另批文件域/预算/首红停。

*草稿 2026-10-12 ≈14:30 SGT*
