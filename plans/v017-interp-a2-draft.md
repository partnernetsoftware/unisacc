# v0.1.7 解释器 A2 原型草案: 16B 预解码表、Direct Threading 与基本块预算

- 基点提交: `8d9802c64dfea5dc484882beab7171ae986573c4` (`tools/tape/asm/tape_x86_64.S`, A1 已过 22/22 明文差异与 6/6 替代体击杀)
- 补丁草案文件: `.work/interp-a2-draft.patch` (基于 `8d9802c64` 的 unified diff 原型草案)
- 状态声明: 本文与草案均在 `v0.1.5` 冻结期完成静态设计与编写, **未执行汇编、链接或运行**, 全部运行态证据与性能指标均为 **`UNMEASURED / UNCOVERED`** (待解冻后实测验证).

## (1) A2 核心数据结构与零重定位约束

### 1.1 16B 定长预解码槽 (`a2_insn_t`, 零绝对指针)
每条指令或基本块检查点编码为固定 16 字节 (128 bit), 全程无任何绝对地址, 100% 满足 `tapemode.so` 零重定位 (`SHT_RELA`/`SHT_REL`/`DT_RELR` 全无) 铁律:

| 偏移 | 类型 | 字段 | 语义 |
|---|---|---|---|
| `0..3` | `int32_t` | `disp` | 相对 `.La2_base` 的 PIC 32 位 handler 偏移 (`handler = .La2_base + (int64_t)disp`) |
| `4` | `uint8_t` | `r0` | 目的/第一寄存器号 (`0..15`) |
| `5` | `uint8_t` | `r1` | 第二寄存器号 (`0..15`) |
| `6` | `uint8_t` | `r2` | 第三寄存器号 (`0..15`) |
| `7` | `uint8_t` | `rem` | 若为 `BB_CHECK` 伪槽: 块内指令数 `L` (`1..255`); 若为块内普通指令: 从本指令(含)到块尾剩余指令数 (`L..1`, 用于精确故障退款 `refund = rem - 1`) |
| `8..15` | `int64_t` | `imm` | 立即数/访存偏移; 对 `BB_CHECK` 与慢桥接槽存块首/本条原始 `CODE` 字节偏移; 对预解析的 `jump`/`jumpz`/`call` 存目标槽在预解码表内的字节偏移 (`target_slot_idx * 16`) |

配套 4 字节原始偏移旁表 `orig_pc_off[slot_idx]` (`uint32_t`), 仅在故障/慢桥接时反查 `result.pc`, 热循环零访问.

### 1.2 预解码缓冲与向后兼容边界
- **零 ABI 破坏内联边界 (原型默认)**:
  - 当指令总数 + 基本块头伪槽总数 `<= 2048` 时, 使用入口局部缓冲 (`2048 * 16B = 32768B` 预解码表 + `2048 * 4B = 8192B` 旁表, 总栈增量 `40960B`, 仍为 16B 倍数);
  - 若 `CODE` 超过 `2048` 槽 (或任一指令偏移超过 `uint32_t`), 入口零开销回退既有 A1 主循环 (`.Lrun_next`), 100% 兼容现有 12 槽 `args[0..11]` ABI.
- **v0.1.7 正式 ABI 扩展接缝 (后续)**:
  - 可在 `tapemode-abi` 新增可选 `args[12]` (`a2_buf`) / `args[13]` (`a2_cap`), 支持超大 `CODE` 与跨调用复用预解码缓存.

## (2) 单趟预解码 + 基本块切分

在 `tape_run_x86_64` 入口 `tape_bitmap_x86_64` 验证 `CODE` 合法之后, 增加轻量单趟预解码 `a2_predecode`:
1. **Pass 1 (标定基本块首与指令索引)**:
   - 指令 0 (或 `entry_off`) 为基本块首;
   - 所有 `jump(27)`/`jumpz(28)`/`call(29)` 的合法目标偏移标为基本块首;
   - 所有终结/改变控制流指令 (`jump`, `jumpz`, `call`, `callr`, `callm`, `ret`, `exit`, `sys`, `sys6`, 保留 opcode `32..36,39..41,48..71`) 之后的下一条指令标为基本块首;
   - 单个直落基本块达到 `255` 条指令时强制切分新基本块首 (保证 `uint8_t rem` 永不溢出).
