# win/arm64逐片固定点：c46-2最小分片检查已实跑

**当前决策。** cc已授权在c46-2产物上做最小可复跑分片检查。先以显式Linux slice+pkg构造真实G2/G3 Linux构造器，再用本代构造器生成win/arm64；不执行跨目标PE，不把同一构造器重复两次输出冒充代际链。下面五→六枚举及旧checker布局修复仍为独立未完成合同，单独win/arm64小块不冒称整六片正式固定点通过。

**已核实落点。** `tools/chain-step.cx:521-523`的输出数组、`:550`目标数组都只有五项，`:533`比较数和`:558`循环界限写死5。追加`win/arm64`与`win-arm64/slice-win-arm64`，两数组使用同一派生计数，避免日后只加构造不加比较。**`:617 items=5`指objects/kernels/slices/product/package五类检查，不是片数，不改成6**；另加具名`slices=6`结果。现有`release-build.cx:828,841`、`release-ape.cx:50,69`、`slice-freeze.cx:189`与原生CI已识别第六目标，无须改产品代码。

**既有checker缺口必须同时收口。** `tests/chain-fixed-point.cx:61-62`仍读`g2/candidate`、`g2/receipt.txt`顶层；实际工具`:563`按`g2/<os>-<arch>/`建目录。改为逐片检查真实objects/whole-o布局，缺失、重复、字节不同、目标身份不符都拒绝；不能仅把枚举改6后宣称资格完成。批次仍为`chain-fixed-point`（`tests/batches.tsv:397`），不迁移旧失败或旧资格。

**构造与身份。** G2的Linux两片使用G1构造器，G3的Linux两片使用G2 Linux构造器；whole-o的OSX/Windows各片使用**本代**Linux candidate（`:564-566`），不是每片G2都由G1、G3都由G2。保留该混合协议并显式记录每代/每片target、实际argv与constructor SHA、Root/SID、P3 SHA、输出路径及SHA。whole-o receipt目前缺constructor行时，增加最小`fp-ctor.tsv`记录，不伪造旧receipt。检查器按路由验证链，既核六对最终字节，也核原生格式身份；win/arm64的PE Machine必须0xAA64，不能拿同样相等的x86_64文件冒充。

| 文件域 | 估计净增 | 验收重点 |
|---|---:|---|
| tools/chain-step.cx | 20–40行 | 六目标构造与比较同源计数、逐片身份/构造器证据；五类结果含义不变 |
| tests/chain-fixed-point.cx | 50–80行 | 新布局、六片逐字节与路由身份；第六片缺失/损坏/错target/错构造器、异包/异Root拒绝 |
| 注册/覆盖/说明 | 2–4行 | 明确六片与单宿主混合固定点范围；toolsloc按实际净增申请，不预抬基线 |

**执行门。** 批准后先跑现有负控与新增真实边界回归，再用同一冻结Root/P3实际构造两代六片，六对SHA和原始退出状态全部保留；任一缺失/TIMEOUT/未知即非PASS。沿现行逐作业bound与whole-o既有例外，不新增时限豁免、不缓存结论；保留可恢复片目录。最后13棘轮及同候选正式固定点批次必须绿。

**不证明什么。** Linux上生成并比较PE ARM64仅证明该交叉构造路径的逐片字节固定点，不能标Windows ARM64原生自举，也不是独立第二次构造。原生仍须同候选`windows-11-arm` runner实跑；`id.reproducible-construction`及六片整体固定点不因此关闭。本轮真实G2/G3 win/arm64已比较通过，范围仅下述冻结身份。

## 本轮冻结身份与执行边界

- 只读候选根 `/tmp/c46b`，源rev `b64a34aaa86400ad75f52a1602025e52b7dfdd17`；CI run `42332407482`，cc已报告同候选42/0，不重新跑CI确认。
- APE `/tmp/c46b/out/rc2-b64a34aaa8/up/unisacc-c46-2.com`，SHA256 `c1309d112ba1d389a296ef701f864d56cc06fe281eea98f621a51d8d2b429caf`；pkg同目录 `pkg.bin`，SHA256 `29e13aeb8c62a09c0323a9d86dcd51ab8b35684696264b3fc66571d79654ef6c`。不得混用旧`/tmp/c46a`的a101材料。
- 正规最小四逻辑job：G1构造G2 Linux（`entry-build --only`分批再`--link-only`）；G2 Linux构造G2 win/arm64；G2构造G3 Linux；G3 Linux构造G3 win/arm64。所有输出 `/tmp/winarm-fixed-build`；whole-o工具写根为独立git clone `/tmp/winarm-fixed-root`，HEAD固定b64，不再以c46b或其snapshot作可写ROOT。Linux每作业≤60秒；whole-o沿用冻结release-build既有120秒例外，不新增超时豁免。
- 误入口失败保留：首次把只读snapshot误作release-build ROOT，rb_freeze阶段git archive失败，rc1/13.44秒，无PE产物。cc实核原APE/pkg SHA未变、snapshot无out目录、无在途release-build，未发现写痕、不需清理。已改为独立私有ROOT/OUT；不删除原失败或在原冻结树清理。
- G2/G3 Linux均真实重编44对象及模块，SHA同G1：`2e62f8b7614d48bf0eb493bc6bacf86726e650b3b066adf2fedf89c5dcccc707`（取自实际manifest linux_sha）。每代`fp-ctor.tsv`记录Root/rev/pkg/APE/ctor SHA、实际argv、收据SHA与作业句柄，不用复制产物假装代际。

## 最终实际证据与可复跑入口

- G2 win：`bg_310`，whole-o 58.84秒、外层73.30秒、rc0；G3 win：`bg_312`，whole-o 56.72秒、外层71.02秒、rc0。两者位于 `/tmp/winarm-fixed-build/g{2,3}/win/slice-win-arm64`，SHA均 `1eee599bbcec474b31893ce8f4b85f46947145660cbd3dae74f07333866d4a56`；父`cmp`实核两代互等且同c46-2原win/arm64片。
- 父在当前main干净私有克隆实际执行：`UNISACC_WAVE_REUSE=1 timeout 55 sh ./unisacc.com tools/check-win-arm64-fixedpoint.cx -- /tmp/winarm-fixed-generations.tsv --test-loader /tmp/loader-index-test-loader-native /tmp/winarm-fixed-native-probe.cx`，输出 `WIN_ARM64_FIXEDPOINT PASS_G2_G3_BYTE_FIXEDPOINT failed=0 missing=0`。同时真实默认-run Linux分片probe打印预期字符串并rc0；Windows原生项仍`NOT_CLAIMED`。
- `/tmp/winarm-fixed-manifest.tsv`只有冻结候选、不含两代证据。控制批实际执行三条：缺代际必须rc3；Linux ELF替代winarm且同步其SHA仍必须rc1；错误SHA必须rc1，输出 `WIN_ARM64_FIXEDPOINT_CONTROLS failed=0`。无参批入口要求显式绝对`TEST_WINARM_FP_MISSING_MANIFEST`及`TEST_WINARM_FP_APE`，缺绑定具名rc2，不回落仓根产品。
- 负控批由`tools/register.cx`实际登记为`win-arm64-fixedpoint-controls`并实跑PASS；只验证拒绝合同，不能把缺代际的NOT_CLAIMED记成真实固定点PASS。父最终13棘轮全过，工具真实/基线78616，新增检查65行。源码、候选、构造收据及PE身份绑定任一错误均不获PASS。

