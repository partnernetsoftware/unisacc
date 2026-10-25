# ujs.com — 产品宿主

> 规格：[`../prd.md`](../prd.md) §0.1  
> **纪律**：只用根 **`unisacc.com` + `.cx`（`-run`）**；禁止产品链 `.c`／`.py`。

## 状态

| 代 | 入口 | 状态 |
|---|---|---|
| **APE** | `build-ape.cx` + `ape-pack.cx` | **主产物形**；本机 `sh ./ujs/ujs.com` 烟测 |
| **本机原生** | `build.cx` | 快迭代 |
| **六目标原生** | `build-six.cx` | ELF／Mach-O／PE → 默认 `/tmp/ujs-six-out` |

## 源（均无 `#include`）

- `ujs_main.cx` — CLI
- `../seed/stage0/compiler_min.cx` — `-DUJS_HOST` stage0 编译器

## 命令

```bash
# 仓库根
./unisacc.com -run ujs/product/complete.cx    # APE + corpus-host
./unisacc.com -run ujs/product/build-ape.cx
./unisacc.com -run ujs/product/build.cx
./unisacc.com -run ujs/product/corpus-host.cx # 25 PASS / 3 REFUSE
```

`build.sh` 仅转调 `build.cx`（兼容旧习惯）。

## 用法

```bash
sh ./ujs/ujs.com --version          # APE 须经 sh
sh ./ujs/ujs.com FILE.ujs -o OUT.wasm
```

## 诚实边界

- **已收**：APE／六目标／`?:`／`switch`／`null??`／`for-of`／`+=`
- **未收**：用户函数、箭头、`...rest`（需多函数或嵌入 `compiler_core`）
- **不冒充**：CX3／CX4 内存安全；跨机切片执行收据

不 tip／不 Draft，除非另授。
