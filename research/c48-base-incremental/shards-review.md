# BASE6 compare分片/report只读复核

结论：在父已声明的“同一冻结、无写者窗口中新鲜运行compare→report”条件下，未发现会让本次演练确定性假VERIFIED的实现漏洞。三产物实际比较及全键收据绑定没有被stat替代；stat只用于后续汇总阶段。接受范围应写`VERIFIED_FROZEN_RUN_SHARDS`，不能扩大成跨run可缓存PASS或抗并发篡改的证明。未重跑大比较、未改任何输入。本复核也没有替父确认42个compare/report进程最终退出。

## 核对身份和小证据

只读源码 `tools/net-incremental-rehearsal.cx` SHA256 `acb0e342fe0315521f147398b3948e5985c84d7661c6f76f5db8fe4ff579188b`；执行者 `out/c48-base-incremental/rehearsal-v2/controller-shards.exe` SHA256 `1e53e3040606b68fdd746d20dba25fb2c114e1d5ce1a4f38a464d4a63e5d2104`。

实读重建part00与复用part01，window均为`7472408abe39689ec509196cb39d3cdd9b8e6aeee28f18ba0c05da666f90f55a`。小型out/.cx独立审计实际session13516 rc0：计划42个顺序index、42唯一网名，BUILD3/REUSE39；两part正文SHA/完整尾及同window匹配。

| 材料 | 字节/正文SHA | 整文件SHA256 |
|---|---|---|
| compare-00.tsv |4448B / 995e269cdfdf2b9a82cd589abbe43ef1ea9c978dd439789c9af41a9fac0b791b|eb8211ada8d8c498e955654f83b606e68669ecf8b4fe07116cea7cefd633de96|
| compare-01.tsv |3413B / c3b1092db982a910ca004e27ff47cdfbd6a8096c508c546252fcb657962d2fa5|cd05498a8d105252d42561a2012ec0bf8e1ceca37f81d944bd1fe48c1bf141a4|

诊断源码`out/c48-base-shards-small-audit.cx`，成功日志`out/c48-base-shards-small-audit-v2.log`。首次session92909 rc2因仅调sy_sha256_hex宿主接口失败，加入与被审工具相同的cx_sha256_hex后备后成功；原日志保留。只读取计划与两个几KB part，`artifact_content_comparison=NOT_RERUN`。查看part文件数时仅已有28份，未把这次观察写成42份全部完成。

## 合同逐项

1. **真实三产物比较**（源码59–66行）：逐gen JSON/tbl/net读取左右文件，核长度并memcmp全内容，双方分别SHA256，再核SHA相等。复用39项还核左产物SHA与baseline-index记录相同。不是仅比文件大小/时间，也不是相信旧摘要相同。
2. **profile绑定**（40行profileproof、63行）：每侧须有完成的单模型profile，且恰一个NET行、id正确、rc0；gen/tbl/net SHA等刚读到的实际产物，取对应三个相位时间。完整全量侧42项均核；变更3项另核增量侧。非变更左侧是历史baseline，65行重hash其profile并与baseline-index的profile_sha核同。
3. **全键绑定**（37–38、63行）：requirevalidation重新hash实际tbl/net/checker.exe，并核validation-bind中的三SHA；重新hashfull-key原始输出并核receipt_sha及rc0/claim。checker实际源码逐状态、byte域0..256或reg域-1..topmax比较next/sequence，还比较头维度、字符串与动作声明（`tools/lab-loader/net-check.cx:64–136`）。part00 full-key实际为1105532 observations/4286 states。变化3项两侧都验证；复用39项由全量右侧全键验证+左右tbl/net字节相同传递，不需要把baseline旧PASS冒充本次新执行。
4. **window**（48–52行）：canonical RUN路径、before/after/plan/baseline-index/material/mutation、两个cache收据和实际profile/checker.exe均内容hash；cached还重新核当前两工具源码、当前产品和exe与cache身份；再加入当前controller源与当前self_exe SHA。plan.before/after指向实际snapshot文件SHA。因此另一run路径、不同control源或exe、不同plan/cache都不能直接混用part。它绑定的是这些文件的字节，不是每次重新遍历整个RUN/root源树。
5. **report集合**（69–79行）：固定读compare-00..41，逐一绑定index、计划网名、mode与window；排除重复proof inode；严格左右各gen/tbl/net顺序和期望路径，六个stat、左右声明SHA相等；按预期6或10条META顺序重新contenthash所有小元数据；核十项计时schema、复用项增量时间必须0；核body_sha、index/artifacts/metadata计数/完整尾且无尾随记录，最后再次核proof文件stat。只有42项全部走完才exclusive save最终报告，不会少读最后一项仍输出VERIFIED。
6. **42唯一的精确边界**：报告严格42不同index/proof，网名唯一性由`plan()`调用digests时的查重继承；`model()`及report没有再次独立做网名集合查重。当前plan已由本轮小审计证实42唯一，故本次不构成缺项。若以后把report作为不可信外部计划入口，需重新验证计划完整schema/网名唯一，不应沿用本次生产者可信前提。

