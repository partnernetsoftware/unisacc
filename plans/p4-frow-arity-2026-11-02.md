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

## 四处读方的位置与错法

| 文件:行 | 现状 | 应为 |
|---|---|---|
| `cx/runtime/packagestruct.cx:126`（`walk` 资源循环） | `nf != (ver >= 4 ? 5 : 3)`，随后 `:129-132` 还读 `f[5]`——`nf==5` 时该项未初始化 | `nf != (ver >= 4 ? 6 : 3)` |
| `cx/runtime/packagestruct.cx:72`（`key_conflict`） | 同样 `(ver >= 4 ? 5 : 3)` | `6` |
| `tools/package-refresh.cx:53` | `if((fw!=3&&fw!=5) …)`，但本文件 `:54` 自己写的是 6 个 token 的 `"F %lu %lu %lu 0 0"` | 按 `pver`（`:47` 已有）分叉：P3→3，P4→6 |
| `tools/cxlib/rebuildcontext.cx:65` | `(fw!=3&&fw!=5)` 且 `if(ver>=4&&fw!=5)` | P4→6 |
| `tools/cxlib/streampack.cx:39` | `(n!=3&&n!=5)`，而 `:30` 已接受 `P 4` 头 | P4→6 |

`package-refresh` 这条最能自证：**它自己写 6 个 token，却只肯读 3 或 5**，永远读不回自己的输出。

## 影响面

- `cxps_audit` / `cxps_models` 的使用方全部受影响：`tools/check-packagestruct.cx`、`tools/audit-package-payload.cx`、`tools/cx-library-build.cx:46`、`tools/extract-p3-models.cx:35`、`tools/check-network-real.cx:19`。
- 重建/收养链（`rebuildcontext`、`streampack`、`package-refresh`）读不了 P4 包 ⇒ 一旦产品以 `--compress-resources` 构建，重建与内核刷新路径全断。
- 产品自身解包走 `cxpk_decode`，不受影响；所以这不会在普通运行里暴露，只在审计/重建窗口暴露。

## 与第 3 项的关系

cc 已把「P4 受理正向覆盖」列为第 3 项。**正向覆盖一旦按规范写（6 词行），会直接暴露本条**。所以第 3 项的交付应该包含：

1. 修上面四处字段数（这是被测行为的前提，不是新功能）；
2. 正向用例：读侧（`check-package-decode`）加 6 词 `codec=0` 与 `codec=1` 两行样本；结构侧（`check-packagestruct`）加合法 P4 `cxps_audit rc=0` 与 `cxps_models` 命中；写侧（`release-build rb_p3`）小样本双模式（`--compress-resources` / `--no-compress-resources`）比对包头与再解码；
3. 一条反向用例：5 词 P4 行必须被拒（固定住「6 才是规范」这一点，防止再被改回去）。

## 备注

- 我没有改任何 harness 文件；探针只在 `out/probe/`。
- 文档里原先写的「5-word F row」来自提交说明的措辞，代码实际是 6 词。规范表述应以 `package_decode` 的读取顺序为准：`F n len rawlen codec crc`。
