# cx 核心的稳定性与版本策略(定稿 2026-11-12)

本文规定 cx 包(`cx.*` 与 `ext.*`)的阶段、各阶段允许的变更、宣布稳定的判据、弃用周期,以及由谁判定。逐区设计在 [design-core.md](design-core.md),需求摘要在 [prd_cx.md](../../prd_cx.md) 第 0 节。

核心结论:**何时稳定不由项目所有者决定,也不由任何人凭判断决定。判据由工具 `tools/stable-eval.cx` 求值;只有它输出 `STABLE-READY` 时,集成者才提交翻转 `build/tlib-policy.tsv` 的那一行。**

引用写法同 design-core.md:R、N、Q、P 指四份参照调研,`R6(4)` 表示该文第 6 节第 (4) 条;`[未核]` 表示未实测。

## 1 版本号与现有机制

事实(来自仓内文件,2026-11-12 只读):

- 包版本在 `build/tlib-packages.tsv`,形如 `X.Y.Z`:X 破坏接口,Y 增补,Z 实现发布点。`api_sha` 是 `build/layer-api/PKG.tsv` 的 sha256。
- `build/tlib-policy.tsv` 现只有一行 `stable no`(全局默认)。在此之下 `tools/layer-api.cx --bump` 不升 X 或 Y,任何变更都记 Z,并打印 `pre-stable class=z (would be y|x)`。按包覆盖的写法是加一行 `PKG yes|no`。
- `tools/check-tlib-frozen.cx` 守「包内容 sha 与记录一致,改动必须伴随 `--bump`」;`tools/precommit.cx` 对不一致输出 `TLIB_BUMP_NEEDED`(现为只报告)。
- 包版本与产品版本(0.0.x)是两条线,互不换算。

本文在这套机制上加规则,不替换它。

## 2 阶段

| 阶段 | 标识 | 兼容承诺 | 允许的变更类 |
|---|---|---|---|
| 孵化 incubating | 包名 `ext.*`;策略行 `no`;表内 `stab=incubating` | 无 | 任意。`--bump` 记 Z 并打印本应的 y 或 x |
| 预稳定 pre-stable | 包名 `cx.*`;策略行 `no`;版本 `0.Y.z`,X.Y 冻住只走 Z;表内 `stab=pre` | 无 | 任意,同上。发布说明必须列出本应为 x 的变更 |
| 稳定 stable | 策略行 `PKG yes`;版本自 `1.0.0` 起;表内 `stab=stable` 或 `legacy` | 有,单向门 | Z:实现变更,接口与合同不变。Y:只增。X:只在弃用周期走完后的移除,或第 5 节的安全例外 |
| 弃用 deprecated | 表内 `stab=deprecated:CXDEPnnnn` | 仍受保护,直到移除 | 该行自身不得再改 |
| 移除 removed | 表内 `# removed` 墓碑行 | 无 | 一次 X 升级 |

- 孵化与预稳定的区别在位置不在承诺:两者都不承诺。区别是 `cx.*` 的预稳定包已在目标面内,只差判据;`ext.*` 的包还需先满足提升判据(第 6 节)。
- 不设「随核心发出的 provisional」。从 P6(3):PEP 411 已被取代,其后记承认声明不稳定挡不住依赖。
- 不设通道、火车与日历节奏。拒 R6(10)、P6(11)、N5.6:那些服务于大量外部贡献者;cx 已有快慢两条车道。
- 预稳定期保留 `would be y|x` 输出并进发布说明。从 Q6(12):quickjs-ng 的 0.x 次版本既装新增也装破坏,消费者看不出真实等级。
- 不引入 edition。拒 R6(11):存量代码都在本仓,可用 `tools/refactor.cx` 一次改完。

## 3 什么算破坏

写成清单并由 `layer-api --bump` 与 `stable-eval` 判类。从 R6(5)、P5.1;拒 N5.3 的「按生态后果判断」,原因是后果判断不可机检。

