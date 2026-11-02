# FFI 接桥 + 浮点通道:设计页(2026-11-02,只写 plans)

> 目标:把 `uffi_call` 从"依赖宿主 libffi"改成"走仓里已有的汇编桥",并补上浮点/变参通道。
> 这是一次**构建面变更**(两个 `.S` 进产品对象集),所以和浮点通道合并成一次候选身份扰动。
> 现状前提(实测):`nm unisacc.com.dev/entry.elf | grep -ci bridge` = **0**,桥目前不在产品镜像里;
> `include/unisacc_ffi.h` 也不能读环境变量(cx 拒绝"隐式引入头"),所以开关已由 cx 侧
> `sy_ffi_set_bridge_mode()` 传入(FFI step 1b,已提交)。

## 1. 两个 `.S` 进 entry-build 对象集

`tools/entry-build.cx` 的对象表是 `keys[] / src_rel[]` 两个数组。加两行即可:

```c
keys[N]    = "librarycall.o";   src_rel[N] = "exec/c/librarycall_x86_64.S";
keys[N+1]  = "librarycall.o";   src_rel[N+1] = "exec/c/librarycall_arm64.S";   /* 按 --target 二选一 */
```

这条链路不需要新的编译期特判:挑选规则与既有的"按 target 选源"同一处(entry-build 已有 target 参数)。
Windows 两支(`librarycall_windows_*.S`)不接,记已知缺口(0.0.21 item 4a)。

## 2. `librarycall.h` 怎么进 `include/`

两条路,建议 (a):

- **(a) 复制到 `include/librarycall.h`**:`exec/c/` 是 C 引导侧,`include/` 是 .cx 能看到的头;
  两个 `.S` 与头是同一次构建面变更,复制一份最直白,代价是"两份同名头"要防漂移
  (用 `tools/check-*` 断言行数/符号一致)。
- **(b) 在 `include/unisacc_ffi.h` 里手写原型**(只写 `us_library_bridge_raw` /
  `us_library_bridge_stack_raw` / `us_library_signature`):轻,但原型与 `exec/c/librarycall.h`
  不同步时是静默的 ABI 错。

## 3. 镜像增量(实测,不是估计)

| 目标 | `size` .text | 说明 |
|---|---|---|
| lnx/x86_64 `librarycall_x86_64.S` | **155 B** | `gcc -c -O2` 实测 |
| arm64 `librarycall_arm64.S` | **276 B** | `aarch64-linux-gnu-gcc -c` 实测 |

两个 `.S` 里都有 16 字节栈对齐与 callee-saved 保存,但**不新增数据段**。
所以镜像增量约 **0.15–0.3 KB**,对一个 3 MB 级的产品可忽略;真正的成本是"多一个对象参与链接",
不是字节数。

## 4. 浮点与变参通道(要改汇编)

现状:`librarycall_x86_64.S` 里 `xmm` 出现 **0** 次,`us_library_bridge_raw(entry, args[6], top)`
只传 6 个整数槽;`librarycall.h` 的 `us_library_signature_supported` 还**显式拒绝 variadic**。

### 需要新增的接口

```c
uint64_t us_library_bridge_fp_raw(const void *entry,
                                  const uint64_t args[6], const uint64_t fargs[8],
                                  void *softstack_top, int nfixed, int total);
```

- x86_64 SysV:`xmm0..xmm7` 装 `fargs[0..7]`,`al` = 变参时使用的向量寄存器个数;
  整数槽仍是 `rdi,rsi,rdx,rcx,r8,r9`;第 7 个及以后的参数按 8 字节入栈(已有 `bridge_stack_raw` 承担)。
- arm64 AAPCS64:`d0..d7` 装浮点;`x0..x7` 装整数;变参由寄存器分配自然表达,不需要 `al`。
- 返回:整数走 `rax`/`x0`,浮点走 `xmm0`/`d0`(返回类型由 `ret_kind` 决定)。

### ABI 差异(四目标)

| 目标 | 浮点寄存器 | 变参标记 | 备注 |
|---|---|---|---|
| lnx/x86_64 | xmm0–7 | `al` = 向量寄存器数 | SysV |
| osx/x86_64 | 同上 | 同上 | 同 ABI |
| lnx/arm64, osx/arm64 | d0–7 | 无 | AAPCS64 |
| win/x86_64 | xmm0–3 | `al`(Win64 规则) | **不接**(0.0.21 4a) |
| win/arm64 | d0–7 | 无 | **不接** |

### 测试向量(先用 libffi 路线做预言机)

`sqrt(2.0)`、`strtod("1.25")`、`snprintf("%f", 3.5)`、`getpid()`(对照),
以及混合:`snprintf("%d %f %d", 1, 2.5, 3)`。

## 5. 验证

1. **字节增量**:`stat -c %s` 对比构建前后 entry.elf;记录 `.text` 增量(应 ≈ 155/276 B)。
2. **候选身份链**:构建前后各跑一次 `tools/check-productpkg` 的 `identity-*` 断言,
   确认成员集身份按预期变化且可复现(缓存/会话的 kind 字段仍为 split2)。
3. **逐返回值差分**:同一组向量,在 `UNISA_FFI_BRIDGE=off`(libffi)与 `=force`(桥)下各跑一遍,
   要求**返回值逐字节相同**;浮点用 `memcmp` 比较 8 字节,不用 `==`。
4. **拒绝面**:桥模式下结构体按值 / 128 位 / 变参超限仍必须具名拒绝(沿用 step 1b 的错误串)。
5. **回归**:`check-ffi-bridge` 批次 + `ratchets` + `check-cx-layers` 全绿。

## 6. 风险与顺序

- 最大风险是**汇编层错位**(参数进了错误的寄存器),表现是"返回值对、副作用错"。
  对策:先用只读函数(`sqrt`/`strtod`)验通道,再用有副作用的 `snprintf` 验混合布局。
- 顺序:① 复制头 + 两个 `.S` 进对象集(不接线,镜像应仅增 ~0.3 KB,先验构建链);
  ② 接 `bridge_raw` 到整型/指针路径并做差分;③ 加 `bridge_fp_raw` 与浮点路径;④ 全向量差分。
- 每一步都单独一次候选构建:①之后可以先合入,②③④在一次窗口里做完,避免多次扰动候选身份。
