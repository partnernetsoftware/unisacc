# unisacc 0.0.51 -- 发布说明(草稿,未发布;占位以 {{...}} 标出,未实跑一律写「未跑」)

本版有两部分:**产品**——Y-0 候选(e3 阶段检查点)首次进入发布版;**构造体系**——候选链从快照到 pack 端到端跑通一次,并补齐预检、心跳、看门狗、一键打包等工具。产品证据是同候选六目标原生 CI(见下表)。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。`sh ./unisacc.com --version` 输出 `unisacc 0.0.51`。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | SHA256 `d5d6d0023898ea007fbb975f3118c8cf7e0fcd4c735fab50c78bdc0d56594b4a`,6529460 字节 | 同候选六目标原生 CI 的受测对象;0.0.50 为 76ed4200…,6501348 字节(+28112 字节,来自 Y-0 源增量) |
| 构造源 | `bc5b04bf14806b6783778bf40cb64dfb357bc200`,源 SID `a811885ea6f94dd59cae599a40a45e85dedec1755251245a3e56dbb70e5d5c0c`(source-packages-v2) | 候选表 commit 列=构造源;SID 由 identity-helper 对冻结快照独立复算,与链 ctx 记录一致 |
| 六片 | lnx-x86_64 `e9d18ef4…` 3706794B、lnx-arm64 `39737e45…` 3714938B、osx-x86_64 `3be1542c…` 2493538B、osx-arm64 `345aebd4…` 3170530B、win-x86_64 `e2016465…` 2757120B、win-arm64 `749617b4…` 3485184B | 完整 sha 见 `out/c051-y0-receipt.md` |
| 不动点 | 六片 g1==g2==g3 逐字节相同;kernels g2==g3;fp-compare `slices=same count=6 objects=85 kernels=2 product=same` | 两代相等只证不动点,语义正确靠 oracle 与 CI |
| P4 包 | `56efea53…`,1763636 字节 | 与 0.0.49/0.0.50 相同(模型包不随 Y-0 变) |
| cache-manager | 由 `cache-retain` 在本次构造(x64 `452d9240…`、arm64 `3f6e4cbd…`),与 0.0.49 的借用件同 sha | 三轮确定性构造 sha 相同(各约 5 秒);链内 pack 步骤默认仍传显式参数,待 noborrow 补丁落地 |
| 可复现性 | H8 `release-run --pack` 在私有 0.0.51 表下复现 APE,与链 pack 逐字节相同 | 版本槽之外的字节不含版本 |
| s9 | **未跑**(commit/daily/release 三档均未跑;init 在 55 秒内装不下,已调研并有 native 缓存补丁待落) | 不称全批通过 |
| 原生 CI | **QUALIFIED 42/0**(六目标 candidate 作业与 fetch/ffi-lnx/cell-tools/report 全部 success,测试提交 X'=`30d1c3cd67`) | 运行 `42809902396`;首个 X(`7a964ec8b5`)的运行 42809224934 因草稿尚未创建 fetch 失败,42809291021 因记录文件误带旧 SID 被取消,均不计 |

## 行为变化(升级前请读)

1. **e3 阶段检查点(Y-0)**:phase B 的中间产物带 `.sha256` 收据,不符即拒绝、不回退;修复了旧双相位 B 路径单元数停在 1 的缺陷;Windows 目标无转发桩时走整路由。对正常一次性编译的结果没有影响,同源 y-oracle 对照通过。
2. 其余产品行为与 0.0.50 相同。

## 本版修复与新增(构造体系,随仓库交付)

- **候选链端到端跑通一次**:`tools/candidate-chain.cx`(26 相位 + 检查点 + `--resume`,预检四挂点 `--strict`,`heartbeat.tsv` 心跳,Win 走 st_span 三相、Mac 走 A1/A2/B 分相)。
- **预检**:`tools/preflight.cx` 与 `build/preflight-rules.tsv`(14 条机检规则,每条对应一次真实踩过的隐性前提)。
- **看门狗**:`tools/chain-watch.cx`(RUNNING/STALLED/DONE,心跳存活核实 HUNG/DEAD/UNKNOWN,续跑封顶)。
- **一键打包**:`tools/release-run.cx --pack`(零网络,用 0.0.50 真实输入重放七资产与公开版逐一相同);`tools/version-inject.cx`。
- **集成落地**:`tools/integrate-patch.cx` 的 `--land`、入口校验(git diff 含 index 行)、冻结 sha 复核、涉及 tools 的编译验证与受影响批次实跑。
- **工具层**:`tools/cxlib/procrun.cx pr_run` 唯一子进程入口;`tool_die` 与 cx-lint R3-die;`graph-check` 路由一致性(以 `build/graph.tsv` 为真值);`register.cx` 缺 `out/` 或 `unisacc.com` 时具名拒绝;`phase-budget` 小数解析;tools-compile 登记表补齐(707 源 89 片)。
- **chain-step**:`fixedpoint --from-image`(从分相产物写 phase-N 收据);拒绝 0 字节镜像。

## 已知限制与未声称的性质

- **s9 三档均未跑**;daily 档夜跑未建立(调研结论:init 36 秒,all-waves 冷编 19.3 秒是大头,native 缓存把热 init 压到 16.8 秒,补丁待落)。
- 链的墙钟:端到端 snap->pack **未做一次不间断计量**(UNMEASURED);本版构造期间有多次因隐性前提停摆,每次的原因与处置记录在 `agentwork/ledger.tsv` 与 `out/c051-y0-receipt.md`。
- osx/win 的 g2/g3 镜像经分相路线产出,fp-compare 的 `slice-osx-*` 文件是 `sub/image` 的字节拷贝(`staged-from-image`,sha 相同),根治(`--from-image` 直接按 slice 命名落位)待补。
- `chain_sha` 身份哈希了全部编排工具,任一工具变更使既有收据失效(本版 kernels 因此重做一次);应只哈希影响产物的输入,留 0.0.52。
- `chain-step.cx:134` 的六目标常量顺序与图不一致(graph-check `order_mismatch=1`),留待 D2。
- p3 相位冷编加本体约 80 秒,超 60 秒上限;冷编分相位测量见 `out/opus-h7-coldcompile.md`,单元缓存方案列 0.0.52 M5。
- 两行 0.0.49 首次尝试超预算的 `phase-times` 记录以 waiver 登记(到期 11-30,到期前须补真实 B 相实测)。
- omp-sonnet 窗口被服务端拒绝停摆,不属本产品。

## 验证入口

1. 核哈希:`sha256sum unisacc.com` 应为 `d5d6d0023898ea007fbb975f3118c8cf7e0fcd4c735fab50c78bdc0d56594b4a`。
2. `sh ./unisacc.com --version` 输出 `unisacc 0.0.51`。
3. 冒烟:`printf 'int main(void){return 7;}\n' > t.c && sh ./unisacc.com t.c; echo $?` 应为 7。
4. 复现:`sh ./unisacc.com tools/release-run.cx -- --pack ROOT CANDIDATE_TSV SRC_DIR OUT_DIR`(ROOT 须为带该版本 `release/candidate.tsv` 的私有克隆),产物 APE 应与上表相同。
5. 原生 CI 证据:见上表运行号。
