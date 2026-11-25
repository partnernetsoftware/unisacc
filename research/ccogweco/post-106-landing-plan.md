# 0.0.106 公开后第一批落库计划(2026-11-25 ccogweco)

输入:research/ccogweco/unlanded-inventory.md 的 P1 组。原则:纯文档不单独提交(随下一实质提交或里程碑收尾
一次带上);共享表按 `--keyed` 只带自己行;共享 main 上不用 stash/reset/clean/checkout 动他人路径;
所有仓内写入经 devsvc(f write / f edit),提交经 commit-own 或 devsvc git commit。

## 0 前置与时点

1. release-post 完成、0.0.106 公开、origin/main 含 106 合入提交与 release/evidence/0.0.106/(车树已带的
   path-b-proof、strict、INDEX、ape-receipt 等随合入进 main);确认 `git log --oneline -1` 与 `git status` 干净。
2. 解冻信号由 cc 发;在此之前不推 main。
3. 冻结行由 cc 处理(见第 4 节),本窗口不碰 build/freeze.tsv。

## 1 记忆宫殿串行(同一文件,严格顺序;owner:cc 或 cc 指定集成者)

同一文件三笔不能并行,顺序与命令:

    # 1) agy-h 的失效引用修复
    sh ./unisacc.com tools/devsvc.cx -- f write prd_mempalace.md --from /tmp/refs-fixed.md
    # 2) 16 条决定(v2,基线 a95623fb29;在第 1 步之后重算行号再落)
    sh ./unisacc.com tools/devsvc.cx -- f edit prd_mempalace.md --old-file /tmp/tail-old.txt --new-file /tmp/tail-new.txt
    # 3) prd:43 笔误(161 分钟口径:链止到 X,不是到 F)
    sh ./unisacc.com tools/devsvc.cx -- f edit prd_mempalace.md --old-file /tmp/l43-old.txt --new-file /tmp/l43-new.txt
    # 核对后提交(共享路径,写者只提交自己的两笔语义;由 cc 署名)
    sh ./unisacc.com tools/commit-own.cx -- -m "记忆宫殿:R40/B-t 与 0.0.106 落库、161 分钟口径订正" --whole prd_mempalace.md

- 期望:三笔落完后 `git diff --stat prd_mempalace.md` 只有追加块与第 43 行一处;`tools/doc-links-gen.cx -- --check`
  无 NEW_BROKEN;不重跑棘轮(纯文档)。
- mempalace-decisions-verify.md 同期更新到 16 条,随本提交或下一个实质提交带上。

## 2 研究页与记分表(纯文档,随里程碑收尾一次提交)

- 打包对象:P1-c 全组(u30/e01 两页 v2、dryread、缺口页、按层跑批设计、路线页、0.1.0 草案、w0150/w0191、
  验收页、判定树、y2 备份页、path-c 备页、notes-106-final 留存、seal map、p0 账本、usable-list、unclassified 等);
  skills/unisacc-orch-supervise/team-scoreboard.md 的 0.0.106 行。
- 粒度:一次"0.0.106 里程碑收尾"提交,路径 `research/ccogweco/`、`archive/research-2026-11-25/2026-11-24/`、记分表;
  不逐页提交。
- 期望:`git show --stat` 只含上述路径;不碰 tools/tests。

## 3 工具线第一件:doc-links 纳入棘轮(实质提交,可带自己的文档)

    sh ./unisacc.com tools/doc-links-gen.cx -- --check          # 先确认 result=OK broken=80 baseline=80
    # 改 tools/ratchets.cx 加 doc-links 行(或 chain-ready 项,二选一,见 doclinks-81-investigation 末节)
    sh ./unisacc.com tools/check-ratchets.cx                     # 期望 0 failed
    sh ./unisacc.com tools/affected.cx -- --files tools/ratchets.cx --list-only   # 看波及批
    sh ./unisacc.com tools/register.cx -- --tool tools/check-ratchets.cx         # 若需登记
    sh ./unisacc.com tools/devsvc.cx -- git commit -m "ratchets: 纳入 doc-links NEW_BROKEN/STALE_BASELINE" --whole tools/ratchets.cx --keyed tests/batches.tsv tools/check-ratchets.cx ...

