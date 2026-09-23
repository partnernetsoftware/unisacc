# Asteroid Rush 3D — **对照演示**（Three.js 宿主）

> 产品化主线：[`../engine/`](../engine/)（demo 源码测 · **ship** 发布面）。本目录保留 Three 对照。

## 分层（勿混）

| 层 | 技术 | 职责 |
|---|---|---|
| 宿主页 | HTML + Three.js（本地 `three.module.js`） | 3D 渲染、输入、HUD、计时 |
| 产品 API | `bootRuntime` / `wasm_run` / `compile` | 一次编译，每帧跑 image |
| 仿真 | `sim.ujs`（**UJS-1**） | N 颗小行星积分、包裹、碰撞、得分 |

不是「整个网站编成一个 wasm」，也不是任意 ES；是 **VM 里跑游戏逻辑，浏览器画出来**。

## 本地

```bash
python3 -m ujs web-build          # 需 ujs_full.wasm
# 若尚无 three：
perl -e 'alarm 60; exec @ARGV' curl -fsSL \
  https://unpkg.com/three@0.160.0/build/three.module.js \
  -o ujs/web/game/three.module.js
cd ujs && npm run demo            # http://127.0.0.1:8765/game/
```

自测（**一律带 wall clock**，防死循环 / 超预期挂起）：

```bash
perl -e 'alarm 45; exec @ARGV' python3 ujs/web/game/_selftest_run.py
# 需本机 Chrome；探测开 SwiftShader WebGL
perl -e 'alarm 55; exec @ARGV' node ujs/web/game/_browser_probe.mjs
```

## 操作

- **A/D** 或 ←/→：左右  
- **W/S** 或 ↑/↓：上下  
- **空格**：撞毁后重开  

HUD：UJS 每帧耗时 (ms)、实体数（默认 480）、分数、FPS。  
过大 N 会打满当前 `ujs_full.wasm` 堆；要再加实体需先扩 VM 内存。
