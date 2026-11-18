# 简报节选(反例,改写)

目标:把 struct-member-capacity 批压进时限。

写域(只许改这些路径):tests/cases/struct-member-capacity/,src/cx/lower.c

验收命令(含预期值):先设 UNISACC_E3CACHE=0 跑 `sh ./unisacc.com tools/affected.cx -- --ids struct-member-capacity --run`,预期 pass=1;口径 ratchets-15-0。

截止:20 分钟。

禁区:不得删断言。
