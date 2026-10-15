# S 第1步：产品资源流严格模式

**只是设计**。2026-10-15，cdx2。未改产品、未构建/执行候选、未登记作业。本稿依据 research/c43-plan-proposal.md步骤1、S-step0设计cfb38f3e及回执31ff0800/f1076033；实现须另钉文件域与窗口。不动W/2c2在途文件、7000历史、Draft/Latest/签/公开。

## 1. 推荐：先显式新CLI/合同开关，默认路径后置

建议非稳定诊断入口 `--strict-resource-flow --resource-ledger FILE`，显式开关由参数传递，不用环境变量。开关缺ledger或ledger畸形即具名失败，不悄悄进入兼容模式；无开关保留现有CLI语义，两模式证据分账。CLI语法/目标范围待cc复核，不视作已稳定公开API。

首实施限定program、lnx/x86_64、O0、C→Tape（-S）一条路径；最终每个实际请求都必须显式响应。不以已有e3单阶段fixture冒称整条C→Tape资源完整：实施先在隔离测试适配器采**该完整路径**请求与阶段界标，再固定见证。入口源文件与ledger文件可以由宿主适配器先读字节，核心模型请求不得再查磁盘。读取显式输入不等于磁盘fallback仍获准。

推荐依据：S-step0只证明一输入、一e3路由；ABSENT/EMPTY结果预测仍UNKNOWN，capture重复请求未测；现行产品与库路径依赖RI及包资源，直接默认严格会把未审闭包缺口变成广泛拒绝。显式模式先给可证伪产品路径，再逐路扩展；默认切换须完整资源清单、旧新行为对拍、CLI/库兼容与正式新候选证据，另授，不由一个样本自动晋级。

## 2. 统一ledger语义，机制共用而非复制回放器

资源身份为 `(key_length, key_bytes)`，允许NUL，不能用strlen。语义三分：

|回答|表示与消费者出口|
|---|---|
|PRESENT|存在，长度≥0、显式bytes/hash；0字节是EMPTY，仍返回存在的blob|
|ABSENT|ledger明确声明没有该资源；长度0、无blob，消费者按该阶段规则接受/拒绝/默认，不强制模拟语言错误|
|MISSING|请求key未在允许的ledger闭包中；具名 `MISSING_EXPLICIT_RESOURCE <keyhex>`，接口失败，禁止磁盘/包资源暗补|

现有回放resources.tsv五列 keyhex/presence/blobname/length/hash 可保留兼容，但新公共解码必须严格验证presence只能0/1、长度无符号且有界、hash和blob字节一致、ABSENT字段规范、无重复key/尾随垃圾。当前middleflowrunner.c:24使用atoi，不能直接复制成产品合同解析器；S已有DUP负例是入口之一。重复请求返回同presence/bytes；捕获发现同key变化则拒绝封存，不只取第一次值。允许未请求的额外ledger条目但记录未消费；“漏资源必红”只针对实际请求过的条目，不能捏造假key。

产品和回放器应复用一个小型ledger结构与解码/查找实现（拟exec/c/resourceledger.h），都使用同样的键、presence、长度界和诊断；回放器的观测trace另有标记，不进核心身份。不要求把tests捕获器或Python移植进产品；捕获只是仓外观察工具。

## 3. RI→包资源→磁盘的严格出口

当前exec/c/run.c:610–635：先RI，再包RES/PB，最后path/INCDIR读取，返回1借用、2宿主分配、0不存在。exec/c/core.c:189–191消费0/1/2并缓存资源。这里的0/1/2是**存在性/所有权**，不是新的错误分类；不能把MISSING伪装成0（ABSENT），也不能为错误新添-1后让core误当存在。

严格模式逻辑：先查**声明完整的有效ledger**，再从其中确定供给；RI与包资源只是PRESENT字节的取得位置，不是对缺项的隐式fallback。可采用外部ledger显式引用包内条目（包hash＋key＋valuehash），由适配器组装为同一响应表；不允许“包里能找到就无需声明”。既有完整包清单可成为显式输入的一部分，但必须列出规则与身份，不能只记候选SHA掩盖模型的资源读取。

ABSENT条目应遮蔽同key的RI/包资源/磁盘诱饵；EMPTY应返回存在的0字节blob。重复供给先在组装层按明确规则规范化或拒绝，不让到处存在的“首匹配”改变合同。外部ledger、包资源清单与显式CLI/属性供给须记录各自来源，冲突不能静默覆盖。

MISSING通过现有CLI/库错误传播机制停止本轮（具名接口诊断），不要在core_host_fetch里exit终止嵌入库进程。本刀新CLI对外拟rc1＋空成功输出；S回放/合同工具仍可用rc2表示接口失败，两者记录统一失败分类和key而非强求全产品统一rc。模型自身领域拒绝/未覆盖仍归模型，机制故障/步数或墙钟超时不得冒充拒绝。未提供严格库接口的旧库路径保持兼容模式，不声称已迁移。

## 4. 状态、属性与提交点

每个阶段调用绑定不可变ledger快照，fresh资源cache。run.c:691及:762–768会更新ATTRS/RI，故不能把全编译期间同一key的可变属性冒称不可变响应：阶段结束后显式提交属性输出，下一阶段得到新ledger版本/调用ID。清缓存和换ledger必须在明确阶段边界，不能既沿用旧cache又替换bytes。SOURCE保持当前argv原样逻辑名裁定；物理采证目录不进入合同。

