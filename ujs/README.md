# UJS — 开箱

> 规格（唯一进度）：[`prd.md`](prd.md) · 地图：[`ARCHITECTURE.md`](ARCHITECTURE.md) · Paper B：[`../research/ujs-paper.md`](../research/ujs-paper.md)

## 出货：`ujs.com`

产品链 **只许** 根目录 `unisacc.com` + `.cx`（默认 `-run`）。无产品链 `.c`／`.py`。

```bash
# 仓库根
./unisacc.com -run ujs/product/complete.cx   # APE + 语料 28/0 + 六目标
./unisacc.com -run ujs/product/build-ape.cx  # 只出 APE
./unisacc.com -run ujs/product/build.cx      # 本机原生（快迭代）
./unisacc.com -run ujs/product/build-six.cx  # 六目标原生 → /tmp

sh ./ujs/ujs.com --version
sh ./ujs/ujs.com FILE.ujs -o OUT.wasm
```

细则：[`product/README.md`](product/README.md)。`ujs/ujs.com` 为生成物（已 gitignore）。

**stage0 语料合同**：`corpus-host.cx` — 28 PASS／0 REFUSE（含 arrow／function+rest）。

**测试套件入口**：

```bash
./unisacc.com -run tests/ujs/run.cx -- --smoke
./unisacc.com -run tests/ujs/run.cx -- --complete
```

见 [`../tests/ujs/README.md`](../tests/ujs/README.md)。

## 目录

| 路径 | 角色 |
|---|---|
| [`product/`](product/) | **出货宿主**：`ujs_main.cx`、构建／APE／语料闸 |
| [`seed/stage0/`](seed/stage0/) | host 编译器 `compiler_min.cx` |
| [`seed/construct/`](seed/construct/) | 构造臂（Python；P1 后置） |
| [`iterate/`](iterate/) | M3：`compiler.ujs`／`compiler_core`／`compile.mjs`（实践脊） |
| [`practice/`](practice/) | Pages／UXE／path-A 实验场 |
| [`weights/`](weights/) | 可复用表数据 |
| `construct` `core` `uxe` `web` `compile.mjs` | **兼容符号链接** → 上表板块 |

## 实践脊（非主产物名）

```bash
node ujs/compile.mjs foo.ujs -o foo.wasm   # 默认 compiler_core
python3 -m ujs ujs2wasm …                  # 构造／对照；非出货必经
./tests/ujs2wasm_compiler.sh
```

不 tip／Draft／签／公开，除非另授。
