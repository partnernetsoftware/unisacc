# unisacc 0.0.46 —— 发布说明（草稿，未发布；标 [待证] 的项在证据到齐前不得写成已通过）

## 是什么
C99 编译器，自己写出可执行文件（ELF、Mach-O、PE），能编译自身；整个工具链是一个文件 `unisacc.com`（APE：加载头、ISA 切片、一个模型包）。

## 候选身份
- 候选 c46-2：APE sha256 `c1309d112ba1d389a296ef701f864d56cc06fe281eea98f621a51d8d2b429caf`，源提交 `b64a34aaa86400ad75f52a1602025e52b7dfdd17`，`unisacc --version` 为 `unisacc 0.0.46`。
- 同候选原生证据：`candidate-native` 运行 42332407482，`QUALIFIED pass=42 fail=0 not_run=0 invalid=0`（六格 × identity/version/hello/run/stdio-crossunit/stream-pipe/smoke）。Windows 两格的 stdio-crossunit 与 stream-pipe 只跑 portable 子集（POSIX 部分在 Windows 无对应原语）。
- 正式版标签所在提交须与候选产品输入相同（`git diff` 对 src cx exec include seed ext kernel 为空）；[待证：最终发布时再核]。

## 相对 0.0.45 修复的已知缺陷
- Windows argv 超过 63 项被截断：已修，容量由 63 提到 16384；SP_ARGC 在 n=40/63/64/65/100/200/1000/4000 八点均得 `ARGC=n+1`（CI 原生）。
- stream 只给 `--route` 不给 `--target` 时前端宏用宿主目标：已修（route 联动 target）；Windows stream-pipe 检查在六格通过（portable 子集）。
- UTF-8 BOM 开头的源文件被拒绝：已修（物理源 BOM 支持，保留对非法位置 BOM 的拒绝）；另修了无 BOM 时显式 `-include` 的宏在 `-run` 丢失的既有缺陷。
- Darwin 单调时钟、sysx 有界等待、strdup 等 Windows 运行期问题：已修。
- k2 构造名与依赖事实身份同步（构造链内部）。
- 行为变化：Darwin 上 `SIGUSR1` 常量由 10 改为 30（与系统一致）；旧编译产物里硬编码的 `raise(10)` 在 macOS 上语义不同。

## 新增（开发/测试基础设施，非用户功能）
- 测试 loader（`tools/test-loader.cx`）：显式绑定 slice/pkg 启动，支持单网替换与按 ISA 本机启动检查；缺绑定具名拒绝，不回落读 `./unisacc.com`。仅开发证据。
- 索引树 `work/units.tsv` 与并行分流 `tools/fan.cx`。

## 已知限制（诚实披露）
- 全批会话：[待证：s5 进行中，结果出来后填 PASS/FAIL/KNOWN-GAP/SESSION-INFRA 计数；SESSION-INFRA 不算合格]。
- 逐切片固定点：[待证：win/arm64 缺口由 cdx 在做]。
- `unisacc stream` 在没有显式绑定时默认读 `./unisacc.com`（相对 `UNISA_CONTAINER` 继承），在其他工作目录可能找不到包或加载到旧包；用 `--models` 显式绑定。
- 沿用 0.0.45：22 种 C99 约束违例不报错（12 种可能产出错误代码）；`-c` 分开编译运行库不跨对象共享；手工拼 stream 管道 tape 无结束标记；预处理器接受两种非法输入；运行库在每对象各带一份；`#include <C:/x.h>` 尖括号不按盘符绝对路径处理；APE 内尚无 win/arm64 分发（用随发独立切片 `slice-win-arm64` 配 `pkg.bin`，`UNISA_CONTAINER` 指向）。
- Windows 上 POSIX 部分（fork/waitpid/kill/F_SETFD/environ）无对应原语，未跑。

## 怎么验证
1. `sha256sum unisacc.com` 应为候选 sha（见上）。
2. `sh ./unisacc.com --version` 输出 `unisacc 0.0.46`。
3. 冒烟：`printf 'int main(void){return 7;}\n' > t.c && sh ./unisacc.com t.c; echo $?` 应为 7。
4. 各平台原生证据：`candidate-native` 运行 42332407482 的 `evidence-<os>-<arch>` 产物。
