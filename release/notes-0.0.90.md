# unisacc 0.0.90 发布说明

整体裁决 NOT_QUALIFIED

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 | 5d1e492d7e41f142947e977c5cbec2f41e3d2ab1 |
| 候选提交 | 91094736413630d9f0e70e1a60ed1bdd5863bc22 |
| 阶段 A 提交(X) | 6d7f11d22c0ea46568a52359d96b005753a79648 (首轮 X1=4a742ba2c2d0569bc2401e47b4a65c5958e82615) |
| 阶段 B 提交(Y1 / Y2) | Y1=c438b6d6283976329a4ed16f4634a01555152c11 / Y2=13a48bb87004fc5c294d8a5aeaf09c43e928e28b |
| 发布 APE SHA256(unisacc.com) | 46fd3db95f97df7fa4729026bd6e1aa7b7ddde834e5887aa87470020da0391e1 |
| 源快照 SID | a2971c5a4255d00495c6d2e9a79416faab94a5fcc3a88e7b4e925480eff9d1d8 |
| pkg.bin | b717fff8f266d4c3c924186657be94cdbbc1c470bdcbe1a6af7ef97bb1cb8eaa |
| slice-win-arm64 | de3e45f516ad4514c59d907e8a46d0efc3b5f20d1909d31555bdbacd74115bec |
| SHA256SUMS | 4019e3e06e67b900981e7121c5437ab6d540cfd674a60fc2c5007705e52e2471 |

## 严格证据

459 批:PASS 457、FAIL 0、UNKNOWN 2、INVALID 0

- 严格预跑分母口径(`PRERUN_PREPARE`): `dvqualify_list=817` 批, 排除计划中未实现 `excluded_planned=12`、异构平台专用 `excluded_foreign_platform=15`、陈旧退役 `excluded_stale=358`(合共排除 `385` 批, 得 `432` 个 `product-qualification` 批, 加上 `extra_non_pq=27` 含 15 片 `R77` `daily-compile-smoke-01..15` 与金丝雀共 `459` 批, 规划为 `158` 个窗口); 实跑 `457 PASS / 0 FAIL / 2 UNKNOWN / 0 INVALID`(2 个已知无生产者批 `cx-booter-ape`、`tools-default-regression`)。
- 六目标原生 CI: 正式发布 CI `run_id=44973535033`(及 `ci-early` `run_id=44964711763`), `lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`、`win/x86_64`、`win/arm64` 六格一次性 `6/6 PASS`(`verdict=QUALIFIED fail=0 invalid=0`)。

## 产品改动

