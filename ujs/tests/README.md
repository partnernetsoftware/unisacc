# UJS 测试套件（产品闸 · host1_package）

> 规格：[`../prd.md`](../prd.md) · 版本：[`../plans/`](../plans/)  
> 纪律：只用 **`unisacc.com` + `.cx`**。  
> **位置**：全部在 **`ujs/tests/`**（不与根 `tests/` 的 unisacc 套件混放）。

## 入口

```bash
./unisacc.com -run ujs/tests/run.cx -- --list
./unisacc.com -run ujs/tests/run.cx -- --smoke
./unisacc.com -run ujs/tests/run.cx -- --complete
```

## 预言与夹具

| 文件 | 角色 |
|---|---|
| [`host1-pass.txt`](host1-pass.txt) | 必须接受（恰好 28）→ [`corpus/`](corpus/) |
| [`host1-refuse.txt`](host1-refuse.txt) | 必须拒绝 → [`neg/`](neg/) |
| [`host1-run-expect.txt`](host1-run-expect.txt) | 跑面金（28；`tag`＋`value`） |

## 验面（0.0.4）／跑面（0.0.5）／日用一条链（0.0.6）

`ujs.com --verify`／`../product/corpus-host.cx`：对 pass 产物做 `\0asm` 节／必导出检查；冒充实文件必须拒绝。  
`ujs.com --run`／`../product/run-host.cx`：迷你 VM 执行 `main_export`，对船载跑金。

**船闸不可达**：根 `tests/ujs2wasm/expect.json`、`run_*.mjs`（archive_only／实践脊）。

## 身份标签

| 标签 | 是什么 |
|---|---|
| `host1_package` | 本套件／`ujs.com` 桌面壳 |
| `m3_scaffold` | `compiler_core`／Pages（另门） |
| `method_construct` | 构造／m4（另弧） |

## 刻意不在本套件

根 [`../../tests/run.cx`](../../tests/run.cx)（**unisacc** MVP）· UXE／npm · 归档 `ujs.sh`。
