# K6-1：`unisa.bits` C 参考对拍（材料+受控读数；不预填口头绿）

**状态（2026-10-11 ≈18:36 SGT，政委代裁授 K6-1）**：实现已落；夹具本机绿。**不**删 `bits.py`；**不** bump；**不** Draft/publish/扩签/动 Latest；**不**改 `__main__` / gold / construct / emit-kernel / 产品验收措辞。基线 tip 开工 **95490302**；`version.h` 仍 **0.0.40**；Latest=v0.0.40。

## 并授钉（K6-1；照草案钉死）

| 钉 | 值 |
|---|---|
| **文件域** | **写**：`seed/unisa_bits.c`；`tests/unisabitscheck.sh`；`tests/gate.sh`（仅加 `unisa-bits` job/req）；`research/c41-k6-1-bits.md`（本页）；追加 `research/c41-k6-inventory-cut.md` 与 `archive/plans/v0.0.41.md` 授权态。**只读**：`unisa/bits.py`。**禁**：`src/`（含 `version.h`）、`exec/` 生成路径、`kernel/`、`weights/`、`release/`、删 `bits.py`、改 `__main__`、bump、Draft、Latest、签、旧全量。 |
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
不称 **穷举**：`same 7` = 断言组点采样（20 手写输入上的七组断言），非 uint64/bigint 全域扫描；更大随机/穷举对拍另排。
不称 **产品闭包含** `seed/unisa_bits.c`：该文件属参考/夹具域，不进产品闭包、不触发产品重封。

## 首红记录

本窗：编译绿；对拍 7/0；负例两侧同拒；未越文件域。**无首红。**

---

*K6-1 2026-10-11 ≈18:36 SGT / 政委代裁授*

## rebase后读数（1838）
- 基：origin/main `524bc114`（#143）；本提交 rebase 后为 `a4a631f7`（原 `c45df54d`，rebase 无冲突，仅 plans/research 与既有 gate 行）。
- 命令：`./tests/gate.sh --suite unisa-bits`（60 s 有界）。
- 结果：`unisa-bits rc=0 0s :: unisabits same 7 diff 0`；`gate suites 1 failed 0 unverified 0`。
- 未改：`seed/unisa_bits.c` 行为、`unisa/bits.py`（未删）、version.h、release/、gold/construct/emit-kernel。

## 原证（1841 补证；首红停窄修后）
- **gate 命令行**：`./tests/gate.sh --suite unisa-bits`（有界 58 s）。UTC：`2026-10-11T10:41:48Z`–`10:41:49Z`；**rc=0**；`unisa-bits rc=0 1s :: unisabits same 7 diff 0`；`gate suites 1 failed 0 unverified 0`。
- **直跑原日志**：`research/c41-k6-1-bits-run-1841.log`（`sh tests/unisabitscheck.sh`；UTC `10:41:55Z`–`10:41:56Z`；rc=0；`unisabits same 7 diff 0`）。
  - 逐项：mask64 SAME；round_up-batch（20 cases）SAME；round-up-singles（20）SAME；round_up-det SAME；mask64-det SAME；NEG-a0 两侧拒绝（py_rc=1 c_rc=1）SAME；NEG-badint 两侧拒绝（py_rc=1 c_rc=1）SAME。
  - identity：tree `16f6e0ee…`；cases sha256 `fa583573…`；bits.bin sha256 `f093ff59…`（本次构建产物）；host-cc `cc (Debian 14.2.0-19) 14.2.0`。
- **源三件 sha256**：`unisa/bits.py` `5a2cad1e71d96b776917fe476cc0b89c7d04768badc40612d11686e9f74b0671`；`seed/unisa_bits.c` `e05a6b470d4295baace5c68f8d7f7a94c7eacea9bd9b04fca295005c99c782f7`；`tests/unisabitscheck.sh`（窄修后）`3b094a75211c5f931d2973283cfab1796da43b29bf45c402cf341005caab7fe9`（窄修前为 `df0fee35…`，见 cdx 1838 审）。
- **首红停窄修**：`no()` 改为立即 `fail "first red: <name>"`（exit 1）；单例 C/Python 与两条 NEG 的 C/Python 运行加 `tests/bound 10` 看门狗。
  - 探针：scratch 中强制 mask64 DIFF，输出 `DIFF mask64` / `unisabits: first red: mask64`，**后续用例未执行**，probe rc=1。
- 限定：本记录只证 unisa-bits 当前范围（identity 与 same 7 / diff 0）；不证 Python bigint 全域等价，不证产品。

## 点采样边界（代裁；消息入口 tip 821344d8；≈18:54 SGT）

| 项 | 钉 |
|---|---|
| same 7 | 断言组：mask64 / round_up-batch / round-up-singles / round_up-det / mask64-det / NEG-a0 / NEG-badint |
| 输入 | 手写 20 对（夹具 `cases.txt`）；点采样，非穷举、非随机扩样 |
| 产品闭包 | `seed/unisa_bits.c` **不在**产品闭包 |
| 另排 | 穷举 / 更大随机对拍另排 |
| 不变 | 既有读数、源件 sha256、1841 裁决措辞不改；禁 bump / Draft / 动 Latest |

本条只钉「点采样」边界；不重裁 PASS/FAIL。
