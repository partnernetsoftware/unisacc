# G1 提交面(条件 import 已合入;host1 符号已提供;尚未真 GPU)

prd 0.3b G1 / W3 同条。

## 核实

`build_mod` 仅源码出现 `host_gpu_submit` 才发 section 2:`env.host_gpu_submit` type `(i32,i32)->()`。fly 无 sid2。

先例:`ship/host-entry.js:210` `host_gpu_submit(ptr,len)` 切 `memory` 再交 `host-browser.js:342`。

## host1

`host1_vm.cx` 接受唯一 import `env.host_gpu_submit(i32,i32)->()`:call 索引 0 为导入,定义函数 idx 减 nimport。实现读线性内存 `[ptr,ptr+len)` 边界检查后 no-op,不崩。**宿主符号已提供;尚未真 GPU。不宣称 G1 完成。**

## 仍未做

(1) 填 UXEP 真包(远超 `MAXSTRLIT=255`)。
(2) 页 `host1Boot` import 对象接真 GPU。
(3) 浏览器 instantiate 与 host1 对齐。

## 状态

条件 import 已合入。词法字节比较,无新 strcmp。fly 无 sid2;`ujs/tests/gpu-submit.ujs` / `gpu-submit-call.ujs` 有 sid2。host1 `--run` gpu-submit-call 不因缺 import trap。批次 `ujs-gpu-import`。G1 未完成(无真 GPU)。



