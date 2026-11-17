# G1 提交面（未实现）

prd 0.3b G1 / 计划 W3 同条。本文件只记账；不改 demo 页。禁区：`compiler_min`、`main`、`three`。

## 现状

`build_mod`（`compiler_min.cx:2268`）无 import 段，仅 export。现 wasm 无 import，可 instantiate。

兑现目标：`env.host_gpu_submit(i32 ptr, i32 len)`。先例：`archive/ship` 胶水 `host_gpu_submit(ptr, len)`（切片 `gameMem.buffer` 再交给 Host 对象形 `host_gpu_submit`）。

## host1Boot

现 wasm 仍无 import 可 instantiate。一旦核侧加 import，`host1Boot` **必须**补 import 对象（`env.host_gpu_submit`），否则 instantiate 失败。本条未做。

## 不做

- 不改 fly / water / ink / oneshot / ghibli 等 demo 页（已调页侧 `host_gpu_submit`，失败回退）。
- 不改 `compiler_min`。
- 不引入 three。

## 状态

未实现。
