# W0206 处置表:v3 改为 B-t 扩展(tests/env-semantics.tsv 追加四行),路径 C 暂停

状态:**路径 C 暂停**(cc 2026-11-25 00:05 重裁)。W0206 的活表是 tests/env-semantics.tsv;
exec/facts/env-semantics.tsv 是 0.0.105 迁移前的旧副本,不改。tests/ 表不是 SID 输入,候选 APE 与 SID 不变,
不需要整链重走;但它在冻结路径且超出已批 B-t 的"单源码检查文件"范围,由 cc 决定有条件试行(依据:R40 把
tests 表列入 C 是因为 batches/coverage/harness 一改全链失效;env-semantics.tsv 的读者可穷举且少)。

## 1 补丁内容(基线 879e446640,一笔,只追加)

- 文件:tests/env-semantics.tsv,仅追加四行:
  UNISACC_PHASE、UNISACC_PHASE_OUT、UNISACC_PHASE_IN、UNISA_FWD_REPLAY。
  前三个按 fwdreplay.cx 的 setenv/读取点写(行 57、62、79、106、112);UNISA_FWD_REPLAY 若要重述语义,
  其 expires 必须保留既有 `0.0.110`,不得改动既有行。
- 格式:八列(name/domain/unset/empty/zero/other/readers/expires),不破 check-envcompat.cx:88-103 的弃用名单。
- 归属:ompgpt 出补丁 research/ompgpt/r40b-env-semantics-tests.patch(按事实写,不凭空),并做第 3 条穷举。

## 2 硬条件(缺一即转回路径 C)

1. 只追加四行;既有行零改动;UNISA_FWD_REPLAY 的 expires=0.0.110 不动;八列格式与弃用名单不破。
2. 消费者全验:
   - check-env-semantics:断言由 125/4(129)转为 133/0(正反各记四条),不是 129/0;
   - check-envcompat:全绿;
   - premerge-selfcheck:对本补丁加 661de08935 的产品 diff 能核 PASS(item14 是否 NA 由 cdx 判);
   - 三者都在候选 APE(5f83ac93…)上以正式 run-batch 复跑为绿,不是静态推断。
3. 全仓 `git grep 'env-semantics.tsv'` 穷举读者:已知 check-env-semantics.cx:83、check-envcompat.cx:77、
   premerge-selfcheck(含其夹具行 174-180)与 cx/runtime/modelcache.cx:219 的注释;其余读者必须零回归,
   任何新读者出现即转 C。
4. gatekeeper G52 独立复核补丁与消费者证据。
5. 回滚:任一消费者红或穷举出新读者,立即转路径 C(新车头整链整预跑,见第 5 节备份)。

## 3 时间表(现在 00:02 后,冻结 03:00)

| 时刻 | 动作 | 归属 | 判据 |
|---|---|---|---|
| 00:02 | 旧候选预跑剩余 31 窗跑完,全红清单 | agy-flow | 旧窗只作定因,不迁移 PASS |
| 00:20 | 证据截止:补丁 + 三消费者绿 + 穷举 + item14 判定 | ompgpt/cdx | 缺一项即转 C |
| 00:20 前 | cdx 二审补丁并判 item14 | cdx | 129 到 133 的八条正反逐一给原行 |
| 00:20-00:25 | 第六笔提交(仅 tests/env-semantics.tsv) | agyneon | 新头 = 879e446640 + 该笔;提交前给 cc 核 |
| 随后 | 预跑收尾后补跑 W0206 与三个消费者批(新车头 frozen-root) | agy-flow | 新 frozen-root 的 frozen-source.rev 写新头;旧 done.W* 不复用 |
| 02:00 前 | 补跑完成、strict 表合并 | agy-flow | 原 FAIL 行保留,SUPERSEDED 折叠 |
| 02:40 | 公开 | cc/agy-flow | 发布准备+打包+执行约 40 分钟;20 分钟余量 |
| 03:00 | 冻结截止 | all | 任何新红即出窗,不延期、不登记预期失败 |

说明:无整链步骤,省下 28 分钟;但链止后候选 APE 未变,发布仍走 release-exec 第 1-5 步。
补跑窗必须带新 source 戳;strict 表将出现"主体在旧 frozen rev、补跑窗在新 rev"的混合来源,
由 release-exec --strict 接受与否在补跑前核一次(未核前按会拒绝准备)。

## 4 旧路径 C 计划(暂停但保留)

若第 2 条任一不成立,回到 v2 执行表:新车头 = 879e446640 + exec/facts 补登记(exec 在 SID,APE 与 SID 变),
chain-ready 全量(DEVSVC_OWNER=cc 单传、25 节点)-> 整链 28 分钟 -> 预跑 55 分钟(约 237 窗全跑)->
发布 40 分钟;最晚起 chain-ready 00:30。旧 APE 5f83ac93 的预跑与补跑作废,只作定因证据。

## 5 披露口径

- W0206 记"补登记 tests/env-semantics.tsv 四行;候选 APE 与 SID 未变,按 B-t 扩展补跑 W0206 与三个消费者批"。
- 若最终转 C,则改记"补登记后重走整链",以最终一次链的 strict 与身份为准。
- 不登记预期失败;未测写 UNMEASURED。
