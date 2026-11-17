# G4 步进合同(设计,未实现)

prd U4 / M2 前置。本文件只定案;本提交不改 `compiler_min`。

## 运行事实

`run_step_body` 每帧调 `main`(func 1)。顶层 `let` 每帧清。现页把 yaw 放 JS,再 `host_set_global`。

0.0.9 `ujsmeta` 只列 globals 名,不规定跨 `run_step` 是否保留线性内存里的槽。

## 定案

选**持久 globals**:`ujsmeta.globals` 里的槽跨 `run_step` 保留;`host_reset` 只清句柄堆/scratch,不把这些槽打回 0。不新开 state 段(少一种运行时布局,M2 帧循环先用现有 yaw/turn)。

## 金值

夹具 `ujs/tests/g4-yaw-n.ujs`(未写):`yaw = yaw + 1; return yaw`。宿主连续 `run_step` 8 次,第 k 次 `i64==k`。检查 `tools/check-ujs-g4-yaw.cx`(未写)。

## 不做

- 本提交不改 `compiler_min` / `host1_vm` / fly。
- 不引入 three。
- 不把三角/sqrt 塞进核(M3)。

## 状态

设计已写。实现与金值未做。

