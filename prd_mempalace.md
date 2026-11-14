# 记忆宫殿(唯一协同入口)

项目所有者 2026-11-14 裁定:agent 协同只用本文件。不再用 `agentwork/` 的任务表、账本、看板记过程;`agentwork/` 退役,现存文件只作工具读取与历史,不再追加。本文件由 prd.md §9 索引;每轮先读本文件,事实、阻塞、下一动作原位更新,旧进度删掉或改写,不追加流水。

## 写法规则

- 只写当前事实:谁在做什么、卡在哪、下一步命令。过程、重跑记录、单个批次 PASS 不写。
- 三类才写进来:别的 agent 必须遵守的决定、不可逆的对外动作、阻塞发布的未验证项。
- 每条事实带证据入口(命令或路径),过期就改,不留"曾经"。
- 任务表行 `| 部分落地 | ID、ID |` 与 `| 已落地 | ID、ID |` 并列:该行的 ID 被 tools/observe-agents.cx 记为 PARTIAL(已落地行记 DONE),其余 ID 仍算在途。
- 多人同改本文件时只提交自己的行:先 git diff 确认,必要时 tools/commit-own.cx --keyed;不得用 git add -p 以外的拆分手段,不得 stash/read-tree。
- 速度数字只来自 tools/run-startup-bench.cx、coldcompile.cx、check-bench-gencode.cx、speed-probe.cx。

## 当前发布与版本

- 已公开:0.0.61(2026-11-14,Latest;tag v0.0.61 与 v0.0.61-src 均指冻结提交 d4b3bf72f7ae;APE sha256 c4c47ecb97ee70865edaf39d91e29f5b22b6c8efdbcfb81c4482517ffc0224c3,7022820 B)。裁决 NOT_QUALIFIED unknown=1(s9 未跑),CI 六目标原生作业全 success(run 43554326355),发布说明 plans/release-notes-0.0.61.md,撤回办法 release/rollback.txt。
- 已装根:本机根已是公开 0.0.61 发布件(sha256 同上)。real5 整链(同 REV b418cb5d86)打包产物 sha256 与之逐字节相同,两次独立整链可复现。
- 实测(0.0.61 发布件 vs 0.0.59 基线):first_e2e 675/676/695 ms 对 796/815;冷编译 -24% 到 -32%;数字与样本数见 build/speed-baseline.tsv 与发布说明。整链 4829 s。
- 公开后大扫除已做:私有构造根只留 log/cxroot.tsv,chains/snap、local-0060、37 个无可执行位的 tlib kit 已清;借用的 osx cache-manager 副本在 ~/.cache/unisacc/borrowed-cm/。

## 起整链的做法(已验证的配方)

1. 私有根的工作树必须检出被构造的 REV(否则驱动工具与 REV 内工具拼法不一致,wf-prep-g2 收据 root 失配)。
2. BOOTSTRAP_FROM 要求祖先 2a46e1a4,而它不是 main 的祖先:用 `git commit-tree <main 树> -p fbdc99ca -p <main HEAD>` 造私有提交,先 `git fetch` main 到私有根。
3. `sh ./unisacc.com tools/candidate-chain.cx -- --rev REV --out 全新目录 --parallel 12`(输出目录必须全新,不能续跑;超时给 5400 s)。
4. 打包:同命令加 `--resume --cache-manager-osx X64 ARM64`(借用 0.0.59 的 osx cache-manager,副本在 ~/.cache/unisacc/borrowed-cm/)。5. 私有根配套文件按 rev 生成,不留旧版:根目录 unisacc.com 用 bootstrap 0.0.59(副本 ~/.cache/unisacc/bootstrap-0.0.59/unisacc.com,sha e43ef368)并配对应的 unisacc.com.build.json(commit 2a46e1a4、product_digest e43ef368);会话根的 cxroot.tsv 必须指向本 rev 的快照(chains/snap/REV10-H8);起链设 UNISACC_IDENT_HELPER 指向链内预构建的 identity-helper,否则首次源身份测量要现场编译 seed/ident.c,冷缓存下 5.8 s、并行下超 8 s 硬限会失败(判定依赖机器安静,是缺陷)。

real6 已起链(2026-11-14 16:25,账本行):REV=b2c557b1ba(origin/main;之后 HEAD 只多文档提交,`git diff b2c557b1ba HEAD -- src cx ext exec tools tests build` 为空),私有提交 70c31f0e9f(父 fbdc99ca 与 REV,树同 REV),PID 3271413 / PGID 3271413(timeout 5400 candidate-chain),输出 /tmp/unisacc-session-build-lr0060/chain-real6,日志 log/real6.out、real6.start、real6.rc;截止 start+5400 s(约 17:55);identity-helper 复用 real5 副本 ~/.cache/unisacc/ident-helper/identity-helper-real5(sha 前缀 db38b78c,seed/ident.c 未变);起链前置:chain-ready --fast 全 PASS(除 P1 SKIPPED 与两条人工项),P1 全量扫描 1190 文件新拒绝 0(8773eb12da);候选类别=库包与运行库(含 CX-U-2、T5、cache-manager 相,按 F11/F12 已知风险);未带 --cache-manager-osx,依赖新 cache-manager 相,失败走 --resume 加借用副本。止损:同一失败两次即停,第二次失败落点在 cxmod 则造不含 CX-U-2/T5 提交的候选。起链期间不跑重测试与速度取数。

## 0.0.62 发布步骤(预备,未执行)

依据 0.0.61 实际做法(F=d4b3bf72f7、X=6b07b710eb、Y=15bea6ef4e、装机 25b905be9d)与各工具开头 usage。前置:real6 整链 rc=0 且 `chain-real6` 内有 p3.pkg 与六片;下列 R=partnernetsoftware/unisacc,CHAIN=/tmp/unisacc-session-build-lr0060/chain-real6,W=$CLAUDE_JOB_DIR/tmp/rel62(全新目录),REV=b2c557b1ba。每步先 `pgrep -af 'candidate-chain|release-run'` 确认无同输入在途;任一步同一失败连续两次即停手上报。

0.0.61 到 0.0.62 的流程差异:(a) cache-manager 已入链(相 cache-manager,产物 CHAIN/cache-manager/x64、arm64),打包不再借用 ~/.cache/unisacc/borrowed-cm,续段命令去掉 `--cache-manager-osx`;(b) 起链前 `tools/chain-ready.cx` 为新增前置(real6 已过 --fast),发布前再跑一次 `--fast` 作为尾部前置;(c) g2/g3 linux 并发 G_LNX_PAIR=2,整链约 3500 s;(d) tools/sweep-plan.cx 取代手工大扫除;(e) tag 仍要 v0.0.62 与 v0.0.62-src 两个,指同一 F。

1. 打包(本地零网络)。若整链未自带打包:`sh ./unisacc.com tools/candidate-chain.cx -- --rev REV --out CHAIN --resume`(不带 --cache-manager-osx;先读 CHAIN 末行确认 rc=0)。输入身份:CHAIN 下 p3.pkg、六片、cache-manager 两份、sources.tsv。预期:CHAIN/ape-final 或 ape 下 unisacc.com,记 sha256 与字节数。回滚:删 W 即可,无外部副作用。
2. 造冻结提交 F(私有,不进 main 历史)。先取被构造 REV 的树(不是 origin/main 当前树;更正 2026-11-14,依据 F16:b2c557b1ba 之后 main 又多 18 个文件改动,F 必须与整链构造的 REV 同树):`T=$(git rev-parse b2c557b1ba^{tree})`,再改 release/candidate.tsv 只动 tag/commit/version 三字段(版本 0.0.62、tag v0.0.62,commit 先留空占位;0.0.61 的 F 即 main 树加此一行改动),用 Edit 工具改表后 `git add release/candidate.tsv`、`T2=$(git write-tree)` 在专用临时 index(GIT_INDEX_FILE=$W/idx,先 `git read-tree` 之外的方式不可用:禁用 read-tree,故改法为在共享 index 之外 `git archive origin/main` 解到 $W/f-tree,git init 后 `git add -f -A`(必须 -f,W1 实证)提交一次取树 T2 再 `git fetch $W/f-tree` 进仓取对象)。然后 `F=$(git commit-tree T2 -p fbdc99ca -p <main 构造 REV 的提交> -m '0.0.62 freeze F on main <REV 全 40 位> (private, candidate table version/tag only)')`。预期:`git diff <REV 全 40 位> F --stat` 只显示 release/candidate.tsv 一行。验收:`git cat-file -p F` 有两个 parent(fbdc99ca 与 REV)。注:candidate.tsv 的 commit 列必须是 F 的 40 位,而 F 的树含该表,二者循环,0.0.61 做法是 F 内表只带 version/tag,commit 与七摘要留给 X 重算;沿用。回滚:F 仅为悬空对象,不引用即无影响。
3. 打 src 标签(外部可见,可撤):`git tag v0.0.62-src F && git push origin refs/tags/v0.0.62-src`。预期:`git ls-remote --tags origin | grep 0.0.62-src` 返回 F。回滚:`git push origin :refs/tags/v0.0.62-src`(仅在草稿阶段未公开前允许,账本先记一行)。
4. SRC_DIR 用 `sh ./unisacc.com tools/pack-srcdir.cx -- CHAIN/ape-final/ape-receipt.txt SRC_DIR --sid SID` 装配(SRC_DIR 须全新;SID 为候选源快照上 `ctx/identity-helper SNAP` 的 64 位输出,real5 实算 73cb7149358013f1fb7f462c9526d5c566587fc8b452ee0ba91ad70e64045c02;real5 的 ape 内无嵌入 sources_sha256 可比对)。release-run --pack(两步合一,按 SRC_DIR 约定把 CHAIN 产物放成 p3.pkg、win-x86_64、lnx-x86_64、lnx-arm64、osx-x86_64、osx-arm64、win-arm64、cache-manager-x64、cache-manager-arm64、sources.tsv;ROOT 内放候选表私有副本):`sh ./unisacc.com tools/release-run.cx -- --pack ROOT ROOT/release/candidate.tsv SRC_DIR W/pack`。实际文件名核对以 tools/release-run.cx 第 9 至 40 行为准;SRC_DIR 若与 CHAIN 布局不符先建符号链接目录,不改产物。预期:state.tsv 步 0 至 9 全 DONE,W/pack/ape/unisacc-next.com、W/pack/layout(zip、SHA256SUMS)、W/pack/candidate.tsv(七摘要已回填)、末尾打印 NEXT gh 命令。随后 `sh ./unisacc.com tools/release-ape.cx -- smoke W/pack/ape/unisacc-next.com` 预期 `RELEASE_APE SMOKE ... failed=0` 且 `--version` 为 unisacc 0.0.62。回滚:删 W/pack。
5. X 提交(main,只带一个文件):把 W/pack/candidate.tsv 的内容用 Write 写入 release/candidate.tsv(commit 列=F 的 40 位,ci_run_id 待填,七摘要回填),`git commit -m '0.0.62 X: release/candidate.tsv(七摘要重算,ci_run_id 待填)' -- release/candidate.tsv`,确认分支为 main 后 `git push origin HEAD:main`,记 X=`git rev-parse HEAD`。预期:`git show --stat X` 只有该表。回滚:`git revert --no-edit X && git push origin HEAD:main`(绝不 force push)。
6. 草稿 release 与资产(私有,可撤):`gh -R R release create v0.0.62 --draft --title 0.0.62 --target F`,再对 W/pack/layout 内七个资产(unisacc.com、pkg.bin、slice-win-arm64、unisacc.com.build.json、build.json、zip、SHA256SUMS)逐个 `gh -R R release upload v0.0.62 W/pack/layout/NAME --clobber`(分批,每次不超过 60 s)。预期:`gh -R R release view v0.0.62 --json isDraft,assets` 为 isDraft=true、7 个资产。回滚:`gh -R R release delete v0.0.62 --yes`(写入 release/rollback.txt 同样的草稿阶段撤回段)。
7. 触发 CI(commit=X):`gh -R R workflow run candidate-native.yml --ref main -f ape_sha256=<unisacc.com sha256> -f commit=X -f release_tag=v0.0.62`。输入身份:ape_sha256 与 release_tag 的资产同一件。等待(有界轮询,总 deadline 45 分钟、至多 30 次、每次间隔 90 s,可用 Monitor 或 wait-housekeeping):`sh ./unisacc.com tools/ci-verdict.cx -- --sha X`;预期末行 `CI_VERDICT X PASS all-jobs-success`(六目标格加 fetch、cell-tools、ffi-lnx、report 共 10 作业);退出码 3 为 UNKNOWN 继续等,1 为 FAIL 停手上报,超 deadline 也停手并核实真实 run 状态,不盲目重触发。run id:`gh -R R run list -w candidate-native.yml --commit X --json databaseId -q '.[0].databaseId'`。回滚:`gh -R R run cancel RUNID`。
8. Y 提交(main):带 plans/release-notes-0.0.62.md(定稿,冻结点、裁决、real6 结果、速度补全 TBD 项)、release/candidate.tsv(仅回填 ci_run_id)、release/decision.txt、release/evidence.tsv、release/rollback.txt(把 0.0.61 版文字的版本号、F、X、run id 换成 0.0.62)。决策文件由 `sh ./unisacc.com tools/release-decide.cx -- --candidate release/candidate.tsv --evidence release/evidence.tsv --assets W/pack/layout > release/decision.txt` 生成;evidence.tsv 八行 step/verdict/path(1 freeze-sid、2 construct 指 real6.out、3 ape-pack、4 s9、5 layout-check、6 draft-assets、7 ci-qualified 指 run URL、8 readback)。判定规则:缺行即 UNKNOWN;任一 FAIL 则 NOT_RUN 不得公开(停手上报);全 PASS 才可称 QUALIFIED;s9 未跑则为 NOT_QUALIFIED unknown=1,发布说明如实写(0.0.61 即此状态,按项目铁律不宣称 QUALIFIED,但步 7 与步 8 以外无 FAIL 即可公开)。提交:`git commit -m '0.0.62 Y: ci_run_id=RUNID, release notes, decision, evidence, rollback' -- plans/release-notes-0.0.62.md release/candidate.tsv release/decision.txt release/evidence.tsv release/rollback.txt` 后 `git push origin HEAD:main`。回滚:forward revert。
9. 不可逆:公开发布(先决条件:rollback.txt 已含公开后撤回段并已提交,账本写一行:原因、F/X/Y、run id、截止;已由项目所有者长期授权,不再请示)。命令:`gh -R R release edit v0.0.62 --draft=false --latest`,并在草稿阶段已有 tag v0.0.62 指 F(若 tag 尚不存在,公开时由 --target F 的草稿创建;若需改标签用 release-run 打印的 `gh release edit v0.0.62 --tag v0.0.62 --target <40 位 F>`,先改标签后公开,不先删旧标签)。预期:isDraft=false,`git ls-remote --tags origin v0.0.62` 返回 F。撤回(公开后):`gh -R R release edit v0.0.62 --draft=true` 且 `gh -R R release edit v0.0.61 --latest`;不删任何 tag。
10. 公开后读回:`gh -R R release download v0.0.62 -p unisacc.com -D W/dl`,`sha256sum W/dl/unisacc.com` 必须等于 candidate.tsv 的 ape_sha256,`sh W/dl/unisacc.com --version` 为 unisacc 0.0.62;SHA256SUMS 核全部资产(release-run 的步 8 同法)。读回日志写 evidence.tsv 步 8,随下一实质提交带上。
11. 本机安装:先 dry-run `sh ./unisacc.com tools/install-ape.cx -- ~/repos/unisacc W/dl/unisacc.com <ape_sha256>`,预期 `INSTALL_APE` 状态非 REFUSE;再同参加 `--write`。预期:ROOT/unisacc.com.ape.sha256 记新 sha,`sh ./unisacc.com --version` 为 0.0.62,unisacc-dev 保留。回滚:把 ROOT/unisacc-dev 之外的旧件(0.0.61 发布件,sha c4c47ecb)用同工具装回:`install-ape.cx -- ROOT 旧件路径 c4c47ecb97ee70865edaf39d91e29f5b22b6c8efdbcfb81c4482517ffc0224c3 --write`(旧件保存在 W/dl 前先复制到 ~/.cache/unisacc/ape-0.0.61/)。装机后提交 ape-receipt.txt 与 plans/root-identity.md(如 25b905be9d),并确认 productpkg-dir-identity 批转绿。
12. 公开后大扫除:`sh ./unisacc.com tools/sweep-plan.cx`(只读计划;tools/sweep-plan.cx 目前仅在工作区未入库,先随实质提交入库再用),按 DELETE 行用 cx_rmtree 删除,KEEP 行(在途链根、borrowed-cm、ident-helper、bootstrap、release*)与 ASK 行不动;ASK 行逐条核对后再定。结果写本文件"当前发布与版本"一节原位更新,旧 0.0.61 状态行同步改写。

