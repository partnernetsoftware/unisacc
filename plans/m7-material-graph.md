# M7 全批材料一键构造（盘点）

目标：候选 APE 一条命令产出可自构造材料目录+`materials.tsv`+每份命令与 sha。**不伪造 ref**（ref12+streams,ref10 只标明来源）。

## 依赖图（s8c REFUSE ~40）

```
candidate APE (unisacc.com)
  ├─ tools/kernel-route.cx ROOT OUT [--isa x86_64|arm64]
  │     → OUT/<isa>.kernel, route.tsv     喂 kernel-route×9
  ├─ tools/object-material.cx prepare ROOT MAT
  │     → objects-material-v1 PREPARED    喂 objects×9
  ├─ tools/ri-flow-material.cx compile|seal ROOT MAT
  │     → ri-flow PREPARED                喂 ri-flow-profile×9（另需与 raw root 一致的 bootstrap）
  ├─ ri-chain 材料链（ri-material / 既有 mat/ri-chain）  ×7 回执须对应当前 APE
  ├─ p3-library 规则输入（p3-pack-ctx 已图，缺 regular-input 文件）×4
  └─ pp-gold 须对应当前产品 content                       ×2
ref / streams,ref：不可自造，标来源，不进一键。
```

批数/成本：objects9（三编译，~40s）→ kernel-route9（十一单元 as+link）→ ri-flow9（compile+seal）。

## 不可构造

ref×12、streams,ref×10：冻结参考/P4，只写来源，不造伪。
