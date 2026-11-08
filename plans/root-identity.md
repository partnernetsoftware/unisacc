# 本地根产品身份（2026-11-08 开发修复）

本地根产品是修复 `--target` 与裸 `-c` 的 0.0.47 开发 APE，非公开发布字节（NOT_RELEASE）。
六切片由冻结源 `3b41cc224ff07b470eec1b885459f1ea011a372b` 本机构造，沿用已装模型包；win/arm64 单列资产。
当前只证明本机格式/执行与回归，不迁移公开版 CI 资格。公开原件及侧车已备份于 `out/c47-driver-fix/before-install/`。

机器可校验的产品身份（本块由 `tools/check-root-identity.cx -- --emit` 写，只含事实；
判定口径在块外。它登记的批次是 `cx-root-identity`，每次全量都会重算并逐行比对）：

<!-- identity-begin -->
product_kind	ape1
product_digest	f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82
unisacc.com	f069d4c92b7f5ede508809aa3abd21e326cbecadb2fb9eccf0bd1ac7004c3e82	5818178
<!-- identity-end -->

上块的 `product_kind=ape1` 表示 `product_digest` 是整个 APE 的 SHA256；split 布局才是成员集合身份。
构造与安装收据在 `out/c47-driver-fix/`，配对 `unisacc.com.build.json` 必须具有有效 JSON、实际字节数和同一摘要。
源记录提交与冻结源之间须核产品输入零差异，不用相同版本字符串替代字节身份。

以下 dev-lnx、173 收据及旧侧车描述是历史记录，不代表当前根产品或当前验证资格。

变更记录：
  2026-11-07 c46-3 APE 884adfef… 6163990B（冻结 44182b4059）；s8 会话内该项仍 FAIL，本更新是主树记录对齐，非同会话追填。

  2026-11-02 薄头 1bfdcd20… → d93c1b24…：dev-lnx 薄头加 ulimit -v 预检（need=53248，
            类B 编译地址下限），entry.elf（de54dd80 含 -run -- 参数透传修复）与
            p3-package.bin（47a2275d）不变。行为等价依据 = check-dev-product 复验 + smoke。

证据绑定口径（aw-m4-fixarg 的 173 收据）：
  aw-m4-fixarg 的 173 个收据（PASS150/FAIL23）绑定的是「薄头 1bfdcd20 + entry.elf de54dd80」。
  薄头换成 d93c1b24（只加了 ulimit 预检，entry.elf 未变）后，按「证据只绑定候选身份」口径：
  - 不改 entry.elf 的批次（全部 173 个）证据沿用 entry.elf=de54dd80 的部分仍有效；
  - 薄头本身的行为（ulimit 预检）只有 check-dev-product + smoke 复验覆盖，未全量重跑；
  - 若要把薄头相关证据严格迁移到 d93c1b24，须重跑受影响批次（走 `sh ./unisacc.com` 的
    批次 = 会经过薄头 exec 的全部）。当前声明：薄头变更仅引入 ulimit 预检，不改变
    entry.elf 的编译/运行行为，故 173 收据对 entry.elf 的结论继续成立，薄头预检单独
    用 check-dev-product + smoke + 类B 验收（40000→rc2 / 100000→rc0）作为证据。

来源：
  entry.elf = entry-build（输入 /tmp/p3fresh/pkg.bin，构造器 c9ec40e7 系）的 lnx/x86_64 片，
              含 nm 修复 bea9f64237 + _unisa_map/stdio 修复。
  p3-package.bin = 现建 P3（--no-compress-resources），sha 47a2275d。
  candidate（不装机）= 09518630cf5991041b875cd818bddb2d0c1cb6058fea2dd7eedd856214ce0b99。

回滚：out/m4-backup/unisacc.com.83343ec1 + ROLLBACK.md（旧 APE 83343ec1）。

## 身份判定规则

一个「已装根产品」的身份由下面四项共同确定，缺一不算：

1. **`unisacc.com` 的 sha256 与字节数**。注意薄头形态下 447 B 就是完整尺寸，不是截断：
   上表 `1bfdcd20…` / 447 B 的头本身不可独立跑，它 exec 旁目录的 `entry.elf`。
   因此只比 `.com` 的 sha 不足以认定产品，必须同时比旁目录。
2. **旁目录内容**（薄头形态）：`unisacc.com.dev/entry.elf` 与 `unisacc.com.dev/p3-package.bin`
   两个 sha256，即上表第 2、3 行。三者任一变化都是新产品。
3. **`--version` 与 rev**：`sh ./unisacc.com --version`（薄头/APE）或直接执行（独立 ELF）。
   调用方式按形态分，见下。
4. **构建收据 + 安装记录**：`release-build` 的 `receipt.txt`、`release-ape` 的
   `ape-receipt.txt`，以及 `plans/release-0.0.43.md` 里 STATE43 的那一行。
   收据里的 `artifact_sha256` 必须与第 1、2 项一致；对不上就是过期记录，不是新产品。

两种形态的调用差异（两者都在本仓历史上出现过，不要假设只有一种）：

| 形态 | 特征 | 调用 | 例 |
|---|---|---|---|
| 薄 shell 头 / APE | 首字节 `#!/bin/sh` | 必须经 `sh`，或直接 exec（shebang 生效时两者等价） | `sh ./unisacc.com --version` → 上表 `1bfdcd20…` |
| 独立 ELF | 首 4 字节 `\177ELF`，无 shebang | 直接 exec；经 `sh` 会报 `Syntax error: "(" unexpected` | M4 候选 `18eae940…`（3,584,633 B，未装机） |

工具要跑产品时按前 4 字节判形态，不要写死 `sh`（`tools/census.cx` 已按此修）。
用一条记录冒充另一条形态的判定，就是 11-01 那 34 个提交错位实例的成因。

## `unisacc.com.build.json` 是旧链侧车

- 新链（`release-build` / `release-ape`）**不写、不读**这个文件；它不是身份的一部分。
- 它现在仍描述旧候选 `33273544`，而根产品是上表 `1bfdcd20…`——两者对不上是常态，不是故障。
- 该文件未被 git 跟踪，`full-candidate` 的 `id.build-record` 检查点名它就是因为它已经过期。
- 判定身份时以本文四项为准；**不要**用 `build.json` 反推「当前产品是什么」，
  也不要为让它「匹配」而去改根产品。
