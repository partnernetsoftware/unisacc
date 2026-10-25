# UJS → `.cx` 迁移债

> 0.0.3：`ujs/` 内已无 `.c`／`.py`（[`archive/ujs-legacy-py-c/`](../../archive/ujs-legacy-py-c/)）。  
> 0.0.4：host1 验面；套件＋语料在 `ujs/tests/`。  
> **余下** = archive 能力真写成 `.cx`，或诚实维持考古；**不是**搬回 `ujs/`。

## 现状

| 项 | 状态 |
|---|---|
| `ujs/product/` + `ujs/tests/` | ✓ 纯 `.cx`／夹具 |
| `ujs/` 树 `.py`／`.c` | ✓ **0** |
| 构造臂／VM／旧包入口 | `archive/ujs-legacy-py-c/` |
| host1 跑面 | **0.0.5 主刀**（产品） |
| archive 真迁（acc／emit／…） | **后置**（W1；`method_construct`） |

## 波次

| 波 | 版 | 定义 |
|---|---|---|
| W0 | ✓ 0.0.3 | 清空 `ujs/` 内 `.py`／`.c` |
| W0b | ✓ 0.0.4 | 验面；`ujs/tests/` 自洽 |
| W0c | **0.0.5** | host1 **跑面**（非 construct 迁） |
| W1 | 0.0.6+ | archive 按族真迁 `.cx`；迁完可删对应遗留 |
| W2 | 后置 | path-A VM／IC 表生成器 |

规则：`-run` 真迁；转调 Python 不算完成。
