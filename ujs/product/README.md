# ujs.com — 产品宿主

> 规格：[`../prd.md`](../prd.md) §0.1 · 对齐 `unisacc.com`  
> **开发纪律**：只用 **`unisacc.com` + `.cx`（`-run`）**；产品链禁止 `.c`／`.py`／第三方语言编排。

## 状态

| 代 | 形态 | 状态 |
|---|---|---|
| **host1** | 默认本机原生（`build.cx`） | **可构建** |
| **host1-six** | 六目标原生（`build-six.cx`） | **可构建** |
| **APE** | 五片多形态一包（`build-ape.cx`+`ape-pack.cx`） | **可构建**；本机经 `sh ./ujs/ujs.com` 烟测 |

## 构建（推荐）

```bash
# APE 主产物（对齐 unisacc.com 形）
./unisacc.com -run ujs/product/build-ape.cx
# 仅本机原生（更快迭代）
./unisacc.com -run ujs/product/build.cx
```

`build.cx` 会调用 **本树的 `unisacc.com`** 编译：

- `ujs/product/ujs_main.cx`
- `ujs/seed/stage0/compiler_min.cx`（`-DUJS_HOST`）
  
产品链 **仅 `.cx`**（无 `.c`／`.py`／zig C 种子）。

写出 `ujs/ujs.com`，并烟测 `arith.ujs → \0asm`。

六目标原生（不落仓、默认 `/tmp/ujs-six-out`）：

```bash
./unisacc.com -run ujs/product/build-six.cx
./unisacc.com -run ujs/product/build-six.cx -- /tmp/my-ujs-six
```

## 用法

```bash
./ujs/ujs.com --version
./ujs/ujs.com FILE.ujs -o OUT.wasm
```

覆盖面 = stage0／UJS-1_ship（与 `compiler.wasm` 同脊）。语料合同：

```bash
./unisacc.com -run ujs/product/corpus-host.cx   # 21 PASS / 7 REFUSE
```

更全表仍走实践脊 `compiler_core`／Pages，直至嵌入 `ujs.com`。

## 完成闸

```bash
./unisacc.com -run ujs/product/complete.cx   # APE 重建 + 语料 21/7
```

**已完成产品形**：APE 单文件 + 六目标原生 + stage0 编译合同 + 纯 `.cx` 链。  
**诚实未收**：`compiler_core` 全表嵌入；win/arm64 进 APE 五片布局外的跨机执行收据。

不 tip／不 Draft，除非另授。
