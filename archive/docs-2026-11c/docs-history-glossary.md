# docs/glossary.md 迁出的历史记录

来源文档：docs/glossary.md。迁移日期：2026-11-13。
说明：这是历史记录，不是当前指引。以下各条按原样保留，括号内为原 file:line。

## 2026-10-23 盘点的说明段 (glossary.md:8)

> 下文词频、Python 路径与 segment/训练术语是 2026-10-23 的历史盘点，不表示默认产品存在训练过程；经典 C/Python 参考现已归档。下文 `unisa/` 历史路径现位于 `archive/reference-py/unisa/`，例如 `gold.py` 的现址是 `archive/reference-py/unisa/gold.py`，不是活工具入口。

## 历史 .cx 错用记录 (glossary.md:67)

| 历史 `.cx` 错用记录 | `adapter/ujswasm/ujsgold.cx` 注释 "one per stage" 说的是**网** → 应用 "one per net"；现已归档的 `archive/testing-c43-legacy/tests/copycontractcheck.cx` 的 `stage` 变量实为构建目标名，不是当前测试入口 |

## unisa/ Python 清零基线 (glossary.md:68)

| `unisa/` 下的 Python | **目标是 100% 零依赖**（主人 2026-10-23 重申：这是硬目标，不是“保留 oracle”）。基线：`python3` 调用点 **2253**、`import unisa` 文件 **86**、剩余 `.py` **452**。三者清零后 `unisa/` 移入 `archive/`（不删，仅人工参考）；迁移前不做任何重命名 |

## 历史迁移原则 (glossary.md:99)

6. 历史迁移原则：每步单独 commit，**不改语义、不改字节**；不将未实施门禁记为绿
