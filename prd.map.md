# UNISA SH —— 记忆宫殿

> 记全局用。规格正文见 [`prd.md`](prd.md)，开工清单见 [`prd.tree.md`](prd.tree.md)。
> 用法：按 **门厅 → 图书馆 → 道场 → 金库 → 神谕厅 → 工坊 → 熔炉 → 靶场 → 装船口** 的顺序走一遍，每个房间只记它的「锚」和那几个数字。走完一圈，整个系统就在脑子里了。

---

## 一、宫殿全景（九个房间）

```mermaid
flowchart TD
    subgraph R1["🚪 一号厅 · 门厅 —— 命题"]
        A1["Shell = 推理器 + 执行器 + 模型数据"]
        A2["① 经典代码 走查/符号表/文件头/重定位<br/>② 模型数据 14 个阶段（含只在对照臂的 isel/combo）<br/>③ 唯一 kernel"]
        A3["kernel: embed → gemv → ReLU → gemv → argmax<br/>部署无 softmax 无 libm"]
        A1 --> A2 --> A3
    end

    subgraph R2["📚 二号厅 · 图书馆 —— Gold"]
        B1["锚：墙上八个书架，每架一张表"]
        B2["pp 18 · lex 121 · parse 270 · type 960<br/>scope 30 · irsel 135 · enc 228 · reloc 6"]
        B3["FULL gold = 完整笛卡尔积<br/>gold 既是标注，也是兜底，也是验证器"]
        B1 --> B2 --> B3
    end

    subgraph R3["🏋️ 三号厅 · 道场 —— 训练"]
        C1["锚：三道门槛刻在地板上"]
        C2["0.85 接管 · 0.995 热跳过 · 1.000 出货"]
        C3["mulberry32 · Adam 0.9/0.999<br/>LR 按 epoch 衰减 18/50/90<br/>batch 16 · 停在 90"]
        C4["超级拟合是目标，不是缺陷<br/>到不了 1.000 = key 编码错了"]
        C1 --> C2 --> C3 --> C4
    end

    subgraph R4["🗄️ 四号厅 · 金库 —— UNS1"]
        D1["锚：一排保险箱，越往里越小"]
        D2["f32 → f16 → i8 → q4 → q2<br/>i8 整张量 · q4/q2 按行 scale"]
        D3["唯一判据：argmax 不变性<br/>逐阶段取体积最小的不变 dtype"]
        D4["实测：8 个止于 q4 · 3 个止于 i8<br/>q2 全线失守 · 混合只省 4.5%"]
        D1 --> D2 --> D3 --> D4
    end

    subgraph R5["🔮 五号厅 · 神谕厅 —— Oracle"]
        E1["锚：厅中一个岔路口"]
        E2["ask stage, key → class_name"]
        E3["acc ≥ 0.85 走网络<br/>否则走 gold<br/>run 末尾报 nets k/10 driven"]
        E4["两条铁律<br/>P-1 只回传类名 禁读 logits<br/>P-2 无条件断言 key 属于 K_s"]
        E1 --> E2 --> E3 --> E4
    end

    R1 --> R2 --> R3 --> R4 --> R5

    classDef room fill:#1f2937,stroke:#6366f1,color:#e5e7eb
    class A1,A2,A3,B1,B2,B3,C1,C2,C3,C4,D1,D2,D3,D4,E1,E2,E3,E4 room
```

```mermaid
flowchart TD
    subgraph R6["🏭 六号厅 · 工坊 —— 前端"]
        F1["锚：一条流水线，六个工位"]
        F2["pp → lex → parse → type → scope → irsel"]
        F3["走查器是经典代码<br/>只有选表那一下问 Oracle"]
        F4["产出：通用 tape<br/>r0–r7 · r7=SP@0x10000 · 64KB LE"]
        F1 --> F2 --> F3 --> F4
    end

    subgraph R7["⚒️ 七号厅 · 熔炉 —— Lowering"]
        G1["锚：炉口分出六条铸道"]
        G2["abi sysno/arg0-5/ret/gate/nrreg<br/>enc 五种 form · reloc 三种<br/>regmap tape 寄存器 → 机器寄存器"]
        G3["lnx osx win × x86_64 arm64"]
        G4["osx sysno = 0x02000000 或上 nr<br/>这一位是承重的"]
        G1 --> G2 --> G3 --> G4
    end

    subgraph R8["🎯 八号厅 · 靶场 —— fold"]
        H1["锚：六个靶子，必须六发全中"]
        H2["每个目标独立：<br/>tape → lower → TargetProgram → 镜像 + 目标机解释"]
        H3["解释器只认 os,sysno 或 os,winapi<br/>绝不回看 tape op"]
        H4["反证：--fault osx_class_bit → 4/6<br/>若仍 6/6，说明测试是假的"]
        H1 --> H2 --> H3 --> H4
    end

    subgraph R9["📦 九号厅 · 装船口 —— Ship"]
        I1["锚：一个箱子，旁边摆着 tcc"]
        I2["kit.zip = weights + MANIFEST + kernel + images"]
        I3["ELF 7f454c46 · MachO cffaedfe · PE 4d5a"]
        I4["权重 ≤ 32KB · kit ≤ 64KB<br/>同输入两次编译字节相同"]
        I1 --> I2 --> I3 --> I4
    end

    R6 --> R7 --> R8 --> R9

    classDef room fill:#1f2937,stroke:#f59e0b,color:#e5e7eb
    class F1,F2,F3,F4,G1,G2,G3,G4,H1,H2,H3,H4,I1,I2,I3,I4 room
```