已验证(2026-11-14,见 W1/W2 行):F 的造法在 /tmp 复演与 d4b3bf72f7 逐位相同,但步 2 的 `git add` 须带 -f(否则 .gitignore 丢 6 个已跟踪文件);SRC_DIR 十个名字在链目录顶层都不存在,须按 ape-receipt.txt 路径建符号链接目录,sources.tsv 来源未查明。未验证:SRC_DIR 在 real6 完成后的实际路径、sweep-plan 实跑、公开命令均未在 0.0.62 上执行;执行时每步以工具实际输出为准,与本节不符处以工具为准并更正本节。

## 阻塞与未验证(发布前清零或如实写进发布说明)

### 阻塞发布

- 审查 整链工具(2026-11-14,读码为主,未执行 pack/release;chain-real5 目录在本机不存在,干跑未做)。缺陷:
  (1) 中 tools/candidate-chain.cx:2440-2446 前置门按 --out 字符串切父目录找 src。--out 为不含斜杠的相对名时 cut=0,HEAD 检查被静默跳过;无显式 cache-manager 时报的是"no in-chain source"的误导拒绝。--out 是符号链接时查的是链接所在目录的 src 而非真实父目录。src 存在但无 .git(归档解出)时 HEAD 检查静默跳过,且只比 HEAD 不查脏工作树。读码推断。建议:门开头要求 --out 绝对路径并先 realpath,src 存在而无 .git 时具名拒绝,另加 git status --porcelain 为空。
  (2) 中 tools/candidate-chain.cx:2448-2450 与 :2864-2880 门检查的 src 是 --out 同级的 src,构造 cache-manager 用的却是 root/tools/cache-retain.cx;两者未断言同一路径,门绿不保证构造所读源与 --rev 一致。读码推断。建议:门里断言 root 的 realpath 等于 src 的 realpath,或构造改读 srcd。
  (3) 低 tools/candidate-chain.cx:2879-2881 两次构造各 90 s,串行共可达 180 s,而 build/chain-phases.tsv 第 497 相超时记 90(gen 的 timeout_s 即 phase.bound)。单次调用不超 phase.bound,但相记录低估,且相合计 180 s 超过"每命令 60 s"口径需限时例外说明。建议:拆成 cache-manager-x64 与 cache-manager-arm64 两相各 90,或在表里写 180 并入 limits.tsv 豁免。读码推断。
  (4) 低 tools/candidate-chain.cx:2879-2882 构造成功(rc 0)后未检查 cmx/cma 大小就 mark 相完成;产物为空时相已记完成,到 pack 才被 :2894 的 fsize<=0 具名拒绝(不会用空文件,该点合格),--resume 时 :2871 的 fsize>0 会令相重跑,不会被错误跳过。建议:mark 前就地检查,把拒绝前移到相内。读码确认。
  (5) 低 tools/candidate-chain.cx:2388 --cache-manager-osx 两参数成对解析,门与 :2866 的 `cm64 && cmarm` 判断一致,未发现缺陷。显式给出的管理器路径只在 pack 相查大小,不查 Mach-O 魔数。建议:加魔数检查。读码推断。
  (6) 低 tools/pack-srcdir.cx:63-75 来源路径存在性相对当前目录检查,符号链接却建在 OUT_DIR 内;receipt 若记相对路径,链接会悬空而检查通过。同时不检查来源非空(0 字节也算存在)。具名拒绝与不留半成品两点合格:全部缺项先收集一次列出,rc 2 后才 mkdir。另 :60 的 miss 缓冲 2048 字节溢出时静默截断缺项名。读码推断。建议:对来源 realpath 后再链接,检查 size>0。
  (7) 低 tools/sweep-plan.cx:57-100 IN_USE 读 /proc,已退出进程无 /proc 条目故不留残余 fd,该点无问题;僵尸进程 fd 已关闭同理。局限:fd 只扫 0-63;他人属主进程的 readlink 因 EACCES 静默失败(漏报在用);cmdline 缓冲 32 KB 截断;引用缓冲满只在 stderr 提示、删除建议仍照出。KEEP 对 ident-helper/bootstrap/borrowed-cm 靠 entry() 的 keep 参数,未逐类核对调用点。tools/sweep-plan.cx:270-280 scan_build 的 git 子进程时限只在 read 返回后判断,管道阻塞时不会超时;超时分支 sy_kill 后不 wait。读码推断。建议:poll 带时限读,kill 后 wait,非本人进程存在时把整份 DELETE 降级为 ASK。
  未审:tools/chain-ready.cx 仅核对所有 argv 带 ./tools/bound 前缀(spawn 路径 172/206 与 sweep-plan 的 git 调用无独立时限),其判定逻辑未细读;tools/chain-phases-gen.cx 只核第 497/498 行(deps compare/cache-manager、超时 90),未重跑 --check;candidate-chain 其余约 2700 行未审;未做任何动态复现(real5 目录缺失,real6 不碰)。--resume 对 497 相:ck_done 加 fsize>0 双条件,不会错误跳过;失败重跑会覆盖半成品。

- 全集批次复验没有有效判定(整节最大缺口):共享检出上 wt gate 在波内因他人改动判 identity=CHANGED,整波记 UNKNOWN。实测累计只有快照根 45 批(PASS 43,FAIL 2,两项均已修,见索引),共享检出 15 批(PASS 14,FAIL 1 即 tsv-lint)。lnx IMPLEMENTED 约 1040 至 1076 批,其余未复验;约 80 个 FTRIM 红批旧估算未证实也未证伪(清单缺)。清零动作:在冻结窗口或 git archive 快照根(需在其中 git init 提交一次、拷入 unisacc.com 与 unisacc.com.build.json)用 all-waves 分波跑;会话目录必须是 /tmp/unisacc-affected-NAME/s,wave 必须用原生执行器(init 约 21 s、executor 约 37 s,wave 参数 `--light ID ID ...`,每波 8 批,wave bound 55 s 减扫描约 17 s,单批固定开销约 27 s)。light 约 742 批约 93 波(推算,非实测),heavy 313 批每波 1 批(推算约 5 小时,须分片,并需 s8c-bind 材料)。慢批(单跑改 heavy):pp-cli-includes、pp-header-locations(超 37 s 被杀),snapshots-cx 临界(批内约 30 s)。osx/win 平台批次无主机无证据。不能清零就把覆盖面原样写进发布说明。
- 候选 s9、同候选 CI、real6 整链结果、候选速度:TBD,见发布说明占位;未出结果不得写 QUALIFIED。
- s9 分波方案(2026-11-14 只读预备,未实跑;命令取自 tools/s9-nightly.cx、s9-nightly-run.cx、release-decide.cx 的 usage 注释)。
  (1) 定义:s9 是同次运行、不跨次缓存结果的全批资格(tools/s9-shards.cx 分片、tools/s9-nightly.cx 编排),档位由 tests/batches.tsv 的 tier 列定:commit 140 批、daily 650 批(不带 --tier 即 FULL)、release 41 批(prd.md 436 行,以 init 实际选出为准)。实测只有 commit 档 3 步 252.7 / 87.3 / 12.7 s(0.0.53);tests/tier-runtime-summary.tsv 预测 daily-layer 约 16111 s(条件值,322 批 UNMEASURED)、commit-layer 约 290 s;release 档墙钟 UNMEASURED;daily 与 release 档至今未跑通一次。
  (2) 分波(ROOT 用仓外私有克隆或快照根并自带 unisacc.com,产物写 $TMPDIR 或快照根,不写仓 out/;每步自动设 UNISACC_BOUND_DEADLINE_MONO,单步 240 s;并发宽度环境变量 S9_WIDTH 1..15,默认 5):
  W0 一次有界:`sh ./unisacc.com tools/s9-nightly.cx -- ROOT RUN --tier commit|release --phase init`(daily 档用 --tier daily),退出 0 即 PREPARED。
  W1..Wn 循环(总次数上限与总 deadline 自设):`sh ./unisacc.com tools/s9-nightly.cx -- ROOT --resume RUN --phase steps --max-steps 1`,退出 3 为 PENDING 续跑,0 为 STEPS_DONE;中断片保持 UNKNOWN,不静默重跑;间隙用 `sh ./unisacc.com tools/s9-profile.cx -- RUN` 只读看进度。
  Wa 汇总:`sh ./unisacc.com tools/s9-nightly.cx -- ROOT --resume RUN --phase aggregate --agg-parts 4`。
  夜跑薄入口:`sh ./unisacc.com tools/s9-nightly-run.cx -- --clone DIR --evidence DIR --tier release|daily [--max-steps N] [--load-max L]`(负载高只加 nice,不降宽)。
  顺序:先 commit 档(约 6 分钟、3 步各小于 5 分钟)得首个 PASS/FAIL 基线,再 release 档,最后 daily 夜跑;依决定 5(c) 只有 commit 档卡发布。
  (3) 进 evidence:release/evidence.tsv 第 4 行 `4<TAB>PASS|FAIL|UNKNOWN<TAB>RUN 证据路径`。release-decide.cx 只认 step/verdict/path 三列,缺行即 UNKNOWN,不解析 s9 内部字段。0.0.61 第 4 行 UNKNOWN(指向 oneshot61F,其 08/09 plan 为 MODE=DRY_RUN),故 NOT_QUALIFIED unknown=1。要变 QUALIFIED:第 1 至 8 行全 PASS,其中第 4 行须由 aggregate 无 FAIL/UNKNOWN/DEFERRED 得出。aggregate 到第 4 行的映射用 `sh ./unisacc.com tools/s9-verdict.cx -- RUN`(只读;读 RUN/aggregate.out 即 `s9-shards aggregate ROOT RUN` 的 stdout 落盘与 RUN/tier,输出 `4<TAB>PASS|FAIL|UNKNOWN<TAB>路径`;仅 release/all 档、全 BATCH 为 PASS 且批数与 tests/batches.tsv 登记一致才 PASS,缺失不全一律 UNKNOWN;检查 check-s9-verdict.cx 批 s9-verdict;未在真实 aggregate 上跑过,UNVERIFIED)。落盘步骤已补(2026-11-14):tools/s9-nightly.cx 的 aggregate 相(单次与 --merge 路径)把 aggregate 完整 stdout 原样写入 RUN/aggregate.out,RUN/tier 由 s9-shards init 写入(封印文件,nightly 不重写);夹具断言见 check-s9-nightly 批 s9-nightly-phases,真实 aggregate 上仍未跑过(UNVERIFIED)。0.0.62 现缺第 2 行(real6 整链)、第 4 行、第 7 行(同候选 CI)。
  (4) 可行性核对:s9-nightly.cx 的 --fake SCRIPT(夹具 tests/fixtures/s9-nightly-fake.cx、检查 tools/check-s9-nightly.cx)可验编排顺序而不跑真批次;--phase init 含 build/prepare,较重,real6 在跑时不做(UNVERIFIED);tools/check-s9-release-tier.cx 核 release 档 id 与 batches.tsv 一致(需 ABS_OUT_DIR,未实跑)。
  (5) 清零最短路径:real6 结束后,在冻结候选快照根跑 commit 档 init、steps(约 3 轮)、aggregate 并回填第 4 行;同候选 CI 回填第 7 行;release、daily 档夜跑,结果如实入发布说明,不卡发布;另补 aggregate -> evidence 第 4 行的小工具并在 build/pkg-gaps.tsv 记缺口。

