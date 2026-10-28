# f9 → ace3：origin 材料验证检查点

本提交只增加显式 `f9-ace3-v1` origin 角色、SourceRoot/ToolRoot 分离和捕获读取；mixed controller、reader、产品消费者尚未获得整包资格。旧329/e252角色保留，未将旧NN功能结果迁移为PASS。

角色表只含材料语义，不含任何导入它的 verifier/controller 自身 raw hash。执行 ELF、源码、模块闭包与 profile 的实际身份，由独立消费者固定。

历史逻辑根是 `/home/devali/repos/unisacc-epoch-fullname-778fc68ebc`；原context/PC不改。独立捕获为 `out/c43-f9-origin-capture-01`，原Root94及record通过保留的 `out/epoch-build-01/Root41-bootstrap.com` 映射；不能用后来已安装D65代替。SourceRoot为仓外 `unisacc-epoch-aggregate-13d8cfe478`，ToolRoot为共享仓。

唯一额外历史外部键 `/usr/bin/x86_64-linux-gnu-gcc-11` 映射到捕获内 `out/origin-host-data-v1/cc`：固定928584字节、SHA `821af3c74506283c179ca413bb33e6b528805a4dd8a5c09df125e5ad560a9e89`，数据无执行位。其0400模式不是原CC执行器mode等价，实际mode进入endpoint/mapping见证；捕获数据从不被执行，无活宿主fallback。实际CC路径经 `digest` 校核固定身份；通用内部load接口不声明同样的强制身份政策。端点视图仍非原子，也不提供ABA历史证明。

实际日志位于 `out/c43-f9-origin-exec-01`：

- compile-02：rc0，原ELF `72e1d772…`；固定CC映射前 snapshot-first-01 rc2，拒绝 `origin-path-outside-logical-root`。构造预算55秒不变，该次曾与grk构造短暂重叠，无重跑。
- compile-03：新增固定CC映射后rc0/stderr空，ELF `95d874f3526f49dc0b13b7a8703bb1636a791a8012b71e31690e1fae2e1c42f0`。
- profile-02：rc0，SHA `7fd59c90fc409c4bdd7cee2c1e3054ffd7f9ba95558e159007f92dd0c3ae30ed`。
- snapshot-mapped-01：rc0/stderr空，完整107作业/34模型、原Root94+fe5d record；父进程实际解析新profile/spec/producer/foundation成功。physical-reads=1005，execution-reads=152，elapsed=5.503800080秒。报告SHA `5f043c296f0b2bd0b94b26cf769942312c9215535efe657c50e8d91e908855f2`。
- host-negative-01：独立negative capture中错hash、逃逸symlink、未知外部键各实际拒绝；恢复后完整107/34通过。context/CC与正例逐字cmp一致且mode恢复0400。日志SHA `f4bc85b3459d5d659dada79c9bc2ee108a2acb2fb8ad57eded6d24c83a393f47`。
- host-execbit-01：同字节chmod0500实际拒绝 `origin-fixed-host-data-identity`；恢复0400后完整107/34通过，命令终态rc0/stderr空。
- `out/c43-mixed-profiles-short-02.stdout`：默认-run角色合同9/0；不代表完整复用资格。

默认角色回归入口：`sh ./unisacc.com tests/source-package-mixed-profiles.cx`。该测试另有逐次真实origin正例/显式未知profile/legacy-v3/错误来源/根别名拒绝入口；见源码用法，每次各有独立输出目录。准备的入口不能冒充已执行；本检查点实际父子正例由上列native snapshot证明。

sourcepackage的公共签名除新增显式origin API外不变。共用parser原布尔origin参数改成显式角色指针；普通/recorded调用仍传null，legacy origin明确取旧角色。共用context/producer/basis/映射分支均只在origin模式选择角色；origin层首次提交未新实跑legacy完整正例和非origin union，后续实际43项current union已通过。legacy完整正例仍未新实跑。

## 同日后续材料构造

