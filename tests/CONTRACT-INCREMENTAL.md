# 0.0.43 动态 C 合同改造

范围限定 tests/ 与 release/tools/；不改变产品闭包、候选身份、gatequeue 指纹或 gatedeps 分层。每项命名闸独立验收，旧队列不续跑、不回填结果。

## ABI 推导

动态 C 入口：`sh ./unisacc.com -run tests/abimanifestcheck.cx -- --check`（生成用 `--write tests/abi.manifest`）。Python 两文件仅是已登记的逐字节对照。

初始化列表按词法屏蔽后的括号/方括号/花括号栈切分，仅顶层逗号分项；函数体配对忽略注释、字符串与字符字面量。未闭合注释/字面量、失配分隔符明确拒绝。guard 只读取外层 Windows/POSIX 两分支，嵌套 else 不抢外层分支；额外外层 elif 明确拒绝。loader 条件记录每个 elif 及此前分支，else 记录完整排除链。

自测含 f(a,(b))、复合字面量、引号/注释花括号、嵌套 else，及失配/未闭合负例；源码突变含 elif loader 新槽位和不支持外层 elif guard。清单头以 .cx 标识生成/检查入口。当前396条内容逐字节对拍。

## 后续独立项

APE正式槽位绑定；早段止于lex的封存数据及pp-literals叶子样板。封存不是跳过新候选行为闸的许可；依赖/产物/协议/拒绝覆盖变更必须失效。尚未接入指纹复用。
