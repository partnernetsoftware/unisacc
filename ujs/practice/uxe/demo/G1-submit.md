# G1 提交面(gpu-tri 真 GPU;批次 ujs-gpu-tri)

prd 0.3b G1 / W3 同条。

## 核实

`build_mod` 仅源码出现 `host_gpu_submit` 才发 section 2:`env.host_gpu_submit` type `(i32,i32)->()`。fly 无 sid2。

先例:`ship/host-entry.js:210` `host_gpu_submit(ptr,len)` 切 `memory` 再交 `host-browser.js:342`。

## host1

`host1_vm.cx` 接受唯一 import `env.host_gpu_submit(i32,i32)->()`:call 索引 0 为导入,定义函数 idx 减 nimport。实现读线性内存 `[ptr,ptr+len)` 边界检查后 no-op,不崩。**宿主符号已提供;尚未真 GPU。不宣称 G1 完成。**

## 写包原语(设计,未实现;先与 ccapi 同步再改 compiler_min)

(1) 不选 `host_mem_write` 新 import。host1 现只认唯一 import `env.host_gpu_submit`;再加 import 要改 host1_vm 与 fly 无 import 合同。

(2) 选语言内建 `store_bytes(list) -> i64 ptr`。`list` 为 i64 元素 0..255。实现把字节写入**生成模块**线性内存窗口 `HOST_SCRATCH+65536`(=1015808),长度 `len(list)`。然后 `host_gpu_submit(ptr, n)`。

(3) 上限:第一刀 `n<=4096`。一三角 UXEP v1 约百余字节,远超 `MAXSTRLIT=255`,远小于 128 页内存。超限编译或 emit 失败,不截断。

(4) 所有权:窗口属 wasm 实例线性内存,编译器 `g_heap` 不管。Host(host1 / host1Boot)只读 `[ptr,ptr+len)`。`HOST_SCRATCH=950000` 仍给 `host_scratch` / `host_set_global` 注入(64KiB)。写包窗口与注入窗口不相交。同帧先 `store_bytes` 再 submit;下一帧覆盖同一窗口。

(5) 不做:真 GPU、UXEP 全字段编码器、第二 import、把包塞进字符串字面量。

(6) 下一刀验收(未做):最小 `.ujs` 组 1 三角 UXEP 字节 list,`store_bytes`+`host_gpu_submit`;宿主 no-op 记 len 与校验和;`.cx` 与 AUTO 同期望。未实现前不得记 G1 完成。

## gpu-tri 真 GPU 验收口径

范围:单三角、212B UXEP v3、headless Chrome 138、SwiftShader。FNV/KR/ER 从 hex。检查 title 判定+oracle_fnv 反例+expect_er 反例。

期望中心色:Lambert(环境+两灯,法线=normalize(eye),soft from roughness)+金属混合。**不含** shader rim(`pow(1-ndv,2.4)`)与 ink;容差 24 吸收 rim/量化。灯方向与颜色从包 lights 字节读,不写死。

## 仍未做

(1) M2 核驱动 fly(需 G4)。
(2) 无 chrome:UNMEASURED 退出 2,不是 PASS。

## 状态

M1 收口。批次 `ujs-gpu-tri` PASS。U1/C3 按此范围完成。







