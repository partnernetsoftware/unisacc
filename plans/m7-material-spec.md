# M7 材料规范（从批次 REFUSE 代码抄，非试错）

`materials-prepare` 先按此表检查再跑批。PREPARED≠PASS。

| 类 | 判定源 | 目录/叶 | 计数与 receipt | 构造命令 |
|---|---|---|---|---|
| kernel-route | `tests/kernel-route.cx:81-88` | `route.tsv`；`{x86_64,arm64}/{11unit}.{s,o}`；`{isa}.kernel` | `canonical=YES`；`stale=0`；**input=33 source=22 object=22 kernel=2**（双 ISA×11） | `tools/kernel-route.cx -- ROOT OUT --isa x86_64 --isa arm64` |
| objects | `tools/object-material.cx:12,20,29` `tests/objects.cx:verify` | `main.cx` `value.cx` `main-x86_64.o` `value-x86_64.o` `value-arm64.o` `material.tsv` | banner `objects-material-v1`；`--verify` 打印 `VERIFIED objects-material`；product/tool/netpack sha 对应当前 ROOT | `object-material.cx prepare ROOT MAT`（须在该 ROOT/APE 上构造） |
| ri-flow | `riflowmaterial.cx:5,37-43` | `runtime.c` `material.tsv` `flow-native` `flow-native.tsv` | `ri-material-v1`；bootstrap=产品 sha；**9 个 closure 字段** `exec/c/{run,core}.c core.h codec.h packagefooter.h resourceledger.h` `include/unisa_sha256.h` `tools/ri-material.cx` **`tests/cases/ri-flow.cx`**。误用 `ri-cli.cx` 会 `material hash field missing or duplicate` | `ri-material.cx ROOT MAT tests/cases/ri-flow.cx` 然后 compile\|seal |
| ri-chain | `tools/cxlib/richain.cx:41` `tests/ri-chain-controls.cx:33` | `material.tsv` `model0.tbl` `model1.tbl` `model0.net` `model1.net` | receipt 字节 ≡ 现算；错则 `material-receipt-mismatch` | 现有 ri-chain 材料工具，须对应当前 APE |
| p3-* | `tests/p3-library-members.cx:27` | p3-pack-ctx 下规则文件 | `regular-input`：存在、常规文件、≤8MiB | 图有路径仍缺叶文件则补叶，不造伪 |
| pp-gold | `tools/pp-gold-material.cx:62-64` | `pp.weights` `pp-observe.cx` `material.tsv` | `--verify` 与 `record()` 字节相等；`material-content-mismatch` | `pp-gold-material` 对当前产品重构造 |
| ref / streams,ref | — | 冻结参考 | **不可自造** | 标来源 |

kernel-route 单 ISA 会得到 inputs=22/11/11/1，被 :88 拒绝。objects 必须在同一快照 ROOT 上 prepare，否则 `material receipt mismatch`。

## T2.a 可执行制备入口（2026-11-08）

`check-loader-targets.cx` 与 common 一样支持单个显式 mode 参数（优先于 `TEST_LOADER_TARGET_MODE`），无参数时保留原环境契约；五个绝对材料绑定仍必需。十二个登记格可分别运行 `tools/bound 55 sh ./unisacc.com tools/check-loader-targets.cx -- MODE`，MODE 为 image/object 与六目标的既有组合；每次只证明命名格，不能把一个 ID 的不同环境冒作十二格。旧 ROOT 首轮十二格 argc 前置拒绝证据保留；显式 mode 切换后，`out/ltests-evidence/loader-targets-v2/loader-consumer-results.json` 的十二格均实际 rc0、stderr 空，与首轮其余十一格合计23格 PASS；没有重跑首轮已 PASS 的十一格。

剩余七项窄核（最终包已存在，不再笼统归因缺 P3）：