- U4(Linux ELF 函数声明汇编标签 `__asm__("label")` / `__asm("label")`): 在前端词法(`E1` 新增第 69 号记号类, plain/typed/locations 三路同码, 旧 68 项码位不变)与解析/生成表(`E3` `token-prefixes`、`parse-constants`、`tokenread-byte`、`control-result`、`callcontrol-result`、`update-result`、`errors-template`)实现 `lnx/x86_64` 与 `lnx/arm64` 函数声明与定义的汇编标签改名(覆盖定义直带标签、先声明带标签后定义不带标签 `a01`、同标签重复声明、外部 libc 引用重定向 `a02`、`static` 内部链接函数改名 `a04`、函数取址与函数指针调用 `address.c` 及尾遍字符串池 `SKIPS[tpos]=1` 跳过标签字面量); 验收 `A1A_U4 pass=27 fail=0`、`A1A_U4_SELFTEST mutations=3 killed=3`、`CHECK_ASM_ATTR_REJECT ok=31 failed=0`、`c10`(`tests/diagfx/u_asm.cx:3:5`)升为 `PASS`(诊断合同缺口 `15 -> 14`)、GCC 11.4 独立 10 程序对照 `10/10 PASS`、26 个非标签示例 `-o` 逐字节对拍 `26/26 SAME`。
- R72(`cx/compiler` 临时编译缓存按年龄与目录租约清理): 新增 `cx/compiler/cxscratch.cx` 并接入 `cx/cx0/plan.cx` 与 `src/host.cx`; 宿主与 CX0 在临时缓存目录入口持有共享租约, 仅在启动时非阻塞取得独占租约才执行扫描, 回收超过 86400 秒(1 天)的本用户普通缓存文件(`ua-cxif-*.h`、`ua-bodysrc-*` 等)与遗留暂存文件, 缓存命中按需续龄并跳过符号链接(先红 `6/2` -> 后绿 `8/0`, 阈值 `0` 变异被杀, 4 输入 `-c`/`-o` 字节等价 `8/8 SAME`, 6 目标交叉编译 `12/12`, `srcshape` `lines=521<=525 ifs=89<=91`)。
- `rt-shared-gen` 单元级大块分配状态补登记(R69): 在 `exec/facts/rt-shared.tsv` 的 `exempt_symbols` 补登记 `_unisa_lbin`、`_unisa_lheld`、`_unisa_lev`, `rt-shared-gen.cx -- --check` 输出 `RT_SHARED_GEN SAME groups=2 bytes=3191 unlisted=0`(`unlisted` 由 `9` 清零为 `0`, 3 变异全杀)。
- 快车道质量门与产线加固(R71 / R74 / R77 / R78 / R80 / R69.5):
  - R71 产生后立即备份门: `tools/release-chain.cx` 新增 `backup-evidence` 子命令与 step 5 `execute` 前置校验, 强制将整链 5 件与预跑 2 件关键产物备份至 `~/.unisacc-release-evidence-backups/<ver>`(拒绝含 `/.cache/` 或仓内路径)并核验 SHA256 清单(`CHECK_RELEASE_CHAIN pass=53 fail=0`);
  - R74 预跑窗口规划扩容: `tools/prerun-drive.cx` 将 `PD_MAX_WINS` 由 `512` 扩至 `2048` 并新增 `--max-wins` 与 `--unmeasured-ms` 保守默认耗时(`CHECK_PRERUN_DRIVE ok=39 failed=0`);
  - R77 `tier=daily` 117 入口双隔离编译冒烟: `tier=daily` 批 818 个中 490 个自 `v0.0.86` 起未被冷编, 全量冷编扫 117 个唯一 `tests/*.cx` 入口在 `0.0.89` 下 115 通过、2 失败(`tests/stream-lower-domain.cx` 缺 `rt_tail` 前向声明、`tests/cx-host-run.cx` 缺 `runsh` 前向声明, 均已修复); 新增 `tests/daily-compile-smoke.cx` 与 `daily-compile-smoke-01..15` 将 117 个入口按 LPT 拆为 15 片双隔离(`TMPDIR` + `UNISACC_CACHE_DIR`) `-c -funit` 编译冒烟(`117/117 PASS`, 已并入严格预跑首波 `W0001`:`lane-key-probes,daily-compile-smoke-01..03`、`W0002`:`daily-compile-smoke-04..07`、`W0003`:`daily-compile-smoke-08..11`、`W0004`:`daily-compile-smoke-12..15` 并全部 `15/15 PASS`);
  - R78 变异门证据分类: `tools/mutation-gate.cx` 新增 `--report` 分类(`KILL-named` / `KILL-weak` / `INVALID` / `UNKNOWN`)与同字节无改动变异拦截(`CHECK_MUTATION_GATE ok=29 failed=0`, `CHECK_MUTATION_GATE_QUALITY ok=14 failed=0`);
  - R80 起链前置版本槽硬门: `tools/bg-candidate.cx` `start` 前置强制核验 `chain-ready` `11-version-slot`, 拒绝版本槽与分支不符时起链(`CHECK_BG_CANDIDATE failed=0`);
  - R69.5 字面量长度修正: 修复 28 个 `tools/*.cx` 中的 `strncmp`/`memcmp`/`tx_prefix` 字面量长度不符与越界读。

## 构造过程说明

- 首链 `chain1`(`T1=a4caf61410c7a0fde632ab94e98caf09ac3a3e9f`)虽在 29m47s 零中断完成 `499/499 DONE` 且 `ci-early`(`44960154387`)六格全绿、首轮预跑 `W0000..W0002`(含 15 片编译冒烟 `15/15 PASS`)通过, 但因起链前未将 `release/candidate.tsv` 版本槽切至 `0.0.90`(`11-version-slot FAIL`), 致第 494 相 `pack`(`tools/ape-pack.cx:407`)将 `0.0.89` 注入五目标切片, 产出的候选 APE(`c7a0df11...`)自报 `unisacc 0.0.89`(冻结切表后 `release-run --pack` 会触发 `PACK_APE_CHANGED` 且 `release-prep` 判 `STRICT_CANDIDATE differs / INVALID`)。
- 核实源码后立即终止首轮在途预跑(`-1619985`), 将 `chain1`、`ci-early`(`44960154387`)与首轮预跑整体标注作废(备份保留于 `~/.unisacc-release-evidence-backups/0.0.90/chain1-obsolete/`); 在 `v0.0.90` 提交 `T2=91094736413630d9f0e70e1a60ed1bdd5863bc22` 将 `release/candidate.tsv` 切至 `0.0.90`, 重跑 `chain-ready.cx -- --version 0.0.90` 取得 `20/20 READY=yes`(`11-version-slot PASS expect_self_report=unisacc 0.0.90`)后重起 `chain2`, 29m57s 零中断一次完成 `499/499 DONE` + `PROMOTE_CHECK OK`(同 `SID=a2971c5a4255d00495c6d2e9a79416faab94a5fcc3a88e7b4e925480eff9d1d8`, 新 APE `46fd3db9...` 自报 `unisacc 0.0.90`); 随后 `ci-early`(`run_id=44964711763`)一次性 `6/6 PASS`, 459 批严格预跑(158 窗)一次性完成 `457 PASS / 0 FAIL / 2 UNKNOWN`; 同步在 `main` 落地 `R80`(`188e45e558`)堵住未切版本槽起链漏洞。

