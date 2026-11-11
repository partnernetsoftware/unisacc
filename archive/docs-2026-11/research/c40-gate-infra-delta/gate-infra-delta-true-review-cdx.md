# gate-infra 三文件真实内容审核（cdx，独立审）

授权：机房主任02:08，仅材料§5第1–6项。固定源码a8a114a0，相对b8ea2faa；不是当前漂移tip的全面审核。阅读git show的三文件全文、完整diff、第1步回执及K5-1c收口回执/models.py；未运行门或构建。

- [x] 1. 差集一致：独立git diff摘要06761f9b1300ca5ba3416f8977efdd863db9ac210d5a4f5faed980dbf36a894f与附件全等，仅新增opt/pp两个100755 helper及prepare一条调用替换、两条注释。
- [x] 2. opt语义接受（具名限定）：SEED_GEN严格0/1；最多一参数且仅--o2（空字符串按无flag处理）；显式BIN不可执行rc2，执行失败由exec bound传回，无Python回退；默认键覆盖编译器realpath/字节/version输出/flags和seed源内容，编译失败退出，正常缓存复用须摘要等于sidecar。不把该校验升级为并发安全或核键到exec实体守恒保证；共享缓存并发/中断/损坏缓存未证。
- [x] 3. pp语义接受（具名限定）：六flag枚举、重复成员拒绝，分别检索osx/win成员消除了原相邻空格匹配bug；互斥在生成器启动前rc2。SEED_GEN=0为显式参考选择，非失败回退；C本体对互斥的unsupported-let与helper参数拒绝不混称。locations/no-autoinc交叉行为未升级为已测。
- [x] 4. prepare接线接受：目标case原样，OS/ISA flag传入pp helper，外层b失败在set-e下阻止lower；私有DIR由OUT派生覆盖继承值。models以ROOT为cwd、check=True，失败不发布该工作目录，finally清理；manifest仅普通文件，因此缓存子目录随成功rename保留而不复制入OUT。每冷目标冷编、缓存携带子目录、覆盖DIR三代价与01:23批准范围一致。直接调用prepare失败可留部分OUT，不宣称OUT事务性；嵌套限时不宣称整条路线≤55秒。
- [x] 5. 已知限定接受：头注释过时为已准延期；opt尚无产品消费者。模型seed白名单与helper当前11文件相同，不是永久等价；helper64bit截断键不绑定cc1/as/ld，算键到编译/执行有竞态，共享目录二进制/sidecar分次rename，无并发保证。本审不新增并发、损缓存、中断或RSS已证声明，也不撤销现有限定。
- [x] 6. compilercheck影响在限定范围内接受：compilercheck→pipeline/elf→models冷准备→prepare→pp helper调用链成立；同目标C/Python字节对照与受控六目标已有回执，冷models仅lnx/x86_64 rc0。冷Python真实rc/完整argv、运行时SEED_GEN_CC、冷编单段时间和冷后双源码SHA仍UNKNOWN，按01:43具名收口缺项保留；不称独立Python成功退出、六目标端到端或净提速。新增opt helper虽无compilercheck消费，仍由现有.sh过滤规则进入审核清单。

第7项未勾、不执行。真审只覆盖以上三文件固定delta；不认定tools/fixture实际戳已证，不给其他提交新增文件盖戳。更新exec戳仅消除一个资格失败，恢复窄指纹还须其余谓词及K2b准入满足（正式白名单仍空，不能借审核推成真准入）。本审不修改inventory归因/验收，不选择(a)(b)(c)，不授刷戳、复现或仓内写入。

PASS可进处置（仅表示本固定三文件delta内容审通过，可提交授权人决定下一处置；不是处置许可或gate-infra绿）。
