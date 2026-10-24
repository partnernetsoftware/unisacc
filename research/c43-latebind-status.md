# 0.0.43 晚绑定分类状态表（M5）

### 当前候选结证层（2026-10-24）

本节取代下方历史表作为当前状态；下方2026-10-23采证原文完整保留，不把旧产品结果迁名为当前产品通过。

**受试身份。** 当前产品SHA256为`4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda`，2670104字节，产品来源摘要`c4618639cc3fcfac4f8ce52a11df3bdab93b2c9376eecee63fd70e6550adc57f`。F6的seed/stage2/stage3真实文件及完整收据相等，官方fixedpoint rc0、根与候选ident通过，见[c43-f6-final-evidence/README.md](c43-f6-final-evidence/README.md)、其`stages.json`、`accept-inputs.sha256`。构建固定点不是下列库/平台测试的通过证明。

F3完整CONTRACT记录绑定同一SHA，187项全部执行、186PASS、1个Darwin Terminal归属子项UNVERIFIED、最终rc4；见[c43-f3-final-evidence/README.md](c43-f3-final-evidence/README.md)及`results.json`。该记录中`latebind-p6` rc0（16.596秒），原日志为“26 sources, 130 artifacts byte-identical to the frozen snapshots (lnx/x86_64, public API observers)”，保存于`logs.tar.gz`的`latebind-p6.log`，文件摘要见`log-sha256.tsv`。这是本候选已有的无类型库模式兼容性证据，不等于普通/data全项通过。

现行F4以冻结源码`62aa49e5a3`、同一产品与同源参考运行全1129项；其最终状态须另按完成时收据填写。审查快照时下面待验具名项尚无完成结果，不以旧表或准备缓存补绿。

| 类别 | 本候选现有证明 | 当前状态与待验边界 |
|---|---|---|
| 普通固定签名函数 | F3 `latebind-p6`：26源/130产物对冻结快照逐字节相同，lnx/x86_64 public-API observers | **兼容基线已验；完整迁移合同待F4**。等待`com-latebind`、`lib-latebind`、`com-bindings-neg`、`lib-resolver`及`exec-libraryfn-model`各自结果。P6不证明所有签名、所有控制流或整份lower都无宿主地址。 |
| data | F3 `latebind-p6`不单独替代data的借用/写入/只读/extent/type/missing-provider断言；本层没有这组完整新产品日志 | **待F4**：`lib-latebind`中的2c1、`lib-latebind-W`、`lib-data`、`lib-resolver`、`exec-librarydata-model`分别结证。不得沿用3b17e191历史GREEN为4f0b493c的data PASS。 |
| typed（fixed typed 2c2；variadic能力另分） | 源码只读：`compilerpack.py`的`--librarytyped`默认false，只有显式开启才附加typed parse/lower与ownership-fixedtyped；正常候选构造路径未声明该旗标 | **默认产品迁移UNVERIFIED/未启用，不声称迁移交付**。`exec-librarytyped{partition,lower,reader}-model*`即使模型侧通过，也只能记相应模型范围。`lib-typed-native`、`lib-sig2-native`、`lib-variadic-native`是旧路/宿主ABI证据，F4待验；不能据此宣称默认产品含2c2或全部变参ABI覆盖。 |
| callback | 源码只读：`--librarycallback`默认false且要求`--librarytyped`；模型/候选专用路径与默认产品分离 | **默认产品迁移UNVERIFIED/未启用**。模型侧`exec-librarycallback*`与`lib-callback-candidate-auth`须独立列其专用候选、分片和实际结果。`lib-callback-native`、`lib-callback-forward`、`lib-callback-outcome`、`lib-bindings`及graph/plan等旧路/宿主合同等待F4各项结果；不得把它们变成默认产品callback迁移PASS。 |
| 运行桥（-run宿主转发） | LB-4规定**保持旧路**；本版本没有迁移交付 | Linux当前控制/拒绝边界等待`com-latebind`及相应forward项。`com-forward`和`forward`的Darwin宿主转发需当前候选的Darwin远端具名rc0；本机77继续UNVERIFIED，六格comdemo也不能代全部forward断言。 |

