# script-inventory 时间剖析与不放宽限时的加速方案

2026-10-15，cdx2。**实际跑过仓外副本；实现建议只是设计，未改主检出tests。** 研究路径单笔提交，不碰W/S及他人暂存。测量前确认并避让在途commitgate；本轮每次命令由58s外层限时，计时与准备分开。不得将402秒历史事件归因为本次profile，本稿不诊断那次看门狗。

## 结论

首选等价的“basename出现性预筛＋预计算模式/名字”，无需先拆作业或启缓存。仓外原版profile39.115s，优化profile3.007s（进程墙钟3.050s），约13倍；完整生成INVENTORY.md逐字节同，汇总同：523脚本、468 gated/reached、10library、45listed、0orphan。此结果证明该固定树的等价见证，不证明所有未来输入、负载或平台都已验收。

**没有重复git调用可删。** inventory.py虽import subprocess，但实际没有子进程调用；roots取gate/all/release与CI工作流文本，**不执行**gate --list。既有文档中的“源文本扫描”是实现事实。

## 时间在哪里（cProfile，本机Python3.13）

|项|原版实际|解释|
|---|---|---|
|全脚本|39.115s / 13,126,362调用|profile有开销，不能等同无profile纯墙钟|
|refs(src)|475次，累计37.114s|每个可达脚本及root都遍历全脚本名|
|re.search|246,871次，累计33.311s|大多数脚本名根本不在该源中，也做全源正则扫描|
|Pattern.search|32.443s自身|主要CPU开销，不是磁盘或Git|
|re._compile|0.735s累计|先前“正则缓存抖动为主”的猜测不成立|
|pathlib.name|523,268次，累计2.253s|重复把同一名字重新转Path/name|
|优化refs|475次，累计0.811s|names固定后先用bytes/字符串出现性排除不可能匹配|

证据目录 `/home/box/c43/inventory-profile/`：baseline.prof/optimized.prof、baseline.out、baseline-write.out、comparison.json、baseline-INVENTORY.md、original-inventory.py.txt及optimized-source.txt。仓外树从git对象 `1ad126916e9cb11366959058f26ca4d500330baa` 只导tests/exec/.github/workflows，无.git；这已经证明扫描不依赖Git历史。公开源码树是固定对象导出，不拿共享脏树计时。

原版另跑一次 --write --check（58s内正常结束），保存完整行表；优化也 --write --check，输出摘要与行表字节同。INVENTORY SHA256 `100b4850e8af1dfbfbd64f1e3d1d5837efdfc5e9f036cbc0abafea9c5acfcfa2`。未进行模型构造或产品运行。

## 优先方案 P1：不改变匹配语言的预筛

把固定names编成 `(相对路径, basename, compiled_pattern)` 数组。refs中先判断 `basename not in src`，为真则continue；否则沿用原来的三条件：完整路径出现／斜杠basename出现／原正则边界匹配。原三种匹配都蕴含basename必出现，所以预筛不会漏掉原版匹配。不能以token提取替代原正则而悄悄改变大小写、标点/引用、扩展名与\b语义。

预编译本身不是13倍来源；basename出现性预筛跳过绝大部分长文本正则才是。本轮实验组合预筛和预计算，没有逐优化消融，不编造两者独立加速倍数。下一实施可优先这两项，并缓存rows的basename以省另一批name调用；后项尚未实验。

必须保留全局可达性闭包、同stem模块碰撞现有选择语义、循环引用的visited语义、读取失败原出口与ungated disposition。缓存文本可免重复读取，但只限本次不可变快照；不能读取一次后忽略套件期间输入变化。

拟后续具名回归：basename仅出现在无效边界（须不多收）、完整路径/斜杠/裸名三正例、重名basename不同目录、循环引用、import库、未登记孤儿、已listed孤儿；新旧rows逐字节对拍，主动删root引用/ungated行必显出对应改变。单例≤10s、每批≤50s；首个未预期差异停，不放宽55/58秒。

## P2：分片只作后备，不能按目录各自决定ORPHAN

若P1实装后仍超窗口，再考虑扫描与归类分离：一次快照固定全体names、roots、dispositions；按目录/稳定name分片计算引用边和imports；最终一次全局闭包与归类。片的并集须等于完整names，两两不交，最终结果与旧版逐行相同。跨tests/exec/CI边不能丢失；每片自己的“无孤儿”不构成全局通过。

扫描片是准备结果，不算inventory通过；最终作业必须检测缺片、重复片、过期片及hash不一致。需要更多调度/封套机制，不优于P1当前3秒结果，暂不推荐先开多job。准备和汇总各≤50s，留队列窗口余量，不靠总上限放大。

## P3：持久缓存不是首刀

若要缓存不可变扫描结果，键必须包括**路径成员集合＋每文件内容hash＋roots全部字节＋ungated disposition＋算法版本**。新增/删除文件和新root必须失效；只mtime、目录hash或README不变不够。内容解析边可按文件cache，但全局names改变会改变原refs匹配结果，必须重算受影响边或全失效。读错/未读取到的输入不能用旧cache返回绿。缓存只复用扫描中间物，最终闭包和ORPHAN判定仍执行，不复用PASS。

哈希需读完整相关字节，但本轮主耗时在重复搜索，缓存投入风险高于收益；P1无缓存、无状态的新鲜扫描应先落。

## 登记与提交边界

本稿只提交research/c43-inventory-speed.md；不改tests/。后续P1拟域tests/inventory.py及必要的具名回归：inventory属于登记链来源，改动前需cc安排**登记窗**，同笔验证派生INVENTORY.md是否实质变化（本轮不变，不为提交制造生成噪声），刷新gatedeps中已审输入身份。不得与在途gate.sh/abi/INVENTORY或W/S注册同时修改。

如新增回归脚本，需tests/gate.sh、gatedeps.json、INVENTORY.md/ungated等适用登记与层分类，是否派生ABI须以实际derived-map判断，不泛说全要改。若P2新增片job，gate.sh/计数/层分类/gatedeps及脚本inventory本身都需同窗，不能先落孤儿使全员commitgate红。每个job必须绑定所有语义输入，不按局部目录摘掉全局names/roots。

commitgate默认set仍是gate-layers/script-inventory/checkrun。本研究提交将再次走默认套件，可能仍受未优化原版负载影响；若超时保持HOST_TIMEOUT与未提交，不把研究或产品改记失败。此稿不授自动实施、变限时、正式整闸、发布或旧7000恢复。
