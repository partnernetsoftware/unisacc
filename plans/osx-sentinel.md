# macOS 增量哨兵

入口每次只推进一步；由cc定时触发，不安装后台轮询。读取origin/main后冻结完整提交，在私有快照运行动态osx清单及smoke。原入口14批，随着注册表扩展动态增长；不通过固定数量或截短新批次保留旧绿。

## 一次准备与周期命令

先在Mac上用已装产品编译两份原生控制器（每条≤60秒）：

```sh
tools/bound 60 sh ./unisacc.com tools/osx-lane.cx -b osx/arm64 -o out/osx-lane-native
tools/bound 60 sh ./unisacc.com tools/osx-sentinel.cx -b osx/arm64 -o out/osx-sentinel-native
```

初始化一个不存在的STATE，显式冻结bootstrap、MODELS和lane native/source；这里用本机实测f069载体作为两项输入，并不将他机记录的3d0e当成本机文件：

```sh
tools/bound 60 out/osx-sentinel-native init /Users/wjc/repos/unisacc /Users/wjc/repos/unisacc/out/osx-sentinel-state /Users/wjc/repos/unisacc/unisacc.com /Users/wjc/repos/unisacc/unisacc.com /Users/wjc/repos/unisacc/out/osx-lane-native /Users/wjc/repos/unisacc/tools/osx-lane.cx
```

cc周期触发的单条命令：

```sh
tools/bound 60 out/osx-sentinel-native tick /Users/wjc/repos/unisacc /Users/wjc/repos/unisacc/out/osx-sentinel-state
```

init/tick使用实际原生控制器，不以默认-run的bootstrap身份代替controller字节。STATE绑定controller、lane、bootstrap和MODELS完整SHA；任一版本变化需全新STATE。`init`仅PREPARED，构造和材料准备也不计测试通过。

## 判定与输出

0：没有待办或完成，空闲与完整绿均静默；绿只在所有选中批次真实终态及末端重核完成后向共享`agentwork/ledger.tsv`追加一行，重复tick不再追加。

3：PENDING，一次阶段已推进或剩余时间不足以给子步完整窗口；下次继续同一命令。FETCH与WORK分开，fetch不会抢55秒测试窗口；不在进程内等待远程更新。

1：已证同材料绿base与红head，并逐步二分得到首坏，向`agentwork/items.tsv`追加`OSX-REGRESS-<完整SHA>`，保留观测head、失败批次、最小重放argv、原始证据及负责车道。

2：UNKNOWN、身份/材料/协议拒绝或不可比。没有可信绿base时的红只记BASELINE-RED，不编造first-bad。registry/harness/选择集/模型输入不兼容时不二分，旧候选的PASS不能跨这些边界迁移。

中途崩溃的半成品不自动当PREPARED或PASS；同态再次拒绝时，保留旧STATE及日志，用新的不存在路径（例如`out/osx-sentinel-state-02`）重新init，再将周期命令指向新STATE。它会重新建立绿base，不承接旧STATE的结论。

## 证据边界

新HEAD源驱动由固定bootstrap和显式外部MODELS构造，本入口不是同源NN/完整T1或发布资格。参考原生构造链是`tools/osx-chain.cx`，日常清单执行器是`tools/osx-lane.cx`，两者不混称。

lane每次执行一个原有阶段或完整批次窗口，结果保存stdout/stderr/实际wait状态/verdict四文件。终态只读`--verify-queue ROOT STATE MAP`重算完整选择集、tuple、材料、原始批次终态与哈希；sentinel还独立核验收据，不只信stdout marker、外层退出码或伪造的reaped字段。受控协议夹具只证明流程与拒绝合同，真实日常批次证据另列。

私有克隆/git archive不切换共享main；共享写入只有最终ledger/items追加，不自动提交、push、安装产品或修改发布资产。开发交付的提交与推送由集成者按路径完成。

队列实现第一阶段验证：源ed33f38f…、native1fac17ce…；原default14和scheduler30保留，新init12、完整单job PASS/FAIL各12及恢复重核各1全部实际退出0。最初把恢复末重核合成13项导致两次55秒超时142，原日志保留；按12+1拆成独立步骤，断言未删且预算未提高。主树正式`osx-lane-queue`为12/0/PASS；这些都不是24批全绿。实际冻结ae1e06的动态24批独立推进，终态另记。

控制器最终交付：主树六正式批全部实际PASS/rc0/transport0，总51/0，prepare16、execution10、bisect10、UNKNOWN5、伪wait5、无base红5；最终十三闸13/13。日志`out/osx-sentinel-dev/formal/`，source SHA2b3b4adc…、check SHA b6a5ce7c…、native SHA e43dce3e…。既有`cxlib.toolselfpath`通过宿主能力读取真实进程映像；独立SOURCE入口探针也实际exec同版e43缓存，并非把bootstrap当controller。周期命令仍显式native以保留完整时间窗口。所有证据限工具协议/本机开发范围，不迁成当前主树全批绿或同源NN资格。
