# PP 采集器对齐空白修复（2026-10-23）

实际跑过，全部在 8b82f76d 的 scratch 导出及 unshare -rm 隔离内执行。

首次重封在 capture 步以 rc2 报 header stream not closed。短隔离路径让 strace 输出 `read(...)    = 0`，原 result_suffix 只匹配固定 `) = `，遗漏 EOF，未生成闭合头流资源。

修复扫描最后一个右括号后的空格/制表符与等号，返回数字起点；同步三个调用点。自测覆盖带 syscall 样式文本的负载、对齐空格 EOF、制表符分隔，不改变编译器产品。

验证：ppleafcapture --selftest rc0；正式 reseal-pp-literals 四步 rc0（25.96、31.17、9.20、9.30 秒）。负例检查 rc0，遗漏依赖、改预定义、改 stdarg 内容均拒绝。候选 SHA256：4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda。封件目录摘要：fc0a4656087ce39e1059ff7defa42bae6b57aecc876608c3c592b9a677fcbb6a。

重封输出及工具链均在测量根之外；没有手填身份，没有授权复用，也没有发布。
