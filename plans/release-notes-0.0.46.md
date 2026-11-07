# unisacc 0.0.46 —— 发布说明

本版为正式版（非预发布）。产品证据是同候选六目标原生 CI（QUALIFIED 42/0）；**全批会话未达资格**，数量与原因见“已知限制”首条。Tag 对应 main 提交 `623b62fd0d`，其产品输入（`src cx exec include seed ext kernel`）与 CI 验证的 `44182b4059` 逐字相同（`git diff 44182b4059 623b62fd0d` 对这些目录为空）。

## 是什么
面向 C99 的编译器，自己写出 ELF、Mach-O、PE，可编译自身；产品是单文件 `unisacc.com`（APE 加载头、ISA 切片和一份模型包）。不作完整 C99/POSIX 符合性声明，具体缺口见下。

## c46-3 身份与证据

| 对象 | SHA256 / 身份 | 字节 |
|---|---|---:|
| 冻结产品源 | `44182b4059d51191cb021241c666b521bf34b2ad` | — |
| APE `unisacc.com` | `884adfef1815e87750c86c7036d3074552a66ce18dfa855588ab279c2aa99fcf` | 6163990 |
| `pkg.bin` | `29e13aeb8c62a09c0323a9d86dcd51ab8b35684696264b3fc66571d79654ef6c` | 1663382 |
| `slice-win-arm64` | `bd16566df04a3d3caf248146c03419a62328db3f72f4af7459ddccad8f381b10` | 3325440 |
| 配对 `unisacc.com.build.json` | `56fa8f14317f6150d0e6f182e0a51bf09ffe4168f36166216561477bdf2086e5` | 314 |