## 已知缺口

- 全套 `s9` `daily`/`release` 长跑尚未运行, 整体裁决保持 `NOT_QUALIFIED`;
- `tools/tsv-lint.cx` 未解析引用已知红(`R75`): 全树按条未解析引用数 `11277 > 内置上限 11050`(按唯一 `file:line:state` 为 `10055`), 自 `v0.0.83` 起存在且不在现行棘轮与预跑门内; 已验证 `exec/modelsignature` 清单补静态绑定(`NAME=@str:NAME`)后可降至 `10704 < 11050`, 因 `exec/` 属 A 类输入且 `0.0.90` 冻结范围已封顶, 排 `0.0.91` 修表并并入 `chain-ready`/棘轮只降不升, `0.0.90` 带已知红发布;
- 变异门真实杀伤率 `INCONCLUSIVE`(`R78`): 上一版 `0.0.89` 报告的 `149/212 = 70.28%` 将同字节无改动变异与 `124` 超时/`127`/编译失败均计入 `killed` 且缺逐变异原件, 属上限估计; 本版 `mutation-gate.cx --report` 在 33 项固定样本上实测 `17` 具名杀(`KILL-named`)、`3` 弱杀(`KILL-weak`)、`13` 存活(含 `1` 已裁等价), 真实杀伤率区间 `17/32..20/32 = 53.13%..62.50%`, 跨 `60%` 门线判 `INCONCLUSIVE`, 在 `0.0.90` 关键补丁集上按补丁重跑 `--report` 建立有证据分母的基线排在 `0.0.90` 发布后完成;
- `asm volatile(...)` / `__asm__ __volatile__(...)` 行内汇编拒绝位置缺口: 在 `E1`(`exec/lex/attr-byte.tsv:1346`)直接 `HALT REJECT` 输出 `reject: not covered: inline asm`(拒绝本身正确、无产物, 缺 `file:line:col` 行列号; 为守 `0.0.90` A 类预算不扩文件, 排 `0.0.91` 补齐);
- `tier=daily` 批冷编与运行态范围(`R77`): 818 个 `daily` 批中 490 个自 `v0.0.86` 起未冷编, 117 个唯一 `tests/*.cx` 入口全量冷编扫出的 2 个缺前向声明失败已修复, R77 新增的 15 片 `daily-compile-smoke-01..15`(117/117 入口)已并入本版严格预跑首波(`W0001..W0004`)并全部通过; `daily` 批完整运行态执行仍属 `s9` 长跑范围;
- 2 个无生产者批(`cx-booter-ape`、`tools-default-regression`)在严格预跑中保持 `UNKNOWN(no-producer)`, 独立参考/原生 watchdog 仍有宿主 C 引导依赖;
- 本机严格预跑只在 `lnx/x86_64` 执行; `osx/x86_64`、`osx/arm64`、`win/x86_64`、`win/arm64` 与 `lnx/arm64` 原生由六格原生 CI 覆盖, 未跑全量 459 批。

## U4 与 GCC 11.4 的语言行为差异

- 裸 `asm` 拼写不支持: 严格 C 下 `asm` 非关键字(GCC 默认 gnu 模式接受裸 `asm`, 但吃掉用户标识符 `int asm = 1;` 属回归); `cx` 声明标签仅支持 `__asm__` 与 `__asm`, 声明或顶层位置写裸 `asm("lbl")` 在词法侧具名拒绝(`reject: not covered: inline asm`), 不带 `(` 的普通变量名 `asm` 正常可用;
- 对象/变量声明贴标签拒绝: 对文件作用域全局变量(`a03`)与 `static` 变量贴 `__asm__` 标签, 按本版范围裁定具名拒绝(`error: not covered: asm label on an object declaration`);
- 4 处比 GCC 11.4 更严的具名拒绝(GCC 11.4 严格 C 模式下 `rc=0` 接受或仅告警, U4 均具名拒绝 `rc=1` 且无产物):
  - (1) `a05`(同一函数两次声明给出冲突汇编标签, 如 `__asm__("a1")` 后再声明 `__asm__("a2")`): U4 拒绝(`error: not covered: conflicting asm labels on function declarations`), GCC 11.4 `rc=0` 保留第一个标签(`T a1`)并告警;
  - (2) `a06`(函数定义之后才补汇编标签, 如 `int f(void){...}` 后再声明 `int f(void) __asm__("late");`): U4 拒绝(`error: not covered: asm label after function definition`), GCC 11.4 `rc=0` 忽略改名(`T f`)并告警;
  - (3) `a07`(块作用域局部非 `static` 变量带汇编标签, 如 `int x __asm__("rax_reg") = 1;`): U4 拒绝(`error: not covered: asm label on a block-scope object declaration`), GCC 11.4 `rc=0` 忽略 `asm` 说明符并告警;
  - (4) 块作用域 `extern` 函数声明加汇编标签(如块内 `extern int g(void) __asm__("g2");`): U4 拒绝(`error: not covered: asm label on a block-scope function declaration`), GCC 11.4 `rc=0` 并生成改名后的外部符号引用(`U g2`);
