来源:会话临时目录 /home/devali/.claude/jobs/7f46e032/tmp/R22/design.md 拷贝于 2026-11-24
# R22 整链节点争用超界:结构诊断与裁定

日期 2026-11-24。只读取证,未改仓内文件。标"推断"与"未核"处未经实测。

## 0 裁定(一句话)

选 (d) 余量合同为主、(b) 的最小形态为辅;否决 (a);(c) 只做诊断标签,不改判定。
理由:红的两项不是调度器不够聪明,而是节点单跑耗时已占时限的 62% 到 88%,任何减速 1.2 倍以上的扰动都会击穿;限流只能压低自己造成的争用,压不住别的窗口造成的负载。

## 1 时间与信心耗在哪里(数据)

(1) batch-packages-gen:时限 30 秒(exec/facts/derive.tsv 第 9 行),单跑 18.6 秒,占 62%;整链下 rc=142。击穿只需减速 1.61 倍。
(2) sys-six-target:表内时限 55 秒,但命令自带 --deadline-s 50(tools/flows/chain-preflight.tsv 第 13、14 行),真实界是 50 秒;单跑 44 秒占 88%,整链 49.8 秒 rc=1。rc=1 而非 142 说明是工具内部截止先到,不是 tools/bound。击穿只需减速 1.14 倍。
(3) exec/facts/derive-weights.tsv 把 batch-packages-gen 记为 1 秒,实测 18.6 秒,失真 18 倍;该表头注明是"实测墙钟、冷缓存向上取整",已不成立。池子按权重先起重行,它被当成最轻的行排在最后,与其他重行叠在一起(推断:叠加时序未核)。
(4) 同表内还有同类隐患:product-map 权重 41 对时限 55(75%),tool-index 20 对 30(67%)。这两行今天没红,只是还没轮到(权重是冷缓存值,热值未核)。
(5) 第四次红。前三次都靠人工单跑复核定性为"负载",每次一轮 chain-ready 加人工复跑,估计每次 10 到 20 分钟调度时间(推断,未逐次核账)。

## 2 反复出现的结构原因

(1) 没有余量合同。时限是铁律,但"单跑耗时与时限的比例"没人守。节点可以合法地长到时限的 88% 再入库,此后每次负载波动都是一次误红。铁律说超限要分解,却没有说"逼近即分解"。
(2) 嵌套池各算各的。chain-ready 外层按 load1 取 4 到 6 路(tools/chain-ready.cx 第 868 到 870 行),derive 类另限 4 路;每个 derive 子进程里又是自己的 4 路池(tools/derive.cx 第 17 行);premerge-union 的 derive-a/b/c 片各自再起 derive;chain-preflight 把表内外部节点全部 fork,无并发上限(tools/chain-preflight.cx 第 800 到 838 行)。四层池之间没有共同预算。
(3) 现成的全机令牌池没人接。tools/cxlib/flowadmit.cx 已是跨进程文件槽令牌(容量取核数与内存),s9-daily-flow 用 w=6 接了;chain-ready、derive、chain-preflight、premerge-union 四个文件里搜不到 fa_acquire 或 flowadmit。
(4) 定性靠人。超时后没有任何字段区分"自己算得慢"与"被挤",所以每次都要人工单跑。

## 3 各选项裁定

(a) 按 load1 自适应并发:否决。
- chain-ready 已经有这一机制(load1 不低于核数取 4 路),第四次照样红。
- 项目 11-20 已用数据废除 flow 的负载门(提交 de000a79d6,plans/devsvc.md 第 268、364 行):提交失败率在 load1 三档为 29.9%、37.7%、31.0%,不随负载走;本机常态 load1 为 9 到 15,门槛会长期饿死。再引入是走回头路。
- 负载大半来自别的窗口时,自己降并发收益有限(来源比例未核,见第 6 节实验)。