- 最终 `p3-package.bin` 实际头为 `P 4 40 1823 77`，带 UNIPKG1 footer。`e2-input-binding.cx:18` 仅接受 P3，forced_image 的资源解析仅接受三字段 F，而 P4 为六字段 F；还实际加载七个独立带 footer 的单模型容器（D.route=stream、model=0），逐 M 与候选 image/O2 路由相等，并对真实 stream1 增加 forced-include 资源后执行 UNIPP1 元数据验收。候选参数须真实 canonical 包，不能继续用无该 footer 的最终 APE；P4 fixture 迁移须保持全部断言。
- `stream-model-identity` 现有实现仍只接受P3，且包首P时不剥末尾footer；最终P4需独立受验迁移。所试P3/P4协议改动未实际exercise，已按父指示撤回，不作为本轮16项交付。还须真实streams.json的source_package_sha256、original_model_index、compressed_model_sha256、original_M_record_sha256、com_sha256及七个对应容器；shim的index=-1、M哈希零值不可替代。
- `interfaces-lnx-x86_64` 要七真实独立容器相邻边的 file/stdin 字节相等、UNIPP1/USLATTR1+UNITOK1/tape/target/ELF及精确拒绝；`streams` 要真七级流水及独立 full-product 对照。最终 P4 可提供模型，不自动提供独立运行器。
- 真生产接口是 `stream-pack.cx BASE_COM CANONICAL_PKG NEW_OUT [TARGET [LEVEL]]`（已支持P3/P4）。`tools/streams.md` 指向的 archive/lab-loader-legacy/tools/stream-runtime.c 第8行仍包含不存在的相对 ../exec/c/run.c；同字节执行器在 archive/reference-c/exec/c/run.c。须真实修复材料运行器闭包、构造BASE_COM，再用最终canonical P4打包；整产品分派脚本不合格。
- `properties-cases-det` 的 group_det 实际只构造产品六目标重复镜像、异路径TZ/LANG、环境扰动和本机run-repeat，不消费stream4/5；CLI启动仅要求这些路径。NOT_RUN来自compile/输出缺失或run失败（:88、89、91、96、98），不能归因为缺最终包，须据原失败行定具体子调用。
- `stream-e4-io:75-77` 要UNISA_MAXSTEPS=1时rc3、stdout空且含 `step budget exhausted (budget 1 steps)`，另有真实file/stdin、ENOENT/EACCES/ENOTDIR/目录及恢复。预算/接口不符不是换无关文件可修，需真stream4证据。
- `stream-prune-cases` 仍引用缺失的src/tapeprune.c。只读已核archive/reference-c/src/tapeprune.c的SHA `40059d3c00bd8c7536308d8d183a40a0d4de2b1d784764a87ab94f03abc876d5` 与src/tapescan.h `b3e1ee6db7e608744160bbc59f54eb0cc6c2ae65ae3f22d03e8e34c8e42a1e93` 均与归档前e16028ffa1^相同，参考可合法复用而非缺不可造原件。所试路径/强SHA代码未实际exercise，已按父指示撤回；未来迁移须以真实源先做oracle-only烟测并登记实际输入pins/closure，完整批仍须真stream5。本轮不扩建独立stream运行器。

`tools/materials-prepare.cx` 复用既有 RI native 和 chain 生产者/验证器，分三次调用，避免把多个构造挤进单次 55 秒窗口。调用者提供绝对 `ROOT`、全新 `OUT`（父目录已存在）；下列为历史实际制备接口的可复现命令：旧 ROOT=`HISTORICAL_ROOT`、旧 OUT=`$HISTORICAL_ROOT/out/ltests-ri-prepared` 已真实完成三阶段；原始 `native-build`、`chain-prepare`、`native-verify`、`chain-verify` 的 stdout/stderr/status 与 MAP 均留在该 OUT。重现实验必须换全新 OUT；新 T2 树尚未执行：

```sh
tools/bound 55 sh ./unisacc.com tools/materials-prepare.cx -- "$ROOT" "$OUT" native
tools/bound 55 sh ./unisacc.com tools/materials-prepare.cx -- "$ROOT" "$OUT" chain
tools/bound 55 sh ./unisacc.com tools/materials-prepare.cx -- "$ROOT" "$OUT" seal
```

只有 seal 的两个既有验证器均真实接受后才创建 `OUT/materials.tsv`，包含 `ri-cli` 与 `ri-chain`；原始 stdout/stderr/status 保留。native 必须实际构造 ELF、native.tsv、runtime.c、material.tsv；不以存在性冒充验证，不产生 PASS 收据。失败输出保留供诊断；输出目录和日志独占创建，重试使用新 OUT。