- 与 GCC 11.4 一致同拒及 `p08` 语法边界子例:
  - `a08`(非字符串字面量标签 `__asm__(sym)`): U4(`error: not covered: asm label requires one ordinary string literal in a declaration`)与 GCC 11.4 一致同拒(`rc=1`);
  - `p08` 独立验证语法子例: 空串 `__asm__("")` 与相邻字符串拼接 `__asm__("a" "b")` 具名拒绝(`error: not covered: asm label requires a plain ELF symbol name`), 括号包裹 `__asm__(("x"))` 具名拒绝(`error: not covered: asm label requires one ordinary string literal in a declaration`), 均 `rc=1` 且无产物;
- 非 ELF 四目标具名拒绝: `osx/x86_64`、`osx/arm64`、`win/x86_64`、`win/arm64` 上对函数贴汇编标签执行 `-c` 或 `-o` 均具名拒绝(`error: not covered: asm label unsupported for target`), 无残留产物;
- `-c` 单单元对象符号类别: `unisacc -c`(非 `-funit`)单单元对象将非 `main`/`_start` 函数发为本地文本符号 `t`(既有单单元封装行为, 与 GCC `-c` 的全局 `T` 不同, 非 U4 引入)。

## 回滚办法

- 候选级(未公开前): 保持草稿 release(`v0.0.90`)私有不公开; 若需换候选重跑 CI, 执行 `sh ./unisacc.com tools/release-chain.cx -- retry-ci --version 0.0.90 --run-id <RUN_ID> --evidence-dir release/evidence/0.0.90 --candidate release/candidate.tsv` 归档失败 CI 证据并删除陈旧草稿; 本机若已按候选模式试装 Root, 执行 `sh ./unisacc.com tools/install-ape.cx -- . --rollback --write` 恢复 `.unisacc-root-backups` 中的 `0.0.89` 备份并核 `--version` 为 `unisacc 0.0.89`。
- 公开后(适用窗口: `0.0.90` 公开后至下一版本公开并完成读回, 回退目标 `0.0.89` 公开 APE `384e115227525d21c2392bc03aa251586592d411c685d9db041867b63d7ff3a7`, 详见 `release/evidence/0.0.90/rollback.txt`):
  - (1) 前置与 release 撤回: 保留 `0.0.89` 公开资产与 `release/evidence/0.0.90/` 全部回执; 先将 `v0.0.90` 转回草稿并把 `Latest` 移回 `v0.0.89`, 随后读回 `isDraft`、`tagName`、资产及 `Latest`:
    `gh -R partnernetsoftware/unisacc release edit v0.0.90 --draft=true`
    `gh -R partnernetsoftware/unisacc release edit v0.0.89 --latest`
    `gh -R partnernetsoftware/unisacc release view v0.0.90 --json tagName,isDraft,assets`
    `gh -R partnernetsoftware/unisacc release view v0.0.89 --json tagName,isDraft,assets`
    `gh api repos/partnernetsoftware/unisacc/releases/latest --jq .tag_name`
    若确需删除 release 与 tag(前置: `Latest` 已切回 `v0.0.89` 且确认无外部引用): 执行 `gh -R partnernetsoftware/unisacc release delete v0.0.90 --yes` 与 `git push origin :refs/tags/v0.0.90 && git tag -d v0.0.90`。
  - (2) 共享 `main` 发布记录前向撤销: 从新到旧对 Y2、Y1、X 提交执行 `git revert --no-edit <SHA>` 并 `git push origin HEAD:main`, 不回退冻结后他人代码, 禁止 `stash`/`reset`/`clean`/`checkout`。
  - (3) 本机装机回退: 有备份槽时执行 `sh ./unisacc.com tools/install-ape.cx -- . --rollback --write`; 无备份槽时下载 `0.0.89` 公开 `unisacc.com` 核对 `sha256=384e115227525d21c2392bc03aa251586592d411c685d9db041867b63d7ff3a7` 后执行 `sh ./unisacc.com tools/install-ape.cx -- . <APE_PATH> 384e115227525d21c2392bc03aa251586592d411c685d9db041867b63d7ff3a7 --write`, 随后核验 `sh ./unisacc.com --version` 输出 `unisacc 0.0.89`。