**宿主与源码现状（只读核对）。** `exec/c/libunisacc.c`现在在Linux且未定义时、首个头文件之前定义`_GNU_SOURCE`，因此历史表里的“RTLD_DEFAULT与strict C11的修复路径仍待决定”已过时；不得再要求使用旧cc wrapper作为唯一修法。`ffi.h`/libffi仍是宿主前置。F4环境使用的工具/头/库路径应在env-pin和收据中如实列出，不称为“裸宿主无需前置”通过。此源码核对自身不证明任何动态库测试已经通过。

**未核项。** Windows `.hostcall`/`.hostaddr`仍按L-4分账，接受不计产品绿；普通/data尚未具备本候选F4全项证据，arm64/osx各格没有由本次Linux读审补验；typed/callback专用构造及其债项不由默认候选固定点证明。F4/远端结果到齐后仅据相应原日志更新本节，保留原非零/超时与UNVERIFIED，不改变全局裁定。

---

## 历史采证原文：2026-10-23，产品3b17e191（以下不是当前结论）

性质：状态表草稿，供落 `research/c43-latebind-status.md`。每行标「实际跑过」（附命令与输出摘要）或「只读推断」。未核的行写 UNVERIFIED，不用旧日志补绿。依据：`spec.md` LB-1..LB-8、L-4；`plans/v0.0.43.md:12`；路线图 M5（`research/c43-roadmap-0043-and-refactor.md:37`）。

## 0. 受试身份与宿主（实际跑过）

| 项 | 值 |
|---|---|
| 受试产品 | 根 `./unisacc.com`，sha256 `3b17e191a8c6d2e3b69f877bb3a77635ddce26414c518e6f9cc9667802d67ba6`。`unisacc.com.build.json` 写的 commit 是 `b91fdffb`。开跑前和收尾时各核一次，两次 sha 相同 |
| 经典参考 | `/tmp/ua_ref`，sha256 `94a09cf0…bbaa4`。本表各项检查都没有用到它 |
| 树 | 采证时 HEAD 先是 `b7fea757`，后来是 `a19ca6fb`（另一工作流在提交）。`git diff --stat b91fdffb HEAD -- exec/ src/ kernel/ include/ seed/ unisa/ weights/` 只改了 `exec/c/compiler.c`、`exec/c/fwdpaths.h`、`exec/parse2/control-result.tsv`、`src/front_parse.c`（M3 拒绝与 .cx 前奏）。库运行时 `exec/c/libunisacc.c`、`libraryresolver.h`、`librarycall_*.S` 没有变化，所以 lib-* 探针从树里编出的运行时，与产品构建时的运行时同源（diff 实际跑过；「同源」是推断） |
| 宿主 | H1（R10）：16 核 Linux x86_64，glibc 2.35（Ubuntu jammy）。采证期间 load 在 11–18 之间，有另一工作流在并行 |
| 时限 | 每条命令都加了 `timeout 58`。TMPDIR 指向本目录下的 `tmp/`，没有写仓库 |

### 0.1 宿主垫片（只改环境，不改仓库）

不加垫片时，本宿主上所有要从源码编译库运行时的 lib-* 作业都是红的（实际跑过）。红有两层原因：

1. **缺 `ffi.h`**（路线图 B1 已经登记，但当时只写了 latebind-p6）。实测它同样挡住 lib-latebind、lib-latebind-W、lib-data 等所有编 `exec/c/libunisacc.c` 的作业。
   - `lib-data` 无垫片：rc1，`libraryexports.h:11: fatal error: ffi.h`
   - `lib-latebind-W` 无垫片：rc2，原因同上
   - `latebind-p6` 无垫片：rc1，`FAIL: build of the p6probe observer`