旧 ROOT 九项已实际 PREPARED 且消费者 PASS，原始证据为 `$ROOT/out/ltests-evidence/ri-consumer-results.json`（九项 rc0、stderr 空）；逐项判据为：八个 `ri-material-*` 批分别绑定此 `ri-cli`，要求既有 `--verify-native` 接受完整收据、flags、runtime、ELF 后，分别由 `ri-material-controls.cx` 执行 control-valid、corrupt-flags-receipt、corrupt-runtime.c、fifo-receipt、missing-native-receipt、native-byte-flip、native-missing-include、symlink-native；`ri-standalone` 同时绑定此 `ri-cli` 和 `ri-chain`，两个验证器接受。PREPARED 只说明材料齐备且真实验证通过；该历史 PASS 不外推为新 T2 树 PASS，也不外推为 production E1-E3 全流资格。

`tools/units-prepare.cx` 已去掉封存临时目录依赖，改为显式真实 slice/models、精确版本及载体目录：`ROOT ABS_OUT SLICE MODELS VERSION CARRIERS|- loader|swap|seal|seal-loader`。每阶段各自外包 `tools/bound 55`；loader 与 swap 实际编译现有 test-loader/p3-swap，seal 核六个 `carrier.0` 至 `carrier.5` 常规叶并记录实际 SHA/字节数，输出 `material.tsv`、给 `fan --env` 的纯绝对路径 `loader.env` 及可 source 的 `loader-shell.env`（含显式 target/version）。两份环境均保留 `TEST_LOADER_CARRIER_DIR`，不破坏 `units-stage-check` 的非 e2 消费者；载体是否匹配冻结 pin 仍由该既有消费者验证。不再复制旧冻结开发工具，也不把任意 slice/models 推导为同候选 APE；调用者须先核真实提取来源。六个 loader 批的 PREPARED 还要求加载 shell env，且对应现有消费者接受其实际部件/模型，ISA 期望版本须等于显式 VERSION。`loader-common-models`/`loader-target-models` 的各 mode 按既有合同逐格执行，不减少断言。

loader 六批与 fan 的冻结 carrier pin 链分离：`CARRIERS` 可传 `-`，以 `loader`、`swap`、`seal-loader` 三阶段实际编译工具并产生 `loader6-material.tsv` 和可 source 的 `loader6.env`；不要求任何外部 carrier，不输出可冒充 fan 完整材料的 loader.env。六批已有消费者自行生产实际合法载体：`check-loader-nets.cx:16-18,43` 从源码经七级真实过滤；common 从仓内 tests/stages/in 及真实 tokenpp 产物输入；special 构造合同规定的完整 wire frame；targets 逐目标实际过滤；check-test-loader 自造源码并保留真实逐级输出；ISA 只需精确版本。fan 的 `seal` 仍必须显式绑定六个常规 carrier.N，原冻结 pin 校验不变。

真实候选部件复用链（只读核材料，不宣称消费者通过）：永久 `R2-fixedpoint-7b98ec2c06-Ex8782/g2-g3.tar.gz` 的 `out/fixedpoint/g3/lnx-x86_64/entry.elf` 为 1880766 字节、SHA `7617682d39af83e948628566077ac33ce51a076c3da4f79c59f824142a8d8947`；同 member receipt 的 p3_sha 为 `6b99c50491d9268f4e3c5fe828885f3425fe15284ff8d65d03da10f2e21be971`。永久 `R2-APE-final-7b98ec2c06-Ex8782/pkg.bin`（亦同 P4 的 c47-final-pkg.bin）为 1665282 字节、相同 SHA，含 UNIPKG1 footer。其真实 ape-receipt 同时记录这两个 SHA 及最终 APE `fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe`，release-candidate/manifest.tsv 与 build.json 配对该最终 APE。不能改用 P4 routes.tar.gz 内不同编排的 compiler.pkg 冒充最终包。

其余状态以本节最新七项窄核为准：已有最终P4与永久档案，不再将旧P3-only解析或已归档同字节参考误列为缺完整ctx。真独立运行器、未迁移P4 fixture和实际预算/接口失败各自保留，未运行前不转PREPARED/PASS；禁止shim冒容器或伪造冻结证据。

永久错误边界检查入口：`tools/bound 55 sh ./unisacc.com tools/check-materials-prepare.cx -- ROOT NEW_ABS_OUT`（也可无参使用当前仓和自动私有目录）。实际调用准备器，检查缺参、缺 mode、非法 mode、native 拒既有目录且保全哨兵、seal 缺材料拒绝且不写 MAP；保留各次原始 stdout/stderr/status。不构造替身生产者，不将这些负控代替正向完整制备证据。


## 进度 2026-11-07

转绿 **45**。p3 挂起。

## 原缺料记录及2026-11-08更正