(b) 节点声明 cost 与冲突域、devsvc 统一令牌池:采纳最小形态,不新建池。
- 只做一件事:四个调度器的重叶子节点起跑前向 flowadmit 取令牌。不在 devsvc 里另造调度器,不做 cost 学习。
- 令牌等待不计入节点时限(时限管的是执行),但必须计入该门的总截止,并在行里写出 admit_wait_ms。

(c) 超时自动单跑复核并标 CONTENDED:只取诊断一半。
- 不自动重跑改判。理由:同机负载来自别的窗口时,"单跑"并不单;自动重跑把余量债藏起来;与"同一失败两次即停手"相抵。
- 采纳的部分:超时或内部截止的行,附上子进程 CPU 秒与墙钟秒。CPU 秒明显小于墙钟(建议阈值 0.6)时在证据里写 contended=1。判定仍是 FAIL,READY 仍是 no。它只省掉人工单跑定性这一步。

(d) 余量合同:主方案。
- 规则:任一有界节点,单跑墙钟不得超过其真实时限的 50%;超过即分解(分片、增量、预备材料外移),不是调时限。
- 这是把"分治时限铁律"从分钟级延伸到秒级,不放宽任何时限。

## 4 落地切片

S1 两个红节点先分解(第一周第一天,可立即开工)
- 谁:cc 或 omp-go(纯表与工具侧,不碰 devsvc 源码)。
- 改:tools/flows/chain-preflight.tsv 把 sys-six-target-0、-1(--part k/2)改成四行 --part k/4;tools/sixtarget-precompile.cx 已支持任意 --part(tools/check-sixtarget-precompile.cx 第 236 行用过 0/4),预计不必改工具。batch-packages-gen 先量相位(原生生成器预备与执行各占多少,未核),再决定加 --part 分片成两行,或把预备挪出执行窗。同步改 exec/facts/derive-weights.tsv 为实测值。
- 验收:
  - sh ./unisacc.com tools/chain-preflight.cx -- --strict --root . --repo . --rev HEAD,预期四个 sys-six-target 行各 ms 小于 25000,verdict=PASS。
  - sh ./unisacc.com tools/derive.cx -- --check --only batch-packages-gen,预期 OK 且每个分片小于 15 秒。
  - 相关 check(check-chain-preflight、check-derive、check-sixtarget-precompile)失败数 0;若夹具写死节点个数需同步改,不得删断言。
- 收益:两项红在 1.6 倍以内减速下消失(推断,依据第 1 节比例)。代价:半天。回退:还原两张表的行。

S2 余量棘轮(第一周)
- 谁:omp-go 备料,gatekeeper 复核(chain-ready 属关键路径)。
- 改:新增 tools/check-node-headroom.cx 并登记批次。读 exec/facts/derive.tsv、derive-weights.tsv、tools/flows/chain-preflight.tsv 与一张实测表(建议 build/node-cost.tsv,由生成器从 derive 摘要与 PREFLIGHT_ITEM 的 ms 写出,带提交号与测量时 load1)。断言:实测单跑不超过真实时限的 50%;权重与实测相差不超过 2 倍;命令内 --deadline-s 小于表内时限时以较小者为真实时限。
- 现存超标行(product-map、tool-index,待实测确认)登记进 build/phase-times.tsv 带到期整改,不豁免新增。
- 验收:sh ./unisacc.com tools/check-node-headroom.cx,预期 S1 之后 fail=0 或仅剩已登记到期项;变异自证:把 batch-packages-gen 权重改回 1,必须红。
- 收益:同类问题在入库时拦住,而不是在整链时。代价:一天。回退:批次降到 daily 层。

S3 四个调度器接 flowadmit(第一周后半,归 agy-flow 冲突域限流件)
- 谁:agy-flow 实现,cc 集成。
- 改:tools/derive.cx(start 处)、tools/chain-preflight.cx(fork 外部节点前)、tools/chain-ready.cx(start 处,仅重叶子:ratchets、candidate-chain、cx0-extract、gen-all;不含本身只是再分发的 derive 与 premerge 壳,避免父子同时持令牌互锁)、tools/premerge-union.cx(同理只在叶子)。节点权重列加到各自的表里,默认 1。
- 必须守:取不到令牌只推迟起跑;等待有总截止;行里出 admit_wait_ms;DEVSVC_ADMISSION=off 仅检查可用。
- 验收:高并发夹具(DEVSVC_NCPU=4 人为压低容量)下整套 derive --check 全 OK、无 142;check-flowadmit 与 check-derive 失败数 0;prd_mempalace.md 第 74 行的用例原样通过。
- 收益:消除自伤型争用。代价:两到三天。风险:父子双持令牌死锁,故只在叶子取。回退:环境变量关准入。

