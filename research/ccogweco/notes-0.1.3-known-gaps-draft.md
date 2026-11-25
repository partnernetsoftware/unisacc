# v0.1.3 发布说明:已知缺口段草稿

口径:只引用 research/ccogweco/v013-known-gaps-collect.md 的条目与证据路径;标「待补」的条目发布前必须补齐证据或删除,不写成已验。

## 已知缺口

- **C3:I9 进程树收集上限场景未验证。** 上限、上限+1 与 `-ENOSPC` 的假清单用例归 S4;本版不宣称零残留。来源:`research/cdx/c3-invariants.md:23`。
- **C3:macOS/Windows 的 `sy_proc_finished` 路径 UNCOVERED。** 仅 Linux 路径实测,平台支持范围以 `<C3_PLATFORM_EVIDENCE>` 为准。来源:`research/cdx/notes-0.1.3-skeleton.md:51`。
- **裸 fork/waitpid 棘轮未入库前,表外裸调用无可依赖的门。** 存量清单:`research/ccogweco/bare-proc-calls.tsv`(cx、ext、tools/cxlib 共 40 调用/12 文件);棘轮补丁与红绿变异见 `research/ccogweco/s5-bare-proc-ratchet-v3.patch`(私有树夹具 13 断言、RATCHETS ok=20 fail=0,尚未入树)。
- **27 格 UNOBSERVED,不计 PASS。** 证据路径待补(S1/矩阵侧),发布前须补 strict/matrix 回执。
- **nn:Darwin/Windows 未覆盖。** osx/win 四格的精确分量 UNMEASURED;来源:`research/agy-h/slice-nn-progress.md:113`。
- **nn:同尺寸原地写 UNSUPPORTED(文档化),非缺陷。** 证据路径待补。
- **tape:速度门 UNMEASURED。** 15 倍门量化与步数上限见 `research/agy-flow/tape-t14c-speed-baseline.md:80`;T1.6 预解码为候选、索引不做,证据路径待补。

## 使用说明

- 数字复算:`bare-proc-calls.tsv` 的行数/调用数可用 `wc -l`/列统计复算;`s5-bare-proc-ratchet-v3.patch` 的夹具断言数可用其中的 `ck(` 计数复算(13 条),RATCHETS 行只在私有树复跑可得,发布说明引用时注明"私有验证"。
- 待补条目:27 格、nn 同尺寸写、tape 索引三处;补齐前发布说明不得写成已验证或已覆盖。
