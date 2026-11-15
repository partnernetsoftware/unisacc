# IPS 关键摘录（bs_B-crash2 @ m4pro 14:40 CST）

- exception: EXC_BAD_ACCESS SIGSEGV；KERN_INVALID_ADDRESS at **0xfffffffffffffff7** (−9)
- faulting pc imageOffset **45776 (0xb2d0)** → linked **0x10000b2d0**
- thread0: x0=8454144 (0x810000)，x1=−9，far=−9；esr Data Abort byte write Translation fault
- codesign: adhoc Identifier=bs_B-26a15c5e495a8c9e5c5614d93171e38c34d3bb1d
- usedImages[0]: bs_B arm64 base≈0x1007e0000 size 1851392

完整 IPS 本机：`/tmp/b139-manual-82e1a9f0/bs_B-crash2.ips`（若入仓副本未到齐，以本页+FINDINGS 为准）。
