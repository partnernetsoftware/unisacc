# cc → omp-cc 交接(2026-11-02)

主人裁定:编排工作由 omp-cc 接手。主人只给方向和发现,技术决定你拍板并报结论。只有发布第 7 步(签名/Draft/Latest/公开)、花钱、对外发送内容才请示。需要 sudo 就生成幂等、可回滚、先 dry-run 的 `.sh`,把路径给主人。

先读:`CLAUDE.md`、`plans/omp-orchestration-2026-11-02.md`(章程、路由、提交清单)、`plans/cx-tiers-2026-11-02.md`、`plans/layering-refactor-2026-11-02.md`、`plans/omp-board.md`。

## 路由和规矩

- 发信:`/home/devali/repos/moltbaby/bin/envelope <窗口> "标题" @正文文件`。别用裸 send-keys 发消息;正文放文件,否则反引号被 shell 吃掉。
- bdy↔go 可直接沟通;ds、go2 只对编排者。不写“请与某某对齐”。
- 所有提交由编排者做:先跑检查、读结果,再提交,提交后立刻重跑 `tools/ratchets.cx`,符号链接用 `git ls-tree` 核 120000。共享表用 `tools/commit-own.cx --keyed`。删除文件用 `git commit -- 路径`。
- 不碰别的车道:`apps/csih/` 归 csih,`ujs/` 归论文 B。不用 stash/reset/clean/checkout;不 `pkill -f`;单条命令 ≤60 秒。
- 代理的报告要复验。今天复验推翻过:汇编器“不接受”(跳过了预处理)、kernel/*.inc 是机器码路线(其实是数据)、W6“零引用”(漏了 git hook 和 .tsv)。我自己也错过几次,见下。
- 15 分钟督导 omp-bdy 的 cron 属于 cc 会话,已删除。要继续督导请你自己建。

## 各窗口在做什么

| 窗口 | 当前任务 | 下一步 |
|---|---|---|
| omp-bdy | ① 快车道候选:HEAD + 现有 compiler.pkg,release-build p3 → entry-build → release-ape,不碰网 | 私有根验 `plans/w5-install-verification-2026-11-02.md`,再在无 include/ 的空目录验包内头,两种结果分列;再撞腐烂就停并报卡点。安装由编排者定 |
| omp-bdy | ② 写 `plans/kernel-context-narrow-2026-11-02.md`(只计划) | 审过才动代码:kernel 绑定收窄到 exec/c/asm + 汇编器,修 asm-material 输入表 |
| omp-go | FFI C3:x86_64 桥已保存 callee-saved(.text 213B),运行期仍 rc=139,四条用例一条没跑出来;callee-saved 这个归因因此不足以解释。没有 gdb。它提议先用哑目标分开测桥的序言/尾声,等你答复;strace 能看 SIGSEGV 的 si_addr,也可以让 harness 装信号处理器打印崩溃地址和 RIP,不用装 gdb。arm64 未运行,§4 include 枚举未做 | 全部写进 `plans/ffi-bridge-image-2026-11-02.md` §1(工作区有未提交改动),回信后提交。字节表进 include/ 要先审 |
| omp-ds | `plans/include-cwd-shadow-2026-11-02.md`:run-batch + batches.tsv 的 headers 列 + testplan(工作区已有改动,未交付) | 再改 stages.cx 默认头目录置空;之后接 `plans/tools-pins-audit-2026-11-02.md` §8 的计数派生等 |
| omp-go2 | `tools/pins.tsv` + `tools/check-pins.cx`,登记 pins 批次 | 已知断裂如实登记,不放宽。toolsloc 上调已批,以新工具行数为限。还交了 w6-doc-only 扫描和 superharness todo 计划,未审 |

## 待决和已知问题

1. W5 未到候选:131 job 全绿、final VERIFIED,卡在 pack 的 kc_check。补丁 01-04 在 `plans/w5-patches/`,未应用到主树。产物在 `out/epoch-w5`。容量 128 和 pp-negative 两表都未生效。
2. 下次纪元合并跑:W5 补丁、cwd 头修复、FFI 字节表。
3. 现装 `unisacc.com` 是 dev-lnx-only 的 shell 头 + `unisacc.com.dev/`,NOT_RELEASE。
4. cwd 下 include/ 遮蔽系统头:仓内批次验的是树头,不是包内头。修复前,涉及头文件的绿不等于产品绿。
5. `--host-provider` 是死参数(`tools/check-host-provider.cx` 5/6 红,未登记批次)。解析类三项改判慢车道。
6. csih 侧改用 ext.* 的信封还没发(别名链路可用,不紧急)。W3 阶段二暂缓。
7. W4b netname/streamname 两对合并:探针未做,优先级低。W6 source-job-wave 迁移:等 W5 后。
8. omp-bdy 还跑着旧状态条(CSR 显示 100%),纪元后重启:`omp --model <原模型> -r <会话前缀>`。别用 `-c`,四个窗口共用 cwd,可能接错会话。启动脚本带默认模型,omp-go2 重启后因此变成 Kimi。
9. 仓根有一个空文件 `bun`(0 字节,17:27 产生,已被暂存,不是我建的),别提交。查明谁建的,再让它清掉。
10. moltbaby 仓的 balances.ts、probe.ts、SKILL.md、fixtures 等主人提交。

## 我犯过、你别重犯的错

- 把“W5 能复用 kernel”“pin 是今天才断的”“产品无汇编步”说早了,都是没实测就下结论,后来更正。先算再说。
- 信封写了“已提交”而提交其实失败。提交成功才说。
- 把 putc/R15/MAXSTEPS 并入慢车道,被一起卡住,已撤回。用户可见的修复不要绑慢车道。
