# unisacc 0.0.72 发布说明

状态:同候选原生 CI 通过:run 44232611754 attempt 1(检出 4361f88787,输入 commit=X 8407329370),六目标全部作业 success,CANDIDATE_QUALIFY verdict=QUALIFIED pass=42 fail=0 not_run=0 invalid=0。已公开发布并置 Latest(2026-11-18T03:50:50Z);公开读回(c47-1-readback:元数据、标签、六件下载与 sha、版本、smoke 5 项、COMM、布局与 SHA256SUMS)通过,回执 release/evidence/0.0.72/public-readback.txt。冻结提交到公开实测 43 分 47 秒(0.0.71 为 151 分钟)。整体裁决 NOT_QUALIFIED:全套 s9 本版未全跑,产品域仍有 FAIL 与 UNKNOWN,不得称全批资格。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交 F(构造输入、发布标签目标) | 5cdbd305b11f5f01e86160bf4dc26d275cf252e5 |
| 来源 SID | c6d4a47dcd704fc255f5e0cda660142df0cceb90a36b1a1e521355bb60e99202 |
| 发布 APE SHA256(unisacc.com) | cc2cef18812975c8d1bcec5b50e5197b0ed32c31d847f645287f5a08508b16df |
| 发布 APE 字节数 | 7238807 |
| 整链产物 APE SHA256(仅整链证据) | 0c66aba73d84c68128b717bc44558c3d0adaf48c5069c1ca5ffd84c546ccb9f1 |
| 构造记录 SHA256 | 6a4430b5cbe5e1497e7283f888b74a23994bd5d373cba6617c32daa94e7acecc |
| SHA256SUMS SHA256 | b51f8a4d504368a04188d26008e8b6f81e99b55a62f487bd0e593c97f1844c94 |
| 候选表提交 X | 84073293705d7dd55093ffe0e9f6b4a92c7cc433 |
| CI run | 44232611754(candidate-native.yml,输入 commit=X) |

## 发布件与整链产物的关系(如实披露)

发布件 cc2cef18 是配方第 4 步 release-run --pack 的产物:打包输入(P4 包、六目标切片、两份 osx cache-manager)与整链 chain4 的 ape-final 逐项同 sha,唯一差别是打包时注入的版本号。整链在冻结源码快照内打包,版本号取自快照里的 release/candidate.tsv;冻结提交中这张表仍是 0.0.71 行,因此链产物 0c66aba7 实跑 --version 打印 unisacc 0.0.71,发布件打印 unisacc 0.0.72。这是流程缺口:冻结前缺"版本槽准备"(冻结提交应已含目标版本候选行),整链打包后也没有实跑 --version 对照目标版本的门。两条已登记 build/pkg-gaps.tsv,前者列入 0.0.73 前置清单。

严格证据分别实跑:链产物 0c66aba7 三批 PASS(release/evidence/v0.0.72-chain4-strict-2026-11-18.tsv);发布件 cc2cef18 三批 PASS(unit-frontcache-budget、run-warm、cx-driver-cli,release/evidence/v0.0.72-final-strict-2026-11-18.tsv)。链产物的证据不迁移给发布件。

pkg.bin(P4 包)SHA256 664878e1... 与 0.0.71 相同。

## cache-manager 来源

chain4 以显式 --cache-manager-osx 接入 chain2 链内构造的两份 Mach-O(/tmp/unisacc-session-v072-chain2/cache-manager/):x64 sha256 d34bee6cd12c77d1f356ab732bf39ed679706686073a9467e2cf97fe2b8023de(247906 字节),arm64 sha256 0aae3fbc525d12b61eea61d027c8cf12c9bbf9bb9e6a5415cd02cbbbc740ef5a(264418 字节)。chain2 与 chain4 产品输入相同、rev 不同。与 0.0.71 对照:0.0.71 的 cache-manager 在同链内构造。显式 cache-manager 记 checkpoint 的修复(5cdbd305b1)已在 4361f88787 补检查与负控:四种缺失或空文件均 rc 2 具名拒绝且不写 DONE,两份非空写 DONE;用 5cdbd305b1 父版仅接入夹具五项全红,先失败后通过成立。仍存缺口:显式 cache-manager 的身份只靠证据文件记录,checkpoint 行不带身份(已登记)。

## 冻结提交与候选 rev

候选 rev 即冻结提交 5cdbd305b1。相对先前冻结点 4b83fd5de2,src、cx、ext、exec、include 下只差 exec/facts/derive.tsv(读集声明表,不是编译器输入;product-map 读集含整个 exec,故源身份不同)。

## 产品改动

(1) --warm:只编译并写入运行缓存,不执行。
(2) 缓存目录权限收紧为 0700。
(3) tmp 清扫快路限于单元缓存发布。

## 开发工具链改动(概要)

整链显式 cache-manager 记 checkpoint;affected --run 向每批透传候选目录;开发候选尊重批次显式 E3 缓存,嵌套启动清除继承的默认缓存目录。均为开发与验证侧改动,不进入产品。

## 已知缺口与未验证范围

(1) U20 core-compat、U22 材料与平台(无生产者材料与外部主机证据)、U23 stages 字节差异、U25 interfaces-lnx-x86_64 oracle 迁移、U26 产品对应拒绝输入静默接受(c-negative、pp-negative),均延续,未关闭。
(2) 产品资格视图(release/evidence/product-verdict-2026-11-18.tsv,434 行):PASS 150、FAIL 12、UNKNOWN 39、PLANNED 232。12 个 FAIL:interfaces-lnx-x86_64、objects-old-unit-version、objects-unsupported-win-x86_64、objects-unsupported-win-arm64、pp-negative、c-negative、stream-e4-io、stream-prune-io、stdio、stages、dock-audit、harness-contracts。陈旧期望的退役情况见该文件与 build/state-unverified.tsv,未删断言。
(3) UNKNOWN 分类(release/evidence/product-unknown-2026-11-18.tsv,70 行):NO_PRODUCER 68、NO_MATERIAL 1。
(4) s9 全套本版未全跑,第 4 步保持 UNKNOWN。
(5) 发布件严格证据只覆盖三批;其余批次未对发布件实跑。

## 证据入口

release/evidence/0.0.72/INDEX.tsv(逐项登记,含整链各相、严格证据、CI、草稿与公开读回);release/evidence/v0.0.72-chain4-2026-11-18.tsv;release/evidence/v0.0.72-chain4-strict-2026-11-18.tsv;release/evidence/v0.0.72-final-strict-2026-11-18.tsv。撤回方案:release/rollback.txt(回退 Latest 到 v0.0.71,不删标签与 release)。
