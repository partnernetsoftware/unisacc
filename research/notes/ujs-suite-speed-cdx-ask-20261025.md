# 向 cdx：`unisacc.com -run` 拖慢 UJS host1 套件

**来源**：ujs.com 0.0.4 产品闸提速（`ujs/tests/run.cx`）  
**标签**：底座性能／非 UJS 产品合同  
**日期**：2026-10-25

## 现象（本机实测）

| 操作 | 墙钟 |
|---|---|
| `./unisacc.com -run` 空 `main`（`puts("ok")`） | **~1.7 s／次**（连续两次同量级） |
| `./ujs/ujs.com` 编译 `arith.ujs` | **~3 ms** |
| `./ujs/ujs.com --verify` 同 wasm | **~2 ms** |
| smoke 外层：`build.cx` + `corpus-host.cx` + `verify-host.cx` | 各一次 `-run` ≈ **3×1.7 s 固定税** |
| 历史 `wasm-verify.cx -- FILE`（再包一层 `-run`） | **~2.9 s** 验一个已有 wasm |

结论：**单次编译／验面本身很快**；套件墙钟被 **`unisacc.com -run` 每次进程冷启动／装载** 主导。UJS 侧可再削重复编译，但削不掉「每闸一次 `-run`」的底座税。

## 问题（请 cdx 诊断）

1. `-run` 路径上 ~1.7 s 主要花在哪几段？（APE 解压／映射、权重／kernel 装载、CX 编译、首次 exec 扫描？）  
2. 同机连续第二次仍 ~1.7 s：是否几乎无跨调用缓存？  
3. 产品纪律要求日常用 `-run` 而非先落盘可执行文件——有没有 **保留 `-run` 语义** 的提速路径？

## 建议方案（供选型，非指定实现）

| 方案 | 意图 |
|---|---|
| **A. 常驻／暖机** | 可选 daemon 或 `unisacc.com -run-server`：后续 `-run` 只送源／路径，摊销装载 |
| **B. 编译缓存命中加速** | 对未变 `.cx` 跳过再编译；实测第二次仍慢则说明瓶颈不在 CX 编译 |
| **C. 轻量 preload** | 把 kernel／权重 mmap 成可复用映像，`-run` 只映射不重解析 |
| **D. 批模式** | `unisacc.com -run-batch FILE.list`：一次进程跑多个 `.cx`（UJS smoke 可合成一闸） |
| **E. 诚实文档** | 若 1.7 s 是硬下界，写进底座合同；UJS 闸改为少次 `-run` + 批合同 |

## UJS 侧已／将自做（不挡底座）

- 验面不再对 pass 语料 **整表重编译**（复用 `corpus-host` 写出的 wasm）。  
- 减少无意义的「为验一个文件再 `-run` 一层包装」。  
- 套件入口固定在 `ujs/tests/`。

## 验收建议（底座）

给出：空 `-run` 墙钟分解（≥3 段具名）；任选 A–D 的最小探针；目标例如空 `-run` **p50 &lt; 200 ms**（或书面说明不可达原因）。

不 tip／Draft；不改 UJS 产品版号合同。