---

## 二、管线真图（精确数据流）

```mermaid
flowchart LR
    SRC["in.c"] --> PP["pp<br/>dir × defined"]
    PP --> LEX["lex<br/>charc × peek"]
    LEX --> PAR["parse<br/>NT × TOK"]
    PAR --> TY["type<br/>t1 × op × t2"]
    TY --> SC["scope<br/>ctx × kind"]
    SC --> IRS["irsel<br/>family × flavor"]
    IRS --> TAPE["通用 tape"]

    TAPE --> VM["vm.py 解释器<br/>★基准真值"]

    TAPE --> LOW["lower<br/>abi · enc · reloc · regmap"]
    LOW --> T1["lnx/x86_64"]
    LOW --> T2["lnx/arm64"]
    LOW --> T3["osx/x86_64"]
    LOW --> T4["osx/arm64"]
    LOW --> T5["win/x86_64"]
    LOW --> T6["win/arm64"]

    T1 --> X["exec_target<br/>per-arch 寄存器<br/>per-os syscall 分发"]
    T2 --> X
    T3 --> X
    T4 --> X
    T5 --> X
    T6 --> X

    X --> CMP{"六份 stdout/exit<br/>是否全同"}
    VM -. 对拍 .-> CMP
    CMP -->|是| OK["6/6 match"]
    CMP -->|否| BAD["退出码 2"]

    LOW --> IMG["image<br/>ELF / Mach-O / PE"]

    ORC(("Oracle")) -.决策.-> PP
    ORC -.决策.-> LEX
    ORC -.决策.-> PAR
    ORC -.决策.-> TY
    ORC -.决策.-> SC
    ORC -.决策.-> IRS
    ORC -.决策.-> LOW
    W[("weights/*.unisa")] --> ORC
    G[("gold 表")] --> ORC

    classDef net fill:#312e81,stroke:#818cf8,color:#e0e7ff
    classDef cls fill:#064e3b,stroke:#34d399,color:#d1fae5
    class PP,LEX,PAR,TY,SC,IRS,LOW net
    class VM,X,IMG,CMP cls
```

**读图要点**：蓝色是**学出来的表**，绿色是**经典代码**。Oracle 是唯一把两者接起来的点；`weights` 和 `gold` 都插在 Oracle 上——这就是"换权重即换能力"和"gold 是验证器"在同一张图里的样子。

---

## 三、门禁状态机（推进顺序）

```mermaid
stateDiagram-v2
    [*] --> M1
    M1: M1 数学与网络
    M2: M2 权重落盘
    M3: M3 tape + VM
    M4: M4 C99 前端
    M5: M5 Lowering + 六目标
    M6: M6 镜像
    M7: M7 Ship
    M8: M8 验收与度量

    M1 --> M2: acc 全 1.000 且训练两次字节相同
    M2 --> M3: 554e5331 且 quant 阶梯已定
    M3 --> M4: 手写 tape 跑通
    M4 --> M5: 七样例出 tape 且 VM 输出正确
    M5 --> M6: ★ fold 6/6 且 fault 退化到 4/6
    M6 --> M7: 三魔数正确 且 两次编译字节相同
    M7 --> M8: kit 四件套 且 权重 ≤ 32KB
    M8 --> [*]: 对比 tcc 体积

    M1 --> M1: 卡住则改 key 编码 不许加宽网络
```

---

## 四、随身卡（最容易忘的十个数）

| 记 | 是什么 |
|---|---|
| **0.85 / 0.995 / 1.000** | 接管 / 热跳过 / 出货 三道门槛 |
| **38 = 19 + 19** | OPS = SYSOPS + MOPS，顺序写死 |
| **52 = 16+8+8+12+8** | combo 的 h0 五段分解 |
| **9 heads** | form symbol gate sysno arg0 arg1 arg2 ret tls |
| **18 / 50 / 90** | LR 衰减的三个 epoch 断点 |
| **0x02000000** | osx sysno class bit，反向对照就抽它 |
| **0x10000** | r7 初值，也是 64KB 内存上界 |
| **1/19** | type 表 illegal 行的训练下采样率 |
| **6/6 与 4/6** | 正向验收 与 反向对照 |
| **7f454c46 / cffaedfe / 4d5a** | ELF / Mach-O / PE 魔数 |
| **P-1 / P-2** | Oracle 不透明性 / key 全域性——组合等价与唯一真推理义务 |
| **14 个模型** | 每决策点一个；发布路径（`--drive built`）用 12 个，`isel` 与 `combo` 只在对照臂 |
| **21,945 θ** | 全部训练产物；默认 spec 路径 11,892 θ |
| **P-8** | 存在性已构造式证明 —— 开放的是最小性，不是可行性 |
| **q4 / i8** | 8 个阶段止于 q4，3 个止于 i8，**q2 无一幸存** |

---

## 五、三条红线（走出宫殿前默念）

```mermaid
flowchart LR
    L1["① fold 不许退化成<br/>通用 tape 跑六遍自比"] --> R1["那就不是测试"]
    L2["② acc 不达标<br/>不许加宽网络"] --> R2["是 key 编码错了"]
    L3["③ 走查器/符号表/文件头<br/>不许神经化"] --> R3["它们是代数，不是表"]

    classDef red fill:#7f1d1d,stroke:#f87171,color:#fee2e2
    class L1,L2,L3 red
```
