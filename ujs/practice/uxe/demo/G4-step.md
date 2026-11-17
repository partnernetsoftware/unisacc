# G4 步进合同（未实现）

prd / 计划 G4。本文件只记账；不改 `compiler_min`、`main`、`three`。

## 运行事实

`run_step_body` 每帧调 `main`（func 1）。

顶层 `let` 每帧清掉。

现靠 globals（`yaw` / `turn` / `wind`）跨帧。

## 缺口

0.0.9 `ujsmeta` 不解决跨帧持久。

需持久 globals 或 state 段。未实现。

## 不做

- 不改 `compiler_min`。
- 不改 `main`。
- 不引入 three。

## 状态

未实现。
