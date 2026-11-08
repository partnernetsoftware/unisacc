# X2：44根 whole-o 四目标恢复60秒合同

结论：当前C47产品已具备此速度；本轮没有声称新做产品性能优化，而是对固定输入真实顺序构造四次，确认全部低于60秒，并把release-build默认whole-o看门狗恢复为60000ms。四次真实作业均由父持有句柄并poll确认exit0；最终只读CX审计95932实际exit0，四份receipt、实际镜像SHA/大小、MachO/PE目标CPU和ns上界全部通过。与旧C47四片逐字节相同仅是历史身份对照，不代替本轮构造执行。

| 当前目标 | 父会话/终态 | receipt秒 | 外层real秒 | 镜像B | CPU/历史字节 |
|---|---|---:|---:|---:|---|
| osx/x86_64 | 84412/0 | 29.572228 | 31.00 | 2410978 | MachO x86_64/相同 |
| osx/arm64 | 20658/0 | 30.085927 | 30.29 | 3071458 | MachO arm64/相同 |
| win/x86_64 | 68870/0 | 22.826450 | 23.02 | 2663424 | PE x86_64/相同 |
| win/arm64 | 58724/0 | 23.330840 | 23.56 | 3369472 | PE arm64/相同 |

父实际调用：固定rev58093906d，四目标逐次一次，默认60秒、不设UNISACC_RB_WHOLE_MS override，外bound60、UNISA_JOBS=1、PROFILE/RRTIME开启。环境和真实进程终态见父命令证据及parent-terminals.tsv；receipt本身不独立证明环境变量。receipt ns是release-build whole子阶段计时，不等同外层real，也不据单次样本宣称冷暖分布或跨平台原生运行性能。这里实际验证了跨目标生成的文件格式/CPU，没有冒充四平台各自原生执行。

## 固定输入与44根

revision为58093906dcb356d92df13553776dcb19cbff796a，实际快照目录out/release-snap-58093906dc（sha前10位）。CX审计从当前release-build实际rb_whole_src数组读取44项，核快照数组顺序相同，逐根核snapshot/current真实内容一致；合法内部symlink按realpath解析、禁止逃出各自root，并核resolved相对路径一致。初次21747因直接拒绝合法bindstage源码链接而退出2，未记PASS；修正后prepare71318实际exit0。

final-audit.tsv中SOURCE0..43为有序44根清单，SOURCE_COMPLETE的ordered_manifest_sha为5a40ccf45fb7e24a333e2b77b3195f08c0bf5c0491670e958f3fc597f8445f8d。审计还实际执行git diff --exit-code 3b41cc224f 58093906dcb356d92df13553776dcb19cbff796a -- src cx exec seed include，结果EMPTY；这是两固定revision的产品输入差异证明，不冒充整个共享工作树任意文件闭包哈希。

| 执行输入 | 实际SHA256 |
|---|---|
| tools/release-build.cx（默认60000ms） |46671add95316041160768aae6cb37941342907d824857a014f184e761d8a53a|
| out/c48-x2/release-build |5fb91caf66447b0df57a918b275fd578c3e51a7c96ac63e90d16bebe767f811e|
| constructor（直调当前产品） |fc45d417f459a4cd721f1f37485ae438d9d9e998e38bf2d6f28df1f408580db3|
| p3-package.body |0d5696535d0062d038bc71a97b266904b69f174a9b799555c2981d5b182c5ad4|
| 当前产品/候选 |f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82|

所有输入身份由本次CX审计实际读取验证。审计源audit.cx SHA515f66d6dd7c0914edd8d8380e1e02e80314c49dd169fef91cdeb13371dfab50；最终审计TSV SHA023c72b25aa9b34505d6241b3cd387a8a68768b9d04b747ab44a4e74200a8ed0。审计没有启动构造。最终TARGET行各自绑定实际image、receipt、完整profile/RRTIME日志SHA，核日志终末whole-o行与receipt一致；四镜像SHA及C47对照均保留在其中。

## 合同检查与历史失败

- whole-o-deadline正式批PASS/rc0/transport0，独立16/0；直接日志与登记日志都保留。
- slice-freeze-tool最终正式22/0、PASS/rc0/transport0（45879实际exit0）；保留早期entry fixture FAIL/rc2日志。父定位为本机缺/bin/true的fixture入口问题，修复路径后重跑，未删断言；repo.untouched窗口内暂停共享写入，未用并发写制造假失败。
- check-limits PASS；远程改动相关net-deps25/0、interface-digest28/0、func-callgraph0失败的本机正式重验均PASS，原BATCH_RUN与候选身份保留。net-deps日志中的REF_MISSING是明确负控，不隐藏为未知绿。
- 旧120秒例外与旧C47日志不授权本轮完成；本轮四个真实当前receipt低于60秒以及默认源码60000ms共同提供恢复合同证据。AGENTS例外撤销由父集成，本审计不改主文档。

## 归档与边界

此目录只归档四份原始receipt、四目标小stdout/stderr、最终审计（含identity与44根）、父终态表、门禁及早期失败日志和审计CX源；没有复制镜像或构造快照。实际镜像留out/c48-x2，各receipt声明的原始路径与数据不改写。HASHES.sha256排除自身覆盖本目录全部其余文件，可从仓根用shasum -a 256 -c核验。

当前结论只覆盖同产品、固定包、固定revision和44根的一次顺序本机生成；不自动等于完整发布资格、所有性能场景或所有目标原生CI完成。材料与结论分账，未执行事项不记PASS。未提交，父负责按路径集成。