| 类 | 变更 |
|---|---|
| X 破坏 | 删名或改名;改参数个数、顺序或类型;改返回类型;改 typedef 的任何字段;把某个返回码的含义换掉;删去 `errors` 列里已列的码;改所有权或释放责任(`own` 列);收紧前置条件;把某平台单元格从 `impl` 改成拒绝 |
| Y 增补 | 新增函数或类型;新增枚举常量与标志位;在 `errors` 列新增一个码(前提是该码已属 `sy_err_kind` 的某一类);把某平台单元格从拒绝改成 `impl`;放宽前置条件 |
| Z 实现 | 以上都不是:性能、内部结构、消息文本、`impl:说明` 的措辞 |

- 错误码是合同,消息不是。从 N6(4)。
- 新增错误码算 Y 的前提是调用者被要求留兜底分支(design-core.md 第 11 节)。从 R5.6 的 `non_exhaustive`。
- 行为修正(实现与表不符,改实现去符合表)算 Z;改表去符合实现算 X 或 Y,按上表。

## 4 宣布稳定的判据

按包求值。十三条全部 `ok` 才是 `STABLE-READY`;任何一条 `FAIL` 或 `UNKNOWN` 都是 `NOT-READY`。未实跑不记 ok。

| 编号 | 判据 | 如何机检 |
|---|---|---|
| S1 面封口 | `build/layer-api/PKG.tsv` 的每个导出名在 `build/std-surface.tsv` 有一行,或该行 `stab=internal`;反向亦然 | `check-std-surface` 退出 0,且其「无表行的导出名」计数为 0 |
| S2 全部实现 | 该包没有 `status=new` 的行 | 读表 |
| S3 错误合同 | 每行 `errors` 列符合封闭文法:`none`、`0/1`、`null`、`noreturn`、`miss:-1`、`-errno`、`-E名字`,以分号连接,可带 `ret:说明` | 读表,正则 |
| S4 零未核 | 该包行的三个平台单元格没有 `unverified` | 读表 |
| S5 拒绝有名 | 每个拒绝单元格形如 `refuse:ENOSYS cap=NAME`,NAME 在表头 `# cap` 词表内。「本版未实现」一类不合格 | 读表 |
| S6 用例覆盖 | 每行 `test` 列指向 `tests/batches.tsv` 的已登记批次;该批的证据日志里对该行有至少一条成功用例;`errors` 列每个具名码至少一条失败用例;每个拒绝单元格在对应目标上有一条断言 `-ENOSYS` 的用例 | 检查按统一格式输出 `SURF<TAB>名字<TAB>ok|err:-E名字|refuse`,`stable-eval` 从候选证据目录汇总 |
| S7 六目标同候选 | `release/candidate.tsv` 所指候选的 CI 证据里,该包的面批次在 `lnx`、`osx`、`win` 乘 `x86_64`、`arm64` 六格都有实跑结果且通过 | 读候选表与证据;某格只是复用另一格二进制而未在该系统执行的,记 UNKNOWN |
| S8 面静止 | 该包的 `api_sha` 与表内该包各行的名字、签名、`errors`、`own` 四列,在最近连续 3 个公开版本的 tag 上逐字相同,且首末两个 tag 相隔不少于 30 天 | `git show TAG:路径` 取历史内容比对;tag 取 `v0.0.N` 形式的公开版 |
| S9 有消费者且上层已离开 libc | (a) 每个非 legacy、非 internal 的行在本包与其检查之外至少有 1 个调用单元;(b) 产品层(`src/`、`cx/core`、`cx/runtime`、`cx/compiler`、`cx/driver`)在 `tests/cxlibc.tsv` 的直接 libc 计数为 0 | (a) `tools/refactor.cx sites` 或 symindex 计数;(b) 读棘轮表 |
| S10 依赖已稳定 | 该包在 `build/tlib-packages.tsv` 的每个依赖,策略行为 `yes` 或在同一次声明里一并达标;不依赖任何 `ext.*` | 读两张表 |
| S11 所有权合同 | 签名返回或填充资源的每行 `own` 列非空,配对的关闭函数是表内一行;S6 的用例里含重复关闭与关闭后使用各一条(期望 `-EBADF`) | 读表与证据 |
| S12 机制绿 | `layer-api --check`、`check-tlib-frozen`、`tools/ratchets.cx` 对当前树通过 | 调用现有工具,各自不超过 60 秒 |
| S13 独立评审 | `agentwork/ledger.tsv` 有一行 id 为 `STABLE-REVIEW-PKG`,事件含 `accept` 与当前 `api_sha`,角色列不是实现者角色 | 读账本;`api_sha` 不符即 FAIL |