### 仍未验证(缺什么 / 清零动作)

- 新产品验证:cx-namespace CX-U-2 三缺口具名拒绝(57c7427f6f,cx.plan 0.1.3、cx.compile 0.2.7)与别名运行修复在已装 0.0.61 上标 UNVERIFIED needs-new-product。缺:候选产品。动作:整链装好后复跑 tests/cx-namespace.cx。
- check-stdio-buffer:不覆盖 --link-only 与子集编译的缓存 identity 一致性,tty/pty 路径未测,六目标未跑。动作:补断言并在六目标主机各跑一次。
- build/speed-baseline.tsv 未更新;run-startup-bench、check-bench-gencode、speed-probe 对新产物未跑。动作:候选装好后各跑一次并更新基线。
- 整链耗时:real5 3538 s 对 real4 4759 s(-26%)只有一组对照,未控负载,不作结论;pack 37 s。real6 在跑。
- osx cache-manager:借用副本(x86_64 452d9240、arm64 3f6e4cbd)不能由现有输入重现,来源未证实;自产(cache-retain.cx 按 -b osx/*,x86_64 sha 89d0b937、arm64 5096089c)已有 cache-manager 相(5c4ec0e227,argv 修 b54d7db899),但候选上首次实跑与自产件在 macOS 上的运行和签名未验证。动作:real6 看该相;需 osx 主机。
- tsv-lint 不稳:同日三次实跑 PASS、FAIL、PASS,疑与并发写表有关,未定因;冻结窗口 `affected.cx --ids tsv-lint --run -j 1` 为 PASS rc=0 22 s。动作:下次红时先留 git status 与失败行再判。
- wt-gate-probe 为 PLANNED(链路超 55 s 批次上限),6 条断言未验证;win 批次 wt-gate-clean-win 为 PLANNED/NOT_READY 无证据。wt-gate-clean 已 PASS。
- slice-migration-gen:source-missing cx/compiler/frames.cx(HEAD 未跟踪),旧记述,是否仍在未复核。
- affected-iface2 批次未取得实跑结果(affected-tool、affected-list 已修)。
- -c 对象路线无最终产物复用(只丢标记戳,既有状态,每次全跑约 280 ms),真复用排 0.0.63 任务 N15;artifact_hit 戳消失的提交未二分(候选 a3a32d8a09、9b107888db 均未实证)。
- 已入库工具中 tools/status.cx 的测试夹具曾由子代理用 sed -i 改过,属违规,内容未重审。
- cx 模块语法缺口(0.0.62 若做须属 cxmod 三源产品输入,先修别名):`use m::sym as alias` 通过规划但运行报 `no host function slen`(rc 127);`#pragma unisa export` 未实现;`use sys.*` 未设计;仓外脚本找不到包根(需 cxroot.tsv)。
- cx 模块身份缺口:同一文件经软链 tools/cxlib/sys -> cx/sys 被当两个模块(`sy_env exported by both`),使 tools/osx-sentinel.cx 等编不过。修法:cxmod 按 realpath 规范化,排在别名修复 T5 之后同写域串行;冻结的 cx/sys 不改。
- 仓库根 out/:仍写 out/ 的 tools/gen-model-decode-material.cx、tools/csih-ape.cx(csih 车道,须由其维护者迁)、tools/chain-run.cx(--out 须在 ROOT/out 下)。

### 已入库工具(记录,非缺口)

- tools/derive.cx 派生物总闸(六个生成器 --check 全 OK,22 s)、tools/status.cx(--fast 0.97 s 热态)、affected --iface-narrow 按包判定(text.cx 903 -> 765)与 --symbol-narrow(768 -> 4,函数指针与宏引用不识别)。

### 进发布说明的已知缺口(对账 plans/release-notes-0.0.62.md 的 (1) 至 (11),只列差异)

- 该文档漏写:(a) 慢批与单批固定开销(27 s)使全集复验必须分片,不止"约 45 批";(b) tsv-lint 同日不稳;(c) osx cache-manager 借用副本不可重现、自产件未在 macOS 验证(其 (9) 只写相实跑 TBD);(d) wt-gate-probe PLANNED、wt-gate-clean-win 无证据;(e) cx 模块身份缺口(软链双模块)与 alias 运行报 no host function slen(其 (6) 只提别名修复需新产品验证);(f) check-stdio-buffer、speed-baseline 未更新、速度探针未跑;(g) -c 对象路线无最终产物复用在其 (8) 仅列为 0.0.63 项,未说明现状是标记戳丢失。
- 该文档已写且本节无需重复:(1) 至 (4) 平台与 DNS 缺口、(7) s9、(10) dock-audit 与 derive 登记、(11) CI。
- 该文档需更新:(5) 写"FAIL 两项他人在修"已过时,两项均已修(ri-image-pem a1bf73b583,phase-timing-stamps dfe2b5fa75),应改为已修并 PASS,累计数字同步。

### 已解决索引(结论由提交号找回)

- check-candidate-chain rc=139:根因是检查工具 find_sub 把 tx_len 的 -1 转 size_t 越界读,非编译器缺陷;已修 cc22f4539d(七个同型文件同改、草堆上限 16000000、未找到按 FAIL,candidate-chain 加 zz 真断言)。
- affected-tool、affected-list 字面量失败:check-affected own() 漏 affected-iface/affected-iface2,已修 908df53919;tsv-lint 复验 PASS(`_u_so_put` 不再出现)同提交。
- ri-image-pem duplicate-key-row:已修 a1bf73b583。phase-timing-stamps obj.miss/hit 戳:已修 dfe2b5fa75(详细定因在该提交说明)。
- wt-gate 旧"5/8 红":前提不成立,wt-gate-clean 实跑 PASS(check 自建原生 run-batch)。
- 红批复验过程细节(16 核 WAVE_JOBS_MAX=16、超时定因、剩余波数推算)已浓缩进"阻塞发布"首条;原长文见 git show fd7a68eb36:prd_mempalace.md 的本节。

## 下一动作(0.0.62,fable 规划;取舍已定见第 5 条,可派任务树见本节末表)

主题:库包由内往外,网络栈封口(cx.sys netraw/netpoll -> ext.net 清零 new 行;表中 new 共 16 行已落 14 行后以 build/std-surface.tsv 为准)。cx.stream/tlib 重构排 0.0.63。

1. 主题线:L2 netraw 补六参 sendto/recvfrom 与 IPV6_V6ONLY(fable);L3 UDP/unix/resolve(sonnet);L4 带截止时间五个 TCP 行(fable);L5 cx.sys 三行加 np_exec;L6 osx/win 只交有名 ENOSYS 拒绝加 CI 证据。L1(netpoll/sock)已在 main 并复验 PASS。
2. 速度线:冷启动 first_e2e 中位 704.9 -> 670 ms 以下;冷编译 entry-build 13624 -> 12300 ms 以下、whole-front 12846 -> 11600 ms 以下;生成代码先重测 1MB sort 再定目标(120 ms 以下为建议值);run-startup-bench、coldcompile、check-bench-gencode、speed-probe 四个工具对 0.0.61 发布件补跑入 speed-baseline。
3. 发布线(冻结到公开 100 分钟内)。real5(g2/g3 并发,rev b418cb5d)整链实测 3538 s(/tmp/unisacc-session-build-lr0060/log/real5.start 与 real5.end 之差,不含打包续段;与 real4 的对照见阻塞节整链一条,单组对照不作结论);首段以 rc=2 结束,原因是 `REFUSE pack needs --cache-manager-osx`,续段 real5p rc=0。real5 各相分解未入 build/phase-times.tsv(任务 A3)。real4 整链实测分解(RECEIPT 墙钟求和,工具 tools/zz 式 .cx 分析,日志 /tmp/unisacc-session-build-lr0060/log/real4.out):g2/g3 linux 对象相四段约 2480 s(约占 51%,最大头);jobs-gen/tbl/net 三族 801 s(gen 416、tbl 233、net 152)加 jobs-tool 53、final-net 85、p3-guard 181。结论:g2/g3 并发整链已实测(8d70af5101,结果见本条开头),下一步查对象相单批约 320 s 的构成;jobs 三族改按模型流水线(C1 第二段方案,build/chain-phase-outputs.tsv)至多省约 380 s,排其后。R2 拆 g2 对象相(限期 2026-11-30);R3 起链一条命令(根工作树与 REV 不同提交即拒);R4 osx cache-manager 自产(借用副本在 ~/.cache/unisacc/borrowed-cm/);R5 CI 前后零手工;R6 s9 三档首次跑通。
4. 基础设施(不入产品输入):I1 wt-gate 修红;I2 slice-migration-gen 定因;I3 agentwork 归档第 4 批起(第 3 批已退役 archive-records、aw、check-aw、cursor-fleet、dag 及各自检查;awpath 留第 4 批;分类表见提交 b10474a8f0 说明;第 4 批已归档 roles 下 6 个无引用角色文件到 archive/agentwork-retire-2026-11/roles/,ledger/items/dag/WORKFLOW.md/roles/README.md/release-tail-058.md 仍有活引用未动,活依赖清单见该提交说明;observe-agents 已改读 prd_mempalace.md 的「已落地」「部分落地」行,无该文件输出 UNMEASURED;version-metrics 规则违反计数改读 archive/work/ledger-202611.tsv,读不到输出 UNMEASURED,二者不再读 agentwork/);I4 out/ 写入点收尾(gen-model-decode-material、chain-run --out、csih-ape 须由其维护者迁);I5 装发布件后复验 FTRIM 类红与 DEFERRED 批。
5. 已决定(2026-11-14,主调度按推荐定,项目所有者要求能做尽量做):(a) SPEED-11 运行库扫描族进 0.0.62;(b) 发布门不等 osx/win 的 ext.net 实现,只交有名 ENOSYS 拒绝加 CI 证据;(c) s9 只让 commit 档卡发布,daily/release 档夜跑并如实写进说明;(d) 0.0.62 主线为"构造链与批次分治"(C1 相清单化、C3 批次纯函数化、C4 接口哈希表),封网(ext.net/netraw)与 cx 模块语法(别名运行失败修复、export、通配)作并行线,产品输入变更合并为同一类(库包与运行库),构造链/批次工具不入产品输入。

0.0.62 候选整链(real6)就绪判断(fable 第二轮,2026-11-14 15:50,基于 HEAD ebb91f2032;判断非实跑,P1 与前置条件须实跑后才算数):

- 结论:现在不起。等 B2c、B2e 入库且下列前置条件全绿即起;硬截止 16:40,到点 B2c/B2e 未入库就以当时 main HEAD 起链,未完行保留 new 并如实写进发布说明。不等 B2f、B2g、B3(SPEED-11):前两项只增文件,晚到不改变风险;B3 动既有运行库扫描族,补丁未写,等它会把候选里未经新产品验证的 cxmod 改动再压一小时(F4)。B3 若在 real6 起链前探针已绿则并入,否则作同类第二候选 real7,不与其它类合并。
- 理由:(1) 整链一次约 59 分钟,同一时刻只一条;现在起则 UDP 三行与 TCP 五行落在候选之外,封网主题要再跑一次整链。(2) 14:55 以来 pgrep 无在途 candidate-chain,load1 约 5,/ 空闲 98G(15:49 实测),资源不构成等待理由。(3) 工作树有未跟踪的产品输入 ext/net/udp.cx,此刻冻结会得到与在途工作不一致的 REV。
- 产品输入现有变更(`git diff --name-status v0.0.61 HEAD -- src cx ext exec/facts`,28 个路径):
  - 属库包与运行库类:cx/sys 新增 netmsg、netpoll、netraw、refusals 与只增的 sysnums;ext/net 新增 addr、sock、tcp、unix;ext/sysx 六文件与 ext/crypto 三文件(各 2 至 10 行);exec/facts 的 sys-syscalls(加 35 行)、libc99posix(加 111 行)、refusals(新)。sys-syscalls.tsv 的消费者只有 tools/target-gen.cx(`grep -rl sys-syscalls.tsv tools src cx exec build` 实证),产出 cx/sys/sysnums.cx,不进 kernel 事实,不属模型与网类;target-gen 同时写 cx/compiler/targetname.cx 与 cx/runtime/targetname.cx,diff 中两者未变。
  - 跨类(编译计划器,不是库包):cx/cx0/plan.cx(加 199)、src/cxmod.c(加 199)、cx/compiler/cxmod.cx(加 128)、src/driveplan.cx(1 行)。第 5 条 (d) 已决定按缺陷修复并入同一类,此决定只覆盖"修复",逐项判断如下。
- 逐项跨类风险:
  - CX-U-2(57c7427f6f):中。三条是新增拒绝(行为收紧),不是纯修复;链内 build 与 jobs-tool 段用候选编译器编 tools 与 src,任何一处踩到新拒绝即红。缓解:P1 扫描为 0 才起链。
  - T5 与 cxm_checkcalls 上移(4aee1dfb4a、68039f11f1、1a48fc9a0a):中。同样新增拒绝面;a415413ac2 只验了 cxcc 语料 11 文件,cx-host-run 内联夹具未枚举;build/cx0-three-source-diff.tsv 仍有 14 行标"是"的语义分叉(共 181 行),产品件是手写 cx/compiler/cxmod.cx,源级测试经 plan.cx,源级绿不等于产品绿。缓解:P1 加 `tools/check-cxmod-tri.cx` failed=0。
  - 模块身份 realpath(e576000457):低。可选回调,未绑定或解析失败退回原文本比较;driveplan 只多一条绑定语句。
  - cache-manager 相(5c4ec0e227、b54d7db899):不是产品输入,但改了链本身;它是第 497 相(build/chain-phases.tsv 第 500 行,compare 之后),第一次实跑要等到约 3500 s;候选 entry.elf 以 `-b osx/*` 编 tools/cache-retain.cx 未实跑(已装 0.0.61 实跑 rc 0 不等于候选)。缓解:首链不带 `--cache-manager-osx`(验证自产),该相失败则 `--resume --cache-manager-osx` 用借用副本续段(real5p 先例 rc=0),损失只有 pack 段。
  - netmsg 与号表(832d52a8d3):低。只增文件加两行号;lnx/arm64 的 msghdr 布局未实跑(F15)。
- 仍缺项(build/std-surface.tsv 的 new 行,15:49 为 14 行):ext.net 10 行(UDP 3 在途、TCP 3、recv_until/send_all_until 2、resolve 2)、ext.procx np_exec 1 行、cx.sys 3 行;SPEED-11 未动;osx/win 有名 ENOSYS 的 CI 证据未取。
- 起链前置条件(全部实跑,任何一条不过不起;可判定项一条命令:`sh ./unisacc.com tools/chain-ready.cx`,`-- --fast` 跳过 P1;配方五步与账本行仍为人工):(1) REV 为 main HEAD,`git status --short` 无未跟踪或未提交的 src、cx、ext、exec/facts 路径;(2) `tools/derive.cx -- --check` 全 OK,`tools/ratchets.cx` 末行 ok=13 fail=0;(3) `tools/check-cxmod-tri.cx` failed=0 且 GAP 具名,`tools/cx0-extract.cx -- --check --all` EXACT;(4) P1 新拒绝数 0(2026-11-14 复测 HEAD b2c557b1ba:扫全部含 `#pragma unisa use` 的 .cx(tools、src、tests、cx、ext,递归含夹具子目录)共 1190 个,cx0_plan 与产品 cxm_plan 各一次(bound 40,8 路),各 1175 通过 15 被拒,两路逐文件一致;三类新文案(bad symbol name、does not export、call to undeclared function)只在 4 个文件命中:tools/install-fixes/iproto/user.cx 与 main.cx、tools/symindex/fixture/proto/user.cx、tests/cxclash/main.cx,均为有意负例夹具(check-install-fixes 注明 iproto 应报编译错,cxclash 注明 must still be refused),且自 3439ee96cd 起未改,故新拒绝(链内要编的源)为 0;自 3439ee96cd 起变动的 16 个 .cx 全部两路 RC=0;被拒 15 = 上次 7 个原样在内(cxcc_missing、cxstd_unknown、nets、四个 sy_env 重复导出)+ 4 个上述夹具 + 4 个其余负例夹具(tools/install-fixes/uses48/main49.cx 超 48 条 use、tests/cxpack/bad-use/main.cx 缺模块、tests/cxcycle/ping.cx 与 pong.cx 环),后 8 个是递归扫到的子目录负例,上次范围(1057)未含,并非新回归;wall 约 3 分钟。上次 bd67c35af4 实测:扫 1057 个含 `#pragma unisa use` 的 .cx(tools、src、tests、cx、ext),每个经 cx0_plan 与产品 cxm_plan 各规划一次(check-cxmod-tri.cx --cx0/--prod 子进程,bound 40,8 路,wall 110 秒);规划通过 1050,被拒 7,两路结果逐文件一致;被拒 7 个均非三类新文案:tests/cxcc_missing.cx、cxstd_unknown.cx、nets.cx(缺模块,负例或遗留)与 tools/check-loader-special.cx、check-tool-self-path.cx、osx-sentinel.cx、test-loader.cx(sy_env 重复导出,旧行为,四个均为既有问题,不属新拒绝,链内是否编这四个未核)。全库无 `use NAME::*`;bad symbol name、does not export、undeclared function 文案命中 0。限度:只覆盖 plan 层(含 cxm_checkcalls 若其在 plan 内);tests/cx-host-run.cx 内联夹具只读源核对 use 行(28 行,m::x、m::y、amod::ax as ay 等符号级,是故意的正负例,未逐个运行);未做全文件完整编译);(5) `tools/check-candidate-chain.cx` failed=0(含前置门 3 条与 cache-manager argv 3 条);(6) 改过的 .cx 经 `tools/affected.cx --files ... --run` 无新红;(7) 配方第 1 至 5 步(私有根检出 REV、commit-tree 私有提交、UNISACC_IDENT_HELPER、全新 --out、超时 5400 s);(8) `pgrep -af candidate-chain` 无在途,同 REV 无已完成证据;(9) 起链前在本文件记一行:REV、PID/PGID、截止、输出目录。
- 失败的取证与重跑(整链 3538 s,两次即 7076 s,贴近 2 小时线):
  - 取证顺序:读 real6.out 末尾的 REFUSE 或 rc 行 -> 找到发出该文案的源码行再定性(rc 2 与 REFUSE 当确定性缺陷)-> 按落点分五档:(a) 前置门(第 1 秒);(b) build、build-front、jobs-tool(约前 5 分钟,cxmod 新拒绝最可能落在这里);(c) g2/g3 对象相(real5 各约 700 s);(d) compare(固定点不等);(e) cache-manager 与 pack。
  - 重跑:(a)(b) 修后全新目录重起,损失 5 分钟以内;(e) 用 `--resume` 续段,不重跑前段;(c)(d) 属产品输入缺陷,先用 `tools/chain-precheck.cx -- ROOT CHAIN_OUT NEW_OUT init --files LIST` 以失败链的 ctrl/unisacc.com.dev/entry.elf 对改动文件局部预检,过了再起整链。
  - 止损:同一失败两次即停;第二次整链失败且落点在 cxmod 三源,则候选 REV 改为不含那四个提交的私有提交(用 commit-tree 造,不在 main 上 reset 或 revert 他人路径),0.0.62 只带库包,cxmod 改随 0.0.64 的 D0;第三次整链不在当天起。
  - 观察:起链后第 5、20、45、60 分钟各做一次有界检查(读 checkpoint 与真实 PID),不派子代理干等。

real6 结果后的分支与发布排程(fable 第三轮,2026-11-14 16:45;real6 在途,16:41 实测 PID 3271413 存活、real6.out 已有 374 条 RECEIPT 并过 wf-prepare,故 (a)(b) 两档大概率已过,此为推断;上面"就绪判断"的结论、理由、前置条件已执行完毕,只留作证据,失败取证四条仍有效):

- 分支 S(rc=0 且有 p3.pkg 与六片)。时长依据(实测):0.0.61 的 X 提交 11:57:56、CI run 43554326355 创建 11:58:50 结束 12:02:03(193 s)、Y 提交 12:06:48、装机提交 12:55:05(`git log -1 --format=%ci 6b07b710eb 15bea6ef4e 25b905be9d`,`gh run view 43554326355 --json createdAt,updatedAt`);pack 续段 37 s(real5p);s9 commit 档三步 253/87/13 s(0.0.53)。排程(推算,非实测;T0=real6 结束,按 real5 的 3538 s 推为约 17:24):
  - 两路并行(T0 起约 6 分钟):甲路主调度做步 2、步 3(F 与 src 标签,写域:git 对象与远端标签);乙路一个子代理做步 1、步 4(pack 与 smoke,写域 W/pack;步 4 墙钟在 0.0.62 上 UNMEASURED,按 5 分钟估)。汇合点是步 5(X 需要 F 的 40 位与 W/pack/candidate.tsv)。
  - 两路并行(约 5 分钟):步 5(X,主调度,写域 release/candidate.tsv)与步 6(草稿加 7 资产,子代理,写域远端草稿);步 6 不依赖 X。
  - CI 等待(步 7,实测 193 s,加轮询按 5 分钟;唯一的纯等待):同时三路,写域互不相交:(i) s9 commit 档在候选快照根 init、steps、aggregate 并用 `tools/s9-verdict.cx -- RUN` 出第 4 行(约 6 至 8 分钟,卡发布,依决定 5(c));(ii) B5 用候选件路径直接验(不必先装机,见下"装后验证序列"V2 至 V4);(iii) 发布说明 TBD 项、evidence.tsv、rollback.txt 定稿(写域 plans/release-notes-0.0.62.md 与 release/ 三文件,先写在 W 下,步 8 一并提交)。
  - 串行收尾:步 8(Y,约 5 分钟)-> 步 9 公开(1 分钟,不可逆,先记账本行)-> 步 10 读回 -> 步 11 装机 -> 步 12 大扫除(10 与 12 的只读计划可并行,11 在 10 之后)。
  - 公开发布最早墙钟(推算):T0 + 6 + 5 + 8(CI 与 s9 取大)+ 5 + 1 = T0 + 25 分钟,约 17:50;步 2 的树取法与步 4 的 SRC_DIR 映射都是首次执行(发布步骤节"未验证"),各留 10 分钟缓冲,取 18:10 为计划值。自 16:25 冻结起约 105 分钟,略超发布线 100 分钟目标;超出部分全在整链 3538 s,不在发布步骤。
  - 步 2 须改(本轮实证,见 F16):`git diff --stat b2c557b1ba origin/main -- src cx ext exec tools tests build` 16:44 为 18 个文件、加 897 行(产品输入 src、cx、ext、exec/facts 仍为空),故 F 的树必须取 REV 的树(`git rev-parse b2c557b1ba^{tree}`)加 candidate.tsv 一行,不能取 origin/main 的树;验收不变:`git diff b2c557b1ba F --stat` 只一行。
- 分支 C(失败落点在 cxmod 三源:文案为 bad symbol name、does not export、call to undeclared function、exported by both,或发出行在 src/cxmod.c、cx/compiler/cxmod.cx、cx/cx0/plan.cx)。(1) 主调度 5 分钟内读 real6.out 末尾与发出该文案的源码行定性,账本记一行;(2) 一个子代理(写域仅三源加 tools/check-cxmod-tri.cx,15 分钟)出补丁,验收 `tools/check-cxmod-tri.cx` failed=0 且 `tools/chain-precheck.cx -- ROOT chain-real6 NEW_OUT init --files 失败文件` 通过(用失败链的 ctrl/unisacc.com.dev/entry.elf);(3) 全新目录起 real7(59 分钟,推算),公开最早约为重起时刻加 85 分钟;(4) real7 仍落 cxmod 即执行止损:用 `git archive b2c557b1ba` 解到 /tmp,把 cx/cx0/plan.cx、src/cxmod.c、cx/compiler/cxmod.cx、src/driveplan.cx 四个文件换回 v0.0.61 版(`git show v0.0.61:路径`),commit-tree 造私有提交起 real8,但第三次整链不在当天起;发布说明把别名、模块身份、CX-U-2 写为未随版。
- 分支 O(失败落点在别处)。(e) cache-manager 相或 pack:`--resume --cache-manager-osx` 用 ~/.cache/unisacc/borrowed-cm 两份续段(real5p 先例 rc=0、37 s),随后回到分支 S,发布说明第 (9) 项写明自产相失败与文案,不算第二次整链;(c) g2/g3 对象相或 (d) compare 不等:属产品输入缺陷,先 chain-precheck 局部预检,修后起 real7,同分支 C 的时间;若定因为负载或超时,必须先读发出退出码的源码行,不凭"负载"定性;timeout 5400 到点(约 17:55)按真实 PID/PGID 核实后才算终态。
- 整链等待期可由子代理做完的未验证项(单项 15 分钟内,写域互不相交,都不跑重测试):

| 号 | 目标 | 写域 | 验收命令 -> 预期值 | 分钟 |
|---|---|---|---|---|
| W1 | F 造法演练(避开 read-tree 的树取法首次执行) | 仅 /tmp 下全新目录与悬空对象 | 用 0.0.61 数据复演 git archive、init、提交、fetch、commit-tree -> 所得提交的树等于 `git rev-parse d4b3bf72f7^{tree}` | 10 |
| W1 结果(2026-11-14 已验证) | 在 $CLAUDE_JOB_DIR/tmp/w1 复演:`git archive 03c4d5df9d` 解出、用 `git show d4b3bf72f7:release/candidate.tsv` 覆盖表、git init、`git add -f -A`、提交、`git fetch --no-write-fetch-head`、commit-tree -p fbdc99ca -p 03c4d5df9d | 树 7127d64b72(对上 d4b3bf72f7^{tree});父提交两个(fbdc99ca、03c4d5df9d)对上;`git diff --stat 03c4d5df9d F` 只 release/candidate.tsv 一行对上;固定作者 f61 <a@b>、日期 1794628277 +0800 与说明后,提交号 d4b3bf72f7ae999c96c0fa15d8030a7c2d4de073 逐位相同;v0.0.61 与 v0.0.61-src 均指该提交。对不上之处:(1) 不带 -f 的 `git add -A` 因 .gitignore 丢 6 个已跟踪文件(.claude/hooks/guard-edit.ts、.claude/settings.json、research/c37-host-adapter.json、tests/.register.lock、tests/fixtures/elf-text/bridge_arm64.o、bridge_x86_64.o),树变成 3179d0c706,步 2 必须用 `git add -f -A`;(2) 0.0.62 不带固定作者与日期,提交号自然不同,以树与父提交为验收。仓库 refs 前后 for-each-ref 摘要相同。未覆盖:真实 0.0.62 的 candidate.tsv 编辑未做 | 10 |
| W2 | SRC_DIR 与 CHAIN 文件名映射 | 发布步骤节步 4 一句 | 读 tools/release-run.cx 第 9 至 40 行,对 chain-real6 已有产物名只读 ls 比对 -> 十个名字各有来源路径或标明链尾才产生 | 10 |
| W2 结果(2026-11-14 已核) | release-run --pack 要 SRC_DIR 下十个名字(p3.pkg、win-x86_64、lnx-x86_64、lnx-arm64、osx-x86_64、osx-arm64、win-arm64、cache-manager-x64、cache-manager-arm64、sources.tsv,release-run.cx 第 33 至 34、130、375 行)。chain-real5 与 chain-real6 链目录顶层十个名字均不存在(ls 实测 no),需建符号链接目录。来源以 chain-real5/ape-final/ape-receipt.txt 为准:p3.pkg 即 CHAIN/compiler.p4.pkg(存在);lnx-x86_64 即 CHAIN/fixedpoint/g3/lnx-x86_64/entry.elf(real5 存在);lnx-arm64 即 CHAIN/fixedpoint/g2/lnx-arm64/entry.elf(real5 存在);osx-x86_64、osx-arm64、win-x86_64、win-arm64 四片来自 gen1 的 image(real5 来自 ~/.cache/unisacc/chains/snap/b418cb5d86-25fdbc68/out/chain/gen1/{osx-x86_64-b2,osx-arm64-b2,win-x86_64-image,win-arm64-image}/image,均存在);cache-manager 两份 real5 来自 borrowed-cm(两份存在),real6 起为 CHAIN/cache-manager/x64、arm64(链尾产生,现无该目录);sources.tsv 在 real5 与 real6 链目录均无,tools 内未找到生成者(0.0.61 的 /tmp/cc-rel61 类目录已不存在),缺其来源,须先查 candidate-chain 或 release-build 是否输出 sources_sha256 再定。chain-real6 当前(链未完成):只有 compiler.p4.pkg、gen1/lnx-arm64/entry.elf、gen1/lnx-x86_64/entry.elf,无 fixedpoint、cache-manager、ape-final;real6 的 lnx 片是否仍取 fixedpoint/g3 与 g2 须以 real6 完成后的 ape-receipt.txt 为准。另:整链若自带 pack,CHAIN/ape-final/unisacc-next.com 已生成,步 4 的 release-run --pack 仍是重打包与回填,不是取代 | 10 |
| W3 | slice-migration-gen 旧记述复核 | 阻塞节该行 | `git ls-files cx/compiler/frames.cx` 与工具内引用 -> 行改为已消或保留并附当前文案 | 5 |
| W4 | tools/status.cx 夹具重审(曾被 sed -i 改) | tools/check-status.cx 及其夹具 | `git log -p` 对照夹具内容,check failed=0 | 15 |
| W5 | lnx/arm64 netmsg 是否在 CI 作业清单(F15) | 只读,结论写任务表 N14 行 | check-cx-sys-netmsg、check-ext-net-udp 在 candidate-native.yml 的 lnx/arm64 清单内,否则登记 DEFERRED | 10 |
| W6 | rollback.txt、evidence.tsv 0.0.62 骨架 | $CLAUDE_JOB_DIR/tmp/rel62 下草稿(步 8 才入库) | evidence 八行齐,第 2、4、7、8 行占位 UNKNOWN;rollback 含草稿与公开后两段 | 10 |
| W6 结果(2026-11-14 已备骨架,未入库) | $CLAUDE_JOB_DIR/tmp/rel62 下 rollback.txt(草稿删除、取消 run、revert X、公开后退草稿并 Latest 回 v0.0.61、删 v0.0.62 与 v0.0.62-src 远端与本地标签、install-ape 装回 0.0.61 sha c4c47ecb)、evidence.tsv(八行全 UNKNOWN,路径占位,第 4 行为 oneshot62F/evidence.tsv,CI 号写 TBD)、decision.txt(由 release-decide --evidence 读骨架生成,rc=0,八步 UNKNOWN,unknown=8 fail=0,NOT_QUALIFIED;命令未跑只写)。rollback 内 candidate commit、run 号、revert 提交号为 TBD,未编造;install-ape 旧件路径假定 ~/.cache/unisacc/ape-0.0.61/unisacc.com,须步 11 前确已复制 | 10 |
| W7 | 快照根复验一条命令的工具(F13 提前动作,若未造) | tools 下新工具加 check 与批次行(先查 git log --all) | check failed=0;对 2 批 light 夹具出 PASS 行 | 15 |

  必须等候选产品:cx-namespace 三拒绝与别名、模块身份(osx-sentinel)、s9 commit 档、同候选 CI、速度基线四工具、check-stdio-buffer 六目标(只能经 CI)、productpkg-dir-identity、全集批次复验。无法在本版清零、只能如实入说明:osx cache-manager 在 macOS 上的运行与签名(无主机)、win 批次、daily 与 release 档 s9、wt-gate-probe。
- 装后验证序列(B5 展开;V1 至 V4 以候选件路径为参数,CI 等待期即可跑,不必等步 11;未实跑不记 PASS):V1 `tools/release-ape.cx -- smoke W/pack/ape/unisacc-next.com` -> failed=0 且版本 0.0.62;V2 `tests/cx-namespace.cx -- ROOT 候选件 OUT` -> failed=0,UNVERIFIED needs-new-product 标注全部转真;V3 模块身份:`osx-sentinel -funit` 实编 rc=0,且 check-loader-special、check-tool-self-path、test-loader 三个 sy_env 重复导出文件规划通过(P1 被拒数 15 -> 11);V4 tests/module/units.cx 的标注转真;V5(步 11 后)productpkg-dir-identity 批转绿;V6 s9 commit 档(与 CI 并行,见上);V7 快照根全集 light 波(约 93 波,推算,夜间跑,占满核心)。速度补测排序:run-startup-bench(三次不到 1 分钟)插在 V1 之后、s9 起之前(整链刚停、机器最静的一刻);coldcompile 三文件各 3 轮、check-bench-gencode、speed-probe 排在公开之后、V7 与大扫除之前,串行,load1 低于 2 才当门,否则标 noisy 并在发布说明写 UNMEASURED,不为等安静窗口推迟公开。先测速再大扫除(见 F18)。

可派任务树(2026-11-14;每项 15 分钟内可交,写域互不相交;tests/batches.tsv 登记行一律走 `tools/commit-own.cx --keyed`;新建工具先 `git log --all --oneline -- tools/NAME`;预期值是验收门,未实跑不记 PASS)。开工条件:今=已装产品即可;候选=补丁今天可写,验收要含它的新产品;链=要整链窗口(同一时刻只一条)。

| 号 | 目标 | 写域 | 验收命令 -> 预期值 | 分钟 | 开工 |
|---|---|---|---|---|---|
| 已落地 | A1a、A1b、A1c(基线 6bcd7a98fb,冷编译标 noisy,speed-probe 仍 UNMEASURED)、A2、A3、A4(9a97992301)、A5(cache-manager 相 5c4ec0e227)、A6、A7a、A7b(7c7124c12c)、A8、A9、A11、A12、B1(c945e3a153)、B2a、B2b(832d52a8d3,cx.sys 0.1.7)、B2d(3502c3966d,ext.net 0.1.5) | - | `git log --oneline v0.0.61..HEAD` 对应提交 | - | 完 |
| A10 | wt-gate-probe 6 条断言转真(wt-gate-clean 已 PASS) | tools/check-wt-gate.cx | probe 批次拆到 55 秒内,6 条断言实跑通过,不删断言 | 15 | 今 |
| B2c | ext.net UDP 三行(在途:ext/net/udp.cx 与 tools/check-ext-net-udp.cx 15:49 尚未跟踪) | ext/net/udp.cx、tools/check-ext-net-udp.cx 加批次行、std-surface 三行 | check failed=0;超长报文 -EMSGSIZE;截止到期 -ETIMEDOUT;三行 new -> exists | 15 | 在途 |
| B2e | 带截止时间 TCP 三行(sy_tcp_accept_from、sy_tcp_connect_to、sy_tcp_dial)与 sy_recv_until、sy_send_all_until | ext/net/tcp.cx、tools/check-ext-net-tcp.cx、std-surface 五行 | `tools/check-ext-net-tcp.cx` failed=0;五行 new -> exists | 15 | 今 |
| B2f | L5:cx.sys 三行(sy_sig_watch、sy_sig_taken、sy_res_live)与 ext.procx np_exec | 各自新文件加 check 与批次行(冻结层只增文件) | 各 check failed=0;std-surface 的 new 行 14 -> 余数如实 | 15 x 2 | 今,不卡 real6 |
| B2g | sy_resolve、sy_resolve4 定因(表写 impl:getaddrinfo 而 cx 层无 libc) | 只读,结论写回本行 | 给出做法或"本版保留 new"及原因 | 10 | 今,不卡 real6 |
| P1 | 起链前新拒绝扫描(CX-U-2 三拒绝、cxm_checkcalls 上移,对链内会编的源) | 只读;结果写回"0.0.62 候选整链就绪判断"前置条件 (4) | 对 tools/*.cx、src/*.cx 及 cx-host-run 内联夹具经 cx0_plan 规划,新拒绝数 == 0(a415413ac2 只扫了 cxcc 语料 11 文件) | 15 | 今,卡 real6 |
| P2 | phase-timing-stamps 的 -c 两条断言改为真实行为(不恢复 art 标记戳);真复用 N15 另做,属驱动与运行缓存类,不随 0.0.62 | tests/phase-timing-stamps.cx(仅测试,非产品输入) | obj.miss 与 obj.hit 各断言:artifact_hit 0 行、lookup 0 行、inference 恰 7 行且阶段名依次为 e2 e1 e3 tapecheck prune lower elf、output kind=image 1 行、两次对象 sha 相同;STAMP artifact 两行改报 UNMEASURED 加原因 no-final-artifact-reuse-on-c;批次 rc0 断言总数不少于 62;runexec.cx 的 ex_ri_artifact 死代码留给 N15 处理 | 15 | 已落地(2026-11-14):obj.miss/obj.hit 各 5 条真实行为断言(无 artifact_hit、无 lookup、inference 恰 7 行且顺序 e2 e1 e3 tapecheck prune lower elf、output kind=image 恰 1 行),断言 62 -> 68;STAMP artifact 两行改 UNMEASURED(reason=no-final-artifact-reuse-on-c(N15));affected --ids phase-timing-stamps rc=0 PASS,ratchets ok=13,cx-lint 85 过 |
| N15 | -c 最终对象真复用(rc_open/rc_lookup 对对象模式开键,键绑 -o 路径与目标,命中跳过 e2 至 elf,戳报真实 mode) | src/drive.cx、src/runcache.cx、cx/runtime/runexec.cx(产品输入,补丁先交集成者) | 第二次 -c inference 0 行且对象同字节;改源后未命中;phase-timing-stamps 恢复 artifact_hit 0 然后 1 的断言 | 已拆(2026-11-14 读码,详见 prd_cx.md 5.1 "N15 实现前读码"):N15-a `src/drive.cx` 仅 :545 门加 `mode2==5 && !asmw && !opts.funit` 与 :561 命中用 `shown` 写出(rc_ 键已含 mode/level/argv/源路径与字节,-o 字节无关但已由 argv 入键,可接受;`src/runcache.cx` 首切不改);N15-b `cx/runtime/runexec.cx` :763 与 :1014-1036 死码;N15-c `tests/phase-timing-stamps.cx` 装后恢复断言(不入产品输入);各 15 分钟内,a 与 b 不相交,验收命令见 5.1 | 0.0.63(63A),与 L1 同类 |
| B2 证据 | (1) 内建只有号加五参:exec/facts/gen2-literals.tsv:13 `["__syscall6","syscall",6]`,exec/facts/k2-call.tsv:8 `sys25 kind=sys6 op=syscall`,tools/opinfo-cases-gen.cx:61 `.sys6 syscall, r1, r3, r4, r5, r2, r0` 共六个操作数;cx/sys/netraw.cx:41 `if (f) return -ENOSYS`。(2) 产品头早已同法绕过:include/sys/socket.h:136 注明 gate 只带五个,sendto/recvfrom 经 sendmsg/recvmsg。(3) 本机实跑探针(lnx/x86_64,已装产品,`sh ./unisacc.com /tmp/b2probe/p.cx`,rc=0):`sendmsg=4`、`recvmsg=4 namelen=16 from_family=2 from_port_eq=1 data=ping`。未验证:arm64 上的 msghdr 布局(按 LP64 同为 56 字节,未实跑);`.sys6` 各目标的寄存器分配与溢出区未读到编码行。(4) 不选扩内建:加七操作数形态要动 gen2-literals 的 intrinsics、k2-gen2 的 syscalls、k2-call 的 sysrows 新 kind、lower-code/lower-abi 六目标与 tools/facts-gen.cx,属模型与网类,须重建 kernel 与两代 GEN,和 0.0.62 的库包与运行库类不同类,不得同候选;回滚办法:本方案全是新增文件加两行号表,撤对应提交即回现状 | - | - | - | - |
| B3 | SPEED-11 运行库扫描族(T8) | 运行库对应源(产品输入)加其 check | 补丁先交集成者;字节等价回归输出不变 | 15 | 候选 |
| B4 | 0.0.62 候选整链 real6 | 私有构造根 | `tools/candidate-chain.cx -- --rev REV --out 全新目录 --parallel 12 --cache-manager-osx X64 ARM64` -> RELEASE_APE PACKED,墙钟不高于 3538 s,超 5400 s 即停 | 后台有界 | 链 |
| B5 | 需新产品的断言转真 | tests/cx-namespace.cx、tests/module/units.cx 的 UNVERIFIED 标注 | `sh ./unisacc.com tests/cx-namespace.cx -- ROOT ROOT/unisacc.com OUT` failed=0;`osx-sentinel -funit` 实编 rc=0 | 15 | B4 后 |

,2026-11-14;按"分而治之":工作流按耦合边界切,每版只含一类产品输入大改)

版本里程碑:
- 0.0.62(库包与运行库类):封网清零 new 行(netraw 六参改为新增 cx/sys/netmsg.cx 走 sendmsg/recvmsg 三参,不扩 `__syscall6` 内建,不改冻结的 netraw.cx,仍属库包与运行库类,依据见任务表 B2 证据行;UDP/unix/resolve、带截止时间 TCP);SPEED-11 运行库扫描族;别名运行失败修复(cxmod 三源,按缺陷修复并入);分治工具线并行(C1 相清单化、C3 批次纯函数化、C4 接口哈希选批,不属产品输入);平台审计表扩面加有名 ENOSYS 加 CI 证据。
- 0.0.63(驱动与运行缓存类,2026-11-14 按 A12 重排,详见下方 0.0.63 条目):主杠杆是运行库闭包结果复用(上界约 379 ms,成因待 M1 取数证实),零解码只剩约 49 ms 降为次要;speed12b 类重建 e2 网不并入;目标 first_e2e 500 ms 以下,M1 否定则改口径,不放宽。
- 0.0.64(cx 设计类):export、通配、cx.stream/tlib 重构、紧凑前导。0.0.65 候选:osx/win netraw 真实现。
已定三项取舍(主调度按推荐定):(1) 0.0.63 唯一大改类为驱动与运行缓存(原定模型装载零解码,A12 实测后改),cx.stream/tlib 重构与 export/通配挪到 0.0.64;(2) speed12b 不并入 0.0.62;(3) 平台策略:事实表扩面加有名拒绝先行,不做 NT 上的 fork 仿真,osx/win netraw 真实现推到 0.0.65。
第一周任务状态(2026-11-14 实测,已落 main 的才算数):T5 别名运行修复已入库(4aee1dfb4a,三源,断言需新产品验证)、cxmod 模块身份规范路径判同已入库(e576000457,cxm_bind_real;cxm_deps_same 三源对齐在做)、T9 libc 声明清点(build/libc-decls.tsv 1748 条,in_libc99posix=no 从 97.7% 降到约 85%)与 T11 ENOSYS 点清点(口径修正后 70 个点,无拒绝原因 41 个,全在冻结的 cx/sys,须走事实表集中拒绝原因,不逐点补注释)已入库、T10 libc99posix.tsv 30 -> 129 行(stdio 41、string 20、stdlib 38,全 cx-own)已入库、ext 下 34 个 ENOSYS 点补了原因(b0a44e212d)、C1 链相度量(phase-graph)与 C4 接口哈希选批(--iface-narrow)与变异测试补断言已入库。待做:T1 速度基线补跑(整链结束后安静机器)、T2 命中路定因、T3 g2/g3 并发整链已实测(real5,3538 s,见下一动作第 3 条)、T6/T7 封网、T8 SPEED-11、T13 git 查询去重、T14 text 拆面、T15 包索引、T16 win 批次登记。
工具层与反应器(fable 推演,2026-11-14,已定顺序):(1) tools/derive.cx 加 exec/facts/derive.tsv(事实表生成器 -> layer-api -> iface-hash -> batch-packages-gen -> tools-compile --register -> product-map -> tool-index 的依赖序,--check 只读汇总,--write 失败还原),已派;(2) tools/status.cx --fast(状态摘要,10 秒内,主调度每轮只读它),已派;(3) 之后:tools/ask.cx 用量闸(build/ask-budget.tsv,防递归环境标记、去重、断路器、16KB 上限、审计行,只接桩,不接真实后端)与 tools/triage.cx(先 exec/facts/red-fingerprints.tsv 确定性命中,未命中才升级,模型输出只作 SUGGESTION);黑名单:产品输入提交、判决规则、授权与预算 raise、断路器复位、任何写仓内文件的动作不得调用模型。(4) 运行缓存:缓存根与 TMPDIR 解耦(UNISACC_CACHE_DIR)与路径无关键、头文件私有目录(产品输入,0.0.63);热进程价值低(命中约 110 ms)暂不建。(5) 变异测试工具化 tools/mutate.cx(表驱动,先做 affected/precommit/sessionroot/check-archive-refs 四点)。
平台现状事实(2026-11-14 实测):tests/batches.tsv 平台列 lnx/x86_64 1046 行、osx/arm64 36 行、win 0 行;exec/facts/libc99posix.tsv 只有 30 行而 include/ 约 45 个头;cosmocc 参考:errno 等常量走链接期符号、运行库可按平台裁剪(-msysv、-mtinylinux)、跨平台产物字节确定(APE 格式细节与 NT 层未取到原文,UNVERIFIED)。

预见的瓶颈与风险(2026-11-14 前瞻,均未发生;每条给证据入口与提前动作):

| 号 | 很可能出现的问题 | 证据入口 | 提前动作 |
|---|---|---|---|
| F1 | 返工即破 2 小时:整链 3538 s,0.0.62 候选并入三条产品输入线(封网、SPEED-11、cxmod 三源),任一条红就重跑整链,两次即超 | real5.start/end;分治时限铁律 | A4 前置门、A6 三源门、`tools/chain-precheck.cx` 进冻结前必跑;候选冻结前三线各自探针绿 |
| F2 | 打包单点:整链跑完才因缺 osx cache-manager 被拒,且借用副本只在 ~/.cache 一处,丢了就发不了版 | real5.rc=2、real5.out 末行 REFUSE;起整链配方第 4 步 | A4 把拒绝前移到第 1 秒;A5 定因后自产(R4) |
| F3 | 三源漂移放大:手工对齐已漏过一轮,0.0.64 的 export 与通配每条新拒绝文案都要三处同现 | 4367e1b0e5;prd_cx.md 10.3、11.6(`cxm_checkcalls` 只在产品源)、E13 | A6 现在就建门,0.0.64 开工前必须为绿或具名 GAP |
| F4 | 装新产品时红批集中爆发:别名、模块身份、CX-U-2 三拒绝、约 80 个 FTRIM 批、63 个 DEFERRED 批都压在"需新产品" | 阻塞节前两条;prd_cx.md 第 10 节开头 | A2 今天在已装的 0.0.61 发布件上先消一批;B5 清单化,B4 后一次并行跑 |
| F5 | osx/win 无基线:win 批次 0 行、osx 36 行,0.0.65 做 netraw 真实现时没有可比的"之前" | 平台现状事实行;prd_cx_spec.md 末节 UNVERIFIED 汇总 | A9 先登记批次(DEFERRED);CI 平台作业结果回收成表,ENOSYS 拒绝按表断言 |
| F6 | 缓存根耦合 TMPDIR:并发子代理各用全新 TMPDIR 即全冷,源身份首测 5.8 s 在并行下越 8 s 硬限,判定依赖机器安静;解耦排在 0.0.63,而 0.0.62 并发更多 | 起整链配方第 5 步;工具层与反应器第 (4) 项 | A4 强制 UNISACC_IDENT_HELPER;子代理简报统一共享只读缓存根;UNISACC_CACHE_DIR 不早于 0.0.63 候选 |
| F7 | 波及面拖慢快车道:改 cx/text/text.cx 仍选 765 批(窄化前 903/1069),SPEED-11 若动扫描族则每次改动近全量 | 提交 1420444e98 实测;prd_cx.md E6、E7 | A7a、A7b 符号级选批先于 B3 落地 |
| F8 | 速度门假红假绿:冷编译三项目标都以单样本为基线 | build/speed-baseline.tsv 末三行(n=1) | A1b 补到 n>=3 后再把 12300、11600 当门 |
| F9 | 0.0.64 一版两候选(cxmod 三源与运行库头不同类)即两次整链,约 2 小时只花在构造上 | prd_cx.md 10.3"一版一类" | 门:整链降到 2400 s 以内才允许同版两候选,否则 P4 物理拆面顺延 0.0.65 |
| F10 | 通配让闭包依赖目录列表,新增一个文件就改变全部通配使用方的计划字节 | prd_cx.md 10.2 P2"代价如实" | 产品层禁用规则先进 cxapis 类棘轮,早于 P2 实现 |
| F11 | 新链相在链尾才第一次实跑:cache-manager 是第 497 相,失败要到约 3500 s 才知道;以后每加一个尾部相都重演 | build/chain-phases.tsv 第 500 行;阻塞节整链一条"候选 entry.elf 能否 -b osx 未实跑" | 首链失败走 `--resume` 续段;立规:新相入链前须用上一条链留存的 entry.elf 单独预演一次并留证,链目录清扫时保留最近一条的 ctrl/ |
| F12 | 源级绿不等于产品绿:三源差异表仍有 14 行语义分叉,产品带手写 cxmod.cx,测试走 plan.cx;CXM_MAXU 96 对 128 | build/cx0-three-source-diff.tsv(181 行);4a94e2b141 | 起链前 check-cxmod-tri 必跑;装新产品后 B5 第一批就跑 cx-namespace 与 module-units;D0 的 T6 至 T8 提到 0.0.64 首位 |
| F13 | 共享检出上全集复验得不出判定:wt gate 在波内遇他人改动即整波 UNKNOWN,而提交速率约每小时 50 次;装新产品后的红批清单(F4)只能来自快照根,现为手工五步 | 阻塞节红批复验与快照根复验两条 | 把快照根会话做成一条命令的工具(每波 8 批、慢批单列),在 real6 跑的 59 分钟内造好,B5 直接用 |
| F14 | 冻结点落在派生物过期的提交上:derive 派生物今天三次事后重生(028e7da9d5、3fc367fef1、eea4d03204),另有两条说明为"x"的提交含产品输入(f0030a16a8 含 netmsg.cx),发布说明按提交说明取材会漏项 | `git log --oneline v0.0.61..HEAD` | 冻结 REV 前跑 `tools/derive.cx -- --check`(已列前置条件 2);发布说明的变更清单改由 `git diff --name-status` 生成而不是读提交说明 |
| F15 | lnx/arm64 的 netmsg 只有 CI 能证:msghdr 布局按 LP64 推为 56 字节未实跑,`.sys6` 寄存器编码未读;红了要到发布后的 CI 才知道 | 任务表 B2 证据行"未验证" | 起链前确认 check-cx-sys-netmsg、check-ext-net-udp 批次在 CI 的 lnx/arm64 作业清单内;不在则先登记,不记 PASS |
| F16 | 冻结提交与被构造 REV 脱节:real6 起链后 main 继续进工具与测试提交,16:44 `git diff --stat b2c557b1ba origin/main -- src cx ext exec tools tests build` 已是 18 个文件加 897 行;发布步骤步 2 原文取 origin/main 的树,会使 F 不等于被构造的源,且 CI 以 commit=X 的工具验候选件,工具与产品不同提交 | 上述命令;发布步骤节步 2 | 步 2 改取 REV 的树;步 7 前核对 CI 用到的 tools 与 tests 在 REV 到 X 之间的差异并写进发布说明;长期:冻结到公开期间工具提交照常,但 F 只认 REV |
| F17 | 多人同改的 md 无按行提交手段:`git commit -- prd_mempalace.md` 会带走他人未完成的行(本轮开工时该文件第 63 行有他人暂存改动,引用尚未入库的 tools/s9-verdict.cx);commit-own --keyed 只管表 | 本轮开工时 `git status --short` 十个他人暂存路径 | 提交前必看 `git diff HEAD -- 文件`;含他人行时用临时 index(ls-tree 加 update-index --index-info)加 commit-tree 与 update-ref 只带自己的行,或给 commit-own 加 md 行段模式(0.0.63 工具项) |
| F18 | 速度基线口径随 e3cache 目录状态漂移:目录现 1938 项顶到 1 GiB 预算,每次首次运行约多 54 ms;大扫除清空后变成空目录冷态(发布 8 次,590 ms 档);0.0.61 基线未记条目数,0.0.62 补测与 0.0.63 的 E3TRIM 收益都不可比 | 本节 E3TRIM 条实测;build/speed-baseline.tsv 无条目数列 | 取数前用专用 UNISACC_E3CACHE 目录并记条目数;基线行加条目数与 load1 两列;顺序固定为先测速再大扫除 |

0.0.63 起手(fable 第三轮,2026-11-14 16:45;证据入口 prd_cx.md 5.1、5.2 与本节 E3TRIM 条):
- real6 结束前可开工(不动产品输入、不取耗时、不跑重测试,写域互不相交,各 15 分钟内):N4(E4 闭包切法,写 prd_cx.md 5.1 的 E4 行);N12(CI 结果回收成表,新工具加 check);N14 登记;nn 的 X3、X4、X5、X7(只读,写 prd_cx.md 5.2 对应行);N15 拆分(只读读 src/drive.cx、src/runcache.cx 的 rc_open/rc_lookup 与 cx/runtime/runexec.cx 的 ex_ri_artifact,把"需拆"改成 15 分钟项,写任务表 N15 行);C3-1 派工行修正(写域加 src/driveunits.cx);E3TRIM 补丁复核(读 $CLAUDE_JOB_DIR/tmp/e3trim.patch.txt 对 modelcache.cx:239 至 301,只出评审意见);D0 的 T1 至 T4。取耗时的(N1、N2 严格分段、N5 实跑、X1、X2、X6)等整链结束且速度基线补测之后。
- 类别与候选组合(决定):L1 方案 A、L2 路径无关键(C2b)、E3TRIM、N15、零解码(C3z)、C2a 全部落在 src/drive.cx、src/driveroute.cx、src/driveunits.cx、src/runcache.cx、cx/runtime/modelcache.cx、unitcache.cx、runexec.cx、routestep.cx,同属驱动与运行缓存类,规则上可同候选。分两批入候选:63A = E3TRIM 修法 (1)(3)、C2a、N7(C3-1)、L1 方案 A、N15,都是结果不变的加速器,验收统一为开缓存与 UNISACC_RUN_CACHE=0 产物同字节;63B = C2b 与 C3z,C2b 改键语义(N3 已查出相对 include 的头字节不在键内,错误命中就是正确性缺陷),C3z 上界只有约 49 ms。门:63B 只有在 63A 冻结前自身探针已绿才并入同一候选;否则作同类第二候选,且须整链不高于 2400 s(F9 的门,现 3538 s,不满足),不满足即顺延 0.0.64 之前的补丁版,不为它在一版内跑两次整链。同写域串行:modelcache.cx 为 E3TRIM -> C2a -> C3z;runcache.cx 为 N15 -> C2b;driveroute 与 unitcache 归 L1。
- 500 ms 门的拼算(推算,非实测):671 减方案 A 约 116 得约 555,再减 E3TRIM 约 54(随目录条目数变,见 F18)约 500,贴线;63A 单独不保证过门,仍须 L5(a) 或 lower 逐函数缓存,门不放宽。

0.0.63 与 0.0.64 任务(都依赖 0.0.62 新产品已装;开工前逐项拆到 15 分钟):
- 0.0.63(2026-11-14 按 A12 重排;依据与每个杠杆的证据入口在 prd_cx.md 第 5 节 L1 至 L5):零解码上界只有约 49 ms,不再承载 500 ms 门;主杠杆改为运行库闭包结果复用(L1,上界约 379 ms,n=1 且成因为推断)。类别:L1、L2、L4 都落在驱动与运行缓存(`src/runcache.cx`、`src/driveroute.cx`、`cx/runtime/unitcache.cx`、`cx/runtime/modelcache.cx`),与 0.0.62 的库包与运行库类不同类,也不是原定的模型与网类;speed12b 重建 e2 网与跳过 prune 属模型与网类,不得与之同候选。本条与进化路线"0.0.63 唯一大改类为模型装载零解码"冲突,待主调度改那一行。回滚:各项都是纯加速器,撤对应提交即回现状;UNISACC_RUN_CACHE=0 可随时关闭。
  - 今天可只读取数(不改源,互不相交,各 15 分钟内,结果原位写入 prd_cx.md 第 5 节对应行;新源须每次换内容,否则命中运行缓存):
    - 状态(2026-11-14 取数,load1 10.1 至 12.0,非安静机,绝对值偏高;M1 以每次换源内容注释而非换 TMPDIR 避免整程序命中,换 TMPDIR 会令 modelcache 全 miss 污染 model 相;M1 与 M3 已取,M2 部分,M4 部分,M5 未取):M1 部分成立,表与结论见 prd_cx.md L1 行;M2 命中组 model 相 50 至 54 ms、关缓存组 106 至 193 ms(n=2),读入/校验/建布局三分未取;M3 n=3 全为 0 行;M4 同内容两目录 lookup_hit=0,E3CACHE 一组未取;M5 未取(缺 strace 分摊时间)。
    - M1 推理随规模的成因:`UNISACC_RR_TIME=1 sh ./unisacc.com 新源.cx 2>&1 | grep inference`,对 (a) `return 7`、(b) 基准一行 printf、(c) 调一个非 printf 运行库函数,各 n=3 取中位。预期:(a) 合计约 150 ms、(b) 约 530 ms;(c) 若接近 (b) 则 L1 的闭包推断成立,接近 (a) 则推断不成立、L1 须重定。
    - M2 model 相 49 ms 的构成:同命令 `grep -e modelcache -e 'RR_TIME.model'`,对照加 `UNISACC_RUN_CACHE=0` 的一组,n=3。预期:命中组六行 result=hit 且合计约 49 ms;关缓存组更大,差即真 inflate;读入、校验、建布局三分仍 UNMEASURED 则在 `build/pkg-gaps.tsv` 记缺口。
    - M3 单单元路有无单元缓存:`UNISACC_RR_TIME=1 sh ./unisacc.com 新源.cx 2>&1 | grep -c unitcache`。本次 n=1 观察为 0 行;预期 n=3 仍为 0,即单单元路未接单元缓存,L1 须新接。
    - M4 路径无关命中率:同一内容放两个目录各跑一次,读第二次的 `RR_TIME lookup_hit=`。预期现状为 0(键含 cwd 与路径);再设 `UNISACC_E3CACHE=同一目录` 换 TMPDIR 跑,预期 lookup_hit=2(根已可解耦)。
    - M5 残差约 58 ms 细分:`strace -c -f` 与 `strace -r` 各一次首次运行(只读系统工具),按 exec、读产品件、写缓存、子进程分摊;预期给出四项毫秒数,合计与 58 ms 相差 20% 以内,否则仍记 UNMEASURED。
  - 实现项(都动产品输入,等 0.0.62 新产品已装;写域互不相交,各项开工前再拆到 15 分钟):
    - C1 运行库闭包结果复用设计稿,依赖 M1、M3。写域:prd_cx.md 第 5 节 L1 行。验收:行内写明键的全部成分、失效条件、命中与未命中逐字节相同的检查命令。状态(2026-11-14):设计稿已写入 prd_cx.md 5.1 小节,只读读码,未跑任何新实验。要点:(a) 后四流整体按单元拆分再拼接不成立,只有 e3 有可拆先例(`cx/compiler/fwdmerge.cx:233`),prune 与 elf 需整程序,lower 未验证;(b) 方案 A 只复用闭包 e3 带子,约省 116 ms,671 到约 555 ms,单做过不了 500 门;方案 B(闭包 prune 加 lower 结果,键加引用面)再省约 177 ms,取决于只读实验 E2、E3;(c) 键 UNICL1 不含路径,不依赖 C2b;(d) C3 拆为 C3-1 至 C3-5,其中 C3-1 写域越出本树原列的两个文件(需 `src/driveunits.cx` 与入口选择),待主调度改派工行;(e) 未验证项:闭包进入后四流的形态、rc_hex_ident 是否覆盖后四流模型、带子落盘取数入口,均在 5.1 给了取数办法。E1 至 E4 进展(2026-11-14):E1 已测(闭包是同一 .unit 0 内按头文件来源标记交错的文本,非独立段;非用户行占比 97% 至 99.6%);E2、E3 已测(2026-11-14):E2 stream prune 对 -S 带子逐字节无变化且未用函数在 prune/lower 后仍在(标号数 404 对 404),可观察的后四流无函数级可达性裁剪;E3 按标号切 lower 输出比 (c)(d),共有 398 段中 385 同字节(96.7%,按字节 95.4%),差异在启动收尾桩、main、生成标号,@sym 地址只在头部;E4 未测(闭包切法未定)。方案 B 判断:前提 prune 按引用面裁剪不成立(prune 实际不裁),lower 逐函数基本可拆,故方案 B 不以"闭包 prune 结果"成立,可缩为"闭包 lower 逐函数结果缓存"(上界不超 lower 的闭包份,约 (b)到(c) 的 80 ms 量级,未测),拼接需处理头部与生成标号,未决但不再偏否定 prune 部分;500 门仍须 A 加 L5(a)。细节在 prd_cx.md 5.1 分期。
    - C2a 缓存根默认值与 `UNISACC_CACHE_DIR`。写域:`cx/runtime/modelcache.cx` 的 `mc_cache_dir`。验收:两个不同 TMPDIR、不设任何缓存变量,同源第二次运行 `RR_TIME lookup_hit=2`;`tools/check-unit-frontcache.cx` failed=0。
    - C2b 路径无关键,依赖 M4 与 `__FILE__`、诊断、相对 include 的取证。写域:`src/runcache.cx` 的 `rc_hex_sources`。验收:同内容两个目录,第二次 lookup_hit=2 且两次标准输出、标准错误、退出码相同;含 `__FILE__` 的夹具必须仍未命中或输出正确路径。
    - C3 闭包结果复用实现,依赖 C1。写域:`src/driveroute.cx`、`cx/runtime/unitcache.cx`。验收:`sh ./unisacc.com tools/run-startup-bench.cx` first_e2e 三次中位 500 ms 以下且断言 failed=0;关缓存(UNISACC_RUN_CACHE=0)与开缓存的产物字节相同。若 M1 否定推断,此门改口径并在此写明,不放宽不删。
    - C3z 零解码 mmap 布局(原 C3,降为次要),与 C2a 同写 `cx/runtime/modelcache.cx` 故串行在其后;另写 `cx/runtime/routestep.cx`。验收:RR_TIME 六流 model 相合计 10 ms 以下(现约 49 ms),命中与未命中同字节。
    - C4 整链对象相拆分到整链 2400 s 以内(F9 的门),不变。
- 0.0.64:D0 cxmod 三源归一(2026-11-14 决定:先归一再加语法,依据与拆分见 prd_cx.md 10.5;`cx/compiler/cxmod.cx` 改为 `cx/cx0/plan.cx` 的第二投影,验收 `tools/cx0-extract.cx -- --check --all` 两行 EXACT、`tools/check-cxmod-tri.cx` 末行 gaps=0;其中 T1 至 T4 不动产品输入、今天可开工且互不相交,T5 至 T8 动产品输入、都写 plan.cx、串行、等 0.0.62 新产品已装;回滚:T8 之前产品件仍是手写件,撤 T5 至 T7 的提交即回到现状)-> D1 export(依赖 D0:此后每条新拒绝只改 plan.cx 一处;若 D0 的 T8 未落地则 D1 不开工,不退回手工三处对齐;验收 `tests/cx-namespace.cx` 门面正例 2、拒绝 4,`tools/cx0-extract.cx -- --check` EXACT)-> D2 通配(依赖 D1;正例 1、拒绝 5)-> D3 符号级导入收敛(依赖 D0,与 D1、D2 同写 plan.cx 故串行;`tests/module/check.cx`)-> D4 text.text 物理拆面(依赖 D1 的 export;第二候选,受 F9 门约束)-> D5 层感知前导(不依赖 D0 至 D3;末位,超 2 小时移 0.0.65)。D0 至 D3 同属 cxmod 三源一类、同候选;同写域串行,不并发。D4 与 D5 的 cx.stream/tlib 部分已细化为 R0 至 R4(设计与证据见 prd_cx.md 10.6):R2(cx/stream 下层修正,库包源类)与 D4 同类同候选;R3(`st_span` 模块接口,驱动类)移 0.0.65;R4(接口打印覆盖面与包根门面,cxmod 三源类)排 D1 至 D3 之后。
- D1 export 细化(2026-11-14 设计,未实现,不得记 PASS;全文、拒绝文案 X1 至 X13、子任务 E0 至 E10 在 prd_cx.md 10.7):决定 (1) 裸名 `export NAME[,NAME...]` 只表示本模块自有函数的公开名单(D1a),门面重导出改写成 `export use MODULE[::sym]`(D1b,排 D1a 之后;与 prd_cx_spec.md 3.5 现文不一致,由 E0 改写 3.5);(2) 有 export 的单元以 export 为准并与 cxpack.tsv 同文件行互校,无 export 的单元行为不变,cxpack 第三列终态改为生成物;(3) 类型首版仍无条件公开,export 只是规划期可见性,不是链接期隔离。依赖:E4 至 E6、E10 写 plan.cx,等 D0 的 T6、T7、T8;试点 E8(cx/stream/rw.cx,17 名)是库包源类,不与 cxmod 三源类同候选。今天可派:E0(spec 3.5)、E1(只读,定 checkcalls 对未公开函数的归属,现 UNVERIFIED)、E2(tests/export 夹具),E3(check-export,旧产品上记 UNVERIFIED needs-new-product)接 E2。回滚:本条只是文档,撤本提交即可。
- cx.stream/tlib 重构任务(2026-11-14 设计,全部未开工,不得记 PASS;证据入口 prd_cx.md 10.6):R0 状态 2026-11-14 实跑:R0-1/2/4/5 完成,R0-3/6/7 部分(R0-3 只查字面词 kernel、R0-6 check 通过但 18 包未逐一、R0-7 只复现 5 文件基线),定值 K3(sha 有意,跨目标同值)、K5(deadline 产品零使用方),明细见 10.6 "R0 实测";R0 七项只读取数(R0-1 至 R0-7,今天可派,互不相交,各写 `$CLAUDE_JOB_DIR/tmp/R0-N.tsv`,验收命令与预期值在 10.6 第 (4) 段表内)-> R1 tlibpkg 分面与包表登记 stream 两面(纯工具与表,不等版本,依赖 R0-5)-> R2 -> R3 -> R4。已测基线:改 `cx/stream/pipe.cx` 选 126 批、`rw.cx` 选 1 批、`tools/cxlib/tlibpkg.cx` 152 批、`cx/compiler/fwdmerge.cx` 123 批(scanned=1079,HEAD 4966abbf9c)。未验证:任务书所举"tlib 包版本与 kernel 事实表耦合"未找到文件行证据(仅 `tools/cxlib/tlibpkg.cx:86` 取 kernel 哈希),由 R0-3 定论;句柄合同四项 UNVERIFIED 由 R0-6 定值。回滚:本条只是文档,撤本提交即可。
- 0.0.64 之后的优化候选(未排版本):prune 有 callr 时整带回退。实测凡带 stdio 闭包的程序 prune 恒等(`exit` 里一处 `callr`),用户死函数不裁,约每个小死函数多 1.1 ms、82 B(200 个:后三流多约 228 ms,ELF 36,894 对 53,278 B);对 hello 本身只差一个函数。属有意保守合同(`tools/prune-cases-gen.cx:111` 的 indirect-call-preserved),不是缺陷。改法是以取址集合为根继续裁剪,类别为模型与网类,不与驱动与运行缓存类同候选;不删现有断言。证据入口 prd_cx.md 5.1 "E2 定因"。回滚:未动产品,无需回滚。
- nn 下一步(2026-11-14 设计,未实现;全文与候选表 K1 至 K9、实验 X1 至 X7 在 prd_cx.md 5.2):决定 (1) e3 与 lower 的库份走 5.1 的预制与缓存,不动网;网侧只投 prune 与 e2,因为预制落地后基准程序剩余推理约 345 ms(拼算)里 prune 102 加 e2 146 占 72%;(2) prune 先做网内提前回退(K1 甲,输出不变,上界 102 ms,实得待 X2),取址根裁剪(K2)排其后;驱动里先扫 callr 再跳过 prune 的做法违反"差异写成模型步",否决;elf 换回逻辑(上界 47 ms)与学习意义上的量化蒸馏不做;(3) 0.0.64 nn 侧只取数与设计不进产品输入,0.0.65 的 nn 候选只含 prune 一类(模型与网类),不与 netraw 同候选,整链未降到 2400 s 以内则顺延 0.0.66;改网候选发布时须重生成预制清单。依据:A12 分相、M1、E1 至 E3;`RR_TIME` inference 行无步数字段(`cx/runtime/routetime.cx:90`),缩网收益目前只有上界。今可开工:X3、X4、X5、X7(不取耗时);X1、X2、X6 为速度取数,等整链结束。回滚:未动产品,无需回滚。
- 0.0.63 优化候选 E3TRIM(2026-11-14 定因,N1 的 3881 次探测已查清):扫描者是 `cx/runtime/modelcache.cx:239` 的 mc_trim(预算修剪,默认 1 GiB,`UNISACC_E3CACHE_MAX_BYTES` 可调,:72),由 :301 的 mc_cache_publish 在每次成功发布后无条件调用;每个合名条目做 lstat(:274) 加 mc_complete 的 open/fstat/read 8 字节/close(:163-173)。触发条件是"每次发布",不是超阈值:新源首次运行发布 2 次(`src/runcache.cx:405`,E3 结果与 `.host`),空目录冷态发布 8 次(另 6 个 mc-);重复运行不发布,不扫。实测(xfs,load1 4.6 至 5.1,n=3 中位,假 uc- 项):条目 0/2000/4000/16000 时首次 515.6/573.9/649.9/962.9 ms,重复 8.1/8.2/8.2/8.8 ms,斜率约 28 微秒每条目每次首次运行(每次扫描约 14);冷态 590/779/1048/2316 ms。现目录 1938 项、1.1G(已顶到预算,条目数由预算除以均值约 550 KB 决定,属今日重负载下的稳态而非失控堆积),折合每次首次运行约 54 ms,与 M5 残差约 58 ms 相当。判定:预算强制是有意设计(:236-238 注明 O(N)),实现成每次发布全量扫描是可优化点,不是正确性缺陷;但只限字节不限条数,小条目多时条数无界。修法按收益排:(1) 一个进程只修剪一次,放在最后一次发布之后;(2) 降频,目录内记总量账或按写入累计量触发,未近上限不扫;(3) 扫描只 lstat,不逐项 open/read(校验留给命中路径与真正要删的候选);(4) 加条数上限。类别:驱动与运行缓存类(同 L1/L2/L4 的 modelcache.cx),随 0.0.63,不进 0.0.62 的库包与运行库类。未做:未改源码,未测修后数值,未在真实大条目目录上复测;该缓存目录未清理,清理归发布后大扫除。证据入口 prd_cx.md 第 5 节首次与重复之差分相行 E3TRIM 附注。回滚:未动产品,无需回滚。补丁草稿已备(两遍扫描:第一遍只 lstat 求大小上界,不超预算即跳过 mc_complete 的 open/read,超预算再按原样校验淘汰,语义不变),路径 `$CLAUDE_JOB_DIR/tmp/e3trim.patch.txt`;副本内 modelcache-budget lru/concurrent 断言 15/11 failed=0;4000 项改前改后首次运行耗时副本实测无差(约 185-190 ms 对 180-190 ms),因已装产品未用副本源码重建,数字不能证明收益,待 0.0.63 重建后实测。

0.0.63 至 0.0.65 任务树更新(fable 第二轮,2026-11-14 15:50;单项 15 分钟内,写域互不相交;"今"=不动产品输入、已装 0.0.61 即可开工;"装后"=等 0.0.62 新产品已装):

| 号 | 版 | 目标 | 写域 | 验收命令 -> 预期值 | 开工 |
|---|---|---|---|---|---|
| N1 | 63 | M5 残差约 58 ms 细分 | prd_cx.md 第 5 节 M5 行 | `strace -c -f` 与 `strace -r` 各一次首次运行 -> 四项毫秒数,合计与 58 ms 相差 20% 以内,否则记 UNMEASURED | 已跑未闭合(2026-11-14):strace 下四项 13.5/298.9/1.6/9.5 ms 合计约 323,墙钟 954 对无 strace 151,膨胀 6.3 倍,671 ms 冷态未复现;新发现首次运行对 e3cache 约 3881 次逐文件 open/lstat/read,待查;详见 prd_cx.md 首次与重复之差分相行附注;状态 UNMEASURED |
| N2 | 63 | M2 model 相读入、校验、建布局三分 | prd_cx.md 第 5 节 M2 行、build/pkg-gaps.tsv 一行 | `UNISACC_RR_TIME=1` 命中组 n=3 -> 三分各有毫秒数,取不到则 pkg-gaps 记缺口 | 今(部分完成 2026-11-14:strace 间隔估计读入约 2 至 3、校验约 8 至 10、建布局约 38 至 40 ms,load1 6 至 8 noisy,见 prd_cx.md M2 行;严格分段待 modelcache.cx 加打点,pkg-gaps 未记) |
| N3 | 63 | C2b 前置取证:`__FILE__`、诊断路径、相对 include 对路径无关键的影响 | prd_cx.md 第 5 节 L4 行 | 三个夹具各在两个目录运行 -> 列出输出含路径的情形数,给出键须保留路径的条件 | 今(完成 2026-11-14:6 个实验,`__FILE__`/argv[0] 等于命令行路径串故路径串须留键,相对 include 头字节现不在键内为缺口,合同草案见 prd_cx.md L2 行;产物 sha 未取) |
| N4 | 63 | E4 闭包切法取数 | prd_cx.md 5.1 的 E4 行 | 按 5.1 给的取数办法 -> UNMEASURED 改实测或写明缺的落盘入口 | 今 |
| N5 | 63 | speed-probe 加 run 分相(读 UNISACC_RR_TIME 六流行) | tools/speed-probe.cx 及其 check | `tools/speed-probe.cx` 输出含六流 inference 与 model 行;check failed=0;pkg-gaps 对应缺口行销掉 | 今 |
| N6 | 63 | C2a 缓存根默认值与 UNISACC_CACHE_DIR | `cx/runtime/modelcache.cx` 的 mc_cache_dir | 两个 TMPDIR 同源第二次 `RR_TIME lookup_hit=2`;`tools/check-unit-frontcache.cx` failed=0 | 装后 |
| N7 | 63 | C3-1 单文件路接单元缓存入口(写域已修正为 `src/driveunits.cx` 加 `src/drive.cx` :685 入口选择,2026-11-14 读码;与 N15-a 同改 drive.cx,须在其后串行) | `src/driveunits.cx` 与 `src/drive.cx` 入口选择 | `UNISACC_RR_TIME=1` 单文件新源 `grep -c unitcache` 大于 0(现为 0) | 装后,N6 不同写域可并行 |
| N8 | 63 | C4 对象相拆分第一步:定 closure-m*.o 单键 349.8 s 的构成 | build/phase-times.tsv 新增行 | 对 real6 的 checkpoint 用 .cx 求和 -> 单键内编译、推理、落盘三项各有实测行 | real6 后 |
| N9 | 64 | 通配的产品层禁用规则进棘轮(F10 提前动作) | tools/ratchets.cx 的 cxapis 项及其 check | 夹具含 `use m::*` 的产品层文件 -> 棘轮 fail;现仓 ok=13 不变 | 已落地:规则 cx-wildcard-product 在 tests/cxinclude.cx(cxinclude 棘轮;产品层 src/、cx/core、cx/runtime、cx/compiler、cx/driver 出现 use X::* 计违规),另有 --wildcard-file 夹具模式;tools/check-ratchets.cx 新增 4 断言(夹具 fail 且具名、具名导入过、注释内不算、混合只点名违规者)failed=0;ratchets ok=13 fail=0;derive 有 1 项 product-map tsv-drift 属他人在途改动,非本项 |
| N10 | 64 | D0 的 T6:三源差异表 14 行语义分叉逐行归一方案 | prd_cx.md 10.5 的 T6 行 | 14 行各有"上移到 plan.cx"或"宿主片段"的归属,无未归属行 | 今(只读) 已做:实测为 8 行(非 14),全部归属写入 prd_cx.md 10.5 的 T6 归属表(上移 4、宿主片段 3、可删 1);未改源码 |
| N11 | 64 | D0 的 T6 至 T8 实现(同写 plan.cx,串行) | cx/cx0/plan.cx、cx/cx0/projection.tsv | `tools/cx0-extract.cx -- --check --all` 两行 EXACT;`tools/check-cxmod-tri.cx` 末行 gaps=0 | 装后 |
| N12 | 65 | CI 平台作业结果回收成表(F5 提前动作) | tools 下新工具加 check 与批次行(先 `git log --all --oneline -- tools/NAME`) | 对 run 43554326355 出表,行数 == 六目标作业数,每行带结论与作业号 | 今 |
| N13 | 65 | osx/win 有名 ENOSYS 按表断言 | exec/facts/refusals.tsv 的平台列与其 check | 生成器 `--check` OK;41 个无原因点减到 0 或余数具名 | 今(冻结 REV 之后提交,不混入 real6) |
| N14 | 65 | lnx/arm64 netmsg 布局 CI 探针(F15) | tests/batches.tsv 的 arm64 行(commit-own --keyed) | CI lnx/arm64 作业日志含 netmsg 断言 21 条 failed=0;本机状态 DEFERRED | 今登记,CI 后转真 |

速度杠杆下一个可测动作(第二轮,每档一个):(1) 冷启动:N1 的 M5 残差细分,之后才能判断 500 ms 门靠方案 A 加 L5(a) 是否够;(2) 冷编译:现有 n=3 行标 noisy(6bcd7a98fb),在 load1 低于 2 的窗口重跑 `tools/coldcompile.cx` 三文件各 3 轮,极差除以中位低于 10% 才把 12300 与 11600 当门,否则门不生效并写明;(3) 生成代码与分相:N5 让 speed-probe 出 run 分相,出数后入 build/speed-baseline.tsv 的 probe 行,UNMEASURED 才能销。

三个可测速度杠杆(数字只取自 build/speed-baseline.tsv 中四个工具的行):
- (1) 冷启动:run-startup-bench first_e2e 三次 684.051/704.882/714.267 ms(real4 c55533c7,n=3),同工具 repeat_median3 约 27 至 29 ms(0.0.59 与 local-0060 组);首次与重复相差约 650 ms,其中推理约 531 ms、解码约 49 ms(A12 实测,零解码上界只有约 49 ms)。度量:同工具 first_e2e,目标 0.0.62 为 670 以下、0.0.63 为 500 以下。
- (2) 冷编译:coldcompile 的 entry-build 13624、whole-front 12846、ledger-add 6468 ms(real4,各单样本),对 0.0.59 为 -27%、-24%、-32%。杠杆是每单元前导与前端结果复用;度量:同三文件 n>=3 中位。
- (3) 生成代码与分相:check-bench-gencode 与 speed-probe 对 0.0.61 产物 UNMEASURED,先由 A1c 取数再定目标,不预设数字。

## 记忆宫殿(索引)

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 当前协同 | 本文件;agentwork/ 退役 | 本文件 |
| 当前发布 | 已公开 0.0.61(Latest);0.0.62 规划中 | 本文件上节、plans/release-notes-0.0.61.md、prd_release.md |
| 结构总表 | 五层、六片、7 + 1 流、网、执行器、格式、事实表、构造链、测试 | prd.md §3;cx 侧 prd_cx.md |
| 新入口 | src/unisacc.cx 六层;src/core、src/runtime 指向 cx/ | plans/cx-modules.md、prd.md §5 |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | prd.md §7、tests/batches.tsv、tests/coverage.tsv |
| 分层 | 术语层 T、结构层 L0-L4、模块层 cx-L0..3 | prd.md §0、plans/cx-layers.md |
| 棘轮 | 13 项,以 tools/ratchets.cx 末行为准 | tools/ratchets.cx、prd.md §2.2 |
| 事实表 | 事实 -> 生成器(带 --check)-> 产物 | exec/facts/、prd.md §2.4 |
| 构造图与相位 | build/graph.tsv、build/chain-phases.tsv | prd.md §3.7、§2.2 |
| .cx lint 与静态预检 | tools/cx-lint.cx、tools/tsv-lint.cx、tools/chain-precheck.cx | prd.md §2.2 |
| 产品全景表 | 生成块,tools/product-map.cx -- --write 重算 | [prd_productmap.md](prd_productmap.md)(原 prd.md §9.6) |
| 往期发布与历史 | 各版发布说明与历史索引 | prd_release.md、prd_hist.md、plans/release-notes-*.md |
| 归档 | 旧 C/Python 实现、旧 1333 项套件、历史计划与 PRD | prd.md §10、archive/ |
