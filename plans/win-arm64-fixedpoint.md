# win/arm64逐片固定点：扩六片方案（待cc批准，未实施）

**结论。** 编码/落盘链已支持win/arm64，固定点缺第六片是工具枚举缺项，不需新增平台后端。保留当前混合构造协议，补第六片并同步修复旧checker布局；先设计，不重跑已知五片构造来确认。

**已核实落点。** `tools/chain-step.cx:521-523`的输出数组、`:550`目标数组都只有五项，`:533`比较数和`:558`循环界限写死5。追加`win/arm64`与`win-arm64/slice-win-arm64`，两数组使用同一派生计数，避免日后只加构造不加比较。**`:617 items=5`指objects/kernels/slices/product/package五类检查，不是片数，不改成6**；另加具名`slices=6`结果。现有`release-build.cx:828,841`、`release-ape.cx:50,69`、`slice-freeze.cx:189`与原生CI已识别第六目标，无须改产品代码。

**既有checker缺口必须同时收口。** `tests/chain-fixed-point.cx:61-62`仍读`g2/candidate`、`g2/receipt.txt`顶层；实际工具`:563`按`g2/<os>-<arch>/`建目录。改为逐片检查真实objects/whole-o布局，缺失、重复、字节不同、目标身份不符都拒绝；不能仅把枚举改6后宣称资格完成。批次仍为`chain-fixed-point`（`tests/batches.tsv:397`），不迁移旧失败或旧资格。

**构造与身份。** G2的Linux两片使用G1构造器，G3的Linux两片使用G2 Linux构造器；whole-o的OSX/Windows各片使用**本代**Linux candidate（`:564-566`），不是每片G2都由G1、G3都由G2。保留该混合协议并显式记录每代/每片target、实际argv与constructor SHA、Root/SID、P3 SHA、输出路径及SHA。whole-o receipt目前缺constructor行时，增加最小`fp-ctor.tsv`记录，不伪造旧receipt。检查器按路由验证链，既核六对最终字节，也核原生格式身份；win/arm64的PE Machine必须0xAA64，不能拿同样相等的x86_64文件冒充。

| 文件域 | 估计净增 | 验收重点 |
|---|---:|---|
| tools/chain-step.cx | 20–40行 | 六目标构造与比较同源计数、逐片身份/构造器证据；五类结果含义不变 |
| tests/chain-fixed-point.cx | 50–80行 | 新布局、六片逐字节与路由身份；第六片缺失/损坏/错target/错构造器、异包/异Root拒绝 |
| 注册/覆盖/说明 | 2–4行 | 明确六片与单宿主混合固定点范围；toolsloc按实际净增申请，不预抬基线 |

**执行门。** 批准后先跑现有负控与新增真实边界回归，再用同一冻结Root/P3实际构造两代六片，六对SHA和原始退出状态全部保留；任一缺失/TIMEOUT/未知即非PASS。沿现行逐作业bound与whole-o既有例外，不新增时限豁免、不缓存结论；保留可恢复片目录。最后13棘轮及同候选正式固定点批次必须绿。

**不证明什么。** Linux上生成并比较PE ARM64仅证明该交叉构造路径的逐片字节固定点，不能标Windows ARM64原生自举，也不是独立第二次构造。原生仍须`candidate-native.yml:306-318`的`windows-11-arm` runner实跑该候选；`id.reproducible-construction`不因此关闭。当前未实施、未跑新构造，不能记PASS。
