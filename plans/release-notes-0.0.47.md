# unisacc 0.0.47 —— 发布说明（草稿，未发布）

**本页不是发布公告，也不授予候选资格。** 冻结产品源为 `7b98ec2c0618df88115ae1c80cea47144e8614d3`，[版本定义](../src/version.h)为 `UNISACC_VERSION "0.0.47"`；源码冻结不等于新 APE、CI 或全批资格。构造所用旧 bootstrap `884adfef…` 仍报告 `0.0.46`，不得冒报为新产品。

## 是什么

面向 C99 的编译器，自己写出 ELF、Mach-O、PE，可编译自身；产品形态为单文件 `unisacc.com`（APE 加载头、ISA 切片和模型包）。本草稿只记本轮范围内的源码修复与开发证据，不声称完整 C99、POSIX、资源安全或六目标原生验证。

## c47-1 身份与证据

| 对象 | 身份或状态 | 证据边界 |
|---|---|---|
| 冻结产品源 | `7b98ec2c0618df88115ae1c80cea47144e8614d3` | 已冻结源码，不是新 APE 资格 |
| 目标版本 | [src/version.h](../src/version.h)：`0.0.47` | 新产物实际版本输出待证 |
| 新 APE `unisacc.com` | SHA256、字节数待证 | 不填旧 bootstrap 身份 |
| 新 `pkg.bin`、各切片及发布资产 | SHA256、字节数待证 | 须绑定同一候选 |
| 配对 `unisacc.com.build.json` | 提交、源 SID、产品摘要待证 | 不复制旧发布记录 |
| 同候选原生 CI | 待证 | 不沿用旧版通过数 |
| 同候选全批 `s9` | 待证 | 不沿用旧会话结果 |

冻结范围为 A1–A4、A6–A8（含 A6b 多 TU 边界与枚举维度读者修复）、M3-a、M3-b、M12、L0' 与版本；见[冻结计划](v0.0.47-unisacc.md)及[A6b 说明](v0.0.47-corpus-feasibility.md#a6b补齐此前遗漏的多翻译单元边界)。合入后主树 `13/13=25620ms`，cc 已复核元数据与关键工具；这些是源码开发证据，不等价于同候选 CI 或全批资格。M8 仅保留私有增量，本次冻结不合入。

## 相对 0.0.46 修复的缺陷

### A12 中七个修复点：固定 43 例仍有失败

A1–A4、A6–A8 已纳入冻结源。七个修复点不等于 A12 十二类全部关闭；A5 没有主线点。下表为固定原 43 例的开发入口曲线，分母始终为 43。

| 点 | 本点修复标识符 | 通过/失败 |
|---|---|---:|
| 0.0.46 基点 | 原发布产品 `884adfef1815e87750c86c7036d3074552a66ce18dfa855588ab279c2aa99fcf`；私有实验（非发布资产）原收据副本：私有克隆根 `omp-cdx2-m12-cache` 下 `out/release-notes-047/fixed43-c463/results.tsv`，SHA256 `cb7ef879cbeeacad274c3bef0433a53848e70a51276772bfb9168f6f68b0b2c0`，原文 `SUMMARY pass=21 fail=22`（字段以制表符分隔） | 21/22 |
| A1 | `duplicate-case` | 22/21 |
| A2 | `case-outside-switch` | 23/20 |
| A3 | `struct-redefinition` | 24/19 |
| A4 | `conflicting-typedef` | 25/18 |
| A6 | `enumerator-redeclared` | 26/17 |
| A7 | `float-subscript` | 27/16 |
| A8 | `return-pointer-as-int`；同时 `void-call-as-value` 翻绿 | 29/14 |

