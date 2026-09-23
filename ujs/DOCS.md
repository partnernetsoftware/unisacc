# UJS 文档地图

入口按读者分流。**文档必须与代码同步**：改 ABI、目录或门禁时，同会话改相关 README；禁止只写愿景。

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
| [`web/engine/HOST_ABI.md`](web/engine/HOST_ABI.md) | **Host API 真源**（符号 · UXEP/UXIN · 扩展 · `host_llm` 规划） |
| [`web/engine/PLATFORM.md`](web/engine/PLATFORM.md) | 游戏平台方向：多核 ship + **用户自带 LLM API** |
| [`web/game/README.md`](web/game/README.md) | Asteroid + Three **对照**（非主线） |
| [`web/game/exp/README.md`](web/game/exp/README.md) | 同仿真、裸 WebGL 实验 |
| [`FUTURE.md`](FUTURE.md) | P0–P0.4；H 档；实践车；A–D |

**交付面（已验证）**

```
/engine/ship/                 # 本地开发对照
  index.html + asteroid.wasm + engine.wasm

/docs/                        # GitHub Pages（外网测）
  index.html                  # 游戏索引（当前仅 Asteroid）
  uxe/asteroid/               # Asteroid ship 三件套
```

`npm run ship:engine` 同步 asteroid → `docs/uxe/asteroid/`。大富翁已归档，不进索引。Pages 源选 **`/docs`**。

开发对照：`/engine/demo/`（分源 + 页内 compiler）。

自测（一律 wall clock）：

```bash
npm run ship:engine         # 生成上述三件
npm run test:uxe:all        # 一键无人门禁（packet→input→demo→ship→drone*）
npm run test:uxe:ship       # alarm 55 → CDP /engine/ship/
npm run test:uxe:packet     # alarm 20 → UXEP
npm run test:uxe:input      # alarm 15 → UXIN
npm run test:uxe            # alarm 55 → /engine/demo/
npm run test:game           # alarm 45 → sim.ujs node
npm run test:game:browser   # alarm 55 → /game/ Three 对照
```

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
├── DOCS.md                 ← 本页（地图）
├── README.md  TOOLS.md  FUTURE.md  prd.md  package.json
├── scripts/
│   ├── release-artifacts.sh    # 发版 zip（ujs_full + compiler.gen）
│   └── ship-engine.sh          # UXE 发布面（html + 双 wasm）
├── web/
│   ├── wasm_run.js             # 产品 API
│   ├── compiler.js             # 页内编译（demo / playground）
│   ├── BUILD.json              # 指纹（可入库）
│   ├── index.html …            # playground
│   ├── engine/                 # UXE
│   │   ├── HOST_ABI.md         # Host API 真源
│   │   ├── PLATFORM.md         # 平台方向 · BYO LLM
│   │   ├── browser-host.js …   # 分源实现
│   │   ├── core-asteroid.js    # demo JS 核
│   │   ├── core-monopoly.js    # demo / ship-js 核
│   │   ├── demo/               # 源码测试页（含 monopoly/）
│   │   └── ship/               # 发布面（asteroid + monopoly/）
│   └── game/                   # Three 对照 + exp/
├── native/
│   ├── ujs_vm.c                # → ujs_full / engine
│   ├── ujs_ic_net.c            # IC 网（生成/构造）
│   └── uxe_asteroid.c          # → asteroid.wasm（无链 VM）
└── construct/                  # Python：gold · front · build · CLI
```

---

## 文档对齐约定

1. **规格**只认 `prd.md`；UXE / Host API **不**偷偷改 `wasm_run` 契约。
2. **Host 真源**：`web/engine/HOST_ABI.md`；平台叙事：`PLATFORM.md`；README 只摘要。
3. 改 `host_*` / UXEP·UXIN / 交付面 → 同会话更新 HOST_ABI + engine README + DOCS/FUTURE + research 定性指针。
4. 门禁以 `package.json` scripts 为准；文档里的 alarm 秒数须一致。
5. **游戏 wasm 与引擎 wasm 不合包**：`{game}.wasm` ≠ `engine.wasm`。
6. **LLM**：密钥只在 Host/壳；规划符号见 HOST_ABI §7.1，禁止进核与 packet。
