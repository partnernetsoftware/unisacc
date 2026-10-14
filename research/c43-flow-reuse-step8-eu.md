# 名单外读取审计：exec-/unisa- 组（批次记录，只读）

方法：每个作业 `strace -f -e openat` 串行，≤55 秒；读集与 `queue` 名单（`tests/gatequeue.py:186–189` 同样的 git ls-files 名单）对比。
先做未声明的 45 个（exec- 组），再做 143 个已声明的。候选字节与构造缓存另记。

## 批次 00（exec- 未声明，10 个）
| 作业 | rc | 名单外读取 | 结论 |
|---|---|---|---|
| exec-e4-1、exec-e4-2、exec-e4-3、exec-e4-4 | 0 | 无 | 无需声明 |
| exec-selfprep-table | 0 | 无 | 无需声明 |
| exec-selfprep-x86、exec-selfprep-arm | 0 | 无（读候选：构造缓存中的 models.pkg，不是候选） | 无需声明 |
| exec-tableself-1、exec-tableself-2 | 0 | 无 | 无需声明 |
| exec-srcelf-1 | rc124（55 秒） | 无（部分读集，名单内） | 不完整，未声明 |

结论：批次 00 在名单之外没有读取。

## 批次 01（exec- 未声明 2 个 + unisa- 未声明 8 个）
| 作业 | rc | 名单外读取 |
|---|---|---|
| exec-srcelf-2、exec-srcelf-3 | 0 | 无（读候选：构造缓存的模型包） |
| unisa-bits、unisa-hostabi、unisa-peversion、unisa-padded、unisa-zdata、unisa-linalg、unisa-libneed、unisa-uns1 | 0 | 无 |

结论：批次 01 名单外没有读取。

## 批次 02（unisa- 未声明 10 个）
| 作业 | rc | 名单外读取 |
|---|---|---|
| unisa-catalog、unisa-fp、unisa-tape、unisa-image-fat、unisa-image-elf、unisa-image-macho、unisa-image-pe、unisa-ape | 0 | 无 |
| unisa-tapebin、unisa-tapebin-shape | 1 | 无（读集在名单内） |

结论：批次 02 名单外没有读取。注意两项 rc1 是真实失败，不是读取问题：单独直接运行仍是 rc1（`unisatapebincheck.sh`：`DIFF encode`；`unisatapebinshapecheck.sh`：`DIFF render`）。输入文件在工作区没有未提交改动，最近的相关提交是 fdc1c1ab、701d76e8、7b4ac452，尚未定位，需要 owner 判断。

## 批次 03（unisa- 未声明 10 个）
| 作业 | rc | 名单外读取 |
|---|---|---|
| unisa-vm、unisa-gold-prec、unisa-emit-arm-nwmem、unisa-gold-lex-label、unisa-gold-type-label、unisa-gold-binsel-label、unisa-gold-peep-label、unisa-gold-scope-label、unisa-gold-pp-label | 0 | 无 |
| unisa-gold-opinfo-label | 1 | 无（读集在名单内；rc1 未分析，待单独判断） |

结论：批次 03 名单外没有读取。
