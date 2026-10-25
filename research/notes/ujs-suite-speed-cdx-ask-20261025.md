# 向 cdx：`unisacc.com -run` 拖慢 UJS host1 套件

**来源**：ujs.com 0.0.4 产品闸提速（`ujs/tests/run.cx`）  
**标签**：底座性能／非 UJS 产品合同  
**日期**：2026-10-25

## 现象（本机实测）

| 操作 | 墙钟（2026-10-25 复测） |
|---|---|
| `./unisacc.com -run` 空 `main` | **~1.74 s／次** |
| `ujs/tests/run.cx -- --smoke` 全闸 | **~15.7 s** |
| 其中 `build.cx`  alone | **~8.7 s** |
| 其中 `corpus-host.cx` alone | **~3.7 s** |
| `./ujs/ujs.com --verify-list`（28） | **~4 ms**（原生批验，已落地） |

结论：产品侧编译／批验已很快；smoke 仍被 **每次 `unisacc.com -run` ~1.7 s 冷启动** + build 内编译墙钟主导。UJS 已合并验面进 corpus、去掉多余 `-run`；**再砍需要底座**（少次 `-run` 或每次更便宜）。

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

## UJS 侧已做（不挡底座）

- `--verify-list` 一批验（~4 ms）；corpus-host 内完成验面＋冒充实拒绝。  
- smoke＝`build`＋`corpus`（不再另挂 verify-host `-run`）。  
- 套件在 `ujs/tests/`。提交：`203ba6511c`。

## 验收建议（底座）

给出：空 `-run` 墙钟分解（≥3 段具名）；任选 A–D 的最小探针；目标例如空 `-run` **p50 &lt; 200 ms**（或书面说明不可达原因）。

不 tip／Draft；不改 UJS 产品版号合同。