取值说明:

- S8 的「3 个公开版且 30 天」两个条件并用。只数版本不够:本仓公开版间隔可短到一两天(0.0.55 至 0.0.58 在数日内),3 个版本不代表经过了使用;只数天数也不够:没有发布就没有同候选证据。
- S9(a) 是 R6(3) 的「无消费者不稳定」。没有调用者的行在声明前要么找到消费者,要么退回 `ext.*`。
- S13 是唯一带人工判断的一条,但它被做成可机检的事实(账本行加 sha 绑定)。评审者由集成者开独立代理担任,只评面的形状与合同,不重跑测试。从 R6(3) 把 FCP 换成可执行门;拒 N5.2 的双人批准加 48 小时等待。
- 判据不含「项目所有者同意」。

现状对照(仅据表的只读统计,不代表工具求值):全表 `status=new` 108 行、`unverified` 71 格、未实现的拒绝在核心有 21 行,`stab`、`own`、`test` 三列尚不存在,tlib 侧尚无 `cx.sys`、`cx.os`、`cx.mem`、`cx.text` 四个包。所以当前没有任何包接近 `STABLE-READY`。离判据最近的是 `cx.path`(26 行全部 exists,三平台全部 impl)。

## 5 弃用周期

三段加不可变编号。从 N6(7)(去掉按日历大版本推进)、P6(9)(10)、R5.6(弃用不急于删除)。

| 段 | 表内标记 | 行为 | 最短停留 | 版本类 |
|---|---|---|---|---|
| (1) 软弃用 | `stab=legacy` | 只在表与接口说明里标注并写明替代;不发警告;仍有测试;调用点计数进棘轮,只降不升 | 无;可以永久停留 | Z |
| (2) 弃用 | `stab=deprecated:CXDEPnnnn` | `stable-eval --callers` 与 `tools/precommit.cx` 对每个调用点报告(构造期警告,不阻断) | 3 个公开版且 30 天 | Y |
| (3) 移除 | 行移入 `# removed` 墓碑段,保留编号、原签名、移除版本 | 名字从 layer-api 消失 | 不适用 | X |

规则:

- (a) 进入 (2) 之前,替代接口必须已是稳定行至少 1 个公开版。先给替代,再标弃用。从 P5.2。
- (b) 不经过 (2) 不得进入 (3)。从 N5.4(2)。
- (c) 进入 (3) 的前提是仓内调用点为 0(`refactor.cx sites` 实测),并且该次发布说明的「破坏性变更」一节由 layer-api 的差异输出生成,不手写。从 Q6(11)。
- (d) (1) 是合法终态。维护成本低、已在新接口之上重写的旧入口可以一直留着。从 P6(10)。
- (e) 别名或空桩够用时不进 (2)。改名迁移保留旧名为别名并标 legacy,不以删除旧名完成改名。从 N6(8);与本仓工具改名规则一致。
- (f) 编号 `CXDEPnnnn` 从 0001 起,登记在表的弃用行或墓碑行,永不复用;撤销弃用时行回到 legacy,编号保留并注明已撤销。从 N5.4(5)。
- (g) 安全例外:接口本身危险或不安全(内存越界、凭据泄露、数据损毁)时,可以跳过 (2) 的停留期直接移除或改签名。由集成者决定并执行,在 `agentwork/ledger.tsv` 记一行(决定、依据、证据、回滚办法),发布说明单列。从 P5.2 的例外条款;决定者按本仓决策授权是集成者,不是项目所有者。
- (h) 事实稳定条款:孵化或预稳定的行,只要仓内有调用者,破坏它的那个候选必须在同一候选内改完全部调用点并用 `tools/affected.cx` 选批实跑。「没有承诺」不等于「可以让仓内红」。从 N6(3);阈值取 1 个调用者,因为仓内改写成本低。
- (i) 预稳定与孵化阶段没有弃用周期;legacy 标记在这两个阶段只表示「该行将以软弃用身份进入稳定面,新代码不要用」。

