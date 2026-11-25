# 门 5(5-candidate-chain)贴 55 秒限的根因与稳定办法(README;2026-11-25 ccogweco)

只读测量,不改仓,未跑超 55 秒命令。数据取自 cdx 的两处私有日志:
`/tmp/cdx-107-gates.Tzr4Nm/5-warm.log`、`5-cold.log`、`5-repeat.log`、`5-prep-tool.log`、`5-prep-generator.log`,
以及 `~/.local/share/unisacc/research/cdx/car2-chain-ready-gates.md` 第 5 门一节;源码
`tools/chain-ready.cx:1918-2010`、`tools/check-candidate-chain.cx:826-840、:965-995、:1408`。

## 0 先纠正一个数字(重要)

cc 说的"五片各自耗时 44/27/45/16/7"不是秒,是**各片的断言通过数**:

    5-candidate-chain PASS rc=0 pass=139 fail=0 (baseline=137 shards=5: core-a=44,core-b=27,core-c=45,objects=16,watch=7) 50773

44+27+45+16+7 = 139 = pass。日志里没有逐片秒数,故"core-c 构造 chain-step 45 秒"这一句**无逐片证据**,记为
UNMEASURED;能确认的是 core-c(group=13)依赖 generator 原生件(`check-candidate-chain.cx:995` 的 need_gen),
而准备 generator 与 tool 正是冷态 58 秒的主要来源。

## 1 可确认的分相与实测

| 相 | 实测 | 原行/出处 |
|---|---|---|
| 空准备缓存冷跑 | 58.002 s,rc=124(撞外层限) | 5-cold.log(`real 0m58.002s`,rc=124) |
| 原样重复(仍冷) | 58.002 s,rc=124 | 5-repeat.log |
| 预热后准备命中 | tool 0.688 s、generator 0.486 s,`reused=1 reason=identity-hit` | cdx 页第 25 行;runnerprep.cx:86 的 PREPARED 行格式 |
| 预热后全门 | 50.773 s(gate),命令墙钟 51.545 s;pass=139 fail=0 | 5-warm.log:1-5 |
| 五片分工 | core-a/b/c 与 objects/watch 各一 job;聚合取最大片耗时 | chain-ready.cx:1918-2001,chain:core-a..watch 列表 |

结论:50.8 秒的构成里,能被确认的是"准备缓存命中 + 五片并行"的前提;冷态下准备两件原生件就会先撞 58 秒外层限。
真正的贴限风险不是某片构造 chain-step,而是**准备相未命中**,加上五片并行后最长片接近 50 秒。

## 2 起链当天"不超窗口"的稳定办法

### 2.1 准备放在哪个步骤

在正式全门之前、同一私有窗口内,用**同一个 TMPDIR** 串行做两次准备(命令原行取 cdx 页第 14 行):

    export TMPDIR="$S/tmp"
    sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$TMPDIR/unisacc-cr-prep-cache" tool
    sh ./unisacc.com tools/check-candidate-chain.cx -- --prepare "$TMPDIR/unisacc-cr-prep-cache" generator

判据:两条都打 `PREPARED candidate-check tool|generator`;第二次或紧接的重跑应出现 `reused=1 reason=identity-hit`
(冷态 0.688/0.486 秒级)。

随后才跑全门(同一 env、同一 cwd、同一 APE、同一 REV):

    DEVSVC_OWNER=cc sh ./unisacc.com tools/chain-ready.cx -- --repo "$ROOT" --root "$ROOT" \
        --rev "$REV" --version 0.0.107 --only 5-candidate-chain --chain-base "$BASE"

判据:出现 `5-candidate-chain	PASS	rc=0 pass=139 fail=0 (baseline=137 shards=5: ...) <ms>`,ms 小于 55000;
末行 `READY yes failed=none`。若 ms 仍 >55000,按 2.3 处置。

### 2.2 哪些窗口动作会使准备缓存失效

- 换 TMPDIR、换 cwd、换 APE/侧车或换 REV(身份键变);
- 改 `tools/candidate-chain.cx`、`tools/chain-phases-gen.cx` 或其 use 闭包、pin 文件;
- 跑 `tables-gen`/`derive`/`register`/`layer-api --bump` 等会动 pin 与登记表的命令(准备键含身份);
- 清 `unisacc-cr-prep-cache` 或同 TMPDIR 启动另一个 chain-ready(定位文件会被清并重建);
- 与编译重负载并行(准备相本身也编译,负载抬高会拖慢)。

### 2.3 失败最短处置

- 准备 rc≠0:读 `PREPARED` 行缺在哪一件,单独重跑该件;不在同 TMPDIR 混跑其他门。
- 全门 ms>55000:保留原行;先确认两件准备是 identity-hit,再重跑一次全门(不改时限、不拆片);
  仍超则把该门放到本轮其他门之后单独跑,并记录原始 ms。

## 3 是否值得作为 107 追加集第 17 份(门 5 分片更均匀)

- 结论:**不值得**。理由:(a) 预热命中后 50.8 秒已在 55 秒内,gate 的每片上限是 60 秒(不是 55),余量真实存在;
  (b) 分片更均匀会改 `chain-ready.cx` 的五片聚合与 `check-candidate-chain.cx` 的 group 划分,连带改 check 与
  预跑判据,风险大于收益;(c) 107 追加集已收口,再塞一件只增加车头与重算面。
- 替代做法:把"两次 --prepare 预热"写进起链手册与 car2-chain-launch 的步骤(已在 cdx 页第 14 行),
  并在全门日志里留 `reused=1 reason=identity-hit` 原行。若 0.1.0 封版车仍 >55,再按 0.1.x 工具线单独立项分片。

## 4 未测项

- 五片各自毫秒数:日志未落,UNMEASURED;本轮不建议为此加插桩(会在门内加一次计时改动)。
- 50.8 秒的分配(准备 vs 五片)只有边界值可推,不宣称精确比例。
