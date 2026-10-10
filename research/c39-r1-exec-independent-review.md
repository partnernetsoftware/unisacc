# exec 戳独立二审：待真审→审完交机房主任

审核人cdx2；fetch后origin/main与本地tip2ca3d8d7。只审不裁，不改gatedeps/ACCEPT/归因，不记APPLIED。

独立核定范围：git diff 8a433c5d^..origin/main -- exec，三条变化为TSV修改、R100夹具迁移、新增夹具；未见同区间其它enc变动。不是照抄作者结论：另读实际状态表、指令kind定义、写字节分支、参考bk_label/call及check.sh，然后以git archive内存读取重算戳。

| 树 | 后缀筛选成员 | 独立重算exec戳 |
|---|---:|---|
| 8a433c5d父树 | 1408 | 476d03bd76172cea775fcbed87e8b2e99c9270b74c90b974a810ff723ee3b77d |
| 8a433c5d | 1408 | 9ae3c35897c7185c7b2ef2c11f6985d5281aafb424fe57b9668c2eabd3d34036 |
| fetch后origin/main | 1408 | 9ae3c35897c7185c7b2ef2c11f6985d5281aafb424fe57b9668c2eabd3d34036 |

算法按compilercheck inventory_suffixes、仓相对路径、归档权限及内容SHA的有序JSON；无写刷新脚本。txt不纳此戳，所以夹具迁移/新增须另审，不能凭戳相同免审测试变化。

逐条语义：
1. x86-procs-result的LADDR取TGT/LABD后判0；只把原未定义分支转到LA.undefined。既有已定义LA.1/LA.in/LA.end不改。
2. x86-line-byte里call的KND=3，jump=1，jumpz=2。新分支仅KND3赋la=0返回；其它KND走原DEAD.undef→undefined，未把jump/jumpz放行。
3. WR.c仍先PUSH返回点调用LADDR，然后输出232(E8)、d=la-(OFF+5)四字节。OFF0未定义call字节按公式为e8 fb ff ff ff，与参考back_encode bk_label未定义返回0及普通call公式一致；未独立执行原生指令。
4. 新状态只读KND、使用临时k并写la；未改目标标签表、尺寸/布局。编码接受范围是所有raw tape未定义call，非“编码器证明不可达”；可达源代码的未定义函数是否仍被前端拒绝是另一层义务。本审不把该编码能力当源级合法性证明。
5. neg-callundef→x86-callundef R100，call Nowhere文本逐字节保留；check.sh正向x86循环要求C/Python与参考编码字节一致。原neg-undef的jump Nowhere仍在拒绝循环，要求两执行器rc1及not covered。新增jump live/unused两call/ret/live ret同时检布局与偏移，未删测试。
6. 参考bk_objmode未定义call走独立relocation分支，当前普通enc审阅不能证明全部对象ABI或原生六平台行为。此前§24合并30项构造对照及COV1产品分片为佐证，不替代这些未声称义务。

审核结论：指定戳变化与三路径diff对应，未发现本范围的阻断；普通x86未定义call编码与所读参考公式一致，其他未定义kind拒绝保留。交机房主任决定审核通过及戳/状态处理，本窗不代裁、不记APPLIED。后续版本发布仍须新候选身份与正式出口。

旁路观察：独立gh查询v0.0.39 release not found；release-smoke最新仍38024805460、Defender最新38025538035，都是旧0.0.38公开字节运行。未见真实新Draft court；这些旧绿不能抵R3。fetch/查询只读，无发布动作；未恢复examples删除、未重构建或复跑重活。
