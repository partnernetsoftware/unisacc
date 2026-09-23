# UJS 文档地图

入口按读者分流。**文档必须与代码同步**：改 ABI、目录或门禁时，同会话改相关 README；禁止只写愿景。

**命令真源**：[`package.json`](package.json) scripts · [`scripts/uxe-gate.sh`](scripts/uxe-gate.sh) · [`scripts/ship-pages.sh`](scripts/ship-pages.sh)。

---

## 三层一句话

| 层 | 是什么 | 不是什么 |
|---|---|---|
| **产品 API** | `wasm_run` + `ujs_full.wasm` | 训练入口 |
| **UXE 嵌入** | Host ABI + `{game}.wasm` + `engine.wasm` | Three 移植 |
| **构造侧** | `construct/` Python gold → 权重 / `web-build` | 应用依赖 |

---

## 产品 / 开箱

| 读什么 | 何时 |
|---|---|
| [`README.md`](README.md) | 安装、`wasm_run`、分发（Release） |
| [`prd.md`](prd.md) | **规格真源**（语言 / API / 验收条款） |
| [`TOOLS.md`](TOOLS.md) | CLI、发版脚本、门禁命令 |
| [`web/README.md`](web/README.md) | `web/` 哪些进 git、哪些进 Release |
| [`package.json`](package.json) | 版本与 npm scripts |

---

## UXE（产品化主线 ≠ Three）

| 读什么 | 何时 |
|---|---|
| [`web/engine/README.md`](web/engine/README.md) | UXE 总览、demo vs ship、文件表 |
| [`web/engine/HOST_ABI.md`](web/engine/HOST_ABI.md) | **Host API 真源** |
| [`web/engine/PLATFORM.md`](web/engine/PLATFORM.md) | 游戏平台方向 · BYO LLM |
| [`web/engine/archive/`](web/engine/archive/) | 下架实践车（大富翁等） |
| [`web/game/README.md`](web/game/README.md) | Asteroid + Three **对照**（非主线） |
| [`FUTURE.md`](FUTURE.md) | 已交付 · 产品化下一刀 · H 档 |

**交付面（外网）**

```
/docs/index.html              # 游戏索引（Asteroid + 无人机）
/docs/uxe/asteroid/           # html + asteroid.wasm + engine.wasm
/docs/uxe/drone/              # html + engine.wasm（JS 核 embed）
```

本地对照：`/engine/demo/` · `/engine/ship/` · `/engine/ship/drone/`。

```bash
npm run ship:pages          # asteroid + drone → docs/uxe/
npm run test:uxe:all        # 无人门禁
```

分项：`test:uxe:packet` · `input` · `uxe` · `ship` · `drone` · `drone:ship`（均 alarm）。  
Three 对照：`test:game` · `test:game:browser`。

---

## 构造侧

| 读什么 | 何时 |
|---|---|
| [`construct/README.md`](construct/README.md) | Python gold / `web-build` |
| [`native/README.md`](native/README.md) | C VM + `{game}.wasm` 源 |
| `python3 -m ujs -h` | 全部 CLI 子命令 |

---

## 论文 / 调研

| 读什么 | 何时 |
|---|---|
| [`../research/README.md`](../research/README.md) | research 目录索引 |
| [`../research/ujs-paper-outline.md`](../research/ujs-paper-outline.md) | Paper B 一页提纲 |
| [`../research/ujs-paper.md`](../research/ujs-paper.md) | Paper B 正文草稿 |
| [`../research/prior-art.md`](../research/prior-art.md) | 对抗性 prior art |
| [`../research/unisacc-paper.md`](../research/unisacc-paper.md) | Paper A（UNISA / C） |

论文只绑套件名与定性产物指针；**不编造 latency / 准确率小数**。

---

## 目录树（与仓库一致）

```
ujs/
├── DOCS.md  README.md  TOOLS.md  FUTURE.md  prd.md  package.json
├── scripts/
│   ├── release-artifacts.sh
│   ├── ship-engine.sh      # asteroid → docs/uxe/asteroid
│   ├── ship-pages.sh       # asteroid + drone
│   └── uxe-gate.sh         # test:uxe:all
├── web/
│   ├── wasm_run.js  compiler.js  BUILD.json
│   ├── engine/             # UXE
│   │   ├── HOST_ABI.md  PLATFORM.md  README.md
│   │   ├── browser-host.js  packet.js  input.js  _cdp.mjs
│   │   ├── core-asteroid.js  core-drone.js  core-monopoly.js（归档车）
│   │   ├── demo/  ship/  archive/
│   └── game/               # Three 对照 + exp/
├── native/                 # C VM · uxe_asteroid.c
└── construct/              # Python gold · front · build
```

---

## 文档对齐约定

1. **规格**只认 `prd.md`；UXE / Host API **不**偷偷改 `wasm_run` 契约。
2. **Host 真源**：`HOST_ABI.md`；平台叙事：`PLATFORM.md`；README 只摘要。
3. 改 `host_*` / UXEP·UXIN / 交付面 → 同会话更新 HOST_ABI + engine README + DOCS/FUTURE。
4. 门禁以 `package.json` + `uxe-gate.sh` 为准。
5. **游戏 wasm 与引擎 wasm 不合包**。
6. **LLM**：密钥只在 Host/壳；规划见 HOST_ABI §7.1。
7. 过期实践车进 [`web/engine/archive/`](web/engine/archive/)，勿再写进「下一刀」。
