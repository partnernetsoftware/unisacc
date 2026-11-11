# osx车道最终安全门

最终13项现行静态棘轮全部实际通过：父会话59831真实退出0，13ok/0fail，54.080秒，原内层55秒和外层60秒未放宽。完整汇总见ratchets-final.log。此前汇总12项绿而coreapi超时，以及独立55秒超时，均保留，不把错误覆盖成历史PASS。

coreapi只改变测试工具的重复构造：本轮以当前产品一次编译当前symindex，再按原参数逐次执行全部模块与别名调用查询，未缓存查询结果。26个模块、194个导出和基线未改；正常检查13.874秒、原8项正负夹具14.322秒均真实退出0，transport0/residue0。源码及输入身份见coreapi-final-v2-identity.tsv，完整原始报告和两检查日志同目录。冻结产品域exec/src/kernel/include/weights/seed/cx无本刀修改。

合入远程冻结门及R1后，[25个提交门相关正式批次](../c48-toolsloc-report-only/post-freeze-report.md)全部实际退出0；行数保持纯统计，新增工具的check/批次登记与冻结门保留。[链检查](../c48-native-selfhost/safety-final-report.md)完整74/0；[最终缓存检查及真实六槽私有载体](../c48-macos-distribution/report.md)16/0、ARM和Rosetta版本0/hello7。旧研究执行源码原字节归档，原SHA仍可核。没有扩静态基线、行数限额或执行时限。

[osx对象合同](../c48-osx-object-contract/report.md)最终六批174/0，runner平台合同17/0；只证明metadata-only unit容器和产品链接，x64执行是Rosetta。完整系统ABI对象、Intel真机、macOS13、Gatekeeper信任及全658功能批/发布资格没有由这些结果获得证明。既有Darwin runner-faults的5项失败在旧新harness均复现，DARWIN-RUNNER-FAULT-CONTRACT仍READY，未删断言或宣称已修。未安装产品、改公开资产、补签或切Latest。
