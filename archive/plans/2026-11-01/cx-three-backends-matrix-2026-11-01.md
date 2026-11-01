# CX 三后端矩阵：首轮 MATCH/DIFF

## 身份

- unisacc 候选：`out/p5-rebuild/ape/unisacc-next.com` sha `552c5f96…`
- native gcc：`out/cxcc-host/gcc_cx_lnx64-x86.bin`
- native clang：`out/cxcc-host/clang_cx_lnx64-x86.bin`
- 根 `.com` 仍为旧 APE；APE 新构建被 `cxproc` 交叉目标可见性阻塞，因此本表**不是 APE 证据**。

## 语料与结果

| 用例 | unisacc plan/run | gcc plan/run | clang plan/run | 结论 |
|---|---|---|---|---|
| `ms1` 多符号 | rc0 / 3，1 CXIF | rc0 / 3，1 CXIF | rc0 / 3，1 CXIF | MATCH |
| `ms2` 整模块+符号 | rc0 / 3，1 CXIF | rc0 / 3，1 CXIF | rc0 / 3，1 CXIF | MATCH |
| `bad` 未知 pragma | rc1 / 1 | rc1 / 1 | rc1 / 1 | MATCH |
| `top` 同 realpath 冲突 | rc1 | rc1 | rc1 | MATCH |

当前首轮 `DIFF=0`，但只覆盖多符号、混序、未知 pragma、realpath 冲突；不能写成全部三后端资格。

## 复现命令

```sh
C=out/p5-rebuild/ape/unisacc-next.com
G=out/cxcc-host/gcc_cx_lnx64-x86.bin
L=out/cxcc-host/clang_cx_lnx64-x86.bin
$C -cx-plan FILE.cx; $C -run FILE.cx
$G -cx-plan FILE.cx; $G -run FILE.cx
$L -cx-plan FILE.cx; $L -run FILE.cx
```

## 未覆盖

- APE 入口上的同一矩阵；APE 构建仍被交叉目标 `cxproc` 可见性阻塞。
- alias、cxpack、注释/续行/条件等其余语料尚未并入本表。
- native PCH 冷/热收益不等于 APE 收益。
