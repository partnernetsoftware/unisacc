CX结构化主线：以module/use、公开接口、私有实例、显式构建输入及生命周期合同组织编译器；原生目标是结构化类型/符号/ABI与所有权接口，而非将include藏进工具。

**论文归属：** 模块化与内存安全叙事归 **Paper D**（[`research/paper-d-intent.md`](../research/paper-d-intent.md)）；不塞进 Paper A。对外条款 [`spec.md`](spec.md)：语言 §1–§7，目录架构 §13（`CX-S-1`…`CX-S-10`）。索引在 [`prd.md`](prd.md)。现行产品阅读见 [`docs/cx/prd.md`](../docs/cx/prd.md)。

- `mem/README.md`：内存库（heap/bytes/arena/grow/slice，Rust 所有权/借用体系）。
- `text/README.md`：文本库（text/build/deflate，Rust str / Node Buffer 体系）。
- `sys/README.md`：系统对接库（env/fs/proc/time/net/posix/mem/dl/sha/guard/ffi/poll/tty，libc/POSIX 边界）。
- `cx0/README.md`：权威CX解析器、经典C兼容投影、不可变单元束与E2资源适配。
- `workspace/README.md`：包图、构建、严格续构、声明资源目录闭包与每次真实执行测试。
- `../tools/cx-driver.md`：源码优先入口，默认编译并运行，调用实际七流网络。
- `../tools/cx-pipeline.md`：逐阶段边界、独立units/tapecheck适配及模型身份核验。
- `core/memory.md`：按实例拥有的缓冲区、move/drop及共享/独占借用单元；默认运行138/0，经新CX阶段链编译后实跑也138/0，不能推为任意CX的编译期内存安全。

已安装产品仍用原驱动，CXIF v0文本包含仍是迁移桥；结构化原生接口直接进入E3、编译期所有权和产品切换仍需实现与验收。旧C只有在生产/构造/测试全部不再读它后才能归档；目前src/cxmod.c是活的生成兼容输入，exec/c执行核仍服务新链。

运行库切片已解决驱动自身编译超时：显式声明树+唯一native provider、最小process模块让新链在原45秒/阶段12秒预算内生成217031B静态driver，该产物再执行native5/0和原driver14/0。接口构造器注册只留provider，独立共享状态/单次构造3/0。见`runtime/README.md`。这些证据绑定旧Rootda4e的模型材料；新装Root94的候选资格另行实跑，不转贴。旧E2/E4超时留存，约0.6MB输出含大量空白，不全是有效代码。

以下owned原型冻结为i64句柄示例，新通用owner/borrow能力在core/memory演进。

Bounded runtime-owned i64 buffer prototype, not compile-time Rust ownership.
Import owned with #pragma unisa use owned. No source #include or pointer out-parameter.
All public operations return owned_result {status,value}; errors set value=0.
new accepts lengths 0..8, zero initializes, and returns a handle; four slots maximum.
length/get return a value; set/drop return value 0. get/set validate handle before index.
move returns a new generation and invalidates all old handle copies; drop invalidates them.
Each slot admits generations 1..255. No wrap: move refuses at 255, retaining the valid owner;
drop retires that slot permanently. new distinguishes full capacity from free retired slots.
Failure changes no live buffer or valid handle. Inputs are checked before any array access.
All storage and implementation helper functions are module-static.
Single-threaded only; no reset, resizing, general heap allocation, or borrow checker.
Handles are ordinary copyable integers: copies alias a live object and guessed live handles can
access it. This is runtime stale-handle/bounds isolation, not linearity, secrecy, or authority.
Arbitrary C, malicious ABI declarations, and data races are outside this module contract.
Run `sh ./unisacc.com cx/check_owned.cx` under a caller-owned <=30 second watchdog.
This module is a reusable bounded runtime facility. It is not yet used by compiler
internals or the construction coordinator. `make owned` is its development entry;
the complete new suite still reports NOT_READY independently of this local batch.
