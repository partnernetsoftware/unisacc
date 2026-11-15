# BASE-6：真实三网增量与42网独立全量对照

结论：在同一冻结RUN窗口内，真实语义微改只使3/42配方摘要变化；只重构造这3网后，与改后独立全量构造42网逐JSON/TBL/NET字节完全一致。全量侧42网、增量侧3网均实际执行完整table==network状态/键穷举，未复用旧PASS。最终分片报告实际退出0，状态为 `VERIFIED_FROZEN_RUN_SHARDS`。本结论不是跨run PASS缓存、并发/ABA保证、独立语言语义oracle或端到端墙钟加速证明。

## 实际结果与计时

| 累计成本（ms） | 增量3网 | 独立全量42网 |
|---|---:|---:|
| build整步 | 12067 | 175355 |
| 完整全键验证整步 | 2573 | 21571 |
| 构造器gen | 2369 | 26522 |
| 构造器tbl | 288 | 4998 |
| 构造器net | 117 | 1947 |
| 三构造器合计 | 2774 | 33467 |

build/validate整步含身份校验等开销，三个构造器时间包含在build中，不能重复相加。以上为各作业累计时间，不是14路并行墙钟；prepare、摘要、调度、compare、report、缓存编译及冷暖条件未计入此表，不能据此直接声称完整流程加速倍数。两份原始summary保留 `full_key_validation=NOT_EXECUTED`，这是构造器计时工具自身的正确范围；真正全键证据是每网独立的full-key及validation-bind。

真实改动只在独立副本 `exec/pp/location-byte.tsv` 的最后一个REJECT字符串：`unreachable`→`BASE6-probe-unreachable`。共享表未改。完整输入审计1548文件，恰1变，结果EXACT；私有表SHA由9990f7e3ab39ae4f3a709e46fe13294ead88596bd530efda9d5ab05217c8340a变为fb7e7530a2059de90102d9b1c3e6ec987f659d2e791265a3f47df65dd2f779a9。42摘要仅index0 shared-e2、24 tokenpp、25 warnpp-shared改变，39网不变。before整体SHA e64c7740d26ad35fbabbef27778293f9532bc262f5199093b4be834fe0842150；after fa23ff5bef7adcb5bceafc30241fa676d78163bd29f8523bbf191d48caf1d93c。

## 执行证据与身份

增量三网build会话69322/53670/97037均实际exit0；完整全键直接终态chunk88d485/a70d65/1f94c3均exit0，分别1105532/247682/1107338 observations，4286/961/4293 states。父持有句柄并逐一poll确认42full（每项build+全键）和42compare全部exit0；比较没有重建模型或重跑全键。每个作业外bound50秒，内部子进程不超过40秒。

原controller源码99c0c86dff0d4c8bea7b95e1071df6251bc1a6131a7f87fb7ab9cbae9fe9abde，native fc67c01d1ead6123e6b31bcd04cfadaed4a6c068142c17d23f2fa40d89b7ef87，负责构造/全键阶段。分片controller源码acb0e342fe0315521f147398b3948e5985c84d7661c6f76f5db8fe4ff579188b，native 1e53e3040606b68fdd746d20dba25fb2c114e1d5ce1a4f38a464d4a63e5d2104，负责compare/report阶段。两者历史不混写。

profile源码f3a78610ea5b06a843f217ce9cccb502c643bfd3f5756b314a557af946f90bf8，实际exe49f845b5d7143c1ce12fe978ece621633641b74c52f3e717fb00e86d64383478；checker源码8efaeec8d9965993b81dd30f51edec61503f53ce5b63ab760c3af3ecc69a3379，exe edbd94d89c7b11a9e7df01c4fb489ef4c466a8c1c67a5a0786a17e4eb7124b11。实际产品候选f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82。本次编译缓存只缓存身份绑定的可执行文件，不缓存PASS；profile/checker分别实编10367/1849ms。

冻结window为7472408abe39689ec509196cb39d3cdd9b8e6aeee28f18ba0c05da666f90f55a。最终报告见 `full-evidence/final-sharded-report.tsv`，实际SHA5ab6a99e00675476c1c09b0dee69a3acc7d0e6d3a09b6b47c097ae3d7a2bbb53；其中42行MATCH均绑定对应proof整个文件SHA，完整末尾实际VERIFIED。

## 分片合同和限制

每个compare仍实际读取两侧JSON/TBL/NET，完整memcmp、双方分别SHA，核profile实际产物SHA、原始全键输出和table/net/checker绑定，再独占发出带bodySHA的当次proof。复用39项将原基线三产物与独立全量三产物实际比较；相等的tbl/net由本次全量侧完整全键验证传递，不冒充重新执行基线旧PASS。

window绑定canonical RUN、before/after/plan/baseline-index/material/mutation、缓存收据和实际exe，以及本次controller源码/native。最终report严格要求42个对应index/net/mode/window的proof、左右三产物顺序和路径、bodySHA/完整尾，重hash所有小metadata及proof。大artifact只查dev/ino/size/mtime/ctime，**不再次content-hash**。故从compare到report必须保持无写者的冻结窗口；秒粒度stat不是抗并发等长改写或ABA保证。proof是内容与受控执行阶段证据，不是签名；不扩展为任意不可信外部receipt的通用离线认证。独立只读审阅及后续收紧项见 `shards-review.md`。

实际proof篡改负控32453 exit0：report因错proof身份退出2、未发布报告，原proof同inode恢复；原始日志与诊断源码已归档。本次正式checker批59343实际exit0，PASS/rc0/transport0，同候选f069…，11/0；原10断言全部保留，新增实编当前controller一次。完整stdout与BATCH_RUN在 `check-final.log`，属于父持有句柄的实际输出归档。

历史失败保留：初始实现4832因不支持memmem编译拒绝，41511因新目录父路径不存在准备失败，初始75504检查启动失败后调整有界子启动；旧整份report56730外bound50超时142且未生成final-report。正式checker先因TMPDIR别名失败2/10，后因重复编译外bound60超时142；最终使用realpath(TMPDIR)+本次实编一次修复，没有提高时限、删断言或缩覆盖。分片缺集合68456实际退出2且无报告，随后同窗口42份真实比较与最终汇总通过。

## 证据布局与归档

`full-evidence/`包含42×6原始小TSV（profile、commands、full-key、validation-bind、build-time、validation-time）、一份共用inputs.tsv、42compare、最终报告、根before/after/plan/material/mutation/baseline-index/cache。共304文件。共用inputs实际SHA6f696d0add405dc20580e664ab6603177c78b4d7f682ecdaa215fdeacba73c1e。`delta-evidence/`保留三个增量网对应小TSV；根另有input-audit及两summary。`job-terminals.tsv`保留代理本次实际历史句柄；父全量/compare终态与正式门禁由父实际poll并整合。

大产物未复制入此目录：当前RUN在 `out/c48-base-incremental/rehearsal-v2/`；初轮research/rehearsal-v1的root归档到out/c48-base-incremental/rehearsal-v1-root-archive。原BASE2的126个gen/tbl/net目录已在比较结束后实际归档到 `out/c48-base-build-products/batch-NN/原名`；旧commands、baseline-index、compare仍保留执行当时的原路径，不伪造历史回执。此后直接重跑原report会因路径移动拒绝，不能把已归档报告当跨窗口缓存。

最终HASHES.sha256排除自身，覆盖本目录其余全部普通文件；从仓根执行 `shasum -a 256 -c research/c48-base-incremental/HASHES.sha256` 可核文件内容。未提交，最终提交/同步由父负责。
