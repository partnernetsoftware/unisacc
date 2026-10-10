# 刷戳回执：compilercheck reviewed_trees.exec fbb12970 → 1d0e0c5c（cc；机房主任 07:14，条件式单键）
## 前置条件（逐条核对，全部满足）
1. fetch 的 rc 为 0。origin/main 为 81b86a47f1a6a49660a9651da368298be80d8150。拿它跟 a7069c89 比，6 个审核目录、gatequeue 和 gatedeps 下的差异是 **0 个文件**；跟 e418bdcf 比，恰好只有两项：A exec/parse2gen/gen-delta.sh、M exec/pipeline/prepare.sh。
2. 写前复算的 pre_rc 为 0，已落盘为 inv-prewrite.rc。exec 共 1412 个成员，stamp 全长是 **1d0e0c5cf47ba3befd18d391491cb24d343dfc242aad7993a454ee57557f1825**，和目标一致；reviewed 为 fbb12970，比对结果 DIFF；其余 5 个目录 EQUAL。
3. 旧值 fbb129700659… 在 gatedeps.json 里只出现 **1 次**。
## 执行
- 写键：在私有 wt（/tmp/cc40-prep/k5-1h/wt，detached 到 81b86a47，status 0）里做字节级替换。替换后把该键还原成旧值，与原 JSON 整体比较，结果相同，说明只改了 families.compilercheck.reviewed_trees.exec 这一个键。diff 是 1 个文件，+1 −1，存于 stamp.diff。
- 写后复算：post_rc 为 0，已落盘为 inv-postwrite.rc。6 个目录全部 EQUAL，exec 的 reviewed 和 stamp 都是 1d0e0c5c。
- 提交：**1b81c2e4fbe3a96ee9999867ab249380145de8ab**，父提交为 81b86a47，只包含 tests/gatedeps.json。提交信息见 msg.txt。
- 推送：推前 ls-remote 为 81b86a47；普通快进，没有 force，push_rc 为 0；推后 ls-remote 为 **1b81c2e4**。日志在 push.log。外层墙钟没有采，记 UNKNOWN。
## 不宣称 / 未做
没有跑门。不宣称 gate-infra 已绿，也不宣称精确闭包已恢复。没有改其他 reviewed_trees 或 guards，没有 bump，没有 Draft，没有碰共享检出。

## 补记（grk 指出；只追加）
共享检出 ~/repos/unisacc-cc 的 HEAD 仍是 1f8de87a，不是 1b81c2e4 的后代，所以它的工作树里 gatedeps 还是更早的值。这是共享检出没跟上，不是撤销。共享检出不归我写，我没动它；什么时候同步，由持有者或授权人决定。
