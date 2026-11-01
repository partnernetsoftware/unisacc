# 宿主驱动前奏裁剪：冷路径提速

## 事实

宿主 `gcc_cx` / `clang_cx` 每个翻译单元都带 `-include include/cx.h`。前奏里的 `cx_read`、`cx_run`、`cx_sha256` 等是 `static` 定义；目标单元不调用它们时，gcc 仍要解析这些函数体。冷编译一个小 `.cx` 的主要成本在这里。

本机 Ubuntu gcc 11.4 / clang 15，同一台机器，10 个不同源文件各编译一次：

| 路径 | gcc | clang |
|---|---:|---:|
| 无前奏的普通 C | 42ms | 100ms |
| 带完整 `cx.h` 的普通 C | 153ms | 131ms |
| `gcc_cx`/`clang_cx` 冷编译 | 169ms | 153ms |
| 本次裁剪后冷编译 | 73ms | 128ms |
| 本次裁剪后热对象缓存 | 34ms | 64ms |

8 模块链冷编译：gcc 1256ms -> 408ms；clang 770ms -> 681ms。

## 规则

- 仅当源单元不含 `cx_` 标识符片段、且不含 `#include` 时，目标编译追加 `-D__UNISA_FTRIM_LIBC=1`。
- `cx_` 检测覆盖前奏全部工具函数名；`#include` 检测覆盖携带头文件自己的 `__UN_*` 裁剪体，宁可保守不裁剪。
- 裁剪只作用于宿主目标编译；驱动自身、link 参数、`CXIF` 展开不变。
- 对象缓存键增加裁剪标记，避免同一源在不同前奏形态下命中错误对象。

## 验收

- `tests/cxcc/units.tsv` 增加 `toolkit`，调用 `cx_sha256` 并校验摘要首字节；两个驱动均 PASS。
- `sh ./gcc_cx.com tests/cxcc/suite.cx -o OUT && OUT`：11 units × 2 drivers，`fails=0`。
- `tests/run-batch.cx ... cx-host-run`：`status=PASS`，候选 `21782dd5…`。
- 七项棘轮 `ok=7 fail=0`。

## 边界

- 尚未采用 PCH；clang 的收益较小，主要来自 gcc 前奏函数体。
- 多单元仍串行编译；本次只降每单元冷成本。
- 未重构建/安装 `unisacc` 产品；本记录只针对宿主驱动。
