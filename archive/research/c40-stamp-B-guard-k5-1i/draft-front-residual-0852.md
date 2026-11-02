# Draft 前剩余材料（cdx；0852，0854 补裁）

只列待办，不实施。依据：0852 裁定及 0854 补裁；`research/c40-freeze-first-observe.md`；`research/c40-k5-inventory-cut.md` 限定收口与更正；`research/c40-stamp-A-k5-1i/stamp-A-true-review-cdx-0804.md`；本目录 0848 收口裁定。K5 限定收口、stamp-A 认键、stamp-B 五 guard 修债已记录，不因此视作全量验收或 Draft 授权。

## 首红停负例补证缺口

- 当前 K5-1i driver 的首红后“后续任务零启动”未演示；0804 真审明确 0727 其他 driver 不互证。helper NEG 拒绝、三片成功及同字节读数不能替代本条。K5-1b 首红后续跑历史偏差仍保留，读数不并入。
- 补证待另授：具名固定提交、夹具与 driver，记录注入点、首个非零子 rc、调用顺序、首红后的启动计数及日志原件；区分预期负例与验收通过，不以首红 NEG 当验收。
- 严格 PGID/SID/所属子进程清空及 TERM 未测；既有 ps-g rc1 不是严格清空证明。UA 未过 shim，calls_after 仅为 shim 观察；post HEAD/status/UA 为事后采样，补证需前后身份。枚举失败记 UNKNOWN，不作组空。
- 原引用 `/tmp/cc40-prep/k5-1i/` 当前不存在，不能以旧路径代原件；本页据仓内真审列缺口。具体范围、执行者和是否顺延由机房主任另裁，若涉及验收变更须授权人具名确认。

## bump + freeze 前置

按 freeze-first 的四项实跑前置，须在另授窗口落实：

1. 开发/预验完成，授权人批准冻结；K5 产品切口达到可冻证据，未完成门闩逐项具名“不依赖/顺延/阻断”并由授权人确认。原 UNKNOWN（含真实 exec-chain 预算、净提速）不改写为绿。
2. 单独版本提交 `src/version.h` → `0.0.40`，作为 freezecheck 起点；当前读取仍为 `0.0.39`。其后产品闭包遵守 `fix:` 或 waiver。
3. 执行时 tip 净树，HEAD / origin/main / ls-remote 一致。本轮未查询远端，不宣称此条件已满足；本材料也须另授入仓。
4. 冻结提交后刷新 gatedeps（refresh ≠ review）；此前 stamp-A/B 不替代冻结后的身份核验。

冻结获批且 bump 后，真实首观察另授，范围仅 preflight（h1source）→第一个 make。届时采 freeze-first 所列树身份、H1 前后 sha、argv/env、实际宿主与声明、失效键、exittable、逐步子 rc/日志 sha、monotonic 与进程清理。垫片 exit 97 仅为预期截停，不是 PASS。0.0.39 禁构造仍依裁定；代码版本闸与冻结+bump 同批再议；改错声明 sha 负例原裁允许顺延，不新列为阻断。

## 谁裁

| 待办 | 裁定归属 |
|---|---|
| 首红停补证范围、执行者、窗口及收口 | 机房主任另裁；本材料不分派实现 |
| 可冻证据、未完成门闩处置、冻结批准 | 授权人（政委/机房主任）具名确认 |
| 单独 bump、冻结后 refresh/review、真实首观察 | 机房主任另裁执行窗口，服从冻结授权；执行者待裁 |
| Draft 可否开及执行者 | 政委/机房主任另裁，0852/0854 未授 |

本轮仅只读核验并落材料/短记，未改 prd、键、代码或验收，未实现、跑门、bump、Draft、提交或推送。材料齐表示剩余清单已整理，不表示上述证据或授权已经闭合。

材料齐
