# 子代理简报模板

复制本文件到简报正文,填空后先跑 `sh ./unisacc.com tools/brief-lint.cx -- 简报文件`,末行 `BRIEF_LINT missing=0 reject=0` 才派工。口径只引用 `tests/accept-profiles.tsv` 的 profile 名,不抄数字。

## 目标

一句话写交付物与成功判据(例:新工具 tools/X.cx 加检查,批次在全新布局冷缓存下实跑通过)。

## 写域(只许改这些路径)

- 路径 1
- 路径 2

写域外一律不改;共享表只经 devsvc 写,只提交自己的行。

## 验收命令(含预期值)

- 本单专属:`命令` -> 预期末行 `...`
- 交付自检:`sh ./unisacc.com tools/handoff-check.cx -- --base 开工提交` -> 末行 `HANDOFF ... failed=0 ... scope=LOCAL`;REVIEW_REQUIRED 交主调度派独立复核。

验收口径块(勾选适用项,删去不适用项):

- [ ] local-evidence-only:自检只算本地证据,不写发布资格结论
- [ ] newcx-0:改过的 .cx 零违规
- [ ] ratchets-15-0:十五项棘轮全过(含 dup-census、check-literal、srcshape)
- [ ] shards-consistent:新工具登记后 tools-compile 分片数与批次行一致
- [ ] tool-registered 与 new-tool-name-history:新工具先 git log --all 查名,带 check、登记、prd.md 能力速查行
- [ ] cold-fresh-layout-33s 与 fresh-layout-no-history:新批次在全新布局冷缓存口径 UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0 下实跑
- [ ] no-gate-bypass:门被挡停下上报
- [ ] patch-first-for-product:产品输入变更补丁先行,先出补丁给集成者核
- [ ] core-path-review:核心路径(devsvc 核心、提交门、锁、选批)提交须独立复核
- [ ] e2e-latency:提速声明按用户实际调用的命令行端到端时延验收
- [ ] thaw-on-fail:冻结后出 FAIL 或 UNKNOWN 即解冻退回候选点
- [ ] shared-registry-serial:同一时间只允许一个子代理改共享登记表(tests/batches.tsv、coverage.tsv、tools-compile-sources.tsv、batch-product-inputs.tsv);其余写自己的补丁行并由指定提交者串行合并;不要跑 register.cx(回滚会整表覆盖他人新增行,缺口 REGISTER-ROLLBACK-CLOBBERS-CONCURRENT),用 devsvc f append 单行追加并用 run-batch 验证;提交前 `git diff HEAD --stat` 核三张表只有自己的行

## 截止

开工后 15 分钟;超过 12 分钟收尾交部分结果。命令单次超过 55 秒分段。

## 禁区

不得改写域外路径;不得删断言、缩覆盖、放宽时限;不得提交他人路径;不得使用 UNISACC_E3CACHE=0;不用 --no-verify,不设 core.hooksPath;未实跑不记通过;不用 sed、awk、python、perl 改仓内文件。