2. **新发现：`RTLD_DEFAULT` 未声明**。补上 ffi.h 之后，`cc -std=c11` 在 glibc 上仍然编译失败：`exec/c/libraryresolver.h:183/219: 'RTLD_DEFAULT' undeclared`。原因是 glibc 的 `dlfcn.h` 只在 `__USE_GNU` 下定义它。这两行由 `5e0058b2`、`4774981e`（2026-09-29）引入。
   - 结论：路线图 B1 的配方「apt install libffi-dev」**不够**，还需要 `-D_GNU_SOURCE`，或者让运行时源码自己定义 `_GNU_SOURCE`。
   - 后一种改法动的是库运行时源码，是否属于产品闭包、算不算 fix 刀，须另裁。本表不改。
   - 证据：`lib-latebind.ffi.log`、`lib-data.ffionly.log`（实际跑过）。

本表的绿都是在下面的垫片下取得的（`env.sh`）：

```
export PYTHONDONTWRITEBYTECODE=1
export C_INCLUDE_PATH=<scratch>/ffi/root/usr/include/x86_64-linux-gnu   # libffi-dev_3.4.2-4 的 deb 解包，与已装 libffi8 3.4.2-4 同版本
export LIBRARY_PATH=<scratch>/ffi/lib                                  # libffi.so -> /usr/lib/x86_64-linux-gnu/libffi.so.8
export PATH=<scratch>/shim:$PATH                                        # cc = /usr/bin/cc -D_GNU_SOURCE "$@"
export TMPDIR=<scratch>/tmp
```

**口径**：垫片下的绿只说明「产品与运行时在补齐宿主前置后行为正确」。它不等于 gate 作业在裸宿主上绿：裸宿主上这些作业仍然是红的，须修宿主配方或修源码。

## 1. 分类状态表

状态词：**GREEN(垫片)** 指在上述垫片下实测通过；**UNVERIFIED** 指未跑，或跑了但没有得到判定（超时、宿主跳过）；**旧路** 指尚未迁移的既有路径。

| 类别 | 条款 | 迁移路（新晚绑定）在根产品上的状态 | 旧路/兼容在根产品上的状态 | 总判 |
|---|---|---|---|---|
| **普通**（kind=0/callkind=0） | LB-1..LB-5 | GREEN(垫片)：com-latebind、lib-latebind 2a/2b、com-bindings-neg、exec-libraryfn-model、lib-resolver | GREEN(垫片)：latebind-p6（无类型库模式基线，130 件逐字节同） | **GREEN(垫片)**，lnx/x86_64 一格。下列边界不计绿：library/bindings/codeprivate 里内嵌的地址尚未迁移（LB-1 末句）；片段 A/B 相同不等于整份 lower 输入/输出无地址，也不计 ABI 签名核验（LB-4） |
| **data**（kind=1） | LB-6..LB-8 | GREEN(垫片)：lib-latebind 2c1、lib-latebind-W、exec-librarydata-model | GREEN(垫片)：lib-data（旧 us_add_symbol 数据注入兼容）、lib-resolver owned-data | **GREEN(垫片)**，lnx/x86_64 一格。Windows 另案；不证明完整 lower 无地址，也不证明全部控制流覆盖（LB-7） |
| **typed**（fixed typed 2c2；variadic） | LB-4（2b 时保持旧路）；2c2 | **UNVERIFIED**。2c2 默认关闭，只在 `--librarytyped` 构建里打包（只读推断，依据：`exec/c/compilerpack.py:403`，Makefile/release 工具不传这个旗标）。根产品不含该路径。模型侧只跑了 exec-librarytypedlower-model 4 片中的第 1 片（绿）；第 2–4 片、exec-librarytypedpartition-model-1..4 没跑。2c2 三笔债（sourcefacts 解耦、seedparse2、stack×3）未跑 | 部分 GREEN(垫片)：lib-typed-native、lib-sig2-native（us_sym 定长 FP/GP17/Pair9）。**UNVERIFIED**：lib-variadic-native 超时 | **UNVERIFIED**（迁移）。旧路部分绿 |
| **callback** | LB-4（保持旧路） | **UNVERIFIED**。`--librarycallback` 要求先开 `--librarytyped`，默认关闭（只读推断：`compilerpack.py:190,405`）。exec-librarycallback{partition,lower,reader}-model*、lib-callback-candidate-auth 都没跑 | 部分 GREEN(垫片)：lib-callback-outcome（含 ASan/UBSan）、lib-bindings（嵌套 native↔script 回调）、lib-typed-native（50 次 typed 嵌套回调）。**UNVERIFIED**：lib-callback-native、lib-callback-forward 超时 | **UNVERIFIED**（迁移）。旧路部分绿 |
| **运行桥**（-run 宿主转发） | LB-4 | **保持旧路**（LB-4：运行桥不迁移）。本版不声称迁移，也没有迁移证据可核 | Linux：com-latebind 的 -run 控制例与 -run 缺 provider 拒绝例都绿（UNISYM1 合同 + 槽位绑定）。macOS dlsym+uffi 转发：com-forward 在本宿主 rc77 UNVERIFIED（仅 Darwin） | **保持旧路**。Linux -run 槽位绑定绿；Darwin 转发 UNVERIFIED |

