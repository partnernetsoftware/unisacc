# ujs.com — 产品宿主（host image）

> [`../prd.md`](../prd.md) · [`../plans/v0.0.7.md`](../plans/v0.0.7.md)  
> 只用 **`unisacc.com` + `.cx`**。标签 **`host1_package`**。

## 版本

`ujs.com 0.0.7`（`--version`；APE `--product ujs 0.0.7`）。host1 编／验／跑语义与 0.0.6 相同。0.0.7 新增的是 lex 扫描器切片，不是 IntNet，也不是其余九网。

## 命令

```bash
./unisacc.com -run ujs/tests/run.cx -- --smoke
./unisacc.com -run ujs/tests/run.cx -- --complete
./unisacc.com -run ujs/product/build.cx
./unisacc.com -run ujs/product/build-ape.cx
./unisacc.com -run ujs/product/build-six.cx
./unisacc.com -run ujs/product/corpus-host.cx
./unisacc.com -run ujs/product/run-host.cx
./unisacc.com -run ujs/product/cli-host.cx
./unisacc.com -run ujs/product/lex-host.cx
./unisacc.com -run ujs/product/verify-host.cx
./unisacc.com -run ujs/product/wasm-verify.cx -- FILE.wasm
```

## CLI

```bash
./ujs/ujs.com --help
./ujs/ujs.com FILE.ujs              # -> FILE.wasm
./ujs/ujs.com FILE.ujs -o OUT.wasm
./ujs/ujs.com --verify OUT.wasm     # 节／必导出
./ujs/ujs.com FILE.ujs --run        # 同进程编并跑
./ujs/ujs.com FILE.ujs -o OUT.wasm --run
./ujs/ujs.com --run OUT.wasm        # host1 迷你 VM
./unisacc.com -run ujs/product/cli-host.cx
```

## 诚实边界

- **是**：APE／六目标／host1 预言／验面／**跑面**／stage0；0.0.7 lex 扫描器切片（`lex-host.cx`）
- **不是**：完整 wasm 引擎；`compiler_core`／IntNet；其余九张构造网；method emit 另弧

不 tip／Draft，除非另授。
