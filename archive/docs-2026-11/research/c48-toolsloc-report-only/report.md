# 工具行数只报告：集成后正式批次实证

结论：集成到625b76e13及本地1315499e6后，21个precommit-toolsloc模式与cx-toolsloc/tools-report/check-tools-report共24个正式批次，全部真实执行并逐会话确认exit0；各唯一BATCH_RUN均PASS、rc0、transport0。状态、句柄、marker及产品身份见 `integrated-status.tsv`。没有用此前私有副本的绿替代本次集成验证。

实际调用现有 `tests/run-batch.cx` 接口 `ROOT ABS_OUT_PARENT BATCH_ID`。先以已装产品实编一次当前harness（session15496 rc0，仅PREPARED），再按正式登记参数执行3组、每组8个独立作业，全部外bound55；批内按登记预算45/55扣除收尾2秒，即43/53秒。每个作业独占TMPDIR与输出目录，不缓存测试结论。全部candidate为 `f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82`，harness核执行前后产品身份相同。

验证保留：现有文件/索引增长与历史逐文件数值不能形成否决，注册合格的601行新工具实际放行；缺配套check、缺批次、未登记check自身/别名、非法.cx源码仍按预期真实拒绝。M8选择/估时/材料门及原14列登记行未删；未因本改动额外跑不受影响的全量。

注册表资格另由集成者实际执行 `tests/run.cx -- --list`：session90127终态rc0，metadata_valid=1、652批。此条为父提供的真实执行证据，本代理按授权没有重复list。集成者只同步coverage两行旧LOC描述为report-only，未改变本轮执行源/参数；最终表与源码SHA见 `source-sha256.tsv`，不把描述文字修订冒充额外功能执行。

历史表完整性由本轮独立.cx探针实际核对：当前 `tests/toolsloc.tsv` 去掉第一行RETIRED头后，与执行时HEAD原blob全部字节相等，探针session79267 exit0。保留全部incoming行，不重写/刷新基线；原始核对输出 `out/c48-no-line-limits/integrated/history-preserved.tsv`。report-only入口的写入拒绝另在私有验证阶段实际rc2，当前cx-toolsloc正式批次实际rc0并输出threshold=NONE。

完整stdout/stderr及原始会话状态留 `out/c48-no-line-limits/integrated/`。本轮证据生成器session91000 actualexit0，将已poll的24个会话状态与各唯一PASS行再次逐项绑定；不是另跑测试或缓存PASS。

范围：本机macOS arm64运行这些现有登记为lnx/x86_64的宿主工具批次，不外推为Linux原生CI资格或六目标产品发布资格。未修改测试源、注册表、PRD/ledger/计划，未提交；仅交本目录3小证据，所有句柄终态。