2. **Pass 2 (发射 16B 槽与 `BB_CHECK` 伪槽)**:
   - 每个基本块首之前发射 1 条 `BB_CHECK` 伪槽: `disp = .La2_bb_check - .La2_base`, `rem = L` (本块指令数 `1..255`), `imm = bb_orig_code_off`;
   - 块内第 `i` 条指令 (`0 <= i < L`) 写入 `rem = L - i`, `orig_pc_off[slot] = insn_code_off`;
   - 热指令 (`imm`, `mov`, `add/sub/mul/xor/and/or`, `shl/sar/shr`, `sdiv/srem/udiv/urem`, `slt/sle/ult/ule/eq/ne`, `load64`, `store64`, `jump`, `jumpz`, `call`, `ret`, `frame`, `exit`, `nop`) 写入各自 A2 Direct Threading handler 偏移;
   - `jump`/`jumpz`/`call` 的合法目标偏移解析为目标基本块 `BB_CHECK` 槽的字节偏移 (若目标非法/非指令边界, 写入 `.La2_bad_target - .La2_base`, 运行到该条时触发 `fault_reason=3 (code)` 与原 PC, 绝不在预解码期提前拒绝不可达的坏分支!);
   - 其余指令 (`loadn`/`storen`/`zero`/`lea`/`callr`/`callm`/`sys`/`sys6`/`argc`/`argv`/保留 opcode) 写入 `.La2_slow_bridge - .La2_base` 与 `imm = insn_code_off`, 统一桥接至既有 A1 慢路径;
   - 末尾追加 1 条哨兵槽: `disp = .La2_code_end - .La2_base`, `rem = 1`, `imm = code_len` (消除主循环每次 `cmpq %r15,%rbx` 边界检查).

## (3) Direct Threading 与基本块预算精确语义证明

### 3.1 寄存器分配 (SysV x86_64)
- `%rbx`: 当前 16B 预解码槽指针 (`a2_insn_t *`)
- `%rbp`: 剩余预算 (`remaining_budget`, 单一来源 `init_budget` 存栈槽 `65744(%rsp)`)
- `%r11`: 常驻 `.La2_base(%rip)` (热指令不调用外部函数, 零重载; 仅在 `.La2_slow_bridge` / 映射访存回调返回后重载 `leaq .La2_base(%rip),%r11`)
- `%r12`: `args` 指针
- `%r13`: `result` 指针
- `%r14`: `CODE` 原始字节流基址 (保持与 A1 一致, 供回退 A1 与故障偏移计算)
- `%r15`: 预解码表基址 `a2_table_base` (用于 `slot_idx = (%rbx - %r15) >> 4` 反查 `orig_pc_off`, 回退 A1 时重载 `%r15 = code_end` 从栈槽 `65760(%rsp)`)

### 3.2 Direct Threading 尾部直跳宏 (`A2_NEXT`)
每条块内热指令 handler 末尾直接推进并分发, 无共用循环头、无预算检查、无 `CODE` 边界检查:
```asm
.macro A2_NEXT
    addq $16,%rbx
    movslq 0(%rbx),%rax
    addq %r11,%rax
    jmp *%rax
.endm
```

### 3.3 基本块预算 (`BB_CHECK`) 与精确状态等价证明
1. **正常直落 (`%rbp >= L`)**:
   - `.La2_bb_check` 读取 `L = 7(%rbx)`, 若 `%rbp >= L`, 一次性 `subq %rax,%rbp`, 直接 `A2_NEXT` 进入块内第一条指令.
   - 块内 `L` 条指令全部正常执行完 (包括在块尾执行 `jump`/`jumpz`/`call`/`ret`/`exit`) 时, 实际执行步数恰好为 `L`, 与提前扣减的 `L` 相等, 零误差.
2. **块内中途提前退出或故障 (`fault` / `exit` / 除零 / 访存越界 / 非法目标 / 栈溢出)**:
   - 设故障发生在块内第 `k` 条指令 (`1 <= k <= L`), 该槽的 `rem = L - k + 1` (含本条).
   - 故障指令本身按 A1 规则已消耗 1 步 (例如除零 `v19_div0_fault` 计 `steps=3`, 非法目标 `v14_bad_branch_target` 计 `steps=1`, 保留 opcode `v16_op48_reserved` 计 `steps=1`), 尚未执行的后续指令数为 `refund = rem - 1 = L - k`.
   - 在 `.La2_fault` 统一执行:
     - `movzbl 7(%rbx),%ecx; decq %rcx; addq %rcx,%rbp` (将未执行的 `L - k` 步精确退回 `%rbp`);
     - `slot_idx = (%rbx - %r15) >> 4`, 从 `orig_pc_off[slot_idx]` 取出精确 `orig_pc`, 写入 `24(%r13)` (`result.pc`);
     - 转入既有 `.Lrun_status` -> `.Lrun_return`, 由 `init_budget - %rbp` 精确结算 `result.steps`.
   - 若故障前该指令尚未计步 (仅限入口容器拒绝与 `CODE` 末尾哨兵 `.La2_code_end`): 哨兵槽在 `BB_CHECK` 之外独立存在, `.La2_code_end` 先判 `testq %rbp,%rbp; jz .La2_budget_at_end`, 再报 `fault_reason=3 (code)` 与 `pc = code_len`, 严格保持 `v08a`/`v08b` 优先级.
