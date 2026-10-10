# B.9b optional: unsigned custody after publish.sh step 3 delete

> 机房主任 19:06 可选执行：从 Windows qualification/company GHA artifacts 重取被删的 `unisacc-unsigned.zip`（sha `04cfd258…`）与 `unsigned-receipt.json`，落仓保管。禁止重发 release；禁止为补四眼改回执 `published` 字段。

## 结果：**取到**

落点：`release/evidence/v0.0.39/`（详见该目录 `README.md`）

| 文件 | sha256 |
|---|---|
| `release/evidence/v0.0.39/unisacc-unsigned.zip` | `04cfd258f6e385555fec7df552417fb198f838e48f85d6cb0dac9a842c98d19d` |
| `release/evidence/v0.0.39/unsigned-receipt.json` | `f3e0098bb50077cd5368bec6adaadffe0c6c48e7d4f4dd280eb7cfbc6051b1e8` |

## 来源
- qualification run `38043394306` artifact `unisacc-sealed-signing-input-1` → 直接得 `unsigned-receipt.json`；`unisacc.com`（before）按 `unsigned_receipt.py` 配方重打 zip → sha 命中 want。
- company run `38043453555` 同名 sealed-signing-input / company-signed 内 receipt 同 sha。
- 现网 release 资产已无这两份（未重传）。

## 边界
未重发 release；未改 `research/r39-release-acceptance.json` 的 `published.*`；未改验收措辞。
