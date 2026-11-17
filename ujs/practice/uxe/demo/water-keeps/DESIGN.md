# water-keeps

开发在分支 `ujs`。
worktree `.claude/worktrees/ujs`。
编 wasm 用 `./ujs/ujs.com FILE.ujs --run`，不要 `unisacc.com` 当 C。

参考 https://what-the-water-keeps.vercel.app/ 灯舟过 8 界。不搬 three/GLB。

`.ujs` 吐 8 个三角形岸+8 色(列表 56,host1 字面量上限 64)。页 WASD 开船,界随 x。

缺口:列表不能超过 64;无网格;舵仍在 Host(无每帧 import)。
两岸 32 灯 MESH_OCTA billboard。
