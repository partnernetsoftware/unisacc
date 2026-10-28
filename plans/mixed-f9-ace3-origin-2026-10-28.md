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

sourcepackage的公共签名除新增显式origin API外不变。共用parser原布尔origin参数改成显式角色指针；普通/recorded调用仍传null，legacy origin明确取旧角色。共用context/producer/basis/映射分支均只在origin模式选择角色；本次尚未新实跑legacy完整正例和非origin union，必须在后续mixed资格中继续验证。

下一步：独立mixed controller/readers固定上述实际执行身份，构造仅3个JSON已变网，再验证旧107+当前43、全部源依赖和四端fresh；31个相同JSON的网不重造，不给当前context伪造kind2收据。当前材料仍73项，尚无新kind4包或NN功能PASS。
