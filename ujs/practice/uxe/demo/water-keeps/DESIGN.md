# water-keeps 移植(host1 wasm,无 three.js)

## 参考

- 可玩原作: https://what-the-water-keeps.vercel.app/ (Kevin Ngo, three+GLB,约 24MB)
- 画风: https://x.com/beyondtheboxcn/status/2122472282824065492 铅笔水彩 3D
- three 一发: https://x.com/anvithbuilds/status/2122408638950371579 (不搬 three)

本仓不搬 GLTF/three。合同:8 界水道,灯舟,A/D 转向,水流自带。

## 落点

| 层 | 路径 | 职责 |
|---|---|---|
| 金 | `ujs/tests/examples/water.ujs` | 16 步 x+=3,越 10 进下一界,`realm+100*x`,金 404 |
| wasm | `water.wasm` | custom `ujsmeta` |
| 页 | `index.html` | 2D 水彩海+舟;Host 输入 |
| 验收 | `?auto=1` | wasm 金 404 且 js16 同值 -> `WATER_OK` |

## 明确不做

three.js;GLB;升 `ujs.com`;把扫描器改 ask 网。
