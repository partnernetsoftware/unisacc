# WF2 处置回执：a3=C（机房主任 22:11 SGT 代裁；cc 写于 2026-10-10T22:12:56+08:00）
- 结论：WF2 填尾**未覆盖 / 顺延**。这是顺延，不是通过。不追绿，不开 a3（方案 A、B 都不跑）。
- 未覆盖项（具名）：
  1. tail 准入：left < span-3 时的 kind=tail START；
  2. tail 尝试 rc142 → DEFER（不作结果）→ 记 lb=2×elapsed → 进 fullwindow → 下一窗在 full 位置先入的恢复链。
- 依据：/tmp/cc40-prep/wf2/receipt-attempt2.md（sha256 1c0b1d0eaa4fa747722cf7c12b0204dc2463d4f7672bf4f6e5ac9a832cfe9ef3）。单窗 rc65，8/8 都是 full，tail、DEFER、142 都是 0 次；共享 backup 摘要前后不变。
- 保留原账：attempt1 receipt-attempt1.md（sha256 5f0865cf2f507fd207d9c349549381f5fc6281dbe6233a2a6c54945e520006fa，不通过：两窗、四个嵌套夹具、写了默认 backup）；attempt2 原账照存。
- predict-a3.md 只是计划，模拟已作废，不作入窗依据。
- 不改产品、不改验收、不改归因、不加白名单、不改仓代码。plans 门闩表里“填尾（WF2）”一行仍是未通过；要不要写成“未覆盖/顺延”入仓，等裁。
