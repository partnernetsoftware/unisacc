# 0.0.94 发布流程格式坑清单(release-gotchas)

作者:csr 日期:2026-11-23  
范围:0.0.94 实跑暴露的格式/边界坑;证据优先 `release/evidence/0.0.94/` 与源码行。  
状态:私有清单(非发布说明);会话内无命令原文的项以源码合同 + 仓内证据文件互证。  
图例:`自校验`=应升工具/门前自拒;`手册`=操作约定,暂难全自动。

---

## (1) inputs.tsv `strict-sha256` = 两段原文件逐字拼接哈希

坑:写成「去表头后拼接」或只对折叠后的严格表哈希,`release-flow` 报 `STRICT_ORDER_IDENTITY`。

合同:`tools/release-flow.cx` 读每个 `segment` 行的绝对路径全文(须以 `\n` 结尾),按输入顺序 `np_put` 进 `strict_bytes`,再对整段缓冲算 SHA256,与 `strict-sha256` 键逐字相等(`read_inputs` 约 L140-158;头注 L3-4)。

证据入口:
- 源码:`tools/release-flow.cx:140-158`(SEGMENT_IDENTITY / STRICT_ORDER_IDENTITY)
- 0.0.94 INDEX 两段预跑说明:`release/evidence/0.0.94/INDEX.tsv` 行 `prefreeze-strict` / `postfreeze-strict`(段一 F4、段二 F5 折叠叙述);严格表路径 `release/v0.0.94-strict-2026-11-23.tsv`(INDEX 相对引用)

自校验:是。`release-flow` 已拒错值;建议在 `release-chain plan` 或 inputs 预检再算一遍拼接哈希并打印期望 digest,避免进入 prep 才红。

---

## (2) decisions 首行 `identity` 须与候选 APE + 拼接严格哈希同值

坑:`identity` 行缺、顺序错、或第三列不是拼接严格哈希 -> `DECISIONS_IDENTITY`。

合同:`tools/release-flow.cx` `exceptions`(约 L219-228):首条有效决策须为三列 `identity <cand.sha[0]> <digest>`,其中 `digest` 与上条拼接 `strict_bytes` 相同;未绑定前任何其它行直接拒。

证据入口:
- 源码:`tools/release-flow.cx:11-12`(头注),`222-228`
- 0.0.94 阶段产物侧决策消费链见 INDEX `exec-4-Y1` / `exec-7-Y2` 与 `release/decision.txt`(Y1 冻结哈希对在 INDEX `exec-4-Y1-frozen`)

自校验:是。已有运行期拒;建议 check 夹具固定「去表头哈希 / 颠倒 identity」先红后绿。

---

## (3) notes-form:8 个占位符 + 6 个精确机器列表格标签

坑:缺 `{version|freeze|ape|sid|pkg|win|sums|summary}` 任一 -> `NOTES_MISSING_TOKEN`;表格标签字面量漂移 -> `NOTES_IDENTITY` / `NOTES_FREEZE`。

合同:`tools/release-flow.cx` `notes`(约 L316-344):
- 8 token:`version,freeze,ape,sid,pkg,win,sums,summary`(L317-325)
- 6 机器列(内置表):`冻结提交`、`发布 APE SHA256(unisacc.com)`、`源快照 SID`、`pkg.bin`、`slice-win-arm64`、`SHA256SUMS`(builtin L320;校验 L336-340,其中后 5 个经 `machine()` 精确前缀 `| 标签 | `)

证据入口:
- 源码:`tools/release-flow.cx:310-344`
- 成品:`release/notes-0.0.94.md`(公开说明,对照机器列)

自校验:是。已有 `NOTES_*` 拒;建议 `release-prep`/`check-release-flow` 增加对 notes-form 的静态 token+标签扫描,不依赖整段 stage。

---

## (4) stage-a `--out` 不可已存在(`NEW_OUT`)

坑:复用旧 stage 目录或未清掉上次失败半成品 -> `NEW_OUT`。

合同:`tools/release-flow.cx` 约 L489-494:非 checking 时 `sy_lstat(out)` 必须为 `-ENOENT`,否则 `fail("NEW_OUT",out)`;先写到 mkdtemp 再 rename 发布。

证据入口:
- 源码:`tools/release-flow.cx:489-494`,`539`(PUBLISH_DIRECTORY)
- 链步:`tools/release-chain.cx:17-18`(`--out STATE/stage`)

自校验:是。已拒;建议 chain 在调 stage-a 前若 `STATE/stage` 存在且无对应成功标记则打印显式清理命令。

---

## (5) `release-chain --from` 只跳过「序号更小且已有 `NAME.done`」的步;手工 prep 须补 `prep.done`

坑:以为 `--from 2` 等于「绝不重跑 prep」;手工跑过 prep 但未写 `STATE/prep.done` 时,`--from 2` 仍会再跑 prep(因 `n < from` 且无 done 标记不 SKIP)。

合同:
- 头注:`tools/release-chain.cx:38-40`
- `step`:`L115-119`(`n < from && sy_exists(m)==1` 才 SKIP)
- 预步 pack/warm:`L700+` 看 `pack.done` / `warm.done`,与 `--from` 交织

证据入口:
- 源码:上列
- 0.0.94 状态目录惯例:`~/.cache/unisacc/release-v094-state/` 可见 `pack.done` 等(本机缓存;INDEX `pack` 行对应 `pack-run.out`)

自校验:部分。建议 `release-chain plan` 列出将 SKIP/将重跑的步及缺失的 `*.done`;手工 prep 后提供 `touch`/`printf` 收据写入约定或子命令 `mark-done prep`。

---

