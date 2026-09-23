# UJS 文档地图

入口按读者分流；**文档必须与代码功能/设计同步**——改 ABI、目录或门禁时，先改代码再改本页与相关 README，禁止只写愿景不改实现说明。

## 产品 / 开箱

| 读什么 | 何时 |
|---|---|
| [`README.md`](README.md) | 安装、`wasm_run`、分发（Release） |
| [`prd.md`](prd.md) | **规格真源**（语言 / API / 验收条款） |
| [`TOOLS.md`](TOOLS.md) | CLI、发版脚本、门禁命令 |
| [`web/README.md`](web/README.md) | `web/` 哪些进 git、哪些进 Release |
| [`package.json`](package.json) | 版本与 npm scripts |

## 游戏 / 引擎（产品化主线 ≠ Three）

| 读什么 | 何时 |
|---|---|
| [`web/engine/README.md`](web/engine/README.md) | **UXE** 总览与文件职责 |
| [`web/engine/HOST_ABI.md`](web/engine/HOST_ABI.md) | Host ABI 契约（核不知浏览器） |
| [`web/game/README.md`](web/game/README.md) | Asteroid + Three **对照**（非主线） |
| [`web/game/exp/README.md`](web/game/exp/README.md) | 同仿真、裸 WebGL 实验 |
| [`FUTURE.md`](FUTURE.md) | P0–P0.3 / A–D；下一刀记账 |

自测（一律带 wall clock）：

```bash
npm run test:uxe:packet     # alarm 20 → UXEP encode/decode
npm run test:uxe:input      # alarm 15 → UXIN encode/decode
npm run test:uxe            # alarm 55 → Chrome CDP /engine/demo
npm run test:game           # alarm 45 → sim.ujs node
npm run test:game:browser   # alarm 55 → /game/ Three 对照
```

## 构造侧

| 读什么 | 何时 |
|---|---|
| [`construct/README.md`](construct/README.md) | Python gold / `web-build` |
| `python3 -m ujs -h` | 全部 CLI 子命令 |

## 论文 / 调研

| 读什么 | 何时 |
|---|---|
| [`../research/README.md`](../research/README.md) | research 目录索引 |
| [`../research/ujs-paper-outline.md`](../research/ujs-paper-outline.md) | Paper B 一页提纲 |
| [`../research/ujs-paper.md`](../research/ujs-paper.md) | Paper B 正文草稿 |
| [`../research/prior-art.md`](../research/prior-art.md) | 对抗性 prior art |
| [`../research/unisacc-paper.md`](../research/unisacc-paper.md) | Paper A（UNISA / C） |

## 布局（与仓库一致）

```
ujs/
├── DOCS.md              ← 本页
├── README.md  TOOLS.md  FUTURE.md  prd.md  package.json
├── scripts/release-artifacts.sh
├── web/
│   ├── wasm_run.js  compiler.js  BUILD.json
│   ├── index.html …     # playground
│   ├── game/            # Three 对照 + exp/
│   └── engine/          # UXE 主线：Host ABI + demo
├── native/
└── construct/
```

## 文档对齐约定

1. **规格**只认 `prd.md`；引擎/Host ABI 是演示与嵌入设计，不偷偷改 `wasm_run` 契约。
2. **设计真源**：`web/engine/HOST_ABI.md` + 同目录实现文件；README 只摘要。
3. 改 `host_*` / packet 布局 / 目录角色 → 同 PR（或同会话）更新 HOST_ABI + engine README + DOCS/FUTURE。
4. 门禁命令以 `package.json` scripts 为准；文档里的 alarm 秒数须与 scripts 一致。
