# ujs.com — 产品宿主（host image）

> [`../prd.md`](../prd.md) · [`../plans/v0.0.4.md`](../plans/v0.0.4.md)  
> 只用 **`unisacc.com` + `.cx`**。标签 **`host1_package`**。

## 版本

`ujs.com 0.0.4`（`--version`；APE `--product ujs 0.0.4`）。

## 命令

```bash
./unisacc.com -run tests/ujs/run.cx -- --smoke
./unisacc.com -run tests/ujs/run.cx -- --complete
./unisacc.com -run ujs/product/build.cx
./unisacc.com -run ujs/product/build-ape.cx
./unisacc.com -run ujs/product/build-six.cx
./unisacc.com -run ujs/product/corpus-host.cx
./unisacc.com -run ujs/product/verify-host.cx
./unisacc.com -run ujs/product/wasm-verify.cx -- FILE.wasm
```

## CLI

```bash
./ujs/ujs.com --help
./ujs/ujs.com FILE.ujs              # -> FILE.wasm
./ujs/ujs.com FILE.ujs -o OUT.wasm
./ujs/ujs.com --verify OUT.wasm     # 节／必导出
```

## 诚实边界

- **是**：APE／六目标／host1 预言／**wasm 验面**／stage0
- **不是**：完整 wasm 解释执行；`compiler_core`／IntNet；method emit 另弧

不 tip／Draft，除非另授。