仅三个JSON已变网完成正规start/finish：shared-e3 `fbae160942499fafa47cbcc2607e75053190a38572603f29de147e484c279611`，warnparse `b29966210ad0beb9b6b7d83a8cd5f575090555aaf780440079c312d4ab45c139`，errorparse `1aa3c025ecee9f043869d3f463c983dac50889bae71c9547a0b33f869590c7dd`。三份与private03材料逐字相同，当前context仍b09d、实测SID仍ace3。原73项不重跑，当前仅76项材料，31个相同JSON的网不重造，不给当前context伪造kind2收据。

冻结SourceRoot原kernel-build/default-run D65完成九相正规KGEN：`out/epoch-kernel-01`，context `58adde8fed71ae734e4e21056ec77b463eb7dbed16d40ac2fecb98898ec5e0eb`，新product-built helper实际0b8f5d与既定pin匹配；双kernel与44份s/o收据齐全。未复制GCC helper。该根 `out/epoch-build-01/checkpoint-net3-only-01.tsv`、`checkpoint-kernel-complete-01.tsv`记录全部真实session和终态。

mixed controller01静审发现origin executing-module仍取SourceRoot，修复为显式profile取ToolRoot后只重编controller02。实际源 `c2257bf86a0af86aa738c21daaef2769eb02724fc5a642ce85b44eca5211886f`，ELF `4343993ccaa402003f4b2e08ec1f249ca54048b11a5642a541fdeba7ac44f912`，无verifier自哈希循环。

新PC为ToolRoot下 `out/c43-f9-ace3-mixed-pc-01`。prepare、declare、declare-finish、pack、pack-finish、seal均真实终态rc0/stderr空，遵守原55秒外层和原40秒native预算。独立静审selection34条精确31equal/3diff，current-union43/models3；origin107/34与四端fresh保留。实际新包：

- compiler.pkg：1,632,679字节，`661c2b02bb9fce9f6d6939a3ee7b97242462daf5ce70b7604cea832d5fe53d6f`
- model-inputs.tsv：`d1fd67a404c8b7cbb6cb13fec4911ab5457cc73ff8ce54201e7e76a2eb27f768`
- package.tsv：`3fb50654c099bb1ad55e4daeafc69a3288f7104d8d30e054b9430798c801b324`
- context.tsv：`8cb8e390d4085b5da57afb3d3dd2959359455518ce1d5391fda8e15f88c09ee2`

这些是kind4材料构造收据，不能迁移旧B1的NN功能PASS。日志为 `out/c43-f9-origin-exec-01/mixed-{prepare,declare,declare-finish,pack,pack-finish,seal}-01.{stdout,stderr}`。

独立mx reader默认-run探针 `out/c43-mixed-reader-01.cx`：session7212真实终态rc0/stderr空，`mixed-reader-01.stdout`一条综合断言通过，检查profile/SourceRoot/ToolRoot、完整origin107/34、当前43/3/31及origin读取1005。该入口实际重新执行controller snapshot并独立观察代码/包前后身份，没有沿用seal结果。reader模块SHA `f36ae9573135b6f511a8a1fcd7a250843a8bc09ed584b08a44c2b327287c434f`。这证明本对显式profile的真实材料正例；不代表所有负控或产品准入已通过。产品构造/准入继续单独执行。

产品消费者接续：default-run observe（session4727）和保存工具的-O2编译（session15288）均真实终态142，原55秒预算不变，空stdout/stderr保留，未生成产品context。分离构造工具采用-O0，产品切片仍要求-O2；工具产物 `product-build-O0-02.elf` 为2,099,776字节，SHA `f9afd1066da04d2bf6c4f4e58b27ecdd36b7bbe41f401b2891e28fc796fef764`。执行代理容量错误后由root接管，原编译退出码暂未取回，明确UNOBSERVED；不因ELF存在记录编译PASS。root在确认原所属进程消失、context不存在后，以该ELF启动独立observe，日志 `product-observe-native-02.*`，原55秒预算，无重复编译或缓存核验结论。

独立native observe（session71096）真实终态0/stderr空；1348次物理读取，材料核验30.929054702秒，产品context `fe3650a0cf346835c8152fdf689fee023ded6bf606bf682fc25e3684e52ec4ec`。报告jobs150是历史107与当前union43之和，不能改称当前150个新构造作业。该正例通过后接五切片构造与独立finish，仍不授功能或平台运行资格。
