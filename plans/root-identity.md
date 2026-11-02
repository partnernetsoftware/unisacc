# 根产品身份（M4 dev-lnx 安装后，2026-11-02）

本地根产品 = lnx-only 开发 APE（薄 shell 头 + 旁目录），非发布物（NOT_RELEASE）。
发布仍待 osx/win 片 + UNITOK2。

三身份 sha256：
  unisacc.com（薄 shell 头）   1bfdcd20e5aa60ff057c42330f3f99615f0ce1fe697b9d3017801a1f75339464
  unisacc.com.dev/entry.elf    be8c63c8beeab3fec33fd69c39b1da6baea5b38ff1accb0b8f76979a6cfd06a7
  unisacc.com.dev/p3-package.bin  47a2275d7e1143b3bf23dc9b11f08123cc44e183d1b2432ef4c1f15c1320c600

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
