# fly 移植(host1 wasm + UXE WebGL1,无 three.js)

开发在分支 `ujs`。
worktree `.claude/worktrees/ujs`。
编 wasm 用 `./ujs/ujs.com FILE.ujs --run`，不要 `unisacc.com` 当 C。

## 原作

- 试玩: https://remix.ui8.ai/fly (Forge 分享页 fighter)
- 像素战斗机、sprite stacking、层叠岛、昼夜天幕
- 追逐/座舱相机;点击滚转;按住推进;浪尖到高空

本页**不是** remix 像素叠层原作。提交面是 UXE 真 3D:船 `MESH_SHIP`、岛 `MESH_OCTA`、水面 `MESH_PLANE`;追机相机;Host 侧 WASD/Space/click。

## 本仓落点

| 层 | 路径 | 职责 |
|---|---|---|
| 玩法金 | `ujs/tests/examples/fly.ujs` | 目标 host1 子集:16 帧满推进,`return x+100*alt`,金 3868 |
| wasm | `fly.wasm` | 含 custom 段 `ujsmeta`;现编可能因顶层 construct 失败 |
| 页 | `index.html` + `fly-b64.js` + `../host1-runtime.js` | 金走 host1Boot;互动物理在 Host |
| 验收 | `?auto=1` | wasm 金=3868 则 `FLY_OK`;金不可用但 GL 跑满则 `FLY_GL_OK` |

GPU/输入停在页上(UXE Host 原则)。InstanceCloud 仅 xyz/scale/yaw;滚转并入 yaw 视觉,无独立 pitch/roll 缓冲。
岛云 billboard,船全姿态。

## 与产品缺口

`fly.ujs` 顶层 construct 使 wasm 金常不可用,故 auto 不以失败像素叠层冒充。互动不走 ujs.main。见 [`../../../../plans/v0.0.8.md`](../../../../plans/v0.0.8.md)。

## 明确不做

three.js;升 `ujs.com` 版本;改 ujs.main/compiler_min/cxraw;把扫描器改成 ask 网。
