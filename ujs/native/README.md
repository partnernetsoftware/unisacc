# native — C 运行时与游戏核

| 文件 | 产物 | 角色 |
|---|---|---|
| `ujs_vm.c` + `ujs_ic_net.c` | `web/ujs_full.wasm`（交付名 **`engine.wasm`**） | UJS VM：`host_run` / list·dict 绑定 |
| `uxe_asteroid.c` + `sim_embed.h` | `web/engine/ship/asteroid.wasm` | `{game}.wasm`：调度 + UXEP；**不**链 VM |
| `sim_embed.h` | （生成） | `ship/build-asteroid.mjs` 写入；已 gitignore |

构建：

```bash
python3 -m ujs web-build          # → ujs_full.wasm
cd ujs && npm run ship:engine     # → asteroid.wasm + 复制 engine.wasm
```

`uxe_asteroid.c` 经 `env.eng_boot` / `env.eng_sim_step` 由页内胶水桥到 `engine.wasm`（双 linear memory，不合包）。
