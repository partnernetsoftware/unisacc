# unisacc 0.0.47 —— 发布说明

本版为正式版（非预发布）。产品证据是同候选六目标原生 CI（QUALIFIED 42/0，运行 42408207540）；**全批会话 s9 仍未达资格（NOT_QUALIFIED）**：445 个批次中 380 PASS、0 个产品方向的真实失败、32 已知缺口、10 基础设施、23 未运行（材料不可构造），详见下文。Tag 所指的 main 提交，其产品输入（`src cx exec include seed ext kernel`）与 CI 验证并冻结的 `7b98ec2c0618df88115ae1c80cea47144e8614d3` 逐字相同（`git diff 7b98ec2c06 v0.0.47 -- src cx exec include seed ext kernel` 为空）。发布后读回、安装与补充证据见末节。

## 是什么

面向 C99 的编译器，自己写出 ELF、Mach-O、PE，可编译自身；产品形态为单文件 `unisacc.com`（APE 加载头、ISA 切片和模型包）。本草稿只记本轮范围内的源码修复与开发证据，不声称完整 C99、POSIX、资源安全或六目标原生验证。

## c47-1 身份与证据

| 对象 | 身份或状态 | 证据边界 |
|---|---|---|
| 冻结产品源 | `7b98ec2c0618df88115ae1c80cea47144e8614d3` | 已冻结源码，不是新 APE 资格 |
| 目标版本 | [src/version.h](../src/version.h)：`0.0.47` | 同候选本地与六目标 CI 实际输出 `unisacc 0.0.47` |
| 新 APE `unisacc.com` | SHA256 `fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe`，6215778 字节 | 已构造并绑定下列 CI 与 s9；非发布资格 |
| 新 `pkg.bin`、切片及发布资产 | 资产摘要和字节数已保存在 `~/.unisacc-baselines/c47-1/release-candidate/manifest.tsv` | 四份 draft 原件与备份比较一致；派生校验表、zip 已重建比较一致，不冒称正式发布 |
| 配对 `unisacc.com.build.json` | SHA256 `0512a1de8ddde26442e9f55ef8dfb40c817f2e1f6c2a93263ab15d4b934dde4f`，314 字节；源 SID `c84ea430ab7b037d983cd0208ec6340ccf878cf3ead00b463629a80dcae53c04` | 冻结源与同候选配对，不复制旧发布记录 |
| 同候选原生 CI | 已完成，见下列运行与限定 | 仅 CI 表格资格，不替代全批或发布资格 |
| 同候选全批 `s9` | 445 = 380 PASS + 0 真 FAIL + 32 KNOWN-GAP + 10 INFRA + 23 NOT-AVAIL | 终态仍为 **NOT_QUALIFIED**，不是全批通过 |

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
- `cx-reaper` 在与进程回收类批次并发时偶发失败（`0.0.46` 同样可复现），已在 `tests/wave-domains.tsv` 加串行域；同窗顺序执行、隔离回收域，不拆会话窗。这不是 `0.0.47` 独有回归的证据，亦不将 s9 原收据改为 PASS。
- 不声称完整 C99/POSIX、完整运行期资源合同、Windows 原生自举或六目标原生均已验证；旧版资格及开发局部结果不得迁移到新候选。

## 验证入口与待证表

以下为已取得的真实材料与边界；**源码门通过不等于发布资格**。全批有原始执行收据，但仍 `NOT_QUALIFIED`；不得把失败、未知、缺材料或未满足前提改记通过。

