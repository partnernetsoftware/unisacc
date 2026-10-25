# ujs.com — 产品宿主（host image）

> [`../prd.md`](../prd.md) · [`../plans/v0.0.2.md`](../plans/v0.0.2.md)  
> 只用 **`unisacc.com` + `.cx`**。标签 **`host1_package`**。

## 版本

`ujs.com 0.0.2`（`--version`；APE `--product ujs 0.0.2`）。

## 命令

```bash
./unisacc.com -run tests/ujs/run.cx -- --smoke
./unisacc.com -run tests/ujs/run.cx -- --complete
./unisacc.com -run ujs/product/build.cx
./unisacc.com -run ujs/product/build-ape.cx
./unisacc.com -run ujs/product/build-six.cx
./unisacc.com -run ujs/product/corpus-host.cx
```

## CLI

```bash
./ujs/ujs.com --help
./ujs/ujs.com FILE.ujs              # -> FILE.wasm
./ujs/ujs.com FILE.ujs -o OUT.wasm
```

## 诚实边界

- **是**：APE／六目标／host1 预言／stage0（含 arrow／fn+rest 内联）
- **不是**：`compiler_core`／IntNet／Pages 必经；method emit 另弧

不 tip／Draft，除非另授。