## 6 孵化与提升

- 面封口之后(design-core.md 第 14 节的行编辑落表并升 v2),新函数只能先进某个 `ext.*` 包,`stab=incubating`。
- 从 `ext.*` 提升进 `cx.*` 的判据,全部机检:
  - (1) 该行的名字、签名、`errors`、`own` 在最近连续 2 个公开版的 tag 上未变。
  - (2) 所属包与其检查之外至少 3 个调用单元。
  - (3) 满足 S3、S4、S5、S6、S11(对该行)。
  - (4) 实现只依赖 `cx.*` 包。
- 提升是一次 Y 升级:目标 `cx.*` 包加行;原 `ext.*` 模块路径留别名,不改函数名。若目标包已稳定,提升进去的行直接是 `stab=stable`,因此提升判据比 S8 的 3 个版本短但加了调用者数量。
- 反向路径:稳定包里的行退出核心,只能走第 5 节的弃用周期。从 P6(12):进入比退出难。

## 7 判定工具 tools/stable-eval.cx

该工具尚未建立。本节是它的合同。建立时按本仓规则:先 `git log --all --oneline -- tools/stable-eval.cx` 查名字历史(本轮 `ls` 未见同名文件,完整历史未查),带 `--fixtures` 自检,登记批次 `stable-gate`,在 `prd.md` 能力速查补一行。

调用:

```
sh ./unisacc.com tools/stable-eval.cx -- [--pkg PKG]... [--root ROOT] [--candidate FILE] [--evidence DIR]
sh ./unisacc.com tools/stable-eval.cx -- --verify [--root ROOT]
sh ./unisacc.com tools/stable-eval.cx -- --callers NAME
sh ./unisacc.com tools/stable-eval.cx -- --declare PKG [--root ROOT]
sh ./unisacc.com tools/stable-eval.cx -- --fixtures
```

输入:

| 输入 | 用于 |
|---|---|
| `build/std-surface.tsv`(v2) | S1 至 S6、S11 |
| `build/layer-api/PKG.tsv`、`build/tlib-packages.tsv`、`build/tlib-policy.tsv` | S1、S8、S10、S12 |
| `tests/batches.tsv`、`tests/cxlibc.tsv` | S6、S9 |
| `release/candidate.tsv` 与 `--evidence` 目录下该候选的批次日志(`SURF` 行) | S6、S7、S11 |
| git 的公开版 tag 与其上的历史文件内容 | S8 与提升判据 (1) |
| `agentwork/ledger.tsv` | S13 |
| symindex 或 `tools/refactor.cx sites` 的调用点计数 | S9(a)、提升判据 (2)、`--callers` |

它组合的现有工具:`tools/layer-api.cx --check`、`tools/check-std-surface.cx`、`tools/check-tlib-frozen.cx --fast`、`tools/ratchets.cx`。它读证据,不重跑面批次;证据必须属于 `release/candidate.tsv` 所指候选,且该候选的 `api_sha` 等于当前树的 `api_sha`,否则 S6、S7、S11 记 UNKNOWN。每个子步骤不超过 60 秒。

输出:

```
STABLE<TAB>PKG<TAB>S1..S13<TAB>ok|FAIL|UNKNOWN<TAB>细节(首个不合格的行或格)
STABLE_VERDICT<TAB>PKG<TAB>STABLE-READY|NOT-READY<TAB>failed=N<TAB>unknown=M<TAB>api_sha=...<TAB>candidate=...
```

