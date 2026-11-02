# 代裁落地：相对 `-include` × forward 单元（方案 A；≈19:20）

墙钟：2026-10-11 ≈19:20 Asia/Shanghai。授源：政委代裁方案 A；材料+NEG 同刀；首红停；禁 bump/Latest/签；可落 hotfix 旁支，勿挡 K6 主叶。

## 根因

`-include FILE` 写入 `opti[]`；`fe_load` 对**每个**单元注入 `#include "FILE"`。
引号 include 按含入文件所在目录解析（`srcpath` dirname）。

`fe_units_fwd` 第二次编译把 stubs 写成 `/tmp/unisacc-forward-*.c` 再并入单元表。
相对 `-include`（如 `apps/csih/h.h`）在该单元上变成找 `/tmp/apps/csih/h.h` → `no such file for #include`。

csih.sh / suite 现用绝对 `"$ROOT/..."` 可规避；**去掉 -include 不是修复**。带目录源路径 + 相对 `-include`（`apps/csih/t.c`）是旁症，**本刀方案 A 不修**（见备选 abspath 旁支 `fix/fwd-include-abspath-20261010`）。

## 方案 A（本刀）

`src/main.c` `fe_units_fwd`：对 **synthetic forward 单元**跳过用户 `opti[]`。

- `opti_skip_path`（`src/front_pp.c`）在第二次 `fe_units` 前设为 `fwdpath`，之后清零。
- `fe_load`（`src/front_parse.c`）当 `path == opti_skip_path` 时不注入 `-include`。
- 用户单元第二次编译仍保留 `-include`（stubs 不需要调用方头）。

## 文件域

| 路径 | 作用 |
|---|---|
| `src/main.c` | `fe_units_fwd` 设/清 `opti_skip_path` |
| `src/front_pp.c` | 声明 `opti_skip_path` |
| `src/front_parse.c` | `fe_load` 跳过匹配单元的 opti 注入 |
| `tests/fwd_rel_include_forward.sh` | POS relative+forward；NEG missing |
| `research/c40-csih-remain-1920/` | 本裁定 |

禁：`src/version.h` bump、Latest、签、动 K6 主叶 / `archive/plans/v0.0.41.md` 主题工作。

## 复测 schema

| 字段 | 值 |
|---|---|
| claim_id | c40-csih-remain-1920-scheme-A |
| base_sha | 24a9c6d52d3f102b07015e973df53261c9e86eb1 |
| tip（落刀后填） | `64603035`（fix commit；旁支 tip 见回执） |
| env_id | Linux box；`tests/build_ref.sh` → `/tmp/fwd-skip-opti-ua` |
| POS | `tests/fwd_rel_include_forward.sh` relative+forward 无 `no such file` |
| NEG | same 脚本 missing.h 须拒绝 |
| verdict | **PASS**（私建实测） |
| authorizer | 政委代裁方案 A |

## 硬禁自检

- [x] 未 bump version.h
- [x] 未动 Latest / 签
- [x] 旁支 `hotfix/relative-include-forward-1920`，未挡 main K6
- [x] 材料 + NEG 同刀

## 实测回执（落刀前私建）

| 字段 | 值 |
|---|---|
| t_start / t_end | 2026-10-11 ≈19:29–19:30 Asia/Shanghai |
| env_id | Linux；UA=`/tmp/fwd-skip-opti-ua`（worktree 私建，未覆写 `/tmp/ua_ref`） |
| POS | `fwd_rel_include_forward.sh` rc=0；手工 `ua -include apps/csih/h.h u.c` 打印 time 且无 `/tmp/... #include` 错 |
| NEG | missing.h → rc=1，stderr 含 `no such file for #include` |
| cli smoke | `tests/cli.sh` ok 68 wrong 0 |
| verdict | **PASS**（首红未触发） |
| tip | `64603035` (`64603035d9c1b9bd02d22096cce0ce2641897937`) hotfix/relative-include-forward-1920；材料钉 tip `1aa2e99e` |
