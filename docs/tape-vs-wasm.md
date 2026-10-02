# tape / tapebin 与 WebAssembly 的比较（供改进方向思考，2026-09-30）

> 只写事实与差异，不下“谁更好”的结论；改进方向放在最后一节，作为 R14-7 / FX-1 / FX-3 / v0.2.x 的输入。tape 事实以 [tapebin-v1.md](tapebin-v1.md) 与 `unisa/tape.py` 为准；wasm 事实以核心规范（WebAssembly Core Specification 2.0）、WASI 与组件模型的公开文档为准。

## 1　一张表

| 维度 | tape / tapebin | WebAssembly（核心 + WASI） |
|---|---|---|
| 抽象机 | **寄存器机**：r0–r7 八个 64 位寄存器，r6 帧指针、r7 栈指针（W-16），显式 `.frame/.arg`、`call/callr/callm` | **栈机**：操作数栈 + 局部变量 + 全局；函数有类型签名，调用由 `call/call_indirect` 按签名检查 |
| 指令集规模 | 71 个 opcode（opset=1 冻结）：整数/浮点算术比较、load/store、跳转、调用、宿主调用（`.hostcall/.librarycall/.sys`）、`.print/.write/.exit` | 核心约 170+（不含 SIMD/线程）；无“打印/退出”类指令，一切 I/O 走导入（WASI） |
| 类型 | 值都是 64 位槽；f32/f64 有专门指令；无函数签名、无类型化验证 | i32/i64/f32/f64/v128/引用类型；**每个函数在加载时做类型验证**，未通过不执行 |
| 控制流 | 标签 + `jump/jumpz`（非结构化，允许任意 goto） | **结构化**：block/loop/if/br，无 goto；这是验证器线性时间、编译器可推导 CFG 的根本 |
| 内存 | 由目标平台的原生地址空间承载；tape 本身不定义边界检查（v0.1.3 计划的安全模式才加） | **线性内存** + 每次访问的边界检查（陷阱），是沙箱的核心保证 |
| 模块/接口 | 一个 tape = 整个程序；符号表里有标签与数据名；无导入/导出段（v0.2.0 清单待定） | 模块有 import/export、表、内存、全局、起始函数；组件模型进一步给出接口类型（WIT） |
| 二进制格式 | tapebin v1：64 B 头（magic/major/minor/opset/flags/target/origin_target）、24 B 节目录、SHA-256、NAMES/CONSTS/RECORDS（+可选 SOURCE_SHA/TEXT_EXACT）、S/ULEB128 | 魔数 + 版本 + 分节（type/import/function/table/memory/global/export/start/element/code/data/custom），LEB128；无内建摘要 |
| 确定性 | tapebin 编码规范（同记录 → 同字节），头内摘要；内容寻址靠它 | 执行确定（除 NaN 位模式、宿主导入）；字节不承诺规范化，同一模块可有多种等价编码 |
| 验证/加载 | 结构校验（目录、LEB、shape、引用越界、未知 opcode 拒绝）；**不做**可达性/栈平衡/类型验证（FX-1 验证器待做） | 加载即验证：类型、栈高度、分支目标、内存/表越界常量；验证是规范的一部分 |
| 平台落地 | 同一 tape 由 unisacc 落成六个原生目标（ELF/Mach-O/PE，无汇编器/链接器）或 `-run` 解释；目标无关但 C 前端语义已按 origin_target 绑定 | 由运行时（浏览器、wasmtime、wasmer、WAMR）JIT/AOT 或解释；平台细节全在运行时里 |
| 宿主交互 | `.hostcall/.librarycall/.libraryaddr/.hostaddr/.sys/.sys6`：直接触达宿主 ABI 与系统调用（能力强、边界弱） | 只能通过导入；WASI 给出能力型的系统接口（预打开目录、权限由宿主决定） |
| 语言前端 | C99（本仓）；第二前端是 R14-7 方向 | C/C++（clang/emscripten）、Rust、Go、Zig、AssemblyScript…… |
| 工具链与生态 | 自有：编译器 + 运行时 + 门禁 + 包（v0.2.x）；单文件签名 `.com` | 成熟：多运行时、调试信息（DWARF）、组件注册表（warg）、浏览器原生 |
| 逆向/保密 | 与 `.class`/wasm 同级（plans/ideas.md FX-1 第 4 条） | 同级 |

