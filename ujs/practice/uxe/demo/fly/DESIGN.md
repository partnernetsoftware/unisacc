# fly 移植(host1 wasm,无 three.js)

## 原作

- 试玩: https://remix.ui8.ai/fly (Forge 分享页 fighter)
- 像素战斗机、sprite stacking、层叠岛、昼夜天幕
- 追逐/座舱相机;点击滚转;按住推进;浪尖到高空

## 本仓落点

| 层 | 路径 | 职责 |
|---|---|---|
| 玩法金 | `ujs/tests/examples/fly.ujs` | host1 子集:16 帧满推进,`return x+100*alt`,金 3868 |
| wasm | `fly.wasm` | `ujs.com fly.ujs -o`,无 import,导出 `main_export`/`i64_of_export` |
| 页 | `index.html` + `fly-b64.js` | 裸 canvas 2D 岛+叠层机;Space 推进,A/D 倾斜,click 滚转,C 切相机 |
| 验收 | `?auto=1` | 按金表同公式跑 16 帧,DOM 出现 `FLY_PLAY_OK` |

不是 UI8 源码搬运。GPU/输入停在页上(UXE Host 原则)。核只出整数状态。

## 与产品缺口

host1 一次 `main_export` 无每帧输入,不能把键盘写进 wasm 步进。UXE `sim.wasm`+`host_input_read` 才是完整游戏核。本 demo 用 host1 做可回归金,互动物理在页上与 `.ujs` 公式对齐(`?auto=1` 对拍)。

下一候选(一类大改):host1 `run_step`+输入槽,或把 fly 核编进 UXE path-B `sim.wasm`。

## 明确不做

three.js;升 `ujs.com` 版本;把扫描器改成 ask 网。