- 前置:先测 doc-links-gen 全扫耗时(未测),确认 19 项棘轮总时长不超提交门窗口;超就先放 chain-ready 项。
- 期望:ratchets 自检 0 failed;新棘轮对当前树为绿(79 基线收敛后);提交只带 tools/ratchets.cx、check、登记行。

## 4 解冻时点与 freeze 行(提交归 cc)

- 0.0.106 行:`0.0.106-FREEZE f23c7b56fd… cc 1795546800 …`。公开后二选一:删除(首选)或留到自然过期;
  删除命令由 cc 执行:
      sh ./unisacc.com tools/devsvc.cx -- f edit build/freeze.tsv --old-file /tmp/freeze-old.txt --new-file /tmp/freeze-empty.txt
      sh ./unisacc.com tools/commit-own.cx -- -m "解冻:删除 0.0.106 冻结行" --whole build/freeze.tsv
- **107 需要新冻结行**:107 起链提交(新车头)落定时写 `0.0.107-FREEZE <新F> cc <deadline> src cx ext exec include seed kernel tests release`,
  在 107 合 origin/main 之后、起链之前写;不沿用 106 的 base。
- 顺序:先删 106 行(或等过期),再在 107 车树写 107 行;两者不能同时在表里指向同一版本号。

## 5 107 同步(106 公开后 107 车树要做的)

- 整体:`cd .claude/worktrees/v0.0.107 && git fetch origin && git merge origin/main`;106 合入 main 的约 30 笔中,
  产品闭包已同源,冲突集中在派生表与登记表:
  - build/iface-hash.tsv、build/links.tsv、build/tlib-packages.tsv:两侧都动,取任一侧后
    `tools/derive.cx -- --write`(links)、`tools/iface-hash.cx -- --write`、`tools/layer-api.cx -- --bump-pending`。
  - tests/batch-product-inputs.tsv:两侧追加行,手写并集后 `tools/prerun-inputs.cx -- --check`。
  - tests/batches.tsv、tests/coverage.tsv:合后查重键与 `tools/tables-gen.cx -- --check`(stale=0 hard=0)。
  - build/freeze.tsv:106 行已删,107 行按第 4 节新写;不要让 106 行残留。
  - 车上六文件(`build/freeze.tsv`、`tests/check-fwdstub-replay.cx`、`tests/env-semantics.tsv`、
    `tools/check-chain-ready-freeze.cx`、`tools/check-gen-all-check.cx`、`tools/check-prepush.cx`)已在 107 预演过,
    合并后复核内容一致(sha 与 106 主线相同),不重复改。
- 107 追加集(cc 记十二份)在预跑完成后、106 公开前逐笔落车;本计划不重复其步骤。

## 6 期望提交粒度汇总

| 步 | 提交 | 路径 | 说明 |
|---|---|---|---|
| 1 | 记忆宫殿三笔后一次提交 | prd_mempalace.md | 共享文件,cc 署名 |
| 2 | 里程碑收尾一次 | research/ccogweco/、archive/research-2026-11-25/2026-11-24/、skills/…/team-scoreboard.md | 纯文档合批 |
| 3 | doc-links 棘轮一件 | tools/ratchets.cx、check、tests/batches.tsv(键) | 实质提交,带自己的研究页 |
| 4 | 解冻/107 冻结行 | build/freeze.tsv | cc 执行 |
| 5 | 107 合并后派生表 | 派生与登记表 | 在 107 车树,不推共享 main |

## 7 回滚

- 文档提交:按提交回退对应路径;记忆宫殿回退到合入前 sha。
- 棘轮:删新行并回退 check 变更;doc-links 基线不得放宽。
- 冻结行:删错就恢复原行,不删除后重写;107 行写错按原 deadline 重置并在记忆宫殿留一行。