S4 争用诊断字段(可与 S3 同批)
- 改:tools/derive.cx 与 tools/chain-preflight.cx 在 TIMEOUT 或内部截止行追加 cpu_ms 与 contended=0|1(来自 wait 的资源用量;若 sy_wait_status 不给 rusage,这是库缺口,记 build/pkg-gaps.tsv)。
- 验收:夹具用一个睡眠型子进程触发超时,预期 contended=1 且判定仍为 TIMEOUT rc 非 0;用忙等型子进程,预期 contended=0。

## 5 砍项与延后项

- 砍:(a) 负载自适应的任何新增;chain-ready 第 868 到 870 行现有的 4/5/6 档在 S3 落地后删除,只留令牌。
- 砍:自动重跑改判、PASS-RETRIED 之类的新状态。
- 延后:devsvc 内的 cost 学习、按历史耗时自动排程。先有 S2 的实测表,再谈学习。
- 分开立件,不并入本件:readset-changed-before-output。发码处是 tools/product-map-query.cx 第 245 行,含义是输出前读集内容哈希变了,属于写冲突域(有人在读的同时写了读集内文件),不是 CPU 争用;限流治不了它。若它出现在 premerge 的私有并集树里,则更像各片共用同一目录的自写竞争(推断,未核),应由 premerge-union 的属主单独查,正解是每片读冻结快照。

## 6 我最可能错在哪里,怎样便宜地证伪

(1) 错判来源:若负载主要是整链自己造成的,S3 单独就够,S1 的分解是多余工作。实验:下一次 chain-ready 期间每 2 秒采一次 /proc/loadavg 与按顶层命令归属的可运行进程数,用 .cx 写,十分钟内出结论。自伤占比高于 70% 则 S3 提前,S1 只做 sys-six-target。
(2) 分片不线性:sixtarget 分四片后每片可能仍有固定的头部准备成本,单片降不到 25 秒。实验:现在就单跑 --part 0/4 一次,约 20 秒可知。若固定成本超过 15 秒,改为把准备挪成独立预备节点。
(3) 50% 阈值可能过严,逼出大量无谓分片。证伪条件:S2 首次实测若超标行多于 6 行,阈值改 60% 并只对整链内节点生效。
(4) batch-packages-gen 的 18.6 秒可能大部分是原生生成器冷编译,热跑只要几秒;若如此,问题是预备没命中缓存,而不是节点太重。实验:连跑两次看第二次耗时。
(5) 令牌让整链总墙钟变长,触碰门的总截止。证伪条件:S3 后 chain-ready 总耗时上升超过 20% 即回退到只对 derive 与 preflight 接令牌。

## 7 证据入口

- ~/repos/unisacc/tools/chain-ready.cx 第 848 到 872、1570 到 1594、1740 到 1820 行
- ~/repos/unisacc/tools/derive.cx 第 9 到 19、138 到 155、216、243 行
- ~/repos/unisacc/tools/chain-preflight.cx 第 706 到 707、800 到 838 行
- ~/repos/unisacc/tools/flows/chain-preflight.tsv 第 12 到 15 行
- ~/repos/unisacc/exec/facts/derive.tsv,~/repos/unisacc/exec/facts/derive-weights.tsv
- ~/repos/unisacc/tools/cxlib/flowadmit.cx 文件头
- ~/repos/unisacc/prd_mempalace.md 第 74 行,~/repos/unisacc/plans/devsvc.md 第 268、364、412 行
- 提交 de000a79d6(负载门废除)
