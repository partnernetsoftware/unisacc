# K5-1i guard 旧债材料（stamp-B；tip c928581fd386b2da90cb05e6779c9b480c79cb50；机房主任重采 2026-10-11 08:44:46 CST）
**材料 ≠ 真审 ≠ 写键。** 本页只是材料：不勾“已审”，不写键，不刷五键，不 bump，不开 Draft，不跑全量门。
≈08:36 /tmp remount 后原 `/tmp/cc40-prep/stamp-B-guard-0822/` 真审原件已 wipe；本页为重采。写键仍搁置至活盘 PASS（见 ruling-k5-1i-guard-write-0837.md 更正稿）。

## 0. 范围（五键 only）
拟改且**仅**改下列五字符串（pathspec 概念；本材料不执行写）：

| # | JSON 路径 | 旧值（全长） | 候选新值（全长） |
|---|---|---|---|
| 1 | `suites.exec-chain-1.guards["exec/c/chain.sh"]` | `ae17a9515fa5aed38619486621a9e1682f29e569f14dfb45e2f2954f75954587` | `5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0` |
| 2 | `suites.exec-chain-2.guards["exec/c/chain.sh"]` | 同上 | 同上 |
| 3 | `suites.exec-chain-3.guards["exec/c/chain.sh"]` | 同上 | 同上 |
| 4 | `suites.exec-chain-4.guards["exec/c/chain.sh"]` | 同上 | 同上 |
| 5 | `suites.exec-chain-5.guards["exec/c/chain.sh"]` | 同上 | 同上 |

- 键名确认：五 suite 均为 `exec-chain-{1..5}`；guard 键均为 `exec/c/chain.sh`（非 `./exec/c/chain.sh`）。
- **不改** `families.compilercheck.reviewed_trees.*`（当前 exec=`828a6f67…`，写键 40ec2903 已落）、其它 `guards`、REFERENCE_KEYS（本文件无此顶键）、gatequeue、exec 源、version、bump、Draft。

## 1. tip 与差集（写前核）
- tip **origin/main = c928581fd386b2da90cb05e6779c9b480c79cb50**（`git rev-parse HEAD`）。
- Latest **v0.0.39**；无 Draft v0.0.40（本材料不主张 Draft）。
- 相对写键基 **40ec2903a5f79be248dbffbf320adae530cf0989** 与 **fd6aea3a**：`git diff --name-status <base> HEAD -- tests/gatedeps.json tests/gatequeue.py exec include kernel src unisa weights` → **空**（审核目录 + gatedeps + gatequeue 自写键后无新差集）。
- 因此：若只改五字符串，差集相对 tip 将**恰**为 `tests/gatedeps.json` 内五处同值替换；无成员增删、无其它键夹带。

## 2. 旧值 / 新值溯源（独立复算）
- **旧值** `ae17a9515fa5aed38619486621a9e1682f29e569f14dfb45e2f2954f75954587`：恰等于 `git show 495963b2:exec/c/chain.sh | sha256sum`（本机复算 = `ae17a9515fa5aed38619486621a9e1682f29e569f14dfb45e2f2954f75954587`）。自 K5-1d（1bd685b0，chain→a46974e4…）起文件已漂移；K5-1i（848bc986）再变为现 tip 内容。五键仍钉旧 tip 内容 → 失配债。
- **候选新值**（本机独立）：`sha256sum exec/c/chain.sh` = **`5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0`**；`git rev-parse HEAD:exec/c/chain.sh` blob = `82956486c71734e400ed700d65aab8c89c225d5b`。目标约定值 `5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0` → **全长相等**。
- `tests/gatedeps.json` 正文中旧值出现次数 = **5**；新值出现次数 = **0**。除上述五路径外，无其它 guard 持有旧值。

## 3. 单列：不在本材料处置
- 不代刷其它 suite guards；不重写 reviewed_trees；不跑 exec-chain 门预算；不证产品覆盖/净提速。
- 写键须**另裁**，且须活盘真审 PASS 回执在持久路径；0837 更正稿仍有效：**无 PASS 不得授写键**。

## 4. 审核勾选清单（留白；本页不勾）
- [x] 五键路径具名准确；旧值恰 5 次；无其它 REFERENCE_KEYS / reviewed_trees / 旁路 guard 夹带。
- [x] 候选新值 = tip 上 `exec/c/chain.sh` 内容 sha256 全长（独立复算，得 `5f5d19bab8a0c9a531df4693777ebe3840237df60f80dbcd615f1b715685c0a0`）。
- [x] 相对 40ec2903/fd6aea3a：审核目录+gatedeps+gatequeue 差集空；拟改后差集仅五字符串。
- [x] 材料页未暗示可直接写；未授 bump/Draft/跑门。
- [x] 真审只读；**写入 gatedeps 要另外授权**。

## 收口
材料齐，可进真审裁。

— 机房主任执行器重采 2026-10-11 08:44:46 CST；持久路径 `/home/box/repos/unisacc-cc/research/c40-stamp-B-guard-k5-1i/guard-materials.md`
