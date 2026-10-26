# 0.0.43 同候选 CI 输入草稿

本入口只解决候选输入与身份核验的断链，不是发布资格。当前工作流未触发、工件未上传、六宿主未实跑，统一为 **NOT_RUN**。文件只响应将来另行授权的 `workflow_dispatch`；没有 push、PR、定时、上传、签名、Draft、Latest 或公开动作，原 `ci.yml` 不变。

## 输入合同

人工提供七个非空字面量，不提供分支名、命令或候选 URL：

| 输入 | 含义 |
| --- | --- |
| `producer_run` | 本仓已完成且成功的 Actions 生产运行 ID |
| `artifact_id` | 该运行内一个仍有效的不可变 Actions artifact ID |
| `com_sha256` | 原始 `unisacc.com` 字节 SHA256，不是 ZIP / Actions artifact digest |
| `source_sha256` | 现有 `seed/ident.c` 定义的完整产品输入闭包摘要 |
| `verification_commit` | 精确的40位小写 Git commit；固定测试、计划和辅助代码 |
| `plan_sha256` | 下述六文件规范摘要 |
| `build_record_sha256` | 原始 `unisacc.com.build.json` 字节摘要 |

工件 ZIP 必须正好包含两个顶层普通文件：`unisacc.com` 和 `unisacc.com.build.json`，不得附带测试源码或嵌套目录。单个文件限制64 MiB，归档元数据限制128 MiB。先通过 REST 核工件归属运行、有效期和生产运行成功状态，再下载。下载身份、生产运行 head 和归档摘要会记录；归档摘要不能代替外部提供的 raw com 摘要。

`verification_commit` 可以比产品构建记录的 `commit` 更新，例如只新增测试；其产品闭包必须仍与 `source_sha256` 一致。构建记录的 source/raw-com/bytes 必须匹配；POSIX 分支再默认-run **现成** `seed/ident.c` 重算当前 checkout 的产品闭包，不能只信构建记录自述。这个重算由已独立核哈希的候选执行，不是独立宿主编译器验证，也不证明构建或三阶段链正确。

输入从结构化 Actions `env/with` 传递；严格校验数字 ID 与小写定长摘要。候选命令使用 `.NET ProcessStartInfo.ArgumentList`，不把输入展开成 shell 命令文本。下载 token 仅存在下载步骤，候选执行步骤不接收它。

## 原始字节、计划和软链

checkout 前用 Git 环境配置禁用 `autocrlf` 并要求 `core.symlinks=true`；已有 `.gitattributes` 的 `* -text` 同时保留。checkout 后核 HEAD、已跟踪文件状态，并逐一核 Git `120000` 条目仍是实际软链。Windows 缺软链权限时应失败并留下 NOT_RUN，不能接受文本占位文件或复制展开来冒充相同来源。

规范计划摘要按以下**固定顺序**计算，各项贡献 UTF-8 相对路径、一个 NUL、该文件原始字节 SHA256 的32个二进制字节，最后对拼接流做 SHA256。没有 CRLF 转换，没有摘要文本换行：

1. `tests/coverage.tsv`
2. `tests/batches.tsv`
3. `tests/run.cx`
4. `tests/plan.cx`
5. `tools/cxlib/testplan.cx`
6. `tools/ci-candidate-check.cx`

本机生成待审查的摘要使用默认-run：

```sh
tools/bound 20 sh ./unisacc.com tools/ci-candidate-check.cx -- --plan-sha "$PWD"
```

宿主 PowerShell/.NET 会独立重算 raw com、构建记录与规范计划摘要，然后候选运行的 CX helper 再核这些字节。计划摘要只覆盖所列六文件；其余模块、用例及源码固定于 `verification_commit` 和完整 checkout 校验，不宣称六文件摘要是整个仓库摘要。计划或相关输入改变后必须使用新摘要，不能沿用旧 CI PASS。

## 宿主、进程与输出目标分别记账

| runner 标签 | 实际宿主期待值 | 候选进程期待值 | 本草稿实际证据 |
| --- | --- | --- | --- |
| `ubuntu-24.04` | Linux x86_64 | x86_64 | NOT_RUN |
| `ubuntu-24.04-arm` | Linux arm64 | arm64 | NOT_RUN |
| `macos-15-intel` | macOS x86_64 | x86_64 | NOT_RUN |
| `macos-15` | macOS arm64 | arm64 | NOT_RUN |
| `windows-2025` | Windows x86_64 | PE x86_64 | 待实际运行；草稿仅安排 `--version` 直接进程观察 |
| `windows-11-arm` | Windows arm64 | **PE x86_64，模拟进程** | 待实际运行；草稿仅安排 `--version`，不计 ARM 原生进程 |

运行时记录 `.NET OSArchitecture`、实际 PowerShell `ProcessArchitecture`、候选 PE 头与候选 helper 的编译器预定义宏。矩阵只是期待值；helper 宏是候选编译目标观察，不能改名为独立操作系统进程观察。Windows 分支现在编排一次直接 `CreateProcess` 风格的候选 `--version` 观察，并核精确版本输出及运行前后产物摘要；该草稿尚未远端调度，所以实际进程仍为 NOT_RUN。PE 头只能说明映像架构，WinARM 上不能据此声称 ARM 原生进程。六种输出目标未在本草稿逐一生成或验证，全部独立保留 NOT_RUN，不能把六个 runner 或一个 APE 的切片当作六目标执行通过。