## (6) INDEX.tsv 暂存与未暂存混合 -> `WHOLE_AMBIGUOUS`

坑:对 `release/evidence/V/INDEX.tsv` 用 `commit-own --whole` 时,指数已有一段、工作区又有续写 -> 拒混合。

合同:`tools/commit-own.cx` 约 L1533:`REFUSE WHOLE_AMBIGUOUS: ... Use --staged ... or --keyed`

证据入口:
- 源码:`tools/commit-own.cx:1533`
- 0.0.94 INDEX 多轮追加形态:`release/evidence/0.0.94/INDEX.tsv`(同名步骤多行,如多次 `exec-2-draft-readback` / `exec-3-ci`)

自校验:是。已拒;建议发布窗口脚本在 `--whole` 前对证据路径 `git add` 整文件,或统一改 `--staged`。

---

## (7) `devsvc x --bg` 节点上限 5 分钟(`--timeout-ms` 最大 300000);CI 轮询要续跑

坑:把 release-exec 整段 CI 等待塞进一个 bg 节点,超 `FL_NODE_MAX_MS` 被杀;或 `devsvc wait` 单次最多 55s 误当作业死了。

合同:
- 节点上限:`tools/cxlib/flowexec.cx:83`(`FL_NODE_MAX_MS = 300000`)
- `devsvc x --timeout-ms` 校验:`tools/devsvc-core.cx:1839`
- wait 上限 55s 且超时不杀作业:`tools/devsvc-core.cx:38`,`4262`;头注 L26-27 指向 release-exec 用 `--from-step` 续

证据入口:
- 源码:上列;`tools/release-chain.cx:26-27`,`1537`(execute 须 bg,内部 `--from-step`)
- 0.0.94 CI 长等待:`INDEX.tsv` `exec-3-ci-dispatch` 10:49 -> `exec-3-ci` PASS 11:06(约 17 分钟,远超单节点 5 分钟);首次 run `45102369140` 失败后 `45103237968` 续(INDEX `exec-3b-ci-dispatch-2`)

自校验:部分。建议 chain/exec 打印「勿把 CI 等待塞进单 bg 节点」;wait 超时输出明确 `WAIT_TIMEOUT 不杀,再 wait`。

---

## (8) `release-run --pack` 负载下首次可超 chain 内 55s 界;可 nohup 直跑

坑:chain 步 `0a pack` 经 `np_run(..., 55, ...)`(L708);负载下墙钟 >55s -> `CHAIN FAIL 0a pack ... timeout 55 s`。INDEX 成功 pack 在链外/直跑完成。

合同:`tools/release-chain.cx:11`(各步 55s),`708-713`;头注 L35 已警告 pack+warm 同塞 prep 的风险。

证据入口:
- 源码:上列
- 成功收据:`release/evidence/0.0.94/pack-run.out` 末行 `RELEASE_RUN RESULT PACK_OK`;INDEX `pack` 行(10:38)
- 会话口径(cc):负载下首次约 55s 越界 -> nohup/`devsvc x --bg --timeout-ms 300000` 直跑 `release-run --pack`,再写 `pack.done` / 填 inputs `pack=`

自校验:部分。建议 pack 默认不走 55s 子步、或自动降级为 bg 作业并写 `pack.done`。

---

## (9) `backup-evidence.tsv` 绝对 home 路径须 scrub(`no-home-path`)

坑:备份表写入 `/home/...` 后棘轮红,挡后续 main 提交。

合同:`tools/release-flow.cx:192-203`(`scrub_home` 经 `tools/scrub-home-paths.cx`);仓级棘轮 `tools/no-home-path.cx` + `build/no-home-path-baseline.tsv`。

证据入口:
- 修后文件:`release/evidence/0.0.94/backup-evidence.tsv`(前缀已是 `~/...`)
- 提交:`a845ab2c0a`(说明:backup_path home 前缀改 `~/`,棘轮修后 18/0,`backup --verify` 仍 PASS)
- 源码 scrub 挂钩:`tools/release-flow.cx:192-214`

自校验:是。flow 写证据已 scrub;建议 `backup-evidence` 生成器出表后强制跑 `scrub-home-paths --check` 再入库。

---

## (10) Windows 保留文件名 `nul` 导致 checkout 失败

坑:语料 `tests/pp-equiv-corners/nul.c`(及 `sub/nul.h`)在 Windows 上为保留设备名,`actions/checkout` 报 `invalid path`,整 job 红。

证据入口:
- CI 日志:`release/evidence/0.0.94/ci-45102369140.log` 约 L1251:`error: invalid path 'tests/pp-equiv-corners/nul.c'`(windows-11-arm);同文 L2628(windows-2025)
- INDEX:`exec-3-ci-prior FAIL superseded-by=45103237968`;成功轮 `exec-3-ci PASS 45103237968`
- 改名提交:`f9f592b5ab`(`nul.c`/`nul.h` -> `nulname.c`/`nulname.h`,说明即 0.0.94 CI windows checkout 红)

自校验:是。建议仓门/棘轮拒绝 Windows 保留基名(`nul`,`con`,`aux`,`prn`,`com1`..,`lpt1`..)的路径入库,或 CI Windows job 用 sparse-checkout 排除语料。

---

## 应优先升自校验的排序(建议)

1. Windows 保留名路径门(挡 CI 整轮重跑成本最高)
2. inputs `strict-sha256` / decisions `identity` 预检打印
3. notes-form token+6 列表签静态扫
4. backup-evidence / 证据出表后 `no-home-path --check`
5. INDEX/`--whole` 前提示 staged 整文件
6. pack 超时自动 bg + `pack.done`
7. chain `--from` 缺失 `*.done` 时显式列出将重跑步

