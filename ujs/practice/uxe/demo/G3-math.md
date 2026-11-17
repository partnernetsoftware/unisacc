# G3 数学合同（未实现内建）

prd / 计划 G3。本文件只记账；不改 `compiler_min`、`main`、`three`。

## 编译器事实

`compiler_min` 无 `sqrt` / `sin` / `cos`。

左操作数 f64 的 `%` 拒绝(`compiler_min.cx:1021` `f64 mod`)。

已知洞: `i64 % f64` 不拒,发出 `i64.rem_s` 对 f64 栈,wasm 类型不合法;`verify_wasm_bytes` 不拦。裸名 `sqrt` 静默当全局 0.0。不在本文件修 compiler_min。

核内建落地前不得假设这些算子可进核。

## 分工

旋转 / 相机 / 水波数学归 Host（JS / WebGL shader）。

核只交 i64 表（金、舵、计数）。

## 不做

- 不在核内实现 `sqrt`/`sin`/`cos`/`f64 %`。
- 不改 `compiler_min`。
- 不引入 three。

## 状态

未实现内建。
