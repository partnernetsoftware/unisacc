# UJS → `.cx` 迁移债

> 0.0.3：**`ujs/` 内已无 `.c`／`.py`**（迁至 [`archive/ujs-legacy-py-c/`](../../archive/ujs-legacy-py-c/)）。  
> 余下工作 = **把 archive 里的能力真写成 `.cx`**，不是搬回 `ujs/`。

## 现状

| 项 | 状态 |
|---|---|
| `ujs/product/` | ✓ 纯 `.cx` |
| `ujs/` 树 `.py`／`.c` | ✓ **0** |
| 构造臂／VM／旧包入口 | 在 `archive/ujs-legacy-py-c/` |
| 真迁 `.cx`（acc／emit／…） | **未完成**（0.0.4+） |

## 波次（更新）

| 波 | 版 | 定义 |
|---|---|---|
| W0 | ✓ 0.0.3 | 清空 `ujs/` 内 `.py`／`.c` |
| W1 | 0.0.4+ | archive 内按族真迁 `.cx`，迁完可删对应遗留 |
| W2 | 后置 | path-A VM／IC 表生成器 |

规则不变：`-run` 真迁；转调 Python 不算完成。
