# Gatekeeper 等待证据独立审阅

结论：新增日志把此前抽象的“加载/翻译状态”调查推进到**具体的Gatekeeper安全评估等待用户响应**：两次syspolicyd确实记录Prompt shown及waiting for response。对时间和签名身份能关联的启动，这比“Rosetta缓存损坏”更直接，应优先记为安全提示等待，不应修改kn_load或放宽entitlements。日志的path是散列标识，不能凭它把所有8秒超时、所有同名文件或先前采样逐一归到同一个提示。

本次仅实际读取 `parent/loader-system-log.txt`、核完整SHA及前轮身份材料；不重新运行log show、不启动程序、不操作UI、不删xattr/缓存、不改主树或签名。本报告没有独立重跑签后APE测试；父提供的最终5e11两格version/run7、FFI0、cache108及热性能结果属于父的另组证据，不能反填此前超时。

## 直接日志事实

来源为父bound10的 `/usr/bin/log show --last 20m --style compact`，predicate限定oahd/syspolicyd/amfid及目标名/原inode/PID，父报告实际退出0。原文SHA256 `c8a86f12dc4fd0e59b38a83477c562d816108e3d3adfa0ceb9e5d86210fd17cc`。日志14行：

|本机日志时间|进程与事件|可见身份|
|---|---|---|
|2026-11-09 11:29:28.089|syspolicyd[242]，evaluateScanResult0紧接Prompt shown (5,0)、waiting for response|PST path479920b76d47a35f，Team L2N7M5M544，id osx-x86_64，NOT_A_BUNDLE|
|11:38:16.404|同进程，同样scan0后Prompt shown、waiting|PST pathe2913e08c1a4172b，同Team/id/NOT_A_BUNDLE|
|11:34:31.381及11:39:22起其余新路径|evaluateScanResult2|path分别9f9a320ac8d94646及后续不同散列，同Team/id|

不能把scanResult数字0/2或(5,0)自行解释成稳定公开API枚举；本结论依赖明确的等待文字，而非猜数字含义。NOT_A_BUNDLE只表示评估对象为裸命令形式，不表示签名无效。日志时间是本机显示时间；PST在这里是记录对象标签，不是太平洋时区。

父观察将11:29窗口关联原路径初挂、11:38窗口关联encoding保留quarantine私有成员首挂；这与同字节私有副本成功、另一执行对象不同扫描结果相容。该关联有时间及Team/id支持，但这些行没有明文绝对路径/目标PID/inode，不能独立从path散列反推出具体文件。尤其11:35采样PID82661与11:29或11:38事件不是同一时间点，不能说其769个Rosetta采样已经逐条证明等待同一UI。predicate中的inode/PID筛选条件也不等于每条输出都命中inode/PID，OR条件可能只命中osx-x86_64名字。

## 可收紧与不能推出的结论

- 可确认：这两个同签名标识的安全评估实例曾显示提示并等待回应；此前仅以Rosetta Routine栈推断缓存问题应降为未证假设，不能继续作为既定根因。
- 对已绑定源码的正常cleanenv --version，前报告核实早退链不调用core_run/kn_load；故不应以该挂起要求改RW→RX。新的syspolicyd证据是独立的系统评估等待解释。动态CRT/实际机器码绑定仍是此前明确边界。
- 不能确认：每个8秒142均等待这个提示、提示实际可见于当前桌面、用户已响应/未响应、最终授权结果、AOT是否命中/损坏。142仍是watchdog退出，codesign strict0仍只证明静态签名结构，不能等同Gatekeeper已放行。
- 不能把“同SHA新inode可运行”当必须去quarantine、复制绕过或清缓存的发布步骤。首用安全提示是分发路径和用户体验合同的一部分，应保留真实quarantine与原日志，不能用私有副本成功抹掉它。

最小后续是先在现有launch receipt中补时间窗、实际目标PID、绝对路径/文件SHA及quarantine状态，并与这些安全评估事件关联；不再重挂已知原路径。若尚不能将某个timeout唯一映射到PST对象，保持该作业UNKNOWN/待关联，不强行迁成“Gatekeeper已定位”。正式签后5e11的真实执行及缓存/性能通过可独立记为新候选证据，但不能代替自然下载首用提示、Gatekeeper/公证信任及Intel原生资格。