另案（不在五类之内）：Windows `.hostcall`/`.hostaddr` 走 L-4，win 接受不记产品绿，本表未核。

## 2. 逐项证据（命令 + 输出摘要）

前缀统一为 `cd /home/devali/repos/unisacc && . <scratch>/env.sh &&`，日志都在本目录下。

### 普通

| 作业 | 命令 | 结果（实际跑过） |
|---|---|---|
| com-latebind | `timeout 58 tests/bound 58 sh ./unisacc.com -run tests/latebindcheck.cx -- ./unisacc.com` | rc0，7 s，11 条 ok，`latebindcheck 0 failures`。覆盖：UNISYM1 流两次逐字节同、无地址；mask15 与无 provider 文件的镜像相同；-o/-run 缺 provider 在绑定步拒绝，不写不跑；7 种畸形 provider 文件具名拒绝，且不报成 missing-provider；`.hostaddr` 4/-1 具名拒绝；程序模式下 `.libraryaddr`/`.libraryfn` 被拒绝。**这一项不需要垫片**（只用 .cx 和产品） |
| lib-latebind | `timeout 58 tests/bound 58 sh ./unisacc.com -run tests/liblatebindcheck.cx -- ./unisacc.com` | 垫片下 rc0，20 s。`liblatebind 2a 0 failures`、`2b 0 failures`、`2c1 draft: 0 failures`，共 28 条 ok。2b 覆盖：E3 片段和未绑定 lower 片段都与宿主地址无关、片段内无地址、每个上下文 relocate 后调用自己的函数、injected-only 缺 provider 在 relocate 具名拒绝。无垫片：rc1（ffi.h；补上 ffi 后是 RTLD_DEFAULT） |
| com-bindings-neg | `timeout 58 tests/bound 58 sh ./unisacc.com -run tests/bindingsnegcheck.cx -- ./unisacc.com` | rc0，4 s，`bindings checks 12 bad 0`。plain/hostcall 控制例与参考逐字节同，缺 bindings、坏寄存器、坏操作数三类拒绝的归属正确。不需要垫片 |
| latebind-p6 | `timeout 58 tests/bound 58 sh ./unisacc.com -run tests/latebindp6check.cx -- ./unisacc.com tests/seals/latebind-p6-baseline` | 垫片下 rc0，17 s，`PASS: 26 sources, 130 artifacts byte-identical to the frozen snapshots (lnx/x86_64)`。无垫片：rc1 |
| exec-libraryfn-model | `TMPDIR=<scratch>/tmp timeout 58 ./exec/lower/libraryfncheck.sh` | rc0，35 s（冷准备也在内），`libraryfn-model: 13 fixtures; tbl/net; fragment/reference exact; provenance NEG; legacy baseline; cwd/env replay`。这是模型侧证据，不等于产品接受 |
| lib-resolver | `timeout 58 python3 -B tests/libunisaccresolvercheck.py --package ./unisacc.com` | 垫片下 rc0，7 s。O0/1/2 × owned 15 / process 8 / injected 45 / source 12 / owned-data 104 全部为 0 失败 |

