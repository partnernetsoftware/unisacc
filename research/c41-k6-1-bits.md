# K6-1：`unisa.bits` C 参考对拍（材料+受控读数；不预填口头绿）

**状态（2026-10-11 ≈18:36 SGT，政委代裁授 K6-1）**：实现已落；夹具本机绿。**不**删 `bits.py`；**不** bump；**不** Draft/publish/扩签/动 Latest；**不**改 `__main__` / gold / construct / emit-kernel / 产品验收措辞。基线 tip 开工 **95490302**；`version.h` 仍 **0.0.40**；Latest=v0.0.40。

## 并授钉（K6-1；照草案钉死）

| 钉 | 值 |
|---|---|
| **文件域** | **写**：`seed/unisa_bits.c`；`tests/unisabitscheck.sh`；`tests/gate.sh`（仅加 `unisa-bits` job/req）；`research/c41-k6-1-bits.md`（本页）；追加 `research/c41-k6-inventory-cut.md` 与 `plans/v0.0.41.md` 授权态。**只读**：`unisa/bits.py`。**禁**：`src/`（含 `version.h`）、`exec/` 生成路径、`kernel/`、`weights/`、`release/`、删 `bits.py`、改 `__main__`、bump、Draft、Latest、签、旧全量。 |
| **预算** | 墙钟 **≤30 min**；超则首红停或拆刀。 |
| **首红停** | 编译失败 / 对拍 DIFF / 负例一侧不拒 / 越文件域 → **立即停**；不扩跑、不静默改期望。 |
| **禁** | bump、Draft、publish、扩签、动 Latest、签、删 Python、改产品验收。 |

## 实现

| 路径 | 角色 |
|---|---|
| `seed/unisa_bits.c` | C99 参考：`mask64` / `round-up V A` / `round-up-batch`；`a==0` → `alignment zero`；结果溢出 uint64 → `round_up overflow`（对拍用例均在 uint64 结果域内） |
| `tests/unisabitscheck.sh` | 同输入对拍 + 负例 + C 双跑 + 身份摘要 |
| `tests/gate.sh` | job `unisa-bits`；LIST req `native-posix\|host\|cc,python3\|cdx\|unisa-bits-byte-pair` |

未改：`unisa/bits.py`（保留）、`unisa/__main__.py`、gold/construct/emit-kernel、`src/version.h`、`release/`、gatedeps.json。

## 受控读数（本机；gate `--suite unisa-bits`）

| 项 | 值 |
|---|---|
| tree（开工） | `95490302568f4f71e55aa912bbc3016f58493aac` |
| harness | **same 7 / diff 0**；rc0 |
| 对拍 | MASK64；round_up 20 cases（batch+singles）；C det×2 |
| NEG | a=0：两侧 rc≠0；C 具名 `alignment zero`；Python `ZeroDivision`；badint：两侧拒 |
| host-cc | `cc (Debian 14.2.0-19) 14.2.0` |
| bits.py sha256 | `5a2cad1e71d96b776917fe476cc0b89c7d04768badc40612d11686e9f74b0671` |
| unisa_bits.c sha256 | `e05a6b470d4295baace5c68f8d7f7a94c7eacea9bd9b04fca295005c99c782f7` |
| unisabitscheck.sh sha256 | `df0fee35e0e750bcf5f339726ca8a17d6f2ec5b7b8c53cdb9e2c46872d130f8f` |

## 不称

不称 unisa 全树 C 化；不称删 Python；不称净提速；不称 gate 全量绿；不称 bump/Draft/Latest/签；不称 uint64 溢出域与 Python bigint 逐字节等价（溢出 C 拒、Python 放大整数——共享域外）。

## 首红记录

本窗：编译绿；对拍 7/0；负例两侧同拒；未越文件域。**无首红。**

---

*K6-1 2026-10-11 ≈18:36 SGT / 政委代裁授*
