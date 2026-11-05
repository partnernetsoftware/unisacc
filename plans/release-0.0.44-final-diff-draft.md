# 0.0.44 final:README/CHANGELOG 相对 rc3 的差异草稿(未应用)

前提:主人决定以 rc3 候选(APE `54bb97a2…`,源码 `b1a219cf7d`)作为 0.0.44 final,产品字节不变。rc3→final 只改"证据行",不改支持矩阵与已知限制的内容。下列各处在 final 时替换;方括号是要填的实测值。

## CHANGELOG.md

1. 第 1 行标题:`# 0.0.44-rc3(候选)` → `# 0.0.44`
2. 候选行改为发布行:
   - 旧:`候选:rc3 APE sha256 54bb97a2…,5,608,646B,源码 R4=b1a219cf7d;…`
   - 新:`发布:unisacc.com sha256 54bb97a2672f5010c8815fb0f6ecfb3ee7bb7ca91af18723007430375cc3bbb8,5,608,646B,源码 b1a219cf7d(tag v0.0.44);lnx/x86_64 三代逐字节相同(45bdf636df1e)。`
3. 支持矩阵标题后补一行全批会话证据:
   - `本机全批会话(lnx/x86_64,同一候选):[pass]/[fail]/[missing]/[unknown],399 批;check-known-fail known=[n] unknown=0 stale=0。`
4. 「已知限制(相对 rc1 新增或改写)」→「已知限制」,并把 rc1 段中仍然成立的条目并入(win、c-negative A 类、tape 截断、-c 多对象告警、运行库未去重、pp-negative、stream sidecar、objects-unsupported 合同);删去已被 rc2/rc3 修复的条目(-run 宿主目标、`.cx` 对象告警误报)。
5. rc1、rc2 的过程段保留在下方作历史,标题加"(候选,已被 0.0.44 取代)"。

## README.md

1. 状态句:`Toolchain status (0.0.44-rc3 candidate, APE 54bb97a2, source b1a219cf7d)` → `Toolchain status (0.0.44, APE 54bb97a2, source b1a219cf7d)`;其余逐目标描述不变。
2. 第 85 行附近"The 0.0.44-rc1 candidate (rev 7266862b08) is the APE a76e5fc1 …"改为 0.0.44 的身份:APE `54bb97a2`(5,608,646 字节)、lnx/x86_64 候选 `45bdf636df1e` 三代相同。
3. 构建步骤一节 "the 0.0.44-rc1 candidate unisacc.com was built this way" → "0.0.44 was built this way"。

## 不改的

- 支持矩阵、"win/x86_64 不可用"、所有已知限制的措辞:与 rc3 相同。
- 任何"正式发布到哪里"的话:由主人决定,本草稿不写发布渠道。
