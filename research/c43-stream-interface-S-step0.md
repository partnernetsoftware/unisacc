# S 第0步：显式阶段流合同与 E1→parse2 首见证

**只是设计**。2026-10-15，cdx2。未实现新接口、未执行下述探针，不声称产品或复用资格已获证。仅提交本研究路径；不修改 cdx 暂存、prd/spec、2c1 产品文件，不重建/resume7000，不动发布态。

## 决策与依据

S先交一个可回放、可证伪的阶段合同，而非把阶段函数改名成iostream。依据 plans/v0.0.43.md「范式层」、prd.md「流推导的复用单位是阶段合同」、research/c43-plan-proposal.md步骤0/1。本刀选**产品 E1→E3/parse2**边界：实际路由阶段名 e1/e3，不虚构一个独立parse阶段。E1产物是完整UNITOK定位帧，E3还消费属性、逻辑源名及具名资源；调试dump-tokens不能替代它。首见证只取 program/lnx/x86_64/O0，库/外部合同另列，不抢2c1。

现有 tests/middleflowrunner.c:27–30 已按产品路由采E1输出、attributes、source-label并回放单模型；它是起点，不是新合同已经实现。exec/c/run.c:610–635仍可从RI、包资源、磁盘依次取得资源；第0步测试严格回放应堵住最后fallback，后续默认产品严格模式另授。run.c:669及:760–764说明属性侧流属于阶段间输入，不能只冻结主流。

## 最小阶段合同 SC1（字段定义提案，不另造语言阶段）

首刀仓外记录可用严格TSV清单＋独立bytes文件；统一二进制传输格式后置。关键字节用长度＋hex编码，不用C字符串标识可能含NUL的key。不允许重复字段、未知必需字段或悄悄忽略版本。

|字段|最小定义与约束|
|---|---|
|contract_version / stage_id|SC1；逻辑消费者parse2，实际stage e3及明确路由；版本不支持为接口故障，不猜降级|
|input_format / output_format|格式名、版本、完整字节长度与SHA256；输入绑定真实UNITOK版本，不自行猜魔数尾号；输出绑定本路径实际Tape/封套格式。识别不了则停止，不拿调试文本顶替|
|target / mode / route_policy|显式目标lnx/x86_64、program、优化O0及影响该阶段的能力值；不从uname或环境推断。该阶段不用的字段也须注明“本见证固定，未证明可省略”|
|source / locations|命令行语义逻辑名作为bytes，遵守现行argv原样裁定；物理采证目录不进入核心身份。UNITOK位置/源映射整体保留，引用表须相容，不重写或用plain流冒称located|
|attributes|格式/版本、presence、长度/bytes/hash；PRESENT_EMPTY与ABSENT不同。使用产品实际attr_strip结果，UNITOK与属性必须来自同一次E1提交|
|resources|完整显式key→PRESENT(bytes,length,hash)或ABSENT；还记录stage请求序列/响应摘要。每个请求必须匹配一个条目，没有条目为MISSING_RESOURCE合同失败，绝不能等同ABSENT|
|model / rules|消费者模型字节及格式、规则/构造来源身份；模型哈希绑定合同实例。有限表与执行器身份各自记，不能模型相同就推定执行器相同|
|state_lifecycle|每次调用fresh状态；缓存只能复用已核不可变资源。主流/属性/诊断/模型与资源表在调用期间不可变，响应缓冲至少活到该调用结束；输出接受后原子提交|
|result|分类、阶段所有者、具名原因、原始status/rc、主输出、位置/属性输出、诊断bytes。正常拒绝不发布成功主流；stderr不自动等于拒绝|
|execution_evidence|执行器/build/candidate身份、观察工具身份、显式测试步数预算、墙钟上限、请求轨迹；与核心合同身份分账，仍绑定本次结果，不授摘钩|

资源表不是一次观察到的key名单就算所有输入域完整：首见证只能证明冻结fixture的实际请求完整；泛化需要控制分支负例与规则审计。相同key重复请求必须返回相同presence与bytes；若捕获中变化，拒绝封存，不取第一次掩盖。RI/包资源重名优先级在采证适配器解析成唯一响应；严格回放不再重新查宿主优先级。未请求的额外资源记录为未消费，不能声称每次删任意资源都应红。

## 状态与失败出口

1. VALIDATE_FRAME：只核协议、版本、引用相容和长度界，不代写token/parser语义。
2. RUN_MODEL：通用执行器调有限表，按显式资源响应表取值；新请求无记录即停止。
3. COMMIT：接受才发布完整主流＋位置/属性。内部可流式推导，首刀不外发“成功前缀”。
4. RESET：任一失败清本轮候选输出，不能把前次成功快照当本次结果；失败诊断作为独立输出保留。

结果分类：ACCEPT；阶段具名DOMAIN_REFUSAL/UNCOVERED；INTERFACE_ERROR（坏帧/缺响应/版本）；MECHANISM_ERROR；STEP_BUDGET；HOST_TIMEOUT/INTERRUPTED。探针可用rc0接受、rc1阶段拒绝、rc2接口/工具故障，步骤预算/信号/墙钟另附原因与原始退出状态；不是拟统一产品现有rc。超时/崩溃不算拒绝。上游接受只证明该阶段域，不保证下游目标/ABI可接受。

