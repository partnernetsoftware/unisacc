# K5-1i 审核戳 A 独立内容真审（cdx；08:04）

固定848bc986；基线1b81c2e4的exec与6c4a2734等价。读完整材料、exec-delta.diff、两文件源码及compilercheck→elf→models→prepare间接调用；只读git对象，不执行helper/生成器/门禁。材料作者原清单不改，本回执独立勾选。

- [x] 1. 两M差集准确：chain.sh和parse2gen/gen-delta.sh，均100755不变，无成员增删。原JSON与git blob SHA全等；inv_rc=0已落盘，五其他目录成员/戳与基线相同。测试与gate注册改动在K5-1i四文件域内，未夹带审核键。
- [x] 2. chain语义窄且可接受：只将parse2调用改为bound60 env非空私有DIR sh helper OUT，另加两注释。pp→lex→parse2顺序保留，同T/.seed-gen-cache，继承DIR被覆盖；helper错误仍明确E3 gen failed/exit1，之后tbl/net/probe循环不执行，无静默Python回退。新的e3-gen.out/err保留诊断于scratch，随原EXIT trap删除，不宣称永久日志。SG0仍显式参考，SG1复用可校验binary；显式BIN的既有旁路保留。compilercheck实际调用elf；elf可skip models，models冷分支仅subprocess prepare；prepare全文没有chain调用，helper亦不调用chain。这条具名生产链不存在对chain的间接调用，不对所有动态环境/全仓调用穷举下结论。run1受控证pp0→parse2红3/2、chain1和shim范围停点，不能外推真实exec-chain gate预算已绿。
- [x] 3. helper仅Consumers头注释增加chain，去掉注释后的源码与基线逐字相同，参数/key/sha/退出路径无逻辑改动。沿用此前parse2gen真审的并发、sidecar两次mv/校验到exec竞争、16位key、工具子链和RSS未知限定，不因本注释审宣称已解决。
- [x] 4. 已知限定接受并单列：exec-chain1–5 guards全为ae17a951旧值，从K5-1d起漂移，此次不能靠reviewed.exec新戳掩盖或恢复guards；不代刷。真实exec-chain预算未测，未证明净提速/产品总体覆盖。UA未过shim，calls_after仅shim观察。当前driver首红零启动未演示，0727其他driver不互证；ps-g虽采rc1仍非严格PGID/树清空，TERM未测；post HEAD/status/UA为事后采样。P1内存准入UNKNOWN不改写，本审不授权补flag测峰。
- [x] 5. 只读独立候选复算：git ls-tree -r -z 848bc986 -- exec、按该提交compilercheck声明suffix/exclusions筛选、cat-file blob字节SHA和执行位归一，再json.dumps(sort_keys=True) SHA256；1412成员与inv-848bc986.json逐成员完全同。候选全长828a6f67417de1998627ef73d1346ff9709ef3cccd1cb7f2ebd904baf09a4775。记录键仍1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825，不写gatedeps、不执行条件式单键。

此次真审只接受这两项内容delta，非审批写键、guard修债、跑门、bump、Draft或扩大验收。只读重算未调用仓内执行脚本。

PASS 可进刷戳裁
