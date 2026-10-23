# 0.0.43 候选构建与 comboot 实证

实际执行，构建上下文为 main 1a26da092bc56c5b88c38cac16d59bf9c20799d6。所有命令运行于隔离 scratch；无 Draft、Latest、签名或公开操作。

正常 seed-com 重封装后，分别用既有 comboot.sh stage2、stage3 shard 从前一阶段构造完整六目标、三组模型及模型包和驱动。日志保留每个窗口原状态：75 为有待办，第二阶段窗口02外层超时未计通过；官方完整步骤只有写入真实证明后才续用。最后两个阶段均正常返回0。

comboot-final-proof.log 的官方 fixedpoint 返回0。实际 seed、根产品、第三阶段文件逐字节相同，SHA256 均为 4f0b493c7a83d73a4ae1b348c8400ea3b7eda0ea190051b54a84c712a6730eda，大小2670104。stages.json 保存三阶段及 built_by 链；同时比较了第二、三阶段候选与安装文件的完整 build.json，全部相同。官方 installpair 安装最终第三阶段产品对，根与候选双 ident 校验通过；来源 SHA256 为 c4618639cc3fcfac4f8ce52a11df3bdab93b2c9376eecee63fd70e6550adc57f。

此证据满足构建固定点判据，不代表 F3、F4 或 F5 已验收。随后以完整第三阶段候选及同代 compiler.pkg 冻结验收输入；accept-inputs.sha256 保留实际哈希。

`steps.tsv` 和 steps/ 保存实际二十四个标记及其证明文件；compiler.pkg 体积较大，仅保存实际摘要，不复制包。每个标记的来源和证明 SHA 均重新只读核对。`window-status.tsv` 明确是父会话工具返回值的人工转录，原窗口日志不改写；空的超时窗口不作为成功证据。`final-proof.txt` 保存父会话实际执行的证明命令，`review.md` 标明独立只读文件核对与未独立执行的边界。
