# UJS 遗留 `.py`／`.c`（已迁出 `ujs/`）

**迁出原因（0.0.3）**：主人要求 `ujs/` 下无 `.c`／`.py`；host1 产品链已是 `unisacc.com`+`.cx`。  
本目录为**考古／对照**，**不是**出货必经，**不是** `.cx` 迁移完成证明。

| 原路径 | 现名 |
|---|---|
| `ujs/seed/construct/` | `construct/` |
| `ujs/seed/vm/*.c`·`sim_embed.h` | 根下同名 |
| `ujs/weights/ujs_ic_net.c` | `ujs_ic_net.c` |
| `ujs/__init__.py`·`__main__.py` | 包入口遗骸 |
| `practice/**/_selftest_run.py` | 自测遗骸 |

**日用**：`./ujs/ujs.com` · `ujs/tests/run.cx`。  
**构造续迁**：见 `ujs/plans/cx-migration.md`（真迁 `.cx` 后可再删本档）。  
仓外／m4 适配器若仍需旧 CLI，须自备 `PYTHONPATH` 指向本树并自担债；不得写回 `ujs/`。
