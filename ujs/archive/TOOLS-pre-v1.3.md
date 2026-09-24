# UJS 工具索引（归档）

> **已归档。** 门禁/工具摘要见 [`../prd.md`](../prd.md)；命令真源仍是 `package.json` / `scripts/`。

仓内与 UJS 相关的可执行入口（**不**含 npm publish；产物走 GitHub Release）。

| 工具 | 作用 |
|---|---|
| `./tests/ujs.sh` | **主验收门**：acc / fold / icfold / difftest / ship / front parity / in-page `wasm_run` / **`ujs2wasm` 套件** / BUILD 指纹 |
| `./tests/ujs2wasm.sh` | `ujs2wasm` 语料：`--mode direct` · jtape fold · 负例 · 可选 tinyvm `module validate` |
| `python3 -m ujs web-build` | 生成 `ujs/core` 核产物 + `BUILD.json` |
| `python3 -m ujs ship --out kit.zip` | 构造侧 kit（内部自检） |
| `./ujs/scripts/release-artifacts.sh` | 发版 zip：`ujs_full.wasm` + `compiler.gen.js` + ESM |
| `./ujs/scripts/ship-pages.sh` | **Pages 全量**：asteroid + drone → `docs/uxe/` |
| `./ujs/scripts/ship-engine.sh` | 仅 asteroid → `docs/uxe/asteroid/` |
| `./ujs/scripts/uxe-gate.sh` | `npm run test:uxe:all` |
| `python3 -m ujs run / wasm-run / ujs2wasm / …` | 构造 CLI；`ujs2wasm` 默认直出 `\0asm`（`isel`/`enc` 驱动；无 C）。`--mode vm` 才走 C-VM 垫片 |
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
核对：解压后 `core/ujs_full.wasm` sha256 = `core/BUILD.json`。

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
| [`prd.md`](prd.md) | 产品活规格 + 当前目标（v1.2） |
| [`archive/prd-v1.0.md`](archive/prd-v1.0.md) | 原条款全表 |
| [`archive/FUTURE-pre-v1.2.md`](archive/FUTURE-pre-v1.2.md) | 合并前路线稿 |
| [`FUTURE.md`](FUTURE.md) | 重定向 stub → `prd.md` §7–§8 |

| [`web/README.md`](web/README.md) | git vs Release |
| [`uxe/README.md`](uxe/README.md) | UXE |
| [`uxe/HOST_ABI.md`](uxe/HOST_ABI.md) | Host ABI |
| [`uxe/archive/`](uxe/archive/) | 下架实践车 |
| [`native/README.md`](native/README.md) | C VM / 游戏核 |
| [`../research/README.md`](../research/README.md) | 论文索引 |

## 版本

- 语义版本：`package.json` → `version`
- 指纹：`web/BUILD.json`
- 二进制：Release；勿提交 `ujs_full.wasm` / `compiler.gen.js`（ship 生成物按 `web/README`）
