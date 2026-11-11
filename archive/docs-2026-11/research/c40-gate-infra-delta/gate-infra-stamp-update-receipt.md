# gate-infra 处置 (a) 回执：更新 compilercheck 的 exec 审核戳（cc，2026-10-11T02:27:08+08:00；机房主任 02:24）
## 依据
- 真审：/tmp/cc40-prep/next/gate-infra-delta-true-review-cdx.md（cdx，sha256 6c0be030…），§5 第 1–6 项勾选，末行 PASS 可进处置。
- 裁定：/tmp/cc40-prep/next/ruling-gate-infra-disposition-a-0224.md。
## 执行（cwd ~/repos/unisacc-cc）
1. 写前复算：tip **1157e9af945ca043eb1a2301c9e8f94f767af369**，status 0 行；exec     members  1410  stamp 2d52b9bc856c  reviewed 2d52b9bc856c  EQUAL
include  members    64  stamp 30ec420a64ed  reviewed 30ec420a64ed  EQUAL
kernel   members    41  stamp 650b328eff0a  reviewed 650b328eff0a  EQUAL
src      members    17  stamp 18ce7e3bf8f2  reviewed 18ce7e3bf8f2  EQUAL
unisa    members    48  stamp f485dcd78c2c  reviewed f485dcd78c2c  EQUAL
weights  members    21  stamp e91e14662c78  reviewed e91e14662c78  EQUAL → **rc 0**（rc 在命令后直接取，未经管道）；exec 成员 **1410**、stamp **2d52b9bc856caf12e939c40e29a860fb5a236e71415abfb173257b02971aea27**，与授权的新值逐字相同；其余 5 个目录（include、kernel、src、unisa、weights）与记录值相等。输出：inv-prewrite.out / .json。
2. 修改：tests/gatedeps.json 中旧值只出现 1 次，字节级替换；用 JSON 解析校验——**只有** families.compilercheck.reviewed_trees.exec 由 9ae3c35897c7185c7b2ef2c11f6985d5281aafb424fe57b9668c2eabd3d34036 变为 2d52b9bc856caf12e939c40e29a860fb5a236e71415abfb173257b02971aea27，其余对象完全相等。git diff：1 文件、+1 −1。
3. 写后复算：inv.py → rc 0；exec reviewed = stamp = 2d52b9bc…（EQUAL），其余 5 个仍 EQUAL。输出：inv-postwrite.out / .json。
4. 提交：**fbb0d350f6848b398159da3813eb4e22222faf95**（父 1157e9af，单文件 tests/gatedeps.json，普通路径提交），提交信息引用真审 sha 与本裁定；提交前后 diff sha 一致（0dfe5557…，见 stamp-header.txt）。ls-remote 核对后 FF push。
## 绑定
exec 内容在 1157e9af 与真审基准 a8a114a0 之间无差异（cdx、cdx2 已核）。新戳覆盖的改动即真审的三文件：exec/opt/gen-delta.sh、exec/pp/gen-delta.sh（新增），exec/pipeline/prepare.sh（变更）。
## 不宣称 / 未做
- **没跑任何门**（含 gate-infra）；不宣称 gate-infra 已绿，不宣称 7 个 exec-driver 套件已恢复精确闭包——tools、fixture、K2b 等条件仍须各自满足，正式 K2b 白名单仍为空。
- 没改其他 reviewed_trees 键、inventory、验收措辞、queuecheck 期望；没 bump、没 Draft、没删测；没选 (b)(c)。

## 更正：写前证据文件被覆盖（cdx 02:3x 指出；原文保留）
- 文件时间：inv-prewrite.out 02:26:33.72（sha256 4af4d4f4…）；inv-postwrite.json/.out 02:26:41.61；**inv-prewrite.json 02:27:08.73**——晚于我的写后复算，也晚于提交。它的 sha256 现在是 e6bbafa6…，与 inv-postwrite.json **完全相同**，内容是新戳/EQUAL。
- 我在 gi1 目录的最后一次写入是 02:26:41 的 inv-postwrite.*；**02:27:08 那次覆盖不是我做的**，写入者 UNKNOWN，不归因。所以 inv-prewrite.json **不能**作为原始写前证据。
- 可用的原始写前证据只有：
  1. **inv-prewrite.out**（02:26:33，未被覆盖）：exec members 1410、stamp 2d52b9bc856c、**reviewed 9ae3c35897c7、DIFF**；其余 5 个 EQUAL。
  2. **父提交里的旧键**（git 不可变）：`git show 1157e9af:tests/gatedeps.json` → families.compilercheck.reviewed_trees.exec = 9ae3c35897c7185c7b2ef2c11f6985d5281aafb424fe57b9668c2eabd3d34036。
  3. 成员摘要：写前的完整成员 JSON 已被覆盖，只剩 .out 里的成员数 1410 与 stamp 前缀；不重跑去伪补“原始”文件。
- 上文第 1 条的数值（1410、2d52b9bc…、其余 5 个相等）与 inv-prewrite.out 一致；第 1 条没有写“exec EQUAL”，写前 exec 是 DIFF。

## 撤回与更正（02:3x；只追加，原文不改）——**覆盖是 cc 自己造成的**
- **撤回**上一节“02:27:08 那次覆盖不是我做的，写入者 UNKNOWN”。查实：本回执标题里的写作时刻就是 2026-10-11T02:27:08；我用**不带引号的 heredoc** 写本回执，第 1 条里用反引号包着的 `env -u UA -u MODEL_COM -u UA_RUN python3 /tmp/cc40-prep/gi1/inv.py /tmp/cc40-prep/gi1/inv-prewrite.json` 被 shell 当作**命令替换执行**了。它在**写后**状态下重跑 inv.py，覆盖了 inv-prewrite.json（所以 sha 与 inv-postwrite.json 相同），并把这次运行的输出原样插进了第 1 条。
- **撤回**上一节末句“第 1 条没有写 exec EQUAL”——原文第 1 条确实嵌着 “exec … reviewed 2d52b9bc856c  EQUAL” 等 6 行，那是上述误执行插入的**写后**输出，不是写前读数。写前读数以 **inv-prewrite.out（02:26:33，sha256 4af4d4f4…）为准：exec reviewed 9ae3c35897c7、DIFF**。
- 回执文件本身在 02:27:47 由我追加“更正”一节时改过（只追加）。
- 共享写者问题：这一次**不是**共享写者，是我的写法错误；之前我向三方说“请不要再往 gi1 写文件”的暗示，对这次事件不成立，撤回。
- 写前完整成员 JSON 丢失的限定保留，不重跑。
