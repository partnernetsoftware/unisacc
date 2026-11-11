# d23351a7 publish-order 守卫独立内容重审（只审不裁）

审核范围：d23351a7（`fix: require and retain the receipt-defined publish asset set`）相对其父提交对 `release/tools/publish.sh` 与 `tests/publishordercheck.py` 的差分。对照 R2=A（按回执 `published.public_assets` 要求资产；dmg 仅在回执列出时必需）。daf53c86 仅刷新 `tests/gatedeps.json` 中 `publish-order` 对 `tests/publishordercheck.py` 的 guard 哈希（45c88de6…→04a84b21…），提交说明自承 “hash refresh, not a re-review”——**不得**把 daf53c86 当作本差分的真重审或已审依据。

本窗：独立读 diff、现文件与 publish-order 负例出口；跑 `python3 ./tests/publishordercheck.py` 得 rc0。未改哈希、未刷具名补验、未改验收/裁定措辞、未动 d7480939/a44e4702 归因文档、未代裁「其」口径。

## 差分要点（相对 d23351a7^）

### publish.sh
1. 验收回执解析后新增：读取 `published.public_assets`；须为非空字符串列表；名须匹配 `[A-Za-z0-9][A-Za-z0-9._-]*`；不得重复；必须含 `unisacc.com`；将集合打印为 `required`。
2. 删除硬编码「必须已有 unisacc.com 与 unisacc-macos-universal.dmg」两则 case；改为对 `required` 逐名检查 Draft 上是否已有。
3. 公开前删多余资产：retain 集合从硬编码 `unisacc.com|unisacc-macos-universal.dmg` 改为 `required` 空格分隔集合。
4. 其余资格闸（schema、TAG 绑定、eligible、签后字节、六格/Defender court、owner-promotion、pending）未改。

### publishordercheck.py
1. mock `gh` 资产列表改由 `MOCK_ASSETS` 驱动；acceptance 默认带 `published.public_assets: [unisacc.com, unisacc-macos-universal.dmg]`；`assets=` 可覆盖。
2. 正路径仍要求 verify→delete→publish 顺序，且默认集合下不删 dmg。
3. 新增负例：空/畸形/重复/`unisacc.com` 缺失的 asset 集合 → 非零且无 gh 副作用。
4. 新增：仅声明 `unisacc.com` 时，Draft 只有 com（可带无关 unsigned.zip）可通过；若 Draft 多出未声明的 dmg 则删除 dmg。
5. 新增：回执声明 com+dmg 但 Draft 缺 dmg → 拒绝且不 download/delete/edit。
6. 新增：声明 com+notes.txt 时保留 notes、删 unsigned.zip。
7. 新增：声明 com 但 Draft 无 com → 拒绝且无副作用。

`release/tools/publish.sh` **不在** `publish-order` suite 的 `guards`/`files` 内（只经检查脚本间接覆盖）；guard 哈希变化仅 `tests/publishordercheck.py`。

## 内容结论

| 项 | 结论 | 限定 |
|---|---|---|
| 与 R2=A 一致性 | 符合 | 资产集合来自回执；无 Apple/dmg 版本只要回执不列 dmg 即不硬要；`unisacc.com` 仍强制在集合内。 |
| 失败封闭 | 保持 | 不合格回执/缺声明资产时，在 download/delete/edit 之前拒绝（负例覆盖）。 |
| 保留/删除语义 | 符合 | 只保留回执列出的名；未列出的可删；列出的不可因旧硬编码被误删。 |
| 名合法性 | 收紧 | 拒绝路径穿越与非法字符，避免把异常名送进 `gh release delete-asset`。 |
| daf53c86 | **不是重审** | 仅把 guard 从 45c88de6… 对齐到 d233 后字节 04a84b21…；通过前不得因该 refresh 记已审。 |
| 本记录 | 真重审材料 | 审的是 d233 守卫语义，不是刷哈希。 |

## UNKNOWN / 不做的声称

- 未对真实 GitHub Draft 做端到端 publish.sh 演练（检查为 mock `gh`/`curl`）。
- 不证明 d23351a7 之后其它提交未再改这两个文件的契约（当前 tip 上两文件仍为 d233 字节；本窗 publishordercheck rc0）。
- 不改 `tests/gatedeps.json`、不回退/重刷任何 guard；是否把 publish-order 记「已审」交机房主任裁，本记录不自行改戳。

## 交裁建议

在上述限定下，d23351a7 对 publish 守卫的改动与已裁 R2=A 一致，负例覆盖收紧方向正确；daf53c86 不得替代本重审。可将本文件作真重审材料；是否据此记 publish-order 已审，由机房主任裁定。
