# UJS 工具索引

仓内与 UJS 相关的可执行入口（**不**含 npm publish；产物走 GitHub Release）。

| 工具 | 作用 |
|---|---|
| `./tests/ujs.sh` | **主验收门**：acc / fold / icfold / difftest / ship / front parity / in-page `wasm_run` / BUILD 指纹 |
| `python3 -m ujs web-build` | 生成 `ujs/web` 核产物 + `BUILD.json` |
| `python3 -m ujs ship --out kit.zip` | 构造侧 kit（内部自检） |
| `./ujs/scripts/release-artifacts.sh` | 发版 zip：`ujs_full.wasm` + `compiler.gen.js` + ESM |
| `./ujs/scripts/ship-engine.sh` | UXE 发布面：`html` + `asteroid.wasm` + `engine.wasm` |
| `python3 -m ujs run / wasm-run / js2wasm / …` | 构造 CLI；`python3 -m ujs -h` |
| `npm run build` / `demo` / `test` | 薄封装（`test` → `../tests/ujs.sh`） |
| `npm run ship:engine` | → `scripts/ship-engine.sh` |
| `npm run test:uxe:all` / `:ship` / `:packet` / `:input` | UXE 无人门禁（均 alarm） |

## 发版（手动）

```bash
./ujs/scripts/release-artifacts.sh              # 核
./ujs/scripts/release-artifacts.sh --with-demos # + progs / 演示源
./ujs/scripts/release-artifacts.sh --skip-tests  # 已绿只重打包
```

产出：`dist/ujs-<version>-artifacts.zip`(+`.sha256`)。  
核对：解压后 `web/ujs_full.wasm` sha256 = `web/BUILD.json`。

UXE 游戏包（另一步，不合进核 zip 亦可）：

```bash
cd ujs && npm run ship:engine && npm run test:uxe:ship
# 交付：web/engine/ship/{index.html,asteroid.wasm,engine.wasm}
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
| [`native/README.md`](native/README.md) | C VM / 游戏核 |
| [`FUTURE.md`](FUTURE.md) | 下一刀 |
| [`../research/README.md`](../research/README.md) | 论文索引 |

## 版本

- 语义版本：`package.json` → `version`
- 指纹：`web/BUILD.json`
- 二进制：Release；勿提交 `ujs_full.wasm` / `compiler.gen.js`（ship 生成物按 `web/README`）
