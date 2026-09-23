# UJS 工具索引

仓内与 UJS 相关的可执行入口（**不**含 npm publish；产物走 GitHub Release）。

| 工具 | 作用 |
|---|---|
| `./tests/ujs.sh` | **主验收门**：acc / fold / icfold / difftest / ship / front parity / in-page `wasm_run` / BUILD 指纹与 ABI |
| `python3 -m ujs web-build` | 本地生成 `ujs/web` 产物，并写 `BUILD.json` |
| `python3 -m ujs ship --out kit.zip` | 构造侧 kit（权重 + native + 含 web 的 zip）；偏内部自检 |
| `./ujs/scripts/release-artifacts.sh` | **发版打包（手动）**：跑套件 → 校验指纹 → 打出 `dist/ujs-<ver>-artifacts.zip` 供上传 Release |
| `python3 -m ujs run / wasm-run / js2wasm / …` | 构造 CLI；见 `python3 -m ujs -h` |
| `npm run build` / `demo` / `test` | 包根薄封装（`test` → `../tests/ujs.sh`） |
| `npm run test:uxe` / `test:uxe:packet` / `test:uxe:input` | UXE Host ABI 演示 + UXEP/UXIN（均 alarm） |

## 发版（手动，可复用脚本）

```bash
# 仓库根目录
./ujs/scripts/release-artifacts.sh              # 核：full.wasm + compiler.gen.js + ESM
./ujs/scripts/release-artifacts.sh --with-demos # 另含 progs/*.wasm
# 已绿过、只想重打包：
./ujs/scripts/release-artifacts.sh --skip-tests
```

产出：

- `dist/ujs-<version>-artifacts.zip`
- `dist/ujs-<version>-artifacts.zip.sha256`

然后在 GitHub 上为该 `package.json` version 建 Release，**手动上传** zip（脚本不调用 `gh`）。

核对：解压后 `web/ujs_full.wasm` 的 sha256 必须等于 `web/BUILD.json` 里记录的值。

## 文档

| 文档 | 内容 |
|---|---|
| [`DOCS.md`](DOCS.md) | **地图**（先读；含文档对齐约定） |
| [`README.md`](README.md) | 开箱（JS-first）与分发策略 |
| [`prd.md`](prd.md) | 规格 |
| [`web/README.md`](web/README.md) | 站点目录：git vs Release |
| [`web/engine/README.md`](web/engine/README.md) | UXE 引擎（文件↔职责） |
| [`web/engine/HOST_ABI.md`](web/engine/HOST_ABI.md) | Host ABI 设计真源 |
| [`web/game/README.md`](web/game/README.md) | Asteroid Three 对照 |
| [`construct/README.md`](construct/README.md) | Python 构造侧 |
| [`FUTURE.md`](FUTURE.md) | 未来节点：实践检验 / A–D |
| [`../research/README.md`](../research/README.md) | 论文索引 |
| [`../research/ujs-paper-outline.md`](../research/ujs-paper-outline.md) | 论文 B 提纲 |

## 版本约定（简）

- 语义版本：`ujs/package.json` → `version`
- 指纹：`ujs/web/BUILD.json`（可入库；**不含**大二进制）
- 二进制：只上 Release；勿提交 `*.wasm` / `compiler.gen.js`
