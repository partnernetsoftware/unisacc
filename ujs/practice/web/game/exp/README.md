# 小实验 · 同仿真 / 换渲染壳

**不是**造引擎，也不是替代 Three。只验证一句假设：

> 把岩石从 Three `InstancedMesh` 换成宿主裸 WebGL 实例化后，
> **ujs step ms 应基本不变**（瓶颈在 marshal/VM）；fps / `render ms` 才可能动。

| 不变量 | 变量 |
|---|---|
| `../sim.ujs` + `wasm_run` | 本页 `rawgl.js` 画布 |

打开：`/game/exp/`（需 demo 服）。对照主页 `/game/`（Three）。

HUD 同时报 `ujs` 与 `draw` 毫秒，方便并排看。
