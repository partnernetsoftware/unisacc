# 子代理简报与交付自检

## 用法

(1) 主调度写简报:复制 `plans/briefs/TEMPLATE.md`,填五段(目标、写域、验收命令含预期值、截止、禁区),在验收口径块里勾选适用的 profile(名字见 `tests/accept-profiles.tsv`,只引用名字,不抄数字)。派工前跑:

    sh ./unisacc.com tools/brief-lint.cx -- 简报文件

末行 `BRIEF_LINT missing=0 reject=0` 才派工;缺项行带建议句,可直接粘贴进简报。出现 `UNISACC_E3CACHE=0`(不是禁止句)一律 REJECT。

(2) 子代理交付前在工作树跑:

    sh ./unisacc.com tools/handoff-check.cx -- --base 开工提交

每项一行 `ITEM 名 状态 毫秒 原文`,末行 `HANDOFF passed=N failed=N unmeasured=N review=N scope=LOCAL`。failed 非零不得交付。单次超过 55 秒时按项分段:`--item newcx`、`--item ratchets`、`--item shards`、`--item fresh-cold`。只做静态项用 `--static`;跳过批次实跑用 `--skip-batches`(该项记 UNMEASURED,不得写成通过)。用开发产品实跑批次加 `--candidate 绝对目录`;版本分支允许直接提交产品输入时加 `--allow-product`,输出会记 ALLOWED。

(3) 输出 REVIEW_REQUIRED 的提交(核心路径或提交说明含 no-verify 字样)由主调度派独立复核,自检不代替复核。

(4) 自检结果只算本地证据,不是发布资格;发布资格仍按冻结后全量批次与同候选平台证据判定。

## 与 prd_iteration.md 第 8 章及本日返工的对应

| 教训或返工 | profile | 自检项 |
|---|---|---|
| 冷准备混进验证相撞窗口(第 8 章);0.0.74 struct-member-capacity 拆批重跑约 60 分钟 | cold-fresh-layout-33s | fresh-cold |
| 0.0.75 release-prep 在无历史布局 FAIL,冻结后修约 10 分钟(92e49cac83) | fresh-layout-no-history | fresh-cold |
| release-prep 批全新布局冷缓存 38.4 秒超 33 秒线 | cold-fresh-layout-33s | fresh-cold |
| 夹具与门追补原地打转(第 8 章);dvledger 与 dvqualify 重复 load1 触发 dup-census | ratchets-15-0 | ratchets |
| 合入后 tools-compile 缺片三次 | shards-consistent | shards |
| 度量要核口径(第 8 章);A-04 提速只在服务端口径成立 | e2e-latency | 简报口径(未机判) |
| 证据不入仓等于没有证据(第 8 章) | local-evidence-only | 末行 scope=LOCAL |
| 速度线不能吞掉产品线(第 8 章) | patch-first-for-product | patch-first |
| 跨车道通知先查归属(第 8 章) | core-path-review | core-review |
| 0.0.74 CI osx/x86_64 超时重做约 17 分钟 | cold-fresh-layout-33s | fresh-cold(部分:本机 33 秒线,不含 CI 倍率) |
| 新工具缺登记 | tool-registered | registry |
| 冻结后出 FAIL 或 UNKNOWN | thaw-on-fail | 简报口径(未机判) |
