# UJS → `.cx` 迁移债

> 0.0.3：**`ujs/` 内已无 `.c`／`.py`**（迁至 [`archive/ujs-legacy-py-c/`](../../archive/ujs-legacy-py-c/)）。  
> 0.0.4：host1 **wasm 验面**（产品侧真 `.cx`）；archive construct 续迁仍属后波。

## 现状

| 项 | 状态 |
|---|---|
| `ujs/product/` | ✓ 纯 `.cx`（含 `--verify`／`verify-host.cx`） |
| `ujs/` 树 `.py`／`.c` | ✓ **0** |
| 构造臂／VM／旧包入口 | 在 `archive/ujs-legacy-py-c/` |
| archive 真迁 `.cx`（acc／emit／…） | **未完成**（0.0.5+） |

## 波次（更新）

| 波 | 版 | 定义 |
|---|---|---|
| W0 | ✓ 0.0.3 | 清空 `ujs/` 内 `.py`／`.c` |
| W0b | ✓ 0.0.4 | host1 `\0asm` 验面；套件＋语料在 `ujs/tests/` |
| W1 | 0.0.5+ | archive 内按族真迁 `.cx`，迁完可删对应遗留 |
| W2 | 后置 | path-A VM／IC 表生成器 |

规则不变：`-run` 真迁；转调 Python 不算完成。
