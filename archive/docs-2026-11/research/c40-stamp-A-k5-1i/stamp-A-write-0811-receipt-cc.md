# 写键回执：compilercheck reviewed_trees.exec 1d0e0c5c → 828a6f67（cc；机房主任 08:11，条件式单键）
## 前置条件（三条全部满足）
1. fetch rc 0。origin/main 为 4470a3f97a99c17493f8d9db117deaab83260313。1b81c2e4 到 4470a3f9 之间，六个审核目录加 gatequeue/gatedeps 的差集**恰好**两项：M exec/c/chain.sh、M exec/parse2gen/gen-delta.sh（见 cond1.txt）。
2. 写前复算 pre_rc 0（已写入 inv-prewrite.rc）。exec 共 1412 个成员，stamp 全长 **828a6f67417de1998627ef73d1346ff9709ef3cccd1cb7f2ebd904baf09a4775**，与目标一致；reviewed 为 1d0e0c5c，比对 DIFF；其余五个目录 EQUAL。
3. 旧值 1d0e0c5c… 在 gatedeps.json 中只出现 **1 次**。
## 执行
- 私有 wt /tmp/cc40-prep/k5-1i/wt，detached 到 4470a3f9，status 为 0。做字节级替换后，把该键还原成旧值，与原 JSON 整体比较完全相同，确认只改了 families.compilercheck.reviewed_trees.exec 这一个键。diff 为 1 个文件 +1 −1，存于 stamp.diff。exec-chain 的 guard 没动。
- 写后复算 post_rc 0（已写入 inv-postwrite.rc）。六个目录全部 EQUAL，exec 的 reviewed 和 stamp 都是 828a6f67。
- 提交 **40ec2903a5f79be248dbffbf320adae530cf0989**，父提交 4470a3f9，只含 tests/gatedeps.json。提交信息见 msg.txt。
- 推送：推前 ls-remote 为 4470a3f9；普通快进，没有 force，push_rc 0；推后 ls-remote 为 **40ec2903**。日志在 push.log。墙钟没有采，记 UNKNOWN。
## 不宣称 / 未做
没有跑门，不宣称 gate-infra 已绿，也不宣称精确闭包已恢复。exec-chain 的 guard 仍是旧值 ae17a951，没有动。没有 bump，没有 Draft，没有动共享检出。

## 补记（grk 指出；只追加）
共享检出 ~/repos/unisacc-cc 的 HEAD 仍是 4470a3f9，不是 40ec2903 的后代，所以它工作树里的 gatedeps 还是旧值。这只是共享检出没有跟进，不是写键被撤销。共享检出不归我写，我没有动。新文件里 ae17a951（exec-chain 的 guard 旧值）仍出现 5 次，guard 旧债没有改变。
