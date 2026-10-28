# CX0 的首个独立 CX 单元

`plan.cx` 已由现行 `src/cxmod.c` 提取为 CX 模块。原 C 解析语义暂作迁移基线；所有解析函数与状态在模块内私有，只公开 `cx0_result`、`cx0_plan` 和 `cx0_release`。调用方通过 `#pragma unisa use` 使用，不 include 实现，不访问 `cxm_*` 全局变量。

这是一份**已运行的提取实现**。新 `bundle.cx` 已将规划时观察到的源码/接口及显式资源封存为不可变 `CX0BND1`，独立 E2 适配器已实际消费并产生 UNIPP1；[字节协议、API 与限制](BUNDLE.md) 给出独立复现方法。U1/U2 尚未完成：已安装产品驱动仍调用原 C 规划器，完整配置/依赖映射、规范逻辑 ID、完整 CX0/E2 合同和 CX0 模型化部署仍待实现。当前生产产品仍为七个模型流。

## 接口及所有权

```c
cx0_result cx0_plan(int interface_only, char **sources, int count,
                   char **forced, int nforced, const char *directory);
void cx0_release(cx0_result *result);
cx0_result cx0_capture_plan(char **sources, int count, const char *directory);
```

源数组、强制头数组和字符串只在本次同步调用期间借用。结果的 `text` 和 `diagnostic` 为调用者拥有的分配；使用完必须 `cx0_release`，释放后字段清零。结果可跨另一次调用保留。`directory` 必须是调用者明确提供的现存目录，不读取 TMPDIR 隐式选择接口位置。

- `status=0`：接受，输出 `CX0PLAN1\n` 加原逐单元 TSV；接口模式输出 `CX0IF0\n` 加 CXIF v0 声明文本。
- `status=1`：规划拒绝，例如模块缺失、循环、公开冲突、条件化依赖和模块定义 main。
- `status=2`：无效 API 输入、文件/权限/接口发布故障或工作进程协议故障。只有 ENOENT 归缺失；ENOTDIR、EACCES 和目录输入单列 IO 错误。
- 任何失败均不返回成功计划字节。已生成的正确接口缓存文件可保留，不把缓存的存在记作规划通过。

首版使用 POSIX 工作进程隔离解析状态，父进程不执行旧解析器，不保留其可写全局；结果通过有长度上限的内部管道返回。它只面向单线程 POSIX 调用者。Windows、任意线程并发、Rust 编译期所有权和任意 C99 安全性均未获资格。`cx0_capture_plan` 是束生产者使用的二进制读取观察入口，内部管道不作为公开束协议；公开束格式独立固定在 `CX0BND1`。

接口通过同目录临时文件和无覆盖 link 发布，正文不匹配的现存文件拒绝。当前随带 unistd 没有公开 link 包装，CX 适配器复用 `resourceledger.h` 已有的 syscall 绑定方式，宿主 GCC/Clang 使用系统 link。这不是新依赖清零证据。

## 运行与复现

从仓库根运行；每条命令单独用 `tools/bound` 约束后代。所有输出目录在 `out/` 下，测试目录须不存在；接口目录由调用者制备。

```sh
tools/bound 55 sh ./unisacc.com tools/cx0-extract.cx -- --check
tools/bound 55 sh ./unisacc.com tests/preprocess/cx0.cx -- /ABS/ROOT /ABS/ROOT/out/cx0-native-test
tools/bound 55 sh ./unisacc.com tools/cx0.cx -- plan /ABS/INTERFACES tests/cxhost_main.cx
```

用 grk 的宿主入口只编译，再单独运行程序；不要求它们支持 `-run`：

```sh
tools/bound 55 sh ./gcc_cx.com tests/preprocess/cx0.cx -o out/cx0-gcc-test
tools/bound 30 out/cx0-gcc-test /ABS/ROOT /ABS/ROOT/out/cx0-gcc-cases
tools/bound 55 sh ./clang_cx.com tests/preprocess/cx0.cx -o out/cx0-clang-test
tools/bound 30 out/cx0-clang-test /ABS/ROOT /ABS/ROOT/out/cx0-clang-cases
```

原 C 迁移基线或 `plan.cx.in` 适配器改变后，先运行 `tools/cx0-extract.cx` 再 `--check`，不得手改生成的 `plan.cx`。生成器只转移可见性、SHA API、显式接口目录和 IO 错误传播，保留解析算法；生成头记录输入 C 文件 SHA。GCC/Clang 产物仍为普通 C 产物。

## 本轮证据

同一最终模块在本机 Linux x86_64 三条路径实跑 24/0：unisacc 默认运行、GCC `-o` 后运行、Clang `-o` 后运行。包含手写最小计划、公开/私有声明、直接/传递依赖、假 pragma、循环/冲突/条件拒绝、缺失/ENOTDIR/EACCES/目录、结果持有、错误恢复、强制接口顺序和释放。

- `out/c43-cx0-tests-unisacc-03.run.{stdout,stderr}`
- `out/c43-cx0-tests-gcc-03.{compile,run}.{stdout,stderr}`
- `out/c43-cx0-tests-clang-02.{compile,run}.{stdout,stderr}`
- 实际 CLI：`out/c43-cx0-cli-unisacc-01.{stdout,stderr}`，输出两个单元的 CX0PLAN1。

前两次产品路径编译失败保留：缺 link 包装、首次私有 syscall 适配误传七实参；最终适配按现有六实参入口修正。首次 GCC 编译的条件化公开接口拒绝也保留，随后将平台条件移到私有函数，公开 API 一致。原红不改 PASS。完整合同继续以 `tests/preprocess/CONTRACT.md` 为准。