- 固定套件源码 SHA256：`15b8bba457a24ea87d5d7039bcb629c6655f456908b921f8c264359724a5f108`；执行体 SHA256：`3a16e4dca5e05833d7ec1ec79029321be2eaa0edc4363272732235da2276a109`。同套件与严格 C99 oracle 的历史点见[语料证据](v0.0.47-corpus-feasibility.md#m4-首要指标同套件-a12-下降曲线cdx22026-11-07)。
- A8 主树为 `0adadd496e`，开发 wrapper SHA256 为 `49d315e3edc17bf72625a480a7ce1947709800ea8d2c35418d8e80ff51d948710`。独立测量为 11.96 秒、`rc1`、29/14，**完整批仍失败**；此为独立测量记录，上述历史表尚未补入 A8，不能声称表内已有该行。
- 最终私有 c47 P4 开发包（SHA256 `3aa59130cc66564341511f821c47cddf367e060584c3fb02dbff199cfae68410`）重测原 43 例仍为 29/14、原累计 59 例仍为 45/14，两批均 `rc1`，保留 14 项红；新增边界不混分母。它不是正式 Release `pkg.bin`，开发 wrapper、裸流或源码门均不自动取得发布 APE 资格。
- **A6b 归因更正**：初始 `sn_*` 与 `TX_ZCAP` 症状曾误归因于接口/L0；单独 `-funit` 通过、无 `use` 的纯 C 两 TU 同名不同值枚举仍误拒，证明是 A6 的 END_ 状态跨 TU 漏清，而非 sysnums 常量问题。修复在 TU 边界按既有 `td_unit_high` 扫描并同时清 END_/ENV，保留 scope+1、UNDO13/14、同 TU 拒重及 A7/A8，不改 cxmod/sysnums。
- **枚举维度回归与修复**：VLA 存在性读者此前仍比较 END_ `==1`，现改为非零，修复合法 `enum { N=3 }; int a[N]={11,13,17};`，真动态维度路径保留。四个原存在性读者及别名链已审计，三网 TU 边界 12/12 与两类维度案例均有实跑收据；原/新 ident 对象 `cmp=0`，SHA256 同为 `2b17f06ce3a9b3059072dd9b3012f7bf91028881d321eb146cfcf41905e0aba8`。TU 扫描与额外清零的极限资源成本未独测，详见[A6b 说明](v0.0.47-corpus-feasibility.md#a6b补齐此前遗漏的多翻译单元边界)。
- **A4 未放宽**：自家 71 片揭出三处真实 typedef 违规；实际预处理 TU 经严格 C99 GCC 也报类型冲突。`check-sbload` 的本地 Buf 与导入 Buf 不同，改名 SbBuf；`source-package` 与 `source-package-pack` 去掉重复匿名 typedef，复用导入权威类型（尤其传接口的 SpJob）。两项 current pin 随实际源码 SHA 更新，历史 origin pin 不动，新 ctx 不迁移旧资格。
- 71 片仅覆盖当时 568 源快照：559 PASS、7 XFAIL（4 债务、3 负例）、2 scanner 拒绝，意外失败 0；父已独立核对 568 条收据与 559 个对象。其后新增 `tools/s9-p3-pins.cx`、`tools/check-s9-p3-pins.cx`，当前为 570 源，不能声称原 71 片覆盖新增两源；cc 另有新两工具独立实编记录。动态库存与分片登记修复在途，570 源全量结果仍待证；s9 使用最新测试覆盖时须披露 harness 摘要，不在此改记全批通过。

### M3：两个合法 C99 误拒

- M3-a：修复 form-feed 空白。原 `00216` 默认 `run` 成功，合法与非法边界为 12/0；不把字面量或注释中的字节当空白删除。
- M3-b：修复 `sizeof` 枚举常量。原例默认运行成功，边界 18/18；仅枚举常量复用不求值路径，不推广为全部标识符的新处理方式。
- 两项均为开发证据，见[独立修复记录](v0.0.47-unisacc.md#m3-b-sizeof枚举常量独立修复)；不据此宣称 K&R、Lua 或 SQLite 的其它缺口已关闭。

## 开发与测试基础设施

### M12：完成模型缓存的尽力限额

- 冻结源 `7b98ec2c0618df88115ae1c80cea47144e8614d3` 默认限额为 `1073741824` 字节，可由 `UNISACC_E3CACHE_MAX_BYTES` 配置；只识别完整完成的缓存，按其字节数及 `mtime` 稳定 LRU 淘汰。失败为尽力处理，缓存可能超限，**不是目录硬配额**。
- `UNISACC_RUN_CACHE=0` 关闭磁盘结果缓存与膨胀缓存的读写（[结果缓存开关](../src/runcache.cx#L297-L302)、[膨胀缓存开关](../cx/runtime/modelcache.cx#L36-L39)），但不关闭进程内不可变包资源字节及其解压字节的复用；后者不缓存推理产物（[资源复用合同](../cx/runtime/rescache.cx#L6-L12)、[资源读取路径](../cx/runtime/rescache.cx#L72-L107)）。
- Linux 新 driver 五源均不超过 1160000 字节；八路 32/32；模型 LRU 15/0 与并发 11/0；真实串行、故障及并发场景已有通过记录。18000 条格式完整缓存记录夹具实际淘汰至 2 项、398 字节，新命中与 SHA 有效。
- 冷路径 +0.9%、热路径 -2.5% 已被 cc 接受为噪声范围；不称零开销。强制 `unlink` 失败注入、macOS 与 Windows 原生预算仍未测。
- Windows 既有 `flock` 的 `ENOSYS` 情况会跳过新缓存发布与 `touch`，仍可读已有模型；不能由 Linux 记录推断 Windows 淘汰通过。
- 旧 `0.0.46` 缓存仍无上限；确认没有在途编译后，可清理 `$TMPDIR/unisacc-e3cache-<uid>`。此为旧缓存处理说明，不是新版本硬配额保证。

### L0'：系统调用常量表化

- 源提交 `e61e7f05021d2d57f16052f0e97c440410ea965d`：`exec/facts/sys-syscalls.tsv` → `tools/target-gen.cx` → `cx/sys/sysnums.cx`，编译期高低 16 位常量，不是运行期查表；`sy_setpgid` 搬到 `sys.posix`，签名不变。
- 同一 lexer 口径下，`cx/sys` 为 108→101，`proc` 为 7→0；正式 `cxplatform` 的 23 未下降，不能混作同一指标。四个旧投影 SHA 不变。
- 证据见[L0 签名记录](cx-l0-sig-2026-10-30.md)与[核心接口记录](core-interface-2026-11-02.md)；不扩写为运行期、ABI 或平台资格改进。

## 已知限制与未声称的性质

- **A5 `conflicting-function-types` 推迟**：网指纹与 pointee qualifier 缺口仍需独立处理；逆向 NPC 未实现。七修复点与固定 29/14 都不能表述为 A12 全绿，更不能表述为完整 C99 诊断。
- **TAPE-END 推迟**：手工拼接 stream 管道的 tape 仍缺可靠结束标记，仍有截断风险，不把本轮改动称为管道完整性修复。
- **对象编译合同债未修**：`-c -b osx/*|win/*` 文档写拒绝，实际仍编出对象；[0.0.46 已披露](release-notes-0.0.46.md#管道平台资源与验证边界)，本版未修，grk objects 四批红仍属该合同债。0.0.48 待在“真正拒绝”与“修正文档”之间二选一，尚未裁定，也未宣称已修。
- **E1-LINT-RDOMAIN 是既有债**：旧新均为 20 条 `ERROR` 与 6 条 `WARN`；未放宽断言，不能写成 lint 通过，见[记录](v0.0.47-unisacc.md#m3-a-c99-form-feed-空白修复私有验收2026-11-07)。
- GNU 语句表达式 `({…})` 与 C11 `_Generic` 不属于严格 C99 义务；相关拒绝不能混作 C99 误拒，也不能因 M3-a 修复而宣称它们已支持。
- K&R 旧式参数声明、Lua 未定长 extern 数组与 SQLite 大型 amalgamation 的其它既有缺口未关闭；两项 M3 不等于全部外部语料可用。
- M12 只限制可识别完成缓存的尽力占用；淘汰失败可超限，且非 Linux 原生边界未补齐，不证明资源安全或跨平台硬配额。
- 不声称完整 C99/POSIX、完整运行期资源合同、Windows 原生自举或六目标原生均已验证；旧版资格及开发局部结果不得迁移到新候选。

## 验证入口与待证表

本节供 CI 与全批负责人填真实结果；**当前仅同候选原生 CI 已有下列实证，其余各项仍待证，不据此预写发布或完整候选资格**。结果须同时保存原始收据、身份与未运行项，失败及缺材料不得改记通过。

| 待证项 | 必填真实材料与结果 | 当前状态 |
|---|---|---|
| 新产品身份 | 冻结提交、源 SID、APE/pkg/切片/资产 SHA256 与字节数、配对记录、实际版本输出 | 待证 |
| 正规构造与固定点 | 同候选构造链、各目标收据与实际比较结果；区分字节固定点和原生自举 | 待证 |
| 同候选原生 CI | [运行 42408207540](https://github.com/partnernetsoftware/unisacc/actions/runs/42408207540)，源 `7b98ec2c0618df88115ae1c80cea47144e8614d3`；候选 APE SHA256 `fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe`（6215778 字节），`pkg.bin` SHA256 `6b99c50491d9268f4e3c5fe828885f3425fe15284ff8d65d03da10f2e21be971`，`slice-win-arm64` SHA256 `889f36cd9cc58e9451d43e6544ffeae4b88ef3cd70e3d88273248af7b912da28`；本地配对 `unisacc.com.build.json` SHA256 `0512a1de8ddde26442e9f55ef8dfb40c817f2e1f6c2a93263ab15d4b934dde4f` 位于 `~/.unisacc-baselines/c47-1/R2-APE-final-7b98ec2c06-Ex8782/`。下载原件 `~/.unisacc-baselines/c47-1/ci-42408207540/candidate-native-evidence/` 的六份目标 TSV 与 `table.md` 绑定同源同 APE：lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64、win/x86_64、win/arm64 各 identity/version/hello/run/stdio-crossunit/stream-pipe/smoke 七项 PASS，版本均为 `unisacc 0.0.47`。Windows 两格 stdio-crossunit 与 stream-pipe 仅 portable 子集（原始说明分别为 7/9、6/9）；fork/waitpid/kill/F_SETFD/environ/F_SETFL 等 POSIX 子检查未运行，stdio 的 devfull 两条亦未跑，不计作通过，沿用 0.0.46 的披露边界。Windows 两格原始测量字段保留 `UNMEASURED`，不填零或推算值。 | CI 已完成且 success；原始聚合 `QUALIFIED pass=42 fail=0 not_run=0 invalid=0`（仅指此 CI 表格，非全部子检查或发布资格） |
| 新产品正控 | 同候选命名与诊断正控的原始输出；固定 43 例重新测量另列，不套用开发曲线 | 待证 |
| 全批 `s9` | 会话、快照、候选及材料绑定身份；真实终态总数、失败、未知、缺材料与未运行原因 | 待证 |
| 发布判定 | 仅据上述同候选实际收据填写结论；源冻结、源码门与注册验证不得代替 | 待证 |

复核顺序：先绑定新资产与配对记录，再核实际 `--version`，随后独立记录同候选 CI 与全批；不得从旧 `0.0.46` 模板复制资格数字。当前已有源码、开发及上述同候选原生 CI 证据；其余待证项（含新产品身份完整材料、正规构造与固定点、新产品正控、全批 `s9` 和发布判定）保持待证，是否具备发布资格仍待上述实证。