3. **预算将在本块耗尽 (`%rbp < L`, `.La2_exact_fallback`)**:
   - 不做 `subq %rax,%rbp`;
   - 读取 `bb_orig_off = 8(%rbx)`, 恢复 `%rbx = %r14 + bb_orig_off` 与 `%r15 = code_end` (`65760(%rsp)`), 直接 **`jmp .Lrun_next`** 切入 A1 逐条精确状态机!
   - 因为 `budget=0`、`budget=1`、`budget=mid` 与 `v08b` (`budget == remaining` 且恰达 `CODE` 末尾) 全部落入 `%rbp < L` 慢回退路径, 它们 100% 复用已验证的 A1 逐条引擎, 在数学上保证精确状态等价.

## (4) 解冻后精确状态差异验证计划 (复用 22-vector + A2 专属扩展)

**注意: 以下验证计划在 `v0.1.5` 冻结期禁止执行, 当前状态为 `UNMEASURED / UNCOVERED`.**

1. **复用既有 22 向量明文差异 + ABI 哨兵套件 (`.work/check-tape-interp-diff.cx`)**:
   - 保持同一判分入口与 SHA-256 绑定 (`BASE 85d271c2d` vs `A1 8d9802c64` vs `A2`), 逐字节比对 `host_ret`、`abi_clobber=0x0`、`status/exit/steps/pc/flt` 五字段与 64KB 线性内存 `SHA-256`, 要求 22/22 `MATCH`.
2. **新增 8 个 A2 专属边界向量**:
   - `a2_v20_bb_mid_div0`: 10 指令基本块的第 4 条触发除零, 验证退款 `refund = 10 - 4 = 6` 后 `steps` 与 `fault_pc` 与 BASE 完全一致.
   - `a2_v21_bb_mid_mem_fault`: 8 指令基本块的第 3 条访存越界, 验证退款与 `fault_reason=2`, `fault_pc` 一致.
   - `a2_v22_bb_255_split`: 构造 300 条直落 `add` 指令 (> 255 强制切分 2 个基本块), 分别用 `budget=254`, `255`, `256`, `300`, `301` 对比 BASE 精确一致.
   - `a2_v23_unreachable_bad_jump`: 含不可达的非法偏移 `jump 1` (绕过它正常 `exit`), 验证 A2 预解码不误拒容器, 与 BASE 一致正常退出.
   - `a2_v24_exact_fallback_mid_loop`: 循环体 6 指令, 在最后一次迭代剩 `budget=3 < 6` 触发 `.La2_exact_fallback` 切回 A1, 验证 `status=3`, `steps`, `pc` 与 BASE 一致.
   - `a2_v25_slow_bridge_mmap_return`: 基本块内触发 `sys6(mmap)` -> 慢桥接 -> 返回 A2 继续执行映射访存与算术, 验证 `%r11`/`%r15`/`%rbp` 全完好.
   - `a2_v26_mode5_call_ret_a2`: `mode=5` 6 参数进入 A2 非 0 `entry_off`, 验证入口定位到正确 `BB_CHECK` 槽且顶层 `ret` 正确.
   - `a2_v27_large_code_fallback_a1`: 构造 `> 2048` 条指令的合法容器, 验证自动无损回退 A1 并与 BASE 全字段一致.
3. **新增 2 个 A2 专属同入口替代体 (叠加原有 6 个, 共 8/8 必杀)**:
   - `M7_a2_no_fault_refund`: 去掉 `.La2_fault` 中的 `refund = rem - 1` 退款 -> 被 `a2_v20_bb_mid_div0` 必杀 (`steps` 多扣 6 步).
   - `M8_a2_skip_exact_fallback`: 把 `.La2_bb_check` 的 `jb .La2_exact_fallback` 改成直接 `jb .Lrun_budget` (在块首直接耗尽而非精确走到块内第 `rbp` 条) -> 被 `v07_budget_mid` 与 `a2_v24_exact_fallback_mid_loop` 必杀 (`steps` 与 `pc` 失配).
4. **零重定位与性能实测门禁 (仅解冻且 `load1 < 4.0` 后执行)**:
   - `tools/tape-sidecar-build.cx --check` 与 `tools/check-tape-sidecar-build.cx` (`21/21`, 零重定位/无 `DT_NEEDED`/无 W+X/确定性 SHA-256);
   - 独立微基准按前后双采 `load1 < 4.0`、独立热身 + 5 轮稳态中位数分别报告 `BASE` / `A1` / `A2` 的 `ns/step` 与 `Msteps/s`.