ABSENT是显式合法资源响应，不强行由适配层作语言拒绝；模型可能据此拒绝，也可能走规定默认。EMPTY是存在但0字节，必须与ABSENT做差别控制。漏响应探针必红的意思是合同验证失败，不能伪造成产品拒绝绿。

## 拟新增探针与负例（尚不存在/未跑）

拟入口 `tests/stagecontractcheck.cx`，严格回放机制可在 `tests/stagecontractrunner.c`复用middleflowrunner，但首刀不改共用产品run.c。fixture提议 `tests/seals/stagecontract-e1-e3/`：由同一产品E1采完整UNITOK/attrs/source与e3实际资源；记录所有响应，不预填PASS。若现有middleflow记录版本不明先补身份，不重新调用整闸。

|ID|输入/操作|期望与证伪条件|
|---|---|---|
|S-POS-01|冻结合法UNITOK＋实际完整侧流＋模型|新严格回放与同输入原适配器输出/诊断/位置属性逐字节同；执行器见证范围具名|
|S-POS-02|换cwd/临时目录/未授权环境，核心输入相同|输出与请求轨迹相同；不得读观察目录补资源|
|S-NEG-RESOURCE|删除一个**POS实际请求过**的响应条目；在磁盘放同key诱饵|MISSING_RESOURCE具名key，rc2，无成功主流；一旦仍rc0即合同红。禁止构造假key后自称漏依赖已测|
|S-NEG-ABSENT|把同条目显式设ABSENT|请求被记为已响应，结果依该模型合同；不得报告MISSING_RESOURCE。必须有预测依据，否则UNKNOWN待补，不能预填拒绝|
|S-NEG-EMPTY|将条目改PRESENT_EMPTY|严格保留presence；对比ABSENT。若阶段不区分两者也允许结果相同，不能捏造语义差别|
|S-NEG-DUP|同key重复且响应不同/捕获重复请求不同|接口具名拒绝，不能取首项掩盖|
|S-NEG-FRAME|UNITOK版本不支持或属性长度/位置引用不相容|接口故障；不进入模型，空主输出。格式正确但语法不合法另作模型拒绝控制|
|S-NEG-RESET|一次接受后一次具名拒绝，再次接受|拒绝无前次成功残留，后一次与fresh状态相同|
|S-NEG-MODEL|换模型字节但沿用旧身份/hash|合同不匹配即失败；不复用此前结果|

位置消费语义负例需要格式合法但映射错的UNITOK及固定逻辑名oracle；它属于之后独立located扩展，首刀完整携带位置不能冒称已测所有位置语义。两候选提取模型相同只能证明模型一致；用同一个共享执行器跑不代表两候选各自执行器已获证。

## 文件域、分步预算与首红停

本次只写此研究稿并默认commitgate，不改其他路径、不push。后续实施拟授权域限上述两个tests入口及新fixture目录；现有middleflowrunner仅在单owner同意后必要改动。产品严格模式另刀申请run.c资源入口和组装适配器，不顺带改compiler/librarylatebind/relocate、prd/spec或gatedeps。

|步骤|上限|产出/停止条件|
|---|---|---|
|读取版本/资源协议与冻结身份|≤20s|无法明确版本即停，UNKNOWN具名|
|构建私有合同runner|≤55s|生成准备不计验收；冷准备超时另拆，不能挤进回放批|
|单fixture采E1＋e3轨迹|≤30s，各内层≤15s|入口/输出/status原样保存；首个未预期红停|
|一例严格回放|≤10s；每批≤50s|逐字节/请求验证，不合批准备；对每类NEG只取最小见证|
|负例与生命周期控制|每例≤10s、每批≤50s|未预期接受/诊断/残留任一出现即停，不扩预算硬绿|
|登记及默认commitgate|单命令≤58s|授权后同笔登记；超时不计绿，保持路径提交|

实际队列可能按窗口剩余时间截断，58s只是绝对上限，不是每job获保证预算；正式排队须给准备/回放分作业并留窗口余量。此稿不启动正式调度、不resume历史队列。

## 与 W 可共享的抽象／不得合并的语义

可共用：版本化字节视图与bounds检查；显式presence/length/hash；不可变请求合同与实例响应；调用状态机与接受后提交；具名失败封套；快照生命周期；资源轨迹与证据身份。W的声明/分区/未绑定片段/收据也能使用同样的封套、提交点与借用规则，不为每kind复制机制。

不能共用成隐式大阶段：S的token/Tape/目标语义由模型拥有；W的provider取得、extent/权限核验与按span地址实例化归绑定层。provider地址/handle只在实例里；不得作为S核心资源输入。共享协议解码不等于共享全部拒绝规则，也不把签名/layout推迟至宿主解析。W先固化、fixed typed首复用者、S实施随后，遵守现有刀序。

## 交付判据与明确不授

首见证完成须交完整fixture字节和哈希、实际资源/请求轨迹、正常拒绝与机制失败分类、漏响应+诱饵必红、生命周期负例及命名执行器身份。不以动态C包装Python完成必需核验：.cx承担判定，参考Python若使用只作可关闭oracle，不能成为严格产品路径。

本稿不授产品严格模式默认切换、不授资源摘钩/结果复用/早段免闸、不证明全语言/ABI完整性；不删除Python、不扩Windows、不做全链或新整闸、不改发布材料。后续门槛是可核验的阶段合同证据，绝非仅函数统一命名。
