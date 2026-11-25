# 起链前十二事实交叉核对表(2026-11-25 ccogweco)

只读核对清单;每条给命令原行与期望原行。出处:cdx 的 car2-chain-ready-gates.md(gate 11/12/12p/14/17/5/22)、
car2-17-tlib-bump-gate.md、ompgpt 的 readback-prewarm-gate-evidence.md、car2-runbook.md(PATH 遮蔽)、
tools/chain-ready.cx:1522-1645(freeze-armed)。任何一条不真即不起链,不放宽。

## 十二事实

| # | 事实 | 命令原行 | 期望原行 |
|---|---|---|---|
| 1 | 本地 main 与 origin/main 同 SHA | `git rev-parse main origin/main` | 两行相同 40 位 |
| 2 | 107 冻结行已写 | `grep -P '^0\.0\.107-FREEZE\t' build/freeze.tsv` | 行含 40 位 base、`cc`、deadline 大于当前 epoch、paths 与 106 行同 |
| 3 | 候选槽为 0.0.107 单行 | `grep -n 'v0.0.107' release/candidate.tsv` | 恰一条数据行;门 11 打 `VERSION_SLOT OK version=0.0.107 target=0.0.107 source=arg` |
| 4 | 车树根 APE 与侧车 SHA | `sha256sum .claude/worktrees/v0.0.107/unisacc.com .claude/worktrees/v0.0.107/unisacc.com.build.json` | APE `5f83ac93d201b252ea0b56098475be9e30045875cdf1ce692451cda095ceeb3c`、侧车 `1dc2fed3b15cc02349f30ec18dad1982533f7a358c1bf6f1578b036d829601c3`(106 公共产物;两件须为正规文件) |
| 5 | 门 11/12/12p/14/17 PASS | 逐门分开跑(chain-ready --only 是单变量,不能串多个):`for g in 11 12 12p 14 17; do DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --repo "$PWD" --root "$PWD" --rev "$REV" --version 0.0.107 --only "$g"; done` | 五门各 PASS;14 允许"107 单行槽无法提供 106 启动产品提交对照"的告警,但摘要与长度须相等;多 --only 只会跑最后一个门(source:1692) |
| 6 | tools.cxlib 版本 0.4.12 且 TLIB_FAST bad=0 | `sh ./unisacc.com tools/check-tlib-frozen.cx -- --fast . tools.cxlib` 与 `sh ./unisacc.com tools/layer-api.cx -- --sha-only tools.cxlib` | `TLIB_FAST	pkgs=1	bad=0`;包表版本 `0.4.12`(车树 0.4.11 的下一号) |
| 7 | #14 v2(readback-prewarm-gate v2)入车且自检绿 | `git log --oneline -1 -- tools/release-exec.cx` 含 prerelease warm;`sh ./unisacc.com tools/check-release-exec.cx -- --part g` | `CHECK_RELEASE_EXEC	PASS	part=g	failed=0`;负控(删 hits!=2 判据)转红 `failed=1`;`BATCH_RUN id=release-exec-g status=PASS assertions=4`(readback-prewarm-gate-evidence.md:14-27) |
| 8 | 门 5 热缓存命中且上次热复跑 <54 秒 | 现行写法各一次:`DEVSVC_OWNER=cc sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$TMPDIR/unisacc-cr-prep-cache" tool` 与同命令尾参数 `generator`;再 `grep -m1 'identity-hit' <prep 日志>` 与 `grep -m1 '5-candidate-chain' <gate 日志>` | 两条 `PREPARED ... reused=1 reason=identity-hit`;`5-candidate-chain PASS ... <ms>` 且 ms<54000。旧手册的 "OUT CACHE --prepare" 写法作废 |
| 9 | 门 22 在 F 后重新预备 | `DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --repo "$PWD" --rev "$REV" --version 0.0.107 --only 22-premerge-union` | `PREMERGE_UNION_SUMMARY verdict=PASS conflict=0`;分支 SHA 等于 REV、base 为批准主干。注意:stamp 命中只省预合树生成,premerge-union.cx:672 起的 prep 仍重查生成器,全门预算里重复 prep 不算零 |
| 10 | PATH 无 moltbaby/bin/cc 遮蔽 | `command -v cc; echo "$PATH" | tr ':' '\n' | grep -F 'moltbaby/bin'`;自检 `sh ./unisacc.com tools/chain-precheck.cx` 相关批 | `command -v cc` = `/usr/bin/cc`;grep 无输出;`cc -std=c99 --version` 不出现 `--provider required`(car2-runbook.md:118) |
| 11 | DVQUALIFY_NATIVE_RUN_BATCH 未设 | `env | grep DVQUALIFY_NATIVE_RUN_BATCH` | 无输出(默认 opt-in;若必须出现则 =0) |
| 12 | devsvc status procs=0 | `sh ./unisacc.com tools/devsvc.cx -- status` | 输出 `procs=0`(无在跑 job);同时 `ping` 可达 |

## 失败处置(不重试同一失败两次)

1/2/3:起链前置未齐 -> 停,按 freeze-107-row.md 写行或修槽,不跳过。
4:两件不是正规文件或 SHA 不符 -> 按 cdx 页第 31 行用 106 公开原件替换,复跑 12/12p/14。
5:任一 FAIL -> 只重跑失败门;14 只允许指定告警,摘要/长度不等即 FAIL。
6:TLIB_FAST bad≠0 -> 先按 car2-17 页取 0.4.12 并重算 iface-hash,快检转绿才继续。
7:part g 非 failed=0 -> 检查是否 #14 v2 实际入车;负控若不转红则不成立。
8:无 identity-hit 或 ms>=54000 -> 用 tool 与 generator 两个 --prepare 各一次(现行写法)重备后再跑门 5,不改时限;门 22 的 prep 在 stamp 命中时仍会重查生成器,时间预算不能按零算。
9:conflict≠0 或 SHA 不符 -> 先解冲突并重算 derive,再重备。
10:cc 解析到协作命令 -> 先载干净 PATH,再跑受影响批并留解析原行。
11:变量为 1 -> 清掉再启动预跑,不重启已完成窗。
12:procs≠0 -> 先 `devsvc wait` 或停该 job,再起链。

## 备注

- agy-flow 的 stage-c run 记录以私有日志形式存在,本表引用其结论(门 11/12/12p/14/17 已实跑 PASS、两件启动件为正规文件);
  cdx 页记 `本轮核到 1a1ccc5167 落下的槽真实通过 VERSION_SLOT OK`,正式 F 仍须逐条复跑,不得以私有 PASS 代签。
- 十二事实同时为真才算"起链前检查通过";本页是清单,不是通过声明。
