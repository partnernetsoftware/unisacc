# unisacc 0.0.48 —— 发布说明

本版为正式版（非预发布）。产品证据是同候选六目标原生 CI（QUALIFIED 42/0，运行 42545543777）；**全批会话 s9 仍未达资格（NOT_QUALIFIED）**：提交档（81 项）通过，daily 与 release 两档**未跑**，不称全批通过。

## 是什么

面向 C99 的编译器，自己写出 ELF、Mach-O、PE，可编译自身；产品形态为单文件 `unisacc.com`（APE 加载头、ISA 切片和模型包）。`sh ./unisacc.com --version` 输出 `unisacc 0.0.48`。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | SHA256 `559705097427cf8f4e483bb6e5f702e0714623a81644991c087b17e3505d9032`，6426254 字节 | 同候选六目标原生 CI 的受测对象 |
| 源 SID（`seed/ident.c` 自算） | `798321e8919815b8e4a6c5860be5210c8cd702a8cd7c8b280218acb539b011d4` | main 的源与候选同源；与上一版本相比仅差版本串 |
| 构造 | 131 个构造作业全部退出码 0；42 个网全键验证 35,080,489 个观察、135,914 个状态，全部一致 | 模型包与 T1 候选逐字节相同，版本串没有进入网 |
| 六个切片相对只改版本的控制构造 | Linux×2、Win×2 各仅 1 字节不同（版本串）；Mac×2 各 1 字节版本加覆盖该页的 32 字节代码签名页哈希槽，槽等于该页内容的 SHA-256 | 版本改动不牵动其它字节 |
| 原生 CI | **QUALIFIED 42/0（pass=42 fail=0 not_run=0 invalid=0）** | 运行 `42545543777` |

## 行为变化（升级前请读）

1. **Linux 的裸 `-c` 现在产出单元对象**（与 osx/win 一致）：允许没有 `main` 的文件，可以分开编译再链接。**这些对象不能直接交给宿主 `ld`**（会报 `undefined reference to __init`）。需要可交宿主链接器的整程序对象，请用 `-c -S`。镜像（`-o`）和 `-run` 仍要求 `main`。
2. **tape 文本必须以单独一行 `.end` 结尾。** 缺尾、重复尾或尾后任何字节都会被拒绝；不提供旧 tape 兼容模式。手拼 `unisacc stream` 管道的用户需要更新。
3. **新增文件定位帧 `.unit N B:HEX`**：tape 里记录当前逻辑源文件名，出错时诊断可指向真实 `文件:行`（含 include 与 `#line` 改名）。预处理位置元数据升级为 `UNIPP2`。
4. **osx 与 win 主机上裸 `-S` 的默认目标**改为表驱动（osx 用各自的 `osx/x86_64`、`osx/arm64`，win 用 `lnx/x86_64`），Linux 不变。

## 本版修复与新增

- 修复：裸 `-c` 在 Linux 上曾一度产出的对象把所有从 `main` 不可达的函数剪掉（包括外部可见函数），破坏分开编译再链接；现在保留全部定义。这个问题只存在于开发过程中的候选，未发布。
- 新增：生成器诊断带输入 `路径:行`；外层属性容器的边界检查先于其它缺失判断；多目标 tape 来源帧；`nm`/`readelf` 可见的 GLOBAL/LOCAL 符号与 gcc 同形。
- 工程：补丁集成、选择器按当前注册档位准入、s9 提交档（81 项）在 5 分钟量级内完成；`precommit` 增加冻结窗口闸；工具层的棘轮覆盖 tools/ 与 tests/。

## 已知限制与未声称的性质

- **s9 全批未达资格**：提交档 81 项通过（含本版对象路线的合同更新：`driver-diagnostics` 12/12、`formats-object-ld` 25/25）；daily 约 544 项与 release 约 39 项**没有运行**。
- **Mac、Win、Linux ARM 切片只经交叉构造**，其原生运行证据以上表 CI 为准，不额外声称更多。
- `TAPEBIN-ORIGIN-5`：`tapebin-roundtrip` 的五个 origin-target 夹具与断言不符（旧产品同样如此），按已知缺口登记，未改断言。
- T1 自有的若干棘轮（`srcshape`、`drivetapebin` 调用数、`net-deps` 共享表、八行 `check-literal`）按限期豁免登记，0.0.49 内降回。
- `p1-baseline`、`check-cx-entry` 没有批次注册，不在任何测试档里。
- 构造本身耗时较长（候选构造约 2 小时），0.0.49 的目标是把冻结到发布压到约 10 倍更快；见 `archive/docs-2026-11/plans/v0.0.49-unisacc.md`。

## 验证入口

1. 核哈希：`sha256sum unisacc.com` 应为 `559705097427cf8f4e483bb6e5f702e0714623a81644991c087b17e3505d9032`。
2. `sh ./unisacc.com --version` 输出 `unisacc 0.0.48`。
3. 冒烟：`printf 'int main(void){return 7;}\n' > t.c && sh ./unisacc.com t.c; echo $?` 应为 7。
4. 分开编译再链接：`sh ./unisacc.com -c -o a.o a.c && sh ./unisacc.com -c -o b.o b.c && sh ./unisacc.com -o p a.o b.o`。
5. 原生 CI 证据：见上表运行号；证据表在其 `evidence-<os>-<arch>` 产物里。
