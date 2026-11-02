# P4 资源行字段数错位（只读发现 + 实测证据，2026-11-02）

## 结论

`1bf6f7e8c3`（SIZE-01，P4 资源压缩）的**写方与多数读方对同一行的字段数不一致**：

- 写方 `tools/release-build.cx` 的 `rb_emit_verbatim`（`:227`）与 `rb_emit_packed`（`:252-253`）写 **6 个 token**：
  `F <name-len> <stored-len> <rawlen> <codec> <crc>`
- 产品运行时 `cx/runtime/package_decode.cx:136-145` 也按 **6 个 token** 读，实测接受。
- 但三个工具读方要求 **5 个 token**（或 3|5），因此**任何带资源的 P4 包都会被它们判坏**。

## 实测证据（不是推断）

探针 `out/probe/p4probe.cx`（放在 `out/`，已被忽略；用 `./unisacc.com out/probe/p4probe.cx` 跑），
对同一份合法 P4 跨度：

```
P 4 1 1 1\nD r s in out 0\nM 1 1 1 0\nN F 1 1 1 0 0\nab
```

输出：

```
decode rc=0 why=-
ver=4 nr=1 len=1 rawlen=1 codec=0 crc=0
audit rc=6 ver=9          <- cxps_audit 拒绝（rc 6 = resource row）
audit P3 rc=0 ver=3       <- 同内容 P3 正常
```

即：产品运行时能解，结构审计判坏。

## 五处读方的位置与错法

| 文件:行 | 现状 | 应为 |
|---|---|---|
| `cx/runtime/packagestruct.cx:126`（`walk` 资源循环） | `nf != (ver >= 4 ? 5 : 3)`，随后 `:129-132` 还读 `f[5]`——`nf==5` 时该项未初始化 | `nf != (ver >= 4 ? 6 : 3)` |
| `cx/runtime/packagestruct.cx:72`（`key_conflict`） | 同样 `(ver >= 4 ? 5 : 3)` | `6` |
| `tools/package-refresh.cx:53` | `if((fw!=3&&fw!=5) …)`，但本文件 `:54` 自己写的是 6 个 token 的 `"F %lu %lu %lu 0 0"` | 按 `pver`（`:47` 已有）分叉：P3→3，P4→6 |
| `tools/cxlib/rebuildcontext.cx:65` | `(fw!=3&&fw!=5)` 且 `if(ver>=4&&fw!=5)` | P4→6 |
| `tools/cxlib/streampack.cx:39` | `(n!=3&&n!=5)`，而 `:30` 已接受 `P 4` 头 | P4→6，**且必须同时改 `:44`**，见下 |

`package-refresh` 这条最能自证：**它自己写 6 个 token，却只肯读 3 或 5**，永远读不回自己的输出。

### streampack 不能只放宽字段数（cc 已采纳方案 1）

`:44` 输出头硬编码 `"P 3 1 1 %d\n"`，而资源段是 `memcpy(body+hn+a->bytes, PK+resource_at, resource_n)` 逐字节照搬。
若只把 `:39` 放宽到收 6 词，P4 源的 6 词 F 行就被塞进声明为 `P 3` 的包，`cx_sha256_hex` 只算哈希不校验结构，**静默产出无人能解的坏包**。

改法（已采纳）：`:44` 改为 `"P %d 1 1 %d\n", ver, nr`，`ver` 取源包头版本。M 行在 `ver>=3` 下都是 5 词，P3/P4 通用，资源段照搬即自洽。
- P3 源 → 输出 `P 3`，逐字节与旧行为相同；
- P4 源 → 输出 `P 4`，`cxpk_decode` 与 `cxps_audit` 同时接受。

未采纳的备选：把资源行重写成 3 词——`codec==1` 的行必须解压才能重写，多一段 inflate，收益为负。

## 影响面

- `cxps_audit` / `cxps_models` 的使用方全部受影响：`tools/check-packagestruct.cx`、`tools/audit-package-payload.cx`、`tools/cx-library-build.cx:46`、`tools/extract-p3-models.cx:35`、`tools/check-network-real.cx:19`。
- 重建/收养链（`rebuildcontext`、`streampack`、`package-refresh`）读不了 P4 包 ⇒ 一旦产品以 `--compress-resources` 构建，重建与内核刷新路径全断。
- 产品自身解包走 `cxpk_decode`，不受影响；所以这不会在普通运行里暴露，只在审计/重建窗口暴露。

## 与第 3 项的关系

cc 已把本条定为**队列第 0 项**（先于原 1-5 项）。第 3 项的「P4 受理正向覆盖」依赖它：正向用例一按规范写（6 词行），就会直接暴露本条。

第 0 项交付清单（cc 裁定）：

1. 修上表五处字段数（含 `streampack:44` 保版本）；
2. `check-packagestruct`：6 词合法 P4 → `audit rc=0 ver=4`；5 词 P4 → `rc=6`；
3. `check-package-decode`：6 词 `codec=0`（`rawlen==len`）与 `codec=1` 各一例，断言 `rawlen/codec/crc`；
4. 新增独立检查 `tools/check-streampack.cx`（`tools/register.cx` 登记批次）：
   - P3 源 → 输出与旧行为逐字节相同（头 `P 3`）；
   - P4 源 → 输出头 `P 4`，且产物同时被 `cxpk_decode` 与 `cxps_audit` 接受；
5. P3 路径逐字节不变（现有 P3 用例回归：`check-packagestruct.cx:51/52/69/70/101`、`check-cx-library-build.cx:83`、`check-package-decode.cx` 的 `P 1` 样本）；
6. 验证：`tools/affected.cx --files … --run`、`tools/ratchets.cx`（7/7）、`entry-build` 出新候选但**不自行安装**，先报 cc 与 omp-bdy。

## 备注

- 我没有改任何 harness 文件；探针只在 `out/probe/`。
- 文档里原先写的「5-word F row」来自提交说明的措辞，代码实际是 6 词。规范表述应以 `package_decode` 的读取顺序为准：`F n len rawlen codec crc`。