| 批 | 原始观察 | 最新精确条件 |
|---|---|---|
| stream-prune-cases | classic sources errno=2 | 归档同字节源真实存在且SHA已核；路径迁移代码未验已撤回，仍缺可消费路径与真实stream5 |
| stream-model-identity | not canonical P3 | 最终包是P4；协议迁移代码未验已撤回，仍需受验P4解析、真实七容器及streams.json |
| e2-input-binding | fixture-footer；candidate-selected-e2-model FAIL | 需将P3-only模型和F3-only fixture解析迁移到已有最终P4，并绑定真单模型容器，不能用APE/shim代包 |

以上旧观察不是当前“依赖冻结参考/完整ctx故不可自造”的证据。施工清单仍由父依据实际制备/消费者输出更新。

## streams / PEEP / OPINFO（2026-11-07 定性）

- **unrealizable=179**：`tools/peep-cases-gen.cx:178-180`。`realise()` 返回 0 且 `y==keep` → `UNREALIZABLE keep-by-definition`，**不跑 e4**。200 键里 179 条 keep 不构成测试。
- **手工一例** `0000.store64.store64.none`（y=keep 却被 realise 成可跑？results 记 FAIL rc=2）：want 四行 `_start:/store64/store64/ret`；got **空文件**。diff= want 全文被删。不是 peep 改写错，是 **STREAM4_COM 对 tape rc=2 无 stdout**。
- **三档**：①材料/运行器——21 FAIL 均 `rc=2` 空 `.out`（与 classic sources / product-identity 同类，不是网语义）；②用例表——179 unrealizable 使 keep 网几乎无正例；③网真缺陷——**本例未证明**。不立 P1 网缺陷。OPINFO fail=54 同批运行器，未逐字节。



## M8 后纯 T2.a 移植边界与历史证据归属

本节的历史 ROOT 一律以 `HISTORICAL_ROOT` 表示（只读），由历史材料 manifest/receipt 的 root 字段实际绑定；该别名不是当前私有移植树的 ROOT，不改写运行时 MAP 或材料中的真实绝对输入。当前79c09c401源码基底已实际完成 RI native/chain/seal 与 loader/swap/seal-loader 制备；所选消费者结果见下文，不把文件复制当验收。历史准备记录中16 PREPARED/7 NOT_PREPARED仍是旧 ROOT 历史全表，不回填旧 s9，也不宣称0.0.48资格。

相对冻结 M8 的接口变化：`units-prepare` 删除复制式 `--out/--root/--loader/--slice/--pkg/--carriers` 接口，唯一接口改为上文七个位置参数，不留兼容 shim；OUT 必须新建，阶段产物独占创建，不再幂等跳过。`materials-prepare` 是真实 RI 三阶段聚合入口；`ri-material` 保留原 action/参数与全部 SHA/ELF/flags 拒绝，仅让 -I 与既有归档解析得到的 core.h 所在目录一致，控制消费者同步其精确预期。`check-loader-targets` 仅增加单 mode argv 优先于环境，不改变十二格断言。fan 六载体与原 pin 不放宽、不以 loader6.env 代替。

历史 RI native 收据 `$ROOT/out/ltests-ri-prepared/ri-cli/native.tsv` 绑定 source=`c84ea430ab7b037d983cd0208ec6340ccf878cf3ead00b463629a80dcae53c04`、bootstrap=`fde2db1812533a9a56a66d8e0d1870c310fdcc77d444a2d33210ca96e9ef98fe`、native=`4394b7d8ab25fba8b3dcad81121f340acdeae58b907fdde9fdd62171a3eeddd1`；flags 包含旧 ROOT/exec/c，不能改写收据路径来复用成新 ROOT 材料。

历史 loader 原始环境为 `$ROOT/out/ltests-loader6/loader6.env`：SLICE=`$ROOT/unisacc.com.dev/entry.elf`，MODELS=`$ROOT/out/ltests-c47-materials/p3-package.bin`，VERSION=`0.0.47`，CARRIERS=`-`。`loader6-material.tsv` 保留真实 bootstrap/slice/models/loader/swap/三个源文件 SHA 与字节数。23格证据由 `out/ltests-evidence/loader-consumer-results.json` 中十一项首轮 rc0 和 `out/ltests-evidence/loader-targets-v2/loader-consumer-results.json` 中十二项 rc0 合并，不能把首轮十二项拒绝抹去。可复现制备命令（OUT 必须全新，cd 到明确 ROOT；非本新树已执行）：

