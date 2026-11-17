# G4 步进合同(设计,未实现)

prd U4 / M2 前置。本文件只定案;本提交不改 `compiler_min`。

## 运行事实

`run_step_body` 每帧调 `main`(func 1)。顶层 `let` 每帧清。现页把 yaw 放 JS,再 `host_set_global`。

0.0.9 `ujsmeta` 只列 globals 名,不规定跨 `run_step` 是否保留线性内存里的槽。

## 定案

选**持久 globals**:`ujsmeta.globals` 里的槽跨 `run_step` 保留;`host_reset` 只清句柄堆/scratch,不把这些槽打回 0。不新开 state 段。

### 初始化

顶层 `let yaw = 0` 若每次 main 都跑会清持久槽。定:出现在 `ujsmeta.globals` 的名,**声明初值只在实例创建时写一次**;之后 `run_step`/`main` 再执行同一 `let` 不得覆盖已有槽。无 `let` 的赋值 `yaw = yaw + 1`:编译器必须先把 `yaw` 登记为 global(缺省初值 0),否则夹具编不过。实现时以夹具实编为准,不假设现状已接受未声明名。

### 重置与重放

新局不靠「新 wasm 实例碰巧」。显式 `host_reset_globals()`(或同等 export)把 `ujsmeta.globals` 打回创建初值。金值:(a) 从初态 `run_step` 8 次第 k 次 i64==k;(b) `host_reset_globals` 后再 `run_step` 一次 i64==1;(c) 同输入序列从初态跑两遍,逐帧 i64 相同。核是真相,不依赖实例存活历史。

### 与 host_set_global

输入槽(如 `turn`)每帧由宿主 `host_set_global` 写入;累加槽(如 `yaw`)只核写。`ujsmeta` 增 `in`/`own` 两类(或同等表)。反例:宿主写 `own` 槽须拒绝或忽略,检查断言 yaw 不被宿主写成 99。

## 金值

夹具 `ujs/tests/g4-yaw-n.ujs`(未写):`yaw = yaw + 1; return yaw`。检查 `tools/check-ujs-g4-yaw.cx`(未写):8 帧、reset 回 1、重放一致、持久被改成每帧清零则检查变红。

## 不做

- 本提交不改 `compiler_min` / `host1_vm` / fly。
- 不引入 three。
- 不把三角/sqrt 塞进核(M3)。

## 状态

设计已写。实现与金值未做。