退出码:

| 码 | 含义 |
|---|---|
| 0 | 所求的每个包都是 `STABLE-READY`;或 `--verify` 无违例;或 `--fixtures` 全过 |
| 1 | 任一包 `NOT-READY`(含 UNKNOWN);或 `--verify` 发现违例;或 `--declare` 被拒 |
| 2 | 用法错误或输入不可读(只把 ENOENT 当不存在,ENOTDIR 与 EACCES 报告) |

各模式:

- 默认:求值并报告,不写任何文件。
- `--verify`:对策略行已是 `yes` 的包核稳定后不变式:相对上一个公开版 tag,没有未经弃用周期的 X 类变更;legacy 行的调用点数没有上升;墓碑编号没有复用。进 `tools/precommit.cx` 与发布门。
- `--callers NAME`:列出某行的调用点,供弃用段 (2) 报告。
- `--declare PKG`:重新求值;只有 `STABLE-READY` 才把 `build/tlib-policy.tsv` 加上或改成 `PKG<TAB>yes`,把 `build/tlib-packages.tsv` 该包版本置为 `1.0.0`,把表内该包 `stab=pre` 的行改为 `stable`,并在标准输出打印待追加的账本行文本。否则不写文件,退出 1。
- `--fixtures`:对临时夹具逐条判据各造一正一反,确认每条 FAIL 都能被单独触发。

## 8 声明流程与留痕

- (1) 任何代理或集成者随时可以跑默认模式,看差哪几条。
- (2) 输出 `STABLE-READY` 后,集成者跑 `--declare PKG`,把它写出的三处改动连同账本行作为一个提交推送。提交说明写包名、`api_sha`、候选身份。
- (3) 账本行:id `STABLE-DECLARE-PKG`,事件写判据全 ok、`api_sha`、候选;证据列写 `stable-eval` 的输出路径。回滚办法写明:在含该声明的公开版发布之前,可用一个提交把策略行改回 `no`、版本改回;公开版发布之后不可回滚,只能走弃用周期。
- (4) 不向项目所有者请示。项目所有者的作用止于批准本策略文本;此后改判据本身才需要改本文,并在账本留行。
- (5) 没有 `STABLE-READY` 输出而翻转策略行的提交,由 `--verify` 在提交前检查里报红(该包判据不成立即违例)。
- (6) 稳定声明按包逐个做,不要求六个核心包同一天达标;顺序受 S10 约束:无上游的包先行,`cx.os`、`cx.stream` 在其依赖之后。拆包后各包的实际依赖边未核(`cx.path` 现登记依赖 `cx.base`,落到拆出的哪一包待查)。

## 9 未核实与取值依据

- (1) `tools/stable-eval.cx` 不存在,第 7 节全部是待实现的合同。
- (2) S6 的 `SURF` 行格式是新约定,现有检查都不输出它;现有面检查(`check-cx-sys*`、`check-cx-path` 等)对每行每码的覆盖程度未统计。
- (3) S7 所需的「按包的面批次在六目标 CI 上实跑」现状未核;`docs/cx/prd.md` 记有 `win/arm64` 复用 `win/x86_64` 的 PE,按本文该格在未实际执行前记 UNKNOWN。
- (4) S8 的 3 个版本与 30 天、第 5 节的 3 个版本与 30 天、第 6 节的 2 个版本与 3 个调用单元,是参照 P5.2(至少两个版本)、N5.5(实验期至少跨一个大版本)后按本仓发布节奏取的值,没有本仓数据支撑;调整须改本文。
- (5) `--declare` 写三张表的做法与「有限表只经生成器修改」的关系:它本身即带检查的写入工具;与 `layer-api --bump` 写同一文件的次序冲突未分析。
- (6) 四份参照里的 `[忆]` 条目在本文仍为未核。