资源值借用至少活到本阶段结束；失败清新输出与临时状态，旧快照不得冒充本轮成功。主输出接受后提交，缺key时不截断已有-o文件、不发布前缀或可调用镜像。资源适配失败有独立stderr/status收据，不让上层generic理由覆盖具名key。阶段合同流与宿主路径/读取轨迹分账；禁止把provider地址、handle作为本刀资源闭包的一部分而宣称核心纯化。

## 5. 文件域与W/2c2避让

拟实现域：新exec/c/resourceledger.h；exec/c/run.c的资源组装/查找及阶段cache边界；exec/c/compiler.c仅非稳定CLI解析与模式/ledger传递。必须在W/2c2接线笔提交、接口身份固定后由单owner申请窗口，不能仅靠不同函数名认为无冲突。若共用执行器文件被其他hex/VM刀占用，暂只落ledger合同与tests见证，不抢run.c。

不动exec/c/librarylatebind.h、libraryrelocate.h、libunisacc.c/.h、librarypartition/libraryfn/librarydata模型、prd/spec；不顺带迁移ABI/provider或改包协议。若打包声明新资源必需改compilerpack.py/seed对应实现，先另报实际路径并自举一致性，不默认获授。

tests拟域：tests/resourceflowcheck.cx、必要独立测试适配器，以及middleflowrunner.c只把ledger解码/查找换共用组件；不得扩大其观察资格。gate.sh/gatelayers/gatedeps/refresh/INVENTORY与derived-map在单独登记窗同笔同步；新header落产品闭包，需要新鲜候选与具名suite，而非沿用83e6e7e4新鲜度。

## 6. 拟resourceflowcheck合同（尚未实现/跑）

.cx判定实际PRODUCT严格模式与回放器的状态/bytes/请求trace，参考工具仅作独立oracle，不用.cx包装Python成为产品必需执行器。

|ID|操作|可核预期|
|---|---|---|
|RF-POS|同源/目标/模式与完整显式资源，新旧适配|主流/位置属性/诊断逐字节同，严格调用无磁盘fallback；不要求整个候选字节相同|
|RF-MISSING|删一条完整路径实际请求过的响应；磁盘/包放同key诱饵|产品具名MISSING接口失败，回放同分类与key，无成功输出/已有文件不变；诱饵不得救活|
|RF-ABSENT|同key显式ABSENT且磁盘/包存在诱饵|返回absence，不报MISSING、不读取诱饵；模型语义结果先由该fixture规则预测，否则UNKNOWN|
|RF-EMPTY|同keyPRESENT_EMPTY|请求trace存在且len0，不能变成absence；允许该输入的输出恰好等于ABSENT，不能因此说二者同义|
|RF-ENV|换cwd/include搜索目录及未授权环境/新目录资源|显式输入不变，输出及请求序列不变；source argv若改变须分账，不捏造该变化无效|
|RF-DUP/LENGTH|重复key、非法presence、负/溢出长度、hash错|入口具名接口失败，空成功输出；不走not-covered/兼容fallback|
|RF-STAGE|前阶段产新属性，下一阶段换ledger；删旧attr或偷用cache|新属性严格按新调用身份消费；无旧成功残留或旧资源cache顶替|
|RF-RESET|成功→缺key失败→成功|失败不发布成功产物，第三次与fresh状态逐字节同|
|RF-LEGACY|无严格开关、合法控制与旧CLI对拍|原兼容行为不变，仍单列未迁移磁盘义务；不得将strict失败悄悄重试legacy计绿|

已有S-step0的ABSENT/EMPTY同输出仍是单fixture观察，不能预填新正/负语义。模型身份仍整文件hash，f1076033只发现本输入未进入末态，不授投影为“被消费字节”；另输入可能进入该状态，不能拿RF结果缩身份。

## 7. 分步预算、首红停与实施门槛

每条命令≤58s，每个内层有独立看门狗；正式队列窗口可能更短，58不是保证job预算。

1. ledger协议/解码与差分单元（tests/private）：每例≤5s、每批≤30s。先跑presence/dup/missing，不开完整链。
2. 一条C→Tape资源轨迹捕获：≤30s，内层≤15s；捕获失败停，不能捏造全集。
3. 对完整路径单fixture严格回放：≤10s、每批≤40s。RF-MISSING首个被救活或输出差异即停，交rc/key/bytes/阶段身份。
4. 产品组装与构建：冷准备分为独立≤55s步骤；不与验收合批，超时HOST_TIMEOUT，不重启整闸、不扩大预算。
5. fresh候选RF黑盒：每例≤10s、每批≤40s；随后受影响stage/pipeline义务独立批次≤50s。不以单fixture授默认路径。
6. 单owner登记/commitgate：≤58s；derived检查同步生成身份，新header/输入未登记则拒绝提交，不临时绕门禁。

同一失败不无限重跑：先保原始输入/资源/rc/诊断/工具身份，按接口错、领域拒绝、未覆盖、机制错、预算/宿主超时分桶，再讨论窄修。默认切换需要完整目标路由资格与兼容声明；本设计不授它。

## 8. 复用目标与非目标

可复用的是执行器＋有限表＋显式流接口：资源ledger是通用字节机制，stage规则拥有absent时的语义；W按kind的封套/快照/接受后提交可提供生命周期机制，provider绑定不混入这个阶段。禁止mega-stage、统一所有错误码、模型语义手写进resourceadapter、宿主oracle进核心身份。签名、布局与保留根仍在应有阶段显式已知，迟绑定只实例化地址。
