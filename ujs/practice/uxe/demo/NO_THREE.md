# 替代 three.js:现状与该增强的

三款参考都按「核出整数金、Host 画」落地,不引进 three。

| 参考 | 本仓 | 不是 |
|---|---|---|
| 灯舟 8 界 | `water-keeps/` 金 404 | 24MB GLB 原作 |
| 铅笔水彩镜头 | `ink-walk/` 金 16 | Blender shader |
| three 一发 | `oneshot/` 金 60,裸 WebGL/2D | three.module |

## 架构该增强(一类一条,勿并行)

(1) 步进核进浏览器:host1 `main_export` 每调清局部;UXE `sim.wasm`+`host_input_read` 才是每帧核。缺 import 段。
(2) 提交面:`HOST_ABI` 要一次 `host_gpu_submit` 整包,现 demo 仍是页内 `fillRect`/`scissor`。
(3) 2D/3D 实例化:UXE 只有八面体/船盒;水彩边、点精灵、贴图仍缺(HOST_ABI H3/H4)。
(4) 元数据:wasm 已有 `ujsmeta`;Pages 仍靠 `compile.mjs` meta。
(5) 不该增强:把 three 搬进仓;把扫描器改 ask 网来画游戏。