```sh
for STAGE in loader swap seal-loader; do
  tools/bound 55 sh ./unisacc.com tools/units-prepare.cx -- "$ROOT" "$OUT" "$SLICE" "$MODELS" 0.0.47 - "$STAGE" || break
done
. "$OUT/loader6.env"
```

消费者逐项复现：`tools/bound 55 sh ./unisacc.com tests/run-batch.cx -- "$ROOT" "$NEW_RUN_OUT" ID "ri-cli=$RI_OUT/ri-cli"`（ri-standalone 另加 `ri-chain=$RI_OUT/ri-chain`）；loader 的23个 ID、实际环境及原始调用见旧 ROOT 的 `out/ltests-evidence/loader-consumer-accept.py` 与 `loader-target-accept-v2.py`，参数为 `ID "loader6=$LOADER_OUT"`。这些脚本是历史调用记录，不复制或执行其并发测试。

新目标实际 smoke：`out/t2a-ri-ready` 保存 RI 三阶段及四份 rc0 stage status；control-valid 与 native-missing-include各10断言0失败。`out/t2a-loader-ready` 保存真实 loader 三阶段；注册消费者 loader-isa-start 为 claimed=1/failed=0，其他平台明确 NOT_CLAIMED，stdout见 `out/t2-reg-final/loader-isa-parent.stdout`；image-lnx-x86_64为14断言0失败，见同目录 `loader-target-parent.stdout`。注册 materials-prepare-controls为8断言0失败。上述仅证明所列新ROOT路径，不声称重新执行历史23格或16项全表。

准备器错误边界也已有旧 ROOT 实证：`out/ltests-evidence/fix4-03/materials-prepare-controls.stdout` 为 `CHECK_MATERIALS_PREPARE assertions=8 failed=0`，对应 `results.json` 记录该批 rc0 及原始 run-batch argv；其哨兵证据目录为 `out/ltests-fix4-03/materials-prepare-controls`。这不是新 T2 树的 smoke 结果。RI 四份内部 stage status（native-build、chain-prepare、native-verify、chain-verify）均 rc0 且 stderr 空，PREPARED 与 VERIFIED 原始输出分别保留，不把验证器输出改写为消费者 PASS。

追加失败保持：注册 `materials-prepare-native` 已按修正后的直接OUT参数真实执行，但内层原20秒编译watchdog触发，BATCH_RUN FAIL/rc2；原始stage输出保存在 `out/t2-reg-final/materials-prepare-native`。此前成功制备与本失败的 runtime.c、material.tsv逐字相同；尚未证明超时根因，不以成功材料覆盖本次FAIL，不提高20/40/55秒预算或对相同输入盲目重试。

cc复核后采用方案(b)：从批次表撤下 `materials-prepare-native`，不把已知红注册成IMPLEMENTED/PLANNED daily；仅以 `agentwork/items.tsv` 的 `T2A-NATIVE-PHASE-20S` 登记KNOWN-GAP、原失败日志与分相位方案。分相位尚未实现，原预算整链实测后才重新登记。补丁重基6a3b87ab12；79制备与十INFRA证据仍只绑定79。逐行pin身份依据见 `plans/t2-pin-rationale.tsv`。


Win 负控的历史就绪/PASS 为父当时实际 stdout 见证，banner 为 `WIN_ARM64_FIXEDPOINT_CONTROLS failed=0`，旧 ROOT 的历史准备记录亦记录该结果；原始持久 stdout/status 日志尚未定位，由父追溯，不虚构证据路径，不为确认既有事实重跑。实际缺代输入已核，为 `$ROOT/out/ltests-c47-materials/win-arm64-missing-generations.tsv`，root/native_root 均为旧 ROOT、rev=`7b98ec2c0618df88115ae1c80cea47144e8614d3`，绑定真实 fde APE、pkg、Linux ELF 与 AA64 PE SHA。其余15项有完整原始 JSON；16 PREPARED/7 NOT_PREPARED 是旧 ROOT 历史状态，不冒称本节点或新 T2 树重新验收。可复现原接口（非本次执行）为 `tools/bound 55 sh ./unisacc.com tests/win-arm64-fixedpoint.cx -- "$MISSING_GENERATIONS_MANIFEST" "$APE"`；缺两代 rc3、实际 ELF 冒充 AA64 rc1、错误 SHA rc1 的三个拒绝合同保持原样。此负控 PASS 永远不等于真实 Windows 固定点 PASS。