| 已证项或说明 | 真实材料与结果 | 当前状态与边界 |
|---|---|---|
| 新产品身份与资产 | `~/.unisacc-baselines/c47-1/release-candidate/manifest.tsv` 记录 APE 6215778 字节、`pkg.bin` 1665282 字节、win/arm64 切片 3367424 字节和配对记录 314 字节；摘要见身份表与 CI 行。`SHA256SUMS` 495 字节、SHA256 `efa47424c8c2d1e57d966a92621b081f3febceb6784be254bc94f7c3e093a2fc`；zip 5033618 字节、SHA256 `9c7102d3b06c52162b3e6f229a7d14a9a024d05654e40a9f900664c1854acfae` | draft 四原件已有比较一致记录；派生文件重建一致，不是正式发布读回 |
| 正规构造与固定点 | `~/.unisacc-baselines/c47-1/R2-local-verification-7b98ec2c06-Ex8782/phases.tsv`：冻结源构造 P4、六切片、最终 APE；`fixedpoint-FP001` 记录 `objects=same count82;kernels=same count2;slices=same count5;product=same;package=same`，`RUN_CACHE=0`；win/arm64 G2/G3 独立比较 `cmp0`，摘要均 `889f36cd…` | 已有字节固定点记录；不等于六目标原生自举 |
| 同候选原生 CI | [运行 42408207540](https://github.com/partnernetsoftware/unisacc/actions/runs/42408207540)，源 `7b98ec2c0618df88115ae1c80cea47144e8614d3`；候选 APE SHA256 `fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe`（6215778 字节），`pkg.bin` SHA256 `6b99c50491d9268f4e3c5fe828885f3425fe15284ff8d65d03da10f2e21be971`，`slice-win-arm64` SHA256 `889f36cd9cc58e9451d43e6544ffeae4b88ef3cd70e3d88273248af7b912da28`；本地配对 `unisacc.com.build.json` SHA256 `0512a1de8ddde26442e9f55ef8dfb40c817f2e1f6c2a93263ab15d4b934dde4f` 位于 `~/.unisacc-baselines/c47-1/R2-APE-final-7b98ec2c06-Ex8782/`。下载原件 `~/.unisacc-baselines/c47-1/ci-42408207540/candidate-native-evidence/` 的六份目标 TSV 与 `table.md` 绑定同源同 APE：lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64、win/x86_64、win/arm64 各 identity/version/hello/run/stdio-crossunit/stream-pipe/smoke 七项 PASS，版本均为 `unisacc 0.0.47`。Windows 两格 stdio-crossunit 与 stream-pipe 仅 portable 子集（原始说明分别为 7/9、6/9）；fork/waitpid/kill/F_SETFD/environ/F_SETFL 等 POSIX 子检查未运行，stdio 的 devfull 两条亦未跑，不计作通过，沿用 0.0.46 的披露边界。Windows 两格原始测量字段保留 `UNMEASURED`，不填零或推算值。 | CI 已完成且 success；原始聚合 `QUALIFIED pass=42 fail=0 not_run=0 invalid=0`（仅指此 CI 表格，非全部子检查或发布资格） |
| 新产品正控与保留负例 | 同一 `phases.tsv` 的 `ape-original-smoke`：版本 0.0.47、原 enum-sizeof 与 FF00216 默认运行成功；`release-smoke`：`RELEASE_APE SMOKE ok=5 failed=0`；`whitespace`：`C99_WHITESPACE pass12 fail0`；`ape-A6b-context`：枚举初始化数组、A7/A8、三 TU 正控成功 | 已有同候选实证；`c-negative` 仍 `C_NEGATIVE pass45 fail14`、EXIT1，不将固定 43 开发曲线移作新候选全绿 |
| 全批 `s9` | 永久表 `~/.unisacc-baselines/s9-c47-7b98ec2c06/{five-class,fail-class,session}.tsv`：445 = 380 PASS + 0 真 FAIL + 32 KNOWN-GAP + 10 INFRA + 23 NOT-AVAIL | 已执行并归类，**NOT_QUALIFIED**；0 真 FAIL 仅指未取得产品方向新失败证据，不是无缺陷或全批通过 |
| 发布判定 | 同候选 CI 已有成功记录；全批缺口、基础设施与前提不足仍完整保留 | 当前 **NOT_QUALIFIED**；不得由源码门或 CI 42 项直接授予发布资格 |

全批口径：`full-*` 登记 20 个 = 19 个矩阵 `OPEN` 汇总（记 `KNOWN-GAP`，非动态产品失败）+ 1 个校验器 `full-domain-check` PASS；旧 `s8b`/`c46-3` 的“20 KNOWN-GAP”是登记数，不是执行数，旧会话无逐个 `full` 收据。原始证据索引见 [s9 失败预归类](s9-fail-triage.tsv)。s9 的 `rollup-open` 仍按原收据列 INFRA：记录侧 `ad755` 新增 12 项未同步声明。后补表层记录已由负责人用正式 c47 APE `fde2db18…` 单批验证 `ROLLUP_OPEN domains=19 failed=0`、`negative product-change=12 other=0`：只统一四个对象合同债格为 `NEEDS_PRODUCT_CHANGE`、保留原观察并声明 12 项，12 个必需 C99 格仍 OPEN，未降覆盖、未闭合产品缺口。该表层修复尚未提交，不回填原 s9 会话 65 条非 PASS 收据，也不预写已推送。

s9 身份：`product_source=7b98ec2c06`，`harness_commit>=4cdbb3ac7d`；[会话说明](s9-session.md)登记的 tests 树 SHA 为 `0d515c9ad48dfbbd62b8e0fc11de243b36037115`，**不是 harness hash**。永久 `session.tsv` 头部完整 `harness_sha=f203b83210b96db5dd063beeb2433093f912b5c5a3f4a2323e8e4c2b94462d58`，绑定本页 APE 与源 SID。

比较边界：`~/.unisacc-baselines/s8b-summary/s8b-terminal.md` 的计数表记录 s8b 末次收据 PASS 230、NOT-AVAIL 162；当时后者是材料未注册/未入图导致的原始注册批次未运行口径。与 s9 比较为 **通过 230→380、原始注册批次未运行 162→0**：本轮原始注册批次已实际执行并有收据，包含材料一键构造与绑定带来的覆盖推进（入口见 [s9 材料图](s9-session.md)）。这不是同一产品纯修复增益，也不是五类 `NOT-AVAIL=0`：现存 23 项是执行后材料或子检查未达前提，例如 `properties-cases-det pass=13 fail=0 not_run=1`。对 s8e/s8f 不另报未经核实的完整同分母终态数字；现有 [s9 历史记录](s9-session.md)明确 s8f streams 15 PASS，只作材料覆盖对照，不推算其全批总数，不把历史 PASS 迁为当前资格。

## 发布后待证（仅三项）

| 待证项 | 发布后补入的真实材料 | 当前状态 |
|---|---|---|
| 正式发布 | 正式 tag、Release URL、发布资产及最终资格裁定 | 待发布；本页仍为草稿，不预写成功 |
| 正式资产读回 | 正式 Release 下载原件、配对记录、SHA256/字节数与比较结果 | 发布后补；现有 draft 读回与演练不替代正式读回 |
| 正式安装 | 从正式发布读回资产进行安装、实际版本与使用正控收据 | 发布后补；安装合同检查不等于已经安装 |
