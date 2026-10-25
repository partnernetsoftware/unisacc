# UJS — 开箱

> 规格：[`prd.md`](prd.md) · **版本**：[`plans/`](plans/)（**0.0.4**；下一刀 [`plans/v0.0.5.md`](plans/v0.0.5.md)）  
> **`ujs/` 内无 `.c`／`.py`**；遗留：[`../archive/ujs-legacy-py-c/`](../archive/ujs-legacy-py-c/)  


> 地图：[`ARCHITECTURE.md`](ARCHITECTURE.md) · Paper B：[`../research/ujs-paper.md`](../research/ujs-paper.md)

## 出货：`ujs.com`（host image）

**ujs+wasm 生产力工具**。产品链只用 `unisacc.com` + `.cx`。  
这是 **host image**（标签 `host1_package`），不是 M3／`compiler_core`，也不是构造 IntNet。

```bash
# 仓库根 — 套件
./unisacc.com -run ujs/tests/run.cx -- --smoke
./unisacc.com -run ujs/tests/run.cx -- --complete

# 或直接产品闸
./unisacc.com -run ujs/product/build.cx
./unisacc.com -run ujs/product/complete.cx

./ujs/ujs.com --version          # ujs.com 0.0.4（原生）；APE 用 sh ./ujs/ujs.com
./ujs/ujs.com --help
./ujs/ujs.com FILE.ujs           # 默认写出 FILE.wasm
./ujs/ujs.com FILE.ujs -o OUT.wasm
./ujs/ujs.com --verify OUT.wasm  # host1 \\0asm 结构验面
```

预言：[`tests/host1-pass.txt`](tests/host1-pass.txt)（28）+ [`tests/host1-refuse.txt`](tests/host1-refuse.txt)。  
套件：[`tests/`](tests/)（**在 `ujs/` 下**，不与根 `tests/` 混放）。  

`expect.json`／Node 探针为 archive_only，船闸不打开。

## 目录

| 路径 | 角色 |
|---|---|
| [`product/`](product/) | 出货宿主 |
| [`tests/`](tests/) | **host1 产品套件**（不在根 `tests/`） |
| [`plans/`](plans/) | **0.0.x 版本计划** |
| [`seed/stage0/`](seed/stage0/) | stage0 编译器 |
| [`iterate/`](iterate/) | M3 实践脊（另标签） |
| [`practice/`](practice/) | Pages／UXE |
| [`seed/construct/`](seed/construct/) | 构造臂（P1 后置） |

## 实践脊（非 host image 品牌）

```bash
node ujs/compile.mjs foo.ujs -o foo.wasm   # compiler_core；m3_scaffold
```

不 tip／Draft／签／公开，除非另授。
