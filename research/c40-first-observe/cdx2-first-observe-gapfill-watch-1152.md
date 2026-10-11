# cdx2 首观察补证漏证短记（1152）

墙钟：2026-10-11T11:54:44.714530+08:00。依据 ruling-first-observe-gapfill-1152.md、cc-first-observe-gapfill-1152-receipt.md。只读，仓外记，不代裁最终闭合。

结论：可补项部分闭合，UNKNOWN 部分具名；仍漏逐步 rc 原证及若干采样身份。未见借补证续 B.2；范围 PASS 沿用主任裁定，非产品 PASS。

## 已补与独立复核
- t0→t_make=66665890ns=0.066665890s；补证明确 t0 是 build_candidate 启动前，包含 h1source/bound 启动。t_end 明确 UNKNOWN，未拿 0s 当整轮。
- UA 完整 SHA 独立复算吻合 6a05c3c628961d025a884efd4170639bc98980d51209a3176c19b7cc4d1887ac；仅证明产物字节，不能证明本轮 build_ref rc0 或同源全过程。
- h1-before/after 输出逐字节相同；shared rc97 与 bc_rc97 可由 build-candidate.log 直接支持；make 仍一行，cand 仍仅 step-shared.log。
- gapfill-ps-now.txt 已保存全表；独立按 PGID/SID194489 筛为0行。回执明确这是1152事后采样，不替代1147原表，不证明TERM/逃逸清理。
- 当前 HEAD=origin/main=10c2e9b768f422f8646e930aace687321f528d4f；工作树净。执行窗只见算时差、哈希、读身份、事后ps及仓外回执；未见再运行 build_candidate、真make或B.2。

## 已具名 UNKNOWN
回执列出：t_end、1147 ps原表、identity-after ls-remote、失效键全集/seed内存键/exittable --h1、真make。清理PGID/SID/TERM明确未证明，未升格。

## 尚未闭合/漏列，需补 UNKNOWN 口径
1. UA build_ref、独立 h1前后、内部preflight rc0：现有输出不含原始退出码；回执仍写rc0，只能记“执行者声明，原始rc UNKNOWN”，不属于原证已补。
2. archive38仍仅 d1f05f7d… 与旧回执相互引用，前后完整哈希原件缺；本观察当前复算为 d1f05f7ddbec20556334630cc0b58bd16a05cad729b0d2661221158e60e35bc9，仅当前时点，不能倒填前后。
3. 运行时 version/plan前后SHA、完整结构化argv/env、独立preflight起止、发行版、outer setsid实际身份仍未具名列进UNKNOWN；源输出与原件哈希已在本短记补当前锚点。
4. make行 pid194490是Python记录子进程，ppid194489才是shim shell身份；勿混写。
5. 事后ps本身枚举rc未落原始表；回执0行仅代表特定筛选，不代表所有逃逸已排除。

提效：仅校正回执中“原证/声明/UNKNOWN”的归属，一次具名剩余缺口；禁止为追溯缺失时点重跑首观察或续B.2。

## 本观察当前文件哈希（非运行时前后哈希）
- `/tmp/cc40-first-observe-1147/identity-before.txt`：`f8e33ce2deaeb402b4fc30dfe94ecebba56f66fc04b8f747e0d9aa20ee3779ce`
- `/tmp/cc40-first-observe-1147/identity-after.txt`：`442ba973aef134684c33729d3aa0d75ae6f739eb4c2feecd3133f2bb5c637477`
- `/tmp/cc40-first-observe-1147/h1-before.out`：`edc98163ca3d486a1dace3023dec98d06ae855afd424ae9986c64cc1a29829e6`
- `/tmp/cc40-first-observe-1147/h1-after.out`：`edc98163ca3d486a1dace3023dec98d06ae855afd424ae9986c64cc1a29829e6`
- `/tmp/cc40-first-observe-1147/timing.txt`：`02d199b27da4f07e8e976f10ecba801d9747e8d5acbdc5220f16620bf1295ab7`
- `/tmp/cc40-first-observe-1147/make-calls.jsonl`：`5413e9cc2dd35721f04a35894b81c33f616e8b93748007a54f05ab7da1f171b5`
- `/tmp/cc40-first-observe-1147/build-candidate.log`：`497ff9f1876cde826a0a67a56191b04d77911045d31432160da2854806abff0a`
- `/tmp/cc40-first-observe-1147/cand/step-shared.log`：`13f973cc9980cfdc732a56e34eb7095bb289348e27ca0f4b8b1afe2c64c7b4ba`
- `/tmp/cc40-first-observe-1147/gapfill-ps-now.txt`：`778ac10e9587970635f88ed3a8a66e9a51cc90d44bfaf0e2dd844219e2990740`
