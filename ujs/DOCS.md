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
| [`prd.md`](prd.md) | **产品活规格 v1.1**（交付 · API · UXE · 门禁 · 代理 snapshot） |
| [`archive/prd-v1.0.md`](archive/prd-v1.0.md) | 原条款全表（归档） |
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
/docs/index.html
/docs/uxe/engine.js           # 共享 Host + GPU
/docs/uxe/asteroid/           # index + game.js + wasm
/docs/uxe/drone/              # index + game.js + engine.wasm
/docs/uxe/live/               # WebRTC 实验（观看/假源）；经验 → README.md
```

本地对照：`/engine/demo/` · `/engine/ship/`（旁路 `engine.js`）。

```bash
npm run ship:pages          # engine.js + asteroid + drone
npm run test:uxe:all        # 无人门禁（CDP · snapshot 可读）
```

分项：`test:uxe:packet` · `input` · `uxe` · `ship` · `drone` · `drone:ship`。  
代理开发约定见 [`prd.md`](prd.md) §6（断言写在数上，截图非默认）。  
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
├── archive/                # 过期规格（prd-v1.0…）
├── scripts/
│   ├── release-artifacts.sh
│   ├── ship-engine.sh · ship-pages.sh · uxe-gate.sh
├── web/
│   ├── wasm_run.js  compiler.js  BUILD.json
│   ├── engine/             # UXE：demo/ · ship/（engine.js+game.js）· archive/
│   └── game/
├── native/
└── construct/
```

---

## 文档对齐约定

1. **产品规格**认 `prd.md`（v1.1）；条款全表在 `archive/prd-v1.0.md`。UXE Host **不**改 `wasm_run` 契约。
2. **Host 真源**：`HOST_ABI.md`；平台：`PLATFORM.md`；README 只摘要。
3. 改 `host_*` / UXEP·UXIN / 交付面 / snapshot 约定 → 同会话更新 HOST_ABI + prd §6 + DOCS。
4. 门禁以 `package.json` + `uxe-gate.sh` 为准；**代理默认读 JSON snapshot，不靠截图**。
5. **游戏 wasm 与引擎 wasm 不合包**；页面共享 `engine.js`。
6. **LLM**：密钥只在 Host/壳；规划见 HOST_ABI §7.1。
7. 过期实践车 → `web/engine/archive/`；过期规格 → `ujs/archive/`。