### data

| 作业 | 命令 | 结果（实际跑过） |
|---|---|---|
| lib-latebind 2c1 | 同上 lib-latebind | 16 条 ok：合同逐字节同；E3/lower 数据片段非空且相同；六个快照都不含 provider 地址；A/B 写入互不影响；`readonly-write`、`data-extent-mismatch`、`data-type-mismatch`、`missing-provider` 都带 symbol 256 name 具名拒绝，且不导出、不改宿主；lifecycle 场景下回执不变 |
| lib-latebind-W | `timeout 58 tests/bound 58 sh ./unisacc.com -run tests/liblatebindWcheck.cx -- ./unisacc.com` | 垫片下 rc0，15 s，`W witnesses: 67 assertions, 0 failures`。区分了 ACCEPT、NOT_COVERED（未知只读别名）、SEMANTIC_REJECT 三类；超时和信号不计作拒绝。无垫片：rc2 |
| lib-data | `timeout 58 python3 -B tests/libunisaccdatacheck.py --package ./unisacc.com` | 垫片下 rc0，6 s。scope 为 lnx/x86_64 借用的标量/数据指针，4 个上下文，O0–2；宿主可观测到实际修改；源内定义优先于注入；只读写入被具名拒绝且宿主不变；package_sha256 与受试产品一致 |
| exec-librarydata-model | 先并行跑三次 `timeout 58 sh exec/lower/librarydatacheck.sh --prepare {full,arm,object}`（37 s，都是 prepared，键 a346dc38…），再跑 `timeout 58 ./exec/lower/librarydatacheck.sh --require-cached` | rc0，11 s，`librarydata-model: 54 live-reference fixtures; tbl/net; exact spans/coverage; legacy bytes; cwd/env`。模型侧证据 |

### typed

| 作业 | 命令 | 结果 |
|---|---|---|
| lib-typed-native | `timeout 58 python3 -B tests/librarytypednativecheck.py --package ./unisacc.com` | 实际跑过。垫片下 rc0，7 s。O0/1/2 各 100 次调用，双向九参混合，FP9/integer17/Pair9，含 50 次 typed 嵌套回调。scope 原文写的是 `not complete variadic/callback ABI`。走的是旧路（us_sym） |
| lib-sig2-native | `timeout 58 python3 -B tests/librarysig2nativecheck.py --package ./unisacc.com` | 实际跑过。垫片下 rc0，8 s。O0–2 各 100 次。scope 原文写 `not complete bidirectional/variadic/callback ABI` |
| lib-variadic-native | `timeout 58 python3 -B tests/libraryvariadicnativecheck.py --package ./unisacc.com --sanitize` | 实际跑过。rc1，20 s，`subprocess.TimeoutExpired` 发生在 ASan 编译 libunisacc.c 时（脚本内限 20 s）→ **UNVERIFIED**（超时不算判定） |
| exec-librarytypedlower-model-1 | `timeout 58 sh exec/lower/librarytypedcheck.sh --prepare-tools`（15 s）；并行 `--prepare {lnx-x,osx-x,obj-osx-x}`（57 s，均 rc0）；再跑 `timeout 58 ./exec/lower/librarytypedcheck.sh --require-cached 1` | 实际跑过。rc0，49 s：258 + 9×3 fixtures，tbl/net、fragment/reference exact、provenance NEG、legacy baseline。只是 1/4 片，而且是模型侧证据 |
| exec-librarytypedlower-model-2..4、exec-librarytypedpartition-model-1..4、2c2 三笔债 | — | **UNVERIFIED**（未跑） |
| 根产品是否含 2c2 typed 路径 | — | 只读推断：不含（默认构建不传 `--librarytyped`）。`strings` 查不到模型名，但包体经过压缩，查不到不能算证据 |

