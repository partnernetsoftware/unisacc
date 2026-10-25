# UJS → `unisacc.com` + `.cx` 迁移债（高优先）

> 主人 2026-10-25：有 `.c`／`.py` 须**提升优先度**，先安排换成 `.cx` 系列。  
> 产品链 `ujs/product/` **已无** `.c`／`.py`；下列为树内残余。  
> 规则：真实 `-run` 迁移；`.cx` 只转调 Python **不算**完成；不预堆根仓 0.0.44。

## 盘点（Git 跟踪 · 2026-10-25）

| 族 | 数量 | 路径 | 优先 |
|---|---|---|---|
| **构造臂 Python** | ~24 | `seed/construct/**/*.py` | **P0**（日用／对照／acc／ujs2wasm） |
| **包入口** | 2 | `ujs/__init__.py` · `__main__.py`（`python3 -m ujs`） | **P0**（出货叙事仍像 Python 包） |
| **实践自测 py** | 3 | `practice/uxe/_*.py` · `practice/web/game/_selftest_run.py` | **P1**（可删或改 `.cx` 探针） |
| **path-A VM C** | 2+链接 | `seed/vm/ujs_vm.c` · `uxe_asteroid.c`（`native/` 链） | **P2**（非 host1 必经；归档或后置） |
| **权重生成物 C** | 1 | `weights/ujs_ic_net.c` | **P2**（生成物；生成器迁走后重建） |
| **壳脚本** | 6+ | `product/build.sh`（薄）· `scripts/*.sh` · `seed/stage0/*.sh` | **P1**（能删尽删；其余改 `.cx`） |

**已洁净**：`ujs/product/*.cx` 产品链（host1）。

## 波次（插入 0.0.x 主线）

| 波 | 版（建议） | 完成定义 |
|---|---|---|
| **W0 盘点闸** | **0.0.3** | 本文件为真源；`prd` 指针；禁止新增 `.py`／产品链 `.c`；列每族「替代入口」 |
| **W1 入口熄火** | 0.0.3／0.0.4 | `__main__`／常用 CLI 不再是日用必经；文档只推 `ujs.com`／`.cx`；能删的自测 py 删除 |
| **W2 构造切片** | 0.0.4+ | 按依赖族迁：先 `paths`／只读探针，再 `acc`／fold 合同，再 emit；每族删对应 `.py` |
| **W3 VM／权重** | 后置 | path-A `.c` 证明无消费者则归档；`ujs_ic_net.c` 改由 `.cx` 生成器写出 |

## 验收口诀

- 数：`git ls-files 'ujs/**/*.py' 'ujs/**/*.c'` 只许缩。  
- 行为：迁移项用 `./unisacc.com -run …cx` 复现原拒绝／绿。  
- 标签：构造迁完前仍标 `method_construct`／实践债，**不**冒充 host1 已无 Python 依赖树。
