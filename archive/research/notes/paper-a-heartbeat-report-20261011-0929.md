# Paper A 心跳短报 · 2026-10-11 ~09:29 Asia/Shanghai

- **切口：** E 衍生净化
- **主张：** §5.7 随带库／头文件单元组合完稿（timespec／clock_gettime／nanosleep／sleep·usleep／execl；每个随带头单独 `#include` 检查矩阵；含 Windows 上 sys/select.h 缺 poll 等单元组合类缺陷闭合）≠ A 理论／根主张闸门；A 只保留构造+T1+诚实 §5.7 披露：随带库增量与单元组合检查发现。
- **证据：** `research/notes/paper-a-derive-purify-bundled-headers-unit-combo-not-a-gate-20261011-0929.md`；tip before `546a196b`；正文四 blob SAME；Softguess NONE×4。
- **切后 A 更纯：** 攻击面从「随带库／头单元组合未完稿则根主张／实证空」缩回「构造+T1+§5.7 诚实披露；随带库战役旁支挂回 A」。与 libc-boundary／win-posix／cc-interop／c99-clause／product-maturity DISTINCT／正交。
- **仍开挡粘：** §8.1（矩阵等）、8509/8769、Ο1/Ο2；本拍不代裁、不降级。