- 私有 draft：`v0.0.46-c46-3-candidate`。资产已实际下载回读 SHA，备份在 `~/.unisacc-baselines/c46-3/`。构建记录由现成身份/安装工具生成，绑定本源提交、源 SID `95e34c70d087d78fc2527465428042914ec07b48443a2db0fc97303c93d16451` 和新 APE，而非复制旧产品记录。
- 正规链重新执行 131 个作业、final、routes、P4及六片构造，未开启 `UNISACC_WAVE_REUSE`。新包与旧包字节相同是在新构造后确认，不迁移旧候选资格。
- 同候选原生 CI：[42353236985](https://github.com/partnernetsoftware/unisacc/actions/runs/42353236985)，分支 `ci/c46-3`、提交为上表冻结源；原始聚合 `QUALIFIED pass=42 fail=0 not_run=0 invalid=0`，六目标各含 identity/version/hello/run/stdio-crossunit/stream-pipe/smoke。
- **Windows 限定**：两格 stdio-crossunit 和 stream-pipe 只跑 portable 子集。原始说明分别记 7/9 与 6/9；fork/waitpid/kill/F_SETFD/environ/F_SETFL 等 POSIX 子检查未运行，不计作通过。完整原件见备份 `ci-42353236985/table.md`。
- 固定点分开证明：通用五片比较实际得到 objects 81、kernels 2、slices 5、product、package 全部 same；另用专属 checker 核 win/arm64 真实 G2/G3 独立输出、PE Machine AA64、实际构造器链与原始收据，`failed=0 missing=0`。Linux 三代构造器均 `3c660571e790370664b9b3c5e3817e610bb0803f665b9cd8542730a7166c9b63`；Windows 原生自举固定点仍为 `NOT_CLAIMED`。
- 本机 APE 用户入口 smoke 为 5/0。工具改动私有十三棘轮为 13/0，提交 `8484436b0e`；没有提高无关基线。
- **全批会话未达资格（NOT_QUALIFIED）。** 在冻结快照 s8b（`44182b4059` + 候选 APE `884adfef…`，会话身份稳定；旧 s8 因批次 `illegal-action` 写工作目录而作废，s7/s8 的 PASS 与原生 CI 的 PASS 均不沿用）上，445 个批次的终态：230 PASS；27 FAIL；4 UNKNOWN；162 NOT-AVAILABLE（缺材料或材料未绑定，未运行）；20 KNOWN-GAP（`full-*` 需要外部宿主）；`cx-resolver` 与 `wt-gate` 未跑（外层 55 秒限制所致）。27 个 FAIL 逐项归类见 `plans/s8b-terminal.md`：均为记录/摘要滞后于新 APE、快照里缺少 `.git` 或环境变量、检查与表迁移不同步等测试与基础设施原因，未发现编译器产品缺陷；这是 grk 的归类，未对每一项做独立复现，其中 `pins`、`cx-kernel-context`、`cx-decode-sbout`（`011d36b077`）等已在主树修复并以 c46-3 补充复跑，补充复跑不属于同一会话。4 个 UNKNOWN 是 55 秒看门狗在并发下超时（`cx-core-interface`、`cx-layers`、`flow-tool`、`ratchets-tool`），不能记为通过。补充（非同会话）：`wt-gate` 在干净克隆里 3 个 probe 断言（`probe-wave-reports-the-written-path`、`all-receipts-unknown`、`names-the-gate`）失败，用 0.0.45、c46-2、c46-3 三个 APE 都同样失败（0.0.45 与 c46-2 另有 `clean-wave-exit-0` 失败，c46-3 已不再失败），因此是原有问题而非本版回归，条目 `WTGATE-MUTATES-REGISTRY` 跟踪。`cx-layers` 为原有未知（amb=21、out=73），修前修后一致，非回归。这些数字来自 grk 的会话记录，发布前由我抽查了其中 `decode-sbout` 一项。因此本版 **不能宣称全批通过**；产品证据是同候选六目标原生 CI（42/0）。
- 全批会话基础设施说明：第一次会话 s8 因批次 `illegal-action` 在会话根目录写工作目录而被判身份变化作废，已丢弃；改用冻结快照重跑 s8b，快照在 `44182b4059` 基础上仅同步 `tests/illegal-action.cx`（`c05aa85998`，无 IA_WORK 时改用 mkdtemp 临时目录），harness sha 因此从 `1e9337a0` 变为 `8a134c18`，产品输入未变。

## 相对 0.0.45 修复的缺陷

- **Linux 默认 `-run` 进程在 ps/top 中名为 `63`：已修复。** 生成 ELF 仍从 memfd 执行，通过已校验且 dirfd 固定的具名入口保留程序名，不靠落盘 `-o` 冒充默认运行。正式新 APE 实测 `comm=rcprobe.c`，固定 SHA 的旧 c46-2 负控仍为 `63`；同时核验 `/proc/self/exe=/memfd:unisacc-run (deleted)`、原 argv0、空参数/带空格参数、fd9、stdout 和退出码 37。新批 `run-comm` 显式声明两份产品材料；这不是“修复了63项”的计数。
- Windows argv 超过 63 项截断：已修，容量为 16384。既有原生参数边界覆盖 n=40/63/64/65/100/200/1000/4000；这里的 63 是旧参数上限，不是进程名。
- `stream` 只给 `--route` 不给 `--target` 时错用宿主宏：已修，route 联动 target；Windows 验证范围仍受 portable 子集限制。
- UTF-8 BOM 开头的物理源被拒：已修，非法位置 BOM 仍拒绝；无 BOM 时显式 `-include` 宏在 `-run` 丢失的独立既有缺陷也已修。
- Darwin 单调时钟、既有 sysx 有界等待及 Windows strdup 等运行期问题已修；k2 构造名与依赖事实身份同步。
- Darwin `SIGUSR1` 常量改为 30，与系统一致；旧产物硬编码 `raise(10)` 的语义不因此改变。

## 开发与测试基础设施

- 测试 loader：显式绑定 slice/pkg，缺绑定具名拒绝，不回落仓根产品；独立分片启动和单网替换属于开发证据，不自动获得发布资格。
- `work/units.tsv` 与 `tools/fan.cx` 提供索引及分流。
- 工具小修：APE 构造器的 sh 分支恢复 PATH 查找，直接 ELF 分支仍禁查找；已实际完整构造验证。分根固定点清单新增必需 `native_root`，双方原始收据必须绑定同一 rev，不能篡改旧收据令目录相等。
- 内核上下文检查不再把当前源码整树摘要写死为 `REC_V3`：保留kernel变异和忽略边界，新增六个实际工具输入逐项变异必须改变context。正式新输入批9/0；受控遗漏netpack身份的生产者变异必被捕获（9项中1项失败），恢复后9/0及私有13/13。原s8快照断言失败保留，不重钉摘要、不追填旧PASS。

## 已知限制与未声称的性质

### C99 诊断缺口：此前披露的22类未作为本版修复关闭

来源：`plans/rc1-gates-2026-11-06.md`、`plans/defects-c43.md`、`tests/waivers-0.0.44.tsv`。保留失败断言与覆盖；已知缺陷不因 waiver 改记 PASS。

| 类别 | 完整类别名 | 风险 |
|---|---|---|
| A，12类 | `duplicate-case`、`case-outside-switch`、`struct-redefinition`、`conflicting-typedef`、`conflicting-function-types`、`enumerator-redeclared`、`local-redefinition`、`duplicate-parameter`、`void-value-used`、`void-call-as-value`、`float-subscript`、`return-pointer-as-int` | 可能静默产生错误代码，这类输入的产物不可信 |
| B，10类 | `assign-to-const`、`write-through-const-pointer`、`discards-const`、`multiple-storage-classes`、`void-parameter-named`、`incompatible-pointer-init`、`int-to-pointer-init`、`int-plus-assign-string`、`distinct-pointer-compare`、`zero-array-size` | 缺少应有诊断；不据未观察到错码宣称语义安全 |

c46-3 对 A 类 12 项的实测（`plans/v0.0.47-corpus-feasibility.md` 的 A12 表，来源为 grk 实测记录）：8 项被接受并产出错误结果（`duplicate-case`、`case-outside-switch`、`struct-redefinition`、`conflicting-typedef`、`conflicting-function-types`、`enumerator-redeclared`、`float-subscript`、`return-pointer-as-int`），4 项已被拒绝但 stderr 为空（`local-redefinition`、`duplicate-parameter`、`void-value-used`、`void-call-as-value`）。以上 8 项的修复排在 0.0.47 最前；在此之前，约束检查请另用 `gcc -std=c99 -pedantic-errors`。

### 语言、接口与容量

- `_Complex`/完整复数库、trigraph 替换未作为已实现完整能力；`long double` 按 double 表示，无扩展精度。
- 续行前 `__LINE__` 的偏移、拼接头的文件/行位置、头文件内 `#line` 及其部分宏/转义形式仍有既有覆盖限制；不能把主文件的支持推广到所有包含位置。详见 `README.md` 的限制表。
- 生成代码使用私有且不稳定的调用约定；外部互操作走正式 FFI/carrier，不承诺直接兼容宿主 SysV/AAPCS64 ABI。旧 `.unit` 格式对象需要重编。
- 结构成员、typedef/tag/member共享表、源文件数等均有有限容量；多单元重复包含会重复占用共享表，不能当作自动去重。SQLite 大型 amalgamation 已有 `structure member capacity` 拒绝证据，不声称支持完整 SQLite。
- GCC K&R 旧式参数声明仍有 `not covered: parameter`；Lua 的未定长 extern 数组后接其它定义会被 `DM.tent.test4` 误拒。二者均属合法 C99 的已知前端缺口，留到 0.0.47，未混入本候选修复。
- C/POSIX 库并非完整；README 含历史条目，不能把其中旧的 strdup/时钟/缺头描述直接当本候选新测结果。当前树已有 regex/pwd/utime 头，不能再泛称它们全部缺失；平台能力以实际对应收据为准。

### 管道、平台、资源与验证边界

- `-c` 分开编译不跨对象共享运行库，多个 stdio 对象只告警；运行库仍可能每对象重复携带。`-c -b osx/*|win/*` 的拒绝文档与实际产出对象之间仍有既有合同债，不能当完全统一的对象支持承诺。
- 手工拼接 stream 管道的 tape 缺可靠结束标记；预处理器仍有接受非法宏名/未闭合头名的缺口；`#include <C:/x.h>` 不按盘符绝对路径处理。
- `unisacc stream` 未显式绑定时可能读相对 `./unisacc.com`，换工作目录可能找不到或读到旧包；使用 `--models` 显式绑定。
- APE 的 Windows ARM64 原生分发仍有限制；该原生格使用随发 `slice-win-arm64` 与 `pkg.bin`、显式 `UNISA_CONTAINER`。六片被打包不等于外层 PE 同时具有两种机器类型，也不等于 Windows 原生自举已验证。
- RI 参考运行器仍不兼容 P4；`ri-flow-data` 为 `NOT_AVAILABLE`。材料 `PREPARED` 不等于功能通过，相关修复归 0.0.47。
- `sysx.process` 超时路径单次 WNOHANG 是未复现审查疑点：四次实际探针未见僵尸残留，不当已确认缺陷，也未合入私有防御补丁。
- 执行器输入属性的内存放大、OOM/失败/清理与输出预算、步数/取消合同仍有未关闭项；运行期句柄检查不等于 Rust 式编译期所有权，未证明的安全性质不宣称实现。
- 本次命名修复验证的是编译器生成 ELF 的默认运行；不宣称脚本的匿名内存执行兼容。只读跨构造字节固定点与同候选原生 CI 分别记账，不等于完整 RI/语言/资源合同全部通过。
- `src/host.cx` 保留安全具名入口失败后的匿名 `execveat(AT_EMPTY_PATH)` 降级；该路径仍可能显示 `63`。本次已验证正常安全目录下的命名，不保证所有 TMPDIR/权限/容器环境；长名称也受内核 comm 长度限制。此保底路径不等于执行不安全的替换文件。

## 验证入口

### Windows使用
以下是 run `42353236985` 的 PowerShell（`pwsh`）原生调用方式；先使用 [CI 中的 `Invoke-Bounded` 函数](../.github/workflows/candidate-native.yml#L247-L270)（55 秒期限、直接 ProcessStartInfo 启动，并非 cmd 命令）；不是省略该函数后可直接粘贴的独立脚本。
- x86-64：`$com = Join-Path $PWD 'unisacc.com'`；`Copy-Item candidate/unisacc.com $com`；`Invoke-Bounded $com @('--version')`。CI 保留 `.com` 文件名，不改名 `.exe`，未设置此格的 `UNISA_CONTAINER`（[准备](../.github/workflows/candidate-native.yml#L282-L283)、[版本与编译](../.github/workflows/candidate-native.yml#L358-L368)）。
- arm64：`$slicePath = Join-Path $PWD 'slice/slice-win-arm64'`；`$sexe = Join-Path $PWD 'out/slice-win-arm64.exe'`；`Copy-Item -LiteralPath $slicePath $sexe`；`Invoke-Bounded $sexe @('--version')`（[原始命令](../.github/workflows/candidate-native.yml#L304-L318)）；须有 `out` 目录。
- arm64 编译前：`$pkgPath = Join-Path $PWD 'slice/pkg.bin'`；`$env:UNISA_CONTAINER = $pkgPath`；`Invoke-Bounded $sexe @('-o', 'out/hello.exe', 'out/hello.c')`；x86-64 对应为 `Invoke-Bounded $com @('-o', 'out/hello.exe', 'out/hello.c')`。两格运行生成物均为 `Invoke-Bounded (Join-Path $PWD 'out/hello.exe') @()`（[arm64](../.github/workflows/candidate-native.yml#L320-L330)、[x86-64](../.github/workflows/candidate-native.yml#L360-L365)）。
- `candidate/`、`slice/`、`out/hello.c` 是 CI 已准备的材料路径；Release 资产需对应放置再复现。原生 CI 证明上述调用及 portable 子集，不证明 cmd 直接调用、arm64 直接运行未改名文件或 Windows 原生自举；这些方式未验证。


1. 对上表资产执行 `sha256sum`；配对记录的 commit/source SID/product digest 必须对应 c46-3。
2. `sh ./unisacc.com --version` 应输出 `unisacc 0.0.46`。
3. 默认运行：`printf 'int main(void){return 7;}\n' > t.c && sh ./unisacc.com t.c`，退出码应为 7。
4. 命名与负控：`sh ./unisacc.com tools/check-run-comm.cx -- ROOT NEW_PRODUCT OLD_PRODUCT`，旧产品固定为 c46-2，不能用同一新产品自作负控。
5. 原生证据取本轮 run `42353236985`；全批结果另看本轮 s8，不读旧会话结论充数。
