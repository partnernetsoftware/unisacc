# G1 提交面(未实现)

prd 0.3b G1 / W3 同条。

## 核实

`build_mod`(`compiler_min.cx:2268-2410`)只发 section 1/3/5/7/10,无 section 2。export 27。type 11 已是 `(i32,i32)->()`。`NFUNCS=33`。

先例:`ship/host-entry.js:210` `host_gpu_submit(ptr,len)` 切 `memory` 再交 `host-browser.js:342`。

## 最小刀(未做)

(1) 仅源码出现 `host_gpu_submit` 才发 import `env.host_gpu_submit` type 11。无条件加 import 会让现页 `host1Boot` 裸 instantiate LinkError,并打 ujs2wasm 字节基线。
(2) import 占 idx 0 后,所有 call/EXP 绝对索引 +1(`FI(n)`)。`run_step_body` 的 `call 1` 改 `call 2`。
(3) `host1_vm.cx` 须记 import 数并折算,否则 `--run` 静默跑错函数。
(4) 真包远超 `MAXSTRLIT=255`;声明+call 与填 UXEP 是两刀。

## 状态

未实现。不改 compiler_min 本轮。

