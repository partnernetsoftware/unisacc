# R69.6 变异逐例回执

红绿指验收夹具的断言。原批的 PASS、FAIL、UNKNOWN 和退出码列在实际结果中，供分辨夹具和批自身的裁决。

| 例名 | 变异 | 预期 | 实际 |
| --- | --- | --- | --- |
| observed-pass | 删字段 | 红，缺 assertions | 红，原批 PASS，退出 0，缺字段，应为 2 |
| observed-fail | 删字段 | 红，缺 assertions | 红，原批 FAIL，退出 1，缺字段，应为 1 |
| observed-zero | 删字段 | 红，缺 assertions | 红，原批 PASS，退出 0，缺字段，应为 0 |
| observed-timeout | 删字段 | 红，缺 assertions | 红，原批 UNKNOWN，退出 3，缺字段，应为 UNMEASURED |
| observed-pass | 删累加 | 红，正计数归零 | 红，原批 PASS，退出 0，实为 0，应为 2 |
| observed-fail | 删累加 | 红，正计数归零 | 红，原批 FAIL，退出 1，实为 0，应为 1 |
| observed-zero | 删累加 | 绿，真实零保持 0 | 绿，原批 PASS，退出 0，实为 0 |
| observed-timeout | 删累加 | 绿，运输超时分支保持不可测 | 绿，原批 UNKNOWN，退出 3，实为 UNMEASURED |

两个变异各执行四例。删字段为夹具通过 0、失败 4，检出 4/4。删累加为夹具通过 2、失败 2，全部四例中的检出数为 2，受影响的两例均检出。原版绿色对照通过 4、失败 0。单独的旧与新行前缀对拍为 4/4，使用未启用影子字段的基线与完整新版。原行见 probe-green.log、probe-base.log、probe-drop-field.log、probe-drop-count.log 和 compare-wire.log。