## 2　同与异的要点

- **相同的取舍**：都把“平台差异”推到最后一步，都用 LEB128 与分节，都把符号/常量集中到表里，都不承诺保密。
- **根本差异一：控制流形状。** wasm 的结构化控制流让验证器与 JIT 都是线性时间，也让“沙箱”成为格式本身的性质；tape 的自由跳转来自 C 前端与 tape 后端的简单性，代价是安全性与可分析性要靠外部验证器（FX-1）与运行时安全模式（v0.1.3）补。
- **根本差异二：类型与验证。** wasm 函数有签名、每条指令有类型规则；tape 把一切当 64 位槽，类型信息留在 C 前端。这使 tape 编码器/解码器极小，但“加载即安全”做不到。
- **根本差异三：宿主边界。** tape 直接有系统调用与宿主 ABI 指令，是它能自己写出六平台可执行文件的原因；wasm 把一切留给导入，是它能跑在浏览器里的原因。两者对应不同的信任模型。
- **tape 独有**：目标无关 + 自落原生（无第三方运行时）、编码规范性 + 内建摘要（内容寻址天然成立）、三阶段自举一致性可延伸到“发布即证明”。
- **wasm 独有**：规范化的验证与沙箱、组件模型的接口类型、庞大的运行时与语言生态。

## 3　可以借鉴的方向（不改变“tape 是内部 IR、wasm 只是第 7 输出目标”的定位）

1. **加载期验证器（FX-1）按 wasm 的粒度写规格**：栈平衡（`.frame/.arg` 与 `call` 的配对）、跳转目标合法、寄存器使用约束、宿主调用白名单——先做成独立、可审计的经典代码（与 v0.1.3 检查器同一原则：验证不进权重）。
2. **模块化清单（v0.2.0）借 import/export 的形状**：包清单里显式列出导入的宿主能力（对应 `.hostcall/.sys` 的白名单）与导出的入口/接口，运行时按清单强制（v0.2.2 权限沙箱）；接口描述可参考 WIT 的“接口类型”思路，但只做 C ABI 能表达的子集。
3. **结构化控制流不必强加给 tape**，但可在 tape → wasm 后端（FX-3）里做 relooper/stackifier；反过来，若 v0.1.3 的检查器需要 CFG，先在 tapebin 侧记录基本块边界（可选节），不改 opset。
4. **内存边界**：v0.1.3 步骤 1 的胖指针安全模式与 wasm 的线性内存边界检查是同一类保证的两种实现；写清楚 tape 安全模式的语义（陷阱点、错误报告格式）时对照 wasm 的 trap 语义，便于论文 D 的比较段。
5. **确定性与内容寻址是 tape 的强项**：v0.2.x 的包按 tapebin 内容哈希寻址，wasm 生态里这一步是靠外部注册表补的；保持“编码即规范”不被后续可选节破坏（可选节也要规范排序）。
6. **不借的**：栈机、结构化控制流作为 IR 形状、把 I/O 全部改成导入——这些会推翻 S-17 已验证的表/网络与六目标后端，不在 0.1.x 之内。

## 4　与本仓计划的对应

| 主题 | 条目 |
|---|---|
| tapebin v1 格式与门禁 | archive/plans/v0.0.14.md R14-7 落点 A |
| 验证器（可达性、栈平衡、权限） | plans/ideas.md FX-1；v0.1.x 探索区 |
| tape → wasm 第 7 目标 | plans/ideas.md FX-3；plans/v0.1.x.md v0.1.3 及以后 |
| 运行时安全模式、检查器 | plans/v0.1.x.md v0.1.3；research/paper-d-intent.md |
| 包清单、权限、内容寻址 | plans/v0.1.x.md v0.2.x |
