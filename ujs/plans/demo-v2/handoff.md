# handoff 2026-11-18 停手等换窗口

## 原话档

`ujs/plans/demo-v2/requirements-original.md`(只增不改)。prd 0.3 以它为准。

## 视频与帧(不进 docs/)

| | 本体 | 仓内 |
|---|---|---|
| D2 | 无 mp4。卡片 `/tmp/ref/d2-card.jpg`。可玩站 what-the-water-keeps,headless 出不了完整画面 | `ref/d2/` |
| D3 | `/tmp/ref/d3.mp4` sha256 679a804c067ac109c6e155051be646776fee9912a8655760a85039f04698e2ea | `ref/d3/frames` 01 04 07 |
| D4 | `/tmp/ref/d4.mp4` sha256 5052c61911f2c97ee281dd3bba6a32bcd4b0522599ed3a2fce62904291114371 | `ref/d4/frames` 01 04 07 10 13 |
| JSON | `/tmp/ref/d2.json` `d3.json` `d4.json` | |

无 ffmpeg。本机先前无这些视频缓存。

## 各卡状态

- D1 fly: Forge fighterv2,画面 UNMEASURED(headless 空白)。
- D2 water-keeps: 参考=vercel 可玩站。S0 概念稿/UXE 与 c-vt20000 仍有效。取消待确认。
- D3 ink-walk: 视频帧核过多拱石桥小屋阔叶,NPR。仍缺全时长精确抽帧。
- D4 oneshot: 视频帧核过四足机械城堡丘陵积云白烟,NPR。
- D5 ghibli-world: 终局,依赖 D1-D4,当前不开工。

## UXE 优先级

T1 贴图采样 -> T2 素材加载 -> T3 精灵粒子 -> **T4 NPR(描边、赛璐璐色阶、手绘纹理阴影、天空云; D3/D4 前置,先于 T5)** -> T5 倒影/后处理 -> D5 动态世界。

## 已知坑

- D2 可玩站 headless 不完整。
- 无 ffmpeg,chrome seek 多数非 0 秒失败。
- 第三方帧只作仓内对照,不得进 docs/ 与发布页。
- ujs 工作在 ujs 分支做完再合 main; 不 cherry-pick 代办。

## Q2a 石桥几何

基点 3a9d806，本项提交可用 `git log -1 --format=%h -- tools/check-uxe-d3-geometry.cx` 定位。固定光照与机位的 [1280 x 720 截图](look-dev/d3-q2a.png) 对应三孔桥。运行 `tools/check-uxe-d3-geometry.cx` 的 normal、bare 和 sealed 三种输入；sealed 必须失败，bare 应在前景树冠原像素露出桥墩。取点按解析相机投影，屋顶点取右侧坡面中央，不能取山墙内部。下一步先同步 main 并回归，再做 Q2b 的受光色阶。此帧只验几何，最终风格待固定帧与短横移交所有者签字。

## Q2a2 圆弧拱圈

Q2a 提交 271b47b。圆弧版用三行毫米规格定义三个 16 段半圆和贯通的内壁。[固定机位截图](look-dev/d3-q2a2.png) 与 Q2a 可直接比较。拱顶和两个偏心位置取圆方程上下各 100 毫米的投影点，旧台阶在偏心位置失败，压低拱顶也会失败。Chrome 须等 TSV 完成加载再取读回。Q2a2 提交身份可由 `git log -1 --format=%h -- ujs/plans/demo-v2/look-dev/d3-q2a2.png` 查得。下一项先合 main，再开 Q2b。