Windows 的必要后续工作包括：真正的 JobObject 进程树 watchdog、原生路径/模块软链、source identity 的目录 API、注册表执行器。草稿已为无子进程的 `--version` 加入直接 `ProcessStartInfo.ArgumentList` 启动；还没有远端实测，也不能将它扩成需要后代清理的功能执行。现有 `tp_analyze` 要求 POSIX 绝对根，现有 `np_run` 依赖 fork；本草稿不改共享接口、不转用 Git Bash 掩盖差异，也不在 Windows 调用 `np_run`。这些缺口必须先解决，才可加入功能候选执行观察。

## 调度与结果

矩阵 `max-parallel: 1`，每个宿主内各命令顺序执行，不同时构造产品。REST 单请求40/45秒、零自动重试；普通进程外层53秒，预留2秒清理。POSIX 候选命令再用现有 `tools/bound 50` 的进程组/后代清理；其首次启动仍有明确的宿主 C 编译器引导依赖。下载与 checkout 是基础设施，不是产品准备成功或测试 PASS；checkout Action 只能设置整数分钟的 step 限额，当前为1分钟，其内部 Git 尚无独立55秒包装，严格端到端55秒基础设施预算仍未结证。

POSIX 分支按序尝试来源摘要、CX 元数据和当前 `tests/run.cx --all`。helper 仅列注册表，执行批次数固定为0，返回3 / NOT_READY；`--all` 原非零结果保留。即使将来 `--all` 返回0，本草稿仍以3终止，因为完整注册表执行、材料身份、同候选全部宿主和严格三阶段链没有在这里实现。没有 smoke profile，更不将 smoke、元数据合法或材料 PREPARED 升格为资格。

`tests/batches.tsv` 的每批<=55秒只是声明上限，不是已测时长；有材料依赖的一律 UNPREPARED，没有执行的保持 NOT_RUN，PLANNED/未完成覆盖保持原状态。执行器后续应每次只运行一批，以当前候选、source、verification commit、plan、宿主/进程、输出目标和全新 attempt 绑定结果；不得跨 attempt 缓存 PASS。材料下载/准备及逐项身份验证目前不在这个工作流中。

无 Actions token 读权限、工件过期、生产运行或 artifact ID 不符、runner 不可用、下载/checkout超时、软链权限或工具缺失都应失败；未到达的阶段是 NOT_RUN，不能报告通过。禁止为了试跑而 push。本地临时工具只做语法检查与独立计划摘要核验，REST 下载、runner 和宿主适配尚未实际执行，不能称六宿主适配已验证。

## 官方接口依据与下一步

核对日期：2026-10-26。runner 标签依据 [GitHub-hosted runners reference](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)；checkout 使用已核接口的 [actions/checkout v6](https://github.com/actions/checkout/tree/v6)，已通过官方仓 `git ls-remote` 将 v6 固定为 `d23441a48e516b6c34aea4fa41551a30e30af803`，同时核其 action.yml 输入接口；未声称完整审计 Action 的实现。下载直接复用 [GitHub Actions artifact REST API](https://docs.github.com/en/rest/actions/artifacts)，按 run/artifact归属检查后下载 ZIP，不假设 checkout 自带被忽略的 com。读取私有工件需要 `actions: read`，runner 可用性及仓库用量权限尚未实测。

下一步先审查并提交完整技术草稿；待明确授权、可读取的精确私有工件和固定验证 commit 后，才能手动调度。先补 Windows watchdog/路径与完整批次执行，再取得当前候选的真实本机证据、固定点、严格三阶段和全部宿主证据。签名、Draft、Latest、公开仍禁止，本文件不授予这些权限。

## 本地验证边界

2026-10-26：官方 actionlint v1.7.12 校验和匹配，新工作流静态 lint 返回0；官方 PowerShell 7.6.8 Linux x64 发行包 SHA256 与发布页一致，提取的四段 run 脚本全部 ParseFile 语法错误数0，未执行工作流代码。CX helper 默认-run 元数据自测11断言0；缺必要输入返回2。PowerShell 独立计算的六文件计划摘要与 CX 相同；摘要只绑定当时的本地输入，不作为未来候选或 CI 证据。原日志在 `out/c43-candidate-{actionlint-01,pwsh-parse-01,plan-host-01,plan-cx-02}.log`、`out/c43-ci-candidate-{controls-02,absent-input-01}.log`。没有执行任何功能子批次、source identity 重算、产品构造、远端 CI 或上传。

2026-10-27：Windows 分支补入直接启动 PE 的 `--version` 单进程观察，仍明确退出3/NOT_READY，功能、来源闭包和六目标继续 NOT_RUN。当前主树 `out/actionlint` 对修改后工作流返回0；用本地 PowerShell 7.6.8 解析新第三段 run 脚本，ParseFile 错误数0。未在 Windows 运行，此静态证据不授候选进程实跑资格。

同日当前主树的本机输入再核：P75 默认运行 `seed/ident.c` 得完整产品源码闭包 `718017ab98ae13963d0b928496724c0e661990b3972ba452909742ccfa6f6a81`；候选 CX helper 与独立 PowerShell 原始字节算法各自计算六文件计划摘要，均为 `3ca5b2a0d00ffcfca9a0b85a367901594c380eaeb2ab250221a4999a093f249a`。这是本地此时的身份观察；后续清单或工具字节改变须重算，且共享检出尚有其他协作者暂存文件，不能把该摘要当作已固定的验证 commit 或远端 CI 结果。

本地还用产品摘要、活构建记录摘要、现行来源/计划摘要及 Linux x86_64 轴执行 `--check`；为只测元数据路径，producer_run/artifact_id 明示填入**合成的 `1/1`**，绝非真实工件身份。结果 rc3，严格解析 112 批、19 PLANNED、74 UNPREPARED、19 覆盖缺口、执行0，并输出 `QUALIFICATION NOT_READY`；无批次或远端候选运行，不能转用这次合成调用作生产者收据。
