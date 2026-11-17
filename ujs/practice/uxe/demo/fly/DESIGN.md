# fly 移植(host1 wasm,无 three.js)

## 原作

- 试玩: https://remix.ui8.ai/fly (Forge 分享页 fighter)
- 像素战斗机、sprite stacking、层叠岛、昼夜天幕
- 追逐/座舱相机;点击滚转;按住推进;浪尖到高空

## 本仓落点

| 层 | 路径 | 职责 |
|---|---|---|
| 玩法金 | `ujs/tests/examples/fly.ujs` | host1 子集:16 帧满推进,`return x+100*alt`,金 3868 |
| wasm | `fly.wasm` | 含 custom 段 `ujsmeta`;`ujs.com fly.ujs -o` |
| 页 | `index.html` + `fly-b64.js` + `../host1-runtime.js` | 金走 host1Boot;互动物理在 Host |
| 验收 | `?auto=1` | wasm 金与 js16 同 3868 才 `FLY_PLAY_OK` |

不是 UI8 源码搬运。GPU/输入停在页上(UXE Host 原则)。

## 与产品缺口

`run_step` 重跑 main,顶层 let 每帧清零,见 [`../../../../plans/v0.0.8.md`](../../../../plans/v0.0.8.md)。本 demo 金在 wasm,互动在 Host。


## 明确不做

three.js;升 `ujs.com` 版本;把扫描器改成 ask 网。