### callback

| 作业 | 命令 | 结果 |
|---|---|---|
| lib-callback-outcome | `timeout 58 python3 -B tests/librarycallbackoutcomecheck.py --package ./unisacc.com` | 实际跑过。垫片下 rc0，28 s。O0–2：sticky exit23、先失败后成功、native 正常返回、Pair 零值与 canary、owner 隔离与恢复都通过；`outcome normal + ASan/UBSan passed on x86_64` |
| lib-bindings | `timeout 58 python3 -B tests/libunisaccbindingscheck.py --package ./unisacc.com` | 实际跑过。垫片下 rc0，5 s，nested_native_script_callbacks=true。scope 原文写 `no data/FP/variadic imports claimed` |
| lib-callback-native | `timeout 58 python3 -B tests/librarycallbacknativecheck.py --package ./unisacc.com --sanitize` | 实际跑过两次，都是 rc1、20 s、ASan 编译 TimeoutExpired（脚本 `:9` 限 20 s）→ **UNVERIFIED** |
| lib-callback-forward | `timeout 58 python3 -B tests/librarycallbackforwardcheck.py --package ./unisacc.com --sanitize` | 实际跑过。rc1，`tests/bound 20` 包着的编译返回 142（被看门狗终止）→ **UNVERIFIED** |
| exec-librarycallback*-model、lib-callback-candidate-auth、exec-librarycallback-wiring-prepare | — | **UNVERIFIED**（未跑；依赖 typed 缓存基底，加上 `--require-cached` 的准备） |

**超时根因**（实际跑过）：同一份 `exec/c/libunisacc.c` 在 load≈18 时，ASan 编译用了 34.1 s，普通编译用了 6.6 s。三个 sanitize 作业的内层 20 s 限时在本宿主上不够。这属于超时债，处理办法是拆片或预编，不放宽限时；也可以在低负载时复测。

### 运行桥

| 作业 | 命令 | 结果 |
|---|---|---|
| 条款 | `spec.md:842` LB-4 | 运行桥**保持旧路**，本版没有迁移 |
| com-latebind 的 -run 子项 | 同普通行 | 实际跑过：`control: contract stream names slot 1 (mask 3); the program runs`、`refusal -run: missing provider refused at binding, nothing written or run` |
| com-forward | `timeout 58 env STRICT=1 UA=$PWD/unisacc.com UA_RUN=$PWD/unisacc.com ./tests/forward.sh` | 实际跑过。rc77：`UNVERIFIED: required=macOS -run host forwarding (dlsym + uffi bridge) observed=Linux/x86_64` |

## 3. 结论与待办

1. 只有**普通**和 **data** 两类可以在根产品上写绿，范围是 lnx/x86_64 一格，并且以宿主垫片为前提。**typed** 和 **callback** 的迁移路不在默认产品里，写 UNVERIFIED；它们的旧路部分绿。**运行桥**保持旧路。
2. 收口前要定一件事：`RTLD_DEFAULT` 与 `-std=c11` 在 glibc 上不兼容，使本宿主上的 lib-* 门禁作业即使装了 libffi-dev 也是红的。可选两条路：
   - (a) tests 侧统一加 `-D_GNU_SOURCE`，零产品字节；
   - (b) 在运行时源码首行定义 `_GNU_SOURCE`，这会动库运行时，须按 fix 刀走。
   B1 的宿主配方要同步补上这一条。
3. 三个 sanitize 作业（lib-callback-native、lib-callback-forward、lib-variadic-native）在本宿主上的 20 s 编译限时不够，记入超时债。
4. 如果之后为 M3（`9df1417c`）重建根产品，本表全部行要在新 sha 上重跑。晚绑定的运行时源码没有变化，所以预期结果不变，但这只是推断。
5. 本表没有核：Windows（L-4）、arm64/osx 各格、typed lower 第 2–4 片、callback 全部模型片、2c2 三笔债。
