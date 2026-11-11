# 0.0.27 发版复盘（2026-10-05）

版本号提交 11:50 → 公开 14:19，共 2 小时 29 分。其中正常流程约 1 小时 20 分，返工约 1 小时（两次挪 rc、一次因负载暂停、一次只重跑 6 项）。

## 各环节耗时

| 环节 | 起止 | 耗时 | 备注 |
|---|---|---|---|
| 结账、版本号提交 | 11:45–11:50 | 5 分 | C1 矩阵绿 → ledgercheck --final 未结清 0 → f14de6a2 |
| 同源参考 ua | 11:50–11:51 | 1 分 | build_ref.sh |
| 候选构建 | 11:51–11:53 | 2 分 | 七步 + pack，product freshness 已核对 |
| comboot | 11:53–11:59 | 6 分 | 第 2 阶段第一次撞 58 s 上限，续跑一次就过；不动点 a488863e |
| fb12multi + comdemo | 11:59–12:01 | 2 分 | 10/10，19 个程序全过 |
| 封存 + 戳 + 推送 + rc | 12:01–12:03 | 2 分 | GHCR 摘要 0216f5b9 |
| precheck | 12:03–12:13 | 10 分 | 第一轮 3 项失败（2 项工具缺陷、1 项漏设环境变量），修好后第二轮通过 |
| 队列第一轮 | 12:13–13:04 | 51 分 | 620 项，2 红（测试工具缺陷） |
| 修红、挪 rc | 13:04–13:08 | 4 分 | a8081988，rc → 5bfbef1a |
| 队列续跑（含暂停） | 13:08–14:09 | 61 分 | 13:21 因负载暂停（另一会话开的 Windows 虚拟机），13:24 修 release-check 红并挪 rc 到 feb7a694，13:33 续跑；6 红来自缺语料 |
| 补跑 6 项 | 14:09–14:10 | 1 分 | 补语料软链后全绿，final rc=0 |
| Linux 全套 | 14:10–14:18 | 8 分 | 原生 arm64，wall 479 s，只剩客机已知红 |
| Apple 签名公证 | 14:10–14:12 | 2 分 | 与 Linux 并行（不耗本机 CPU） |
| 草稿 + 回执 + Windows 两次签名 | 14:12–14:17 | 5 分 | 资格 37271345056、公司 37271458988，Valid |
| 签后复测 + 公开 | 14:17–14:19 | 2 分 | comdemo、fb12multi 跑签后文件；只留 unisacc.com 和 dmg |

## 问题与解决

| # | 问题 | 根因 | 解决 | 持久化 |
|---|---|---|---|---|
| 1 | precheck 报 4 行未结清 | 版本号提交后，ledgercheck 默认查“版本号 + 1”（v0.0.28） | 972d69d6（precheck 显式传计划）；a8081988（ledgercheck 默认查未归档的当前版） | 已修代码 |
| 2 | precheck 要求本机开 x86 Lima | C2 把 x86 格子迁到 CI 后，precheck 没跟着改 | 972d69d6 删掉这项检查 | 已修代码 |
| 3 | 队列 gate-layers 红 | 新登记的 seed-construct-* 没分层 | a8081988，STAGE_PREFIX 加上 seed-construct- | 0.0.28 R2：封存前先跑契约层 |
| 4 | release-check 在 main 连红三次 | 加时间放大时给 bound.py 加了 1–60 上限，但 workflow 里还写着 120 | feb7a694：改成 60，加 UNISACC_TIME_SCALE=2 | 0.0.28 R3：workflow 里的 bound 参数进静态检查 |
| 5 | rc 挪了两次 | 上面 3、4 两项都是封存之后才暴露 | 闭包外 fix: 后重打 rc，候选字节不变，队列按指纹复用结果 | RELEASE-PIPELINE 补一条：封存前 release-check 和契约层必须先绿 |
| 6 | 续跑后 corpus、diag、warn 共 6 红 | 被 git 忽略的 corpus 不会进 rc 工作树，queue.sh 只从启动目录链 c-testsuite | 8ef0cdc1：从主检出链全部语料，REALPROG_CACHE 同步 | 已修代码 |
| 7 | 负载看门狗失效两次 | 第一次与 queue.sh 同时启动，抢跑后找不到进程就退出；第二次取到的 PID 是外层 zsh，杀了它 queue.sh 成了孤儿 | 第三次直接按 `bash release/tools/queue.sh` 匹配 PID | 0.0.28 R1：看门狗做进 queue.sh |
| 8 | 队列期间 make gatedeps 超过 58 s | 机器被队列占满 | 登记推迟到队列结束后统一做 | 0.0.28 R4：parse2 分片自动发现，不再逐片改 gate.sh/gatedeps |
| 9 | 负载被别的会话推高 | minicon 会话开着 UTM 的 Windows 虚拟机 | 看门狗暂停；董秘去问政委 | 跨会话事项，不在本仓 |

## 下一版要带上的（已写进 archive/plans/v0.0.28.md）

R1 看门狗做进 queue.sh；R2 precheck 跑契约层；R3 workflow bound 参数静态检查；R4 parse2 分片自动发现；U1 把发行包抄到 partnernetsoftware/unisa。