## 边界与后续收紧项（不阻断本次冻结演练）

- report对ARTIFACT只核dev/ino/size/mtime/ctime，**不再次内容hash**；META会重新hash。秒粒度mtime/ctime不足以识别同一秒内原地等长改写；从compare结束到report期间若存在写者，不能声称仍是当前内容证明。代码注释明确要求RUN不可变，父当前使用独占冻结路径，符合其条件。不能把这个证书移植到跨窗口增量缓存或任意外部文件。
- Part bodySHA是内容完整性，不是签名或执行证明。报告不重新执行profileproof/requirevalidation的全部语义，而相信同window实际controller新生成的part；有人可改正文并重算SHA的对抗场景不在合同内。依赖检查应保留“新鲜生成/无他人写入”的流程证据。
- compare在读产物前后有stat检查，但metadata先验证语义、后hash/stamp，report读part前后也仅秒级stat；这些不是并发文件系统事务。源树ROOT也不每次重hash。冻结窗口若失效，应重新建立身份并重跑受影响compare，不能以bodySHA补救。
- profile/checker缓存核主源码SHA、产品SHA及实际exeSHA，不独立证明exe编译时所有导入模块的源闭包。对本次冻结且实际执行的exe身份足够，不能额外宣称来源闭包全部重算。
- 若独立审计不可信receipt，`profileproof`完成标记和requirevalidation的NET_CHECK标记目前为substring级，elapsed仅抽elapsed_ms/rc0，不逐项再校phase/index/id。当前receipt来自冻结受控profile/checker及batch生产者，part又绑定其实际内容，不见当前假绿路径；未来通用离线consumer应改成完整schema解析。

## 计时应怎样说

源码82–85行batch的start在身份校验之前，build_ms含配置/构造器hash、cached身份校验和实际profile子进程；validate_ms含checker身份hash、实际全键检查、full-key输出保存以及validationstamp再次hash tbl/net/checker。profile中的gen/tbl/net时间是其子作业时间，不能和上述包含关系重叠相加。

report仅把part的十项时间累加（75、79行），不是14路并行任务墙钟。增量39个复用项时间明确0，是“未新构造/未单独新增全键作业”的计账，不表示复用验证零开销。prepare、mutation、before/after摘要、plan、缓存编译、全部compare的memcmp/SHA/收据验证、report汇总、调度等待没有纳入build/validate总计。因此可以比较“3项实际构造累计成本 vs 42项实际构造累计成本”及相应全键作业成本，不能把它直接写成完整增量流程墙钟加速比。若要端到端收益，另列准备/调度/compare/report实际墙钟及cold/warm条件。

最终判断：分片解决了单进程重复读取GB级payload超过deadline的问题，没有把本次实际字节检查改成旧结论缓存。当前可接受结论限于同一冻结窗口42项产物相等、对应table-equals-network全键收据内容绑定；不等于表与语言语义独立oracle正确，也不自动包含端到端加速证明。
