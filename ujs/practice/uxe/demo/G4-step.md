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

## 红金(2026-11-17)

夹具 `ujs/tests/g4-let-yaw.ujs`:`let yaw = 0; yaw = yaw + 1; return yaw`(tag i64)。
host1_vm `frame-run` 4 帧与 8 帧末值均为 1。
真引擎 chrome 138 dump-dom `demo/g4-let/` 8 次 goldI64:HUD `G4_LET_RED 1,1,1,1,1,1,1,1`。
两种引擎一致为红。`g4-yaw-n.ujs` 无 let 不能当 G4 验收。

## 金值

绿后:`g4-let-yaw` 8 帧 i64==k。检查走 chrome `<title>` 不走 host1_vm。


## 不做

- 本提交不改 `compiler_min` / `host1_vm` / fly。
- 不引入 three。
- 不把三角/sqrt 塞进核(M3)。

## 状态

设计已写。实现与金值未做。

