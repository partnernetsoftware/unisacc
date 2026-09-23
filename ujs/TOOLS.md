# UJS 工具索引

仓内与 UJS 相关的可执行入口（**不**含 npm publish；产物走 GitHub Release）。

| 工具 | 作用 |
|---|---|
| `./tests/ujs.sh` | **主验收门**：acc / fold / icfold / difftest / ship / front parity / in-page `wasm_run` / BUILD 指纹 |
| `python3 -m ujs web-build` | 生成 `ujs/web` 核产物 + `BUILD.json` |
| `python3 -m ujs ship --out kit.zip` | 构造侧 kit（内部自检） |
| `./ujs/scripts/release-artifacts.sh` | 发版 zip：`ujs_full.wasm` + `compiler.gen.js` + ESM |
| `./ujs/scripts/ship-pages.sh` | **Pages 全量**：asteroid + drone → `docs/uxe/` |
| `./ujs/scripts/ship-engine.sh` | 仅 asteroid → `docs/uxe/asteroid/` |
| `./ujs/scripts/uxe-gate.sh` | `npm run test:uxe:all` |
| `python3 -m ujs run / wasm-run / js2wasm / …` | 构造 CLI；`python3 -m ujs -h` |
| `npm run build` / `demo` / `test` | 薄封装（`test` → `../tests/ujs.sh`） |
| `npm run ship:pages` / `ship:engine` / `ship:drone` | UXE 发布 |
| `npm run test:uxe:all` | UXE 无人门禁（均 alarm） |

## 发版（手动）

```bash
./ujs/scripts/release-artifacts.sh              # 核
./ujs/scripts/release-artifacts.sh --with-demos # + progs / 演示源
./ujs/scripts/release-artifacts.sh --skip-tests  # 已绿只重打包
```

产出：`dist/ujs-<version>-artifacts.zip`(+`.sha256`)。  
核对：解压后 `web/ujs_full.wasm` sha256 = `web/BUILD.json`。

UXE Pages：

```bash
cd ujs && npm run ship:pages && npm run test:uxe:all
# 交付：docs/uxe/{asteroid,drone}/ + docs/index.html
```

## 文档

| 文档 | 内容 |
|---|---|
| [`DOCS.md`](DOCS.md) | **地图**（先读） |
| [`README.md`](README.md) | 开箱与分发 |
| [`prd.md`](prd.md) | 规格 |
| [`web/README.md`](web/README.md) | git vs Release |
| [`web/engine/README.md`](web/engine/README.md) | UXE |
| [`web/engine/HOST_ABI.md`](web/engine/HOST_ABI.md) | Host ABI |
| [`web/engine/archive/`](web/engine/archive/) | 下架实践车 |
| [`native/README.md`](native/README.md) | C VM / 游戏核 |
| [`FUTURE.md`](FUTURE.md) | 产品化下一刀 |
| [`../research/README.md`](../research/README.md) | 论文索引 |

## 版本

- 语义版本：`package.json` → `version`
- 指纹：`web/BUILD.json`
- 二进制：Release；勿提交 `ujs_full.wasm` / `compiler.gen.js`（ship 生成物按 `web/README`）
