# 中段完整流窄实测

模型来源/是否原件见model-origins.tsv。相同网络字节合并存储为每阶段一个model.net；原始双提取件在本次实测分别传入，不以两个相同副本伪称独立构建。3000原候选不可复验，cc仅独立复验7000来源。

E3：产品真实lnx/x86_64/tape/O0路由e2→e1，attr_strip后保留UNITOK定位帧；实际attributes响应为4字节零计数，并非未采；逻辑源名leaf/middle.c固定。资源ledger记录所有实际查询，包括library/cli等明确absent；replay未知key立即失败，禁止原适配器磁盘fallback。core的source参数另有source-label.bytes；采证适配器cli/source按compiler.c的argbytes协议含末尾NUL，E3本例没有读取该key。

lower：合法完整E3→prune tape为输入，接受方向同一窄回放；red里的?\n对参考parse→lower域有实测差异，停止、未修、未把拒绝未验证说成绿。

动态C入口middleflowcheck.cx；middleflowrunner.c为共享C执行器采证/回放包装器，宿主cc无-Iinclude。捕获用现有产品包和真实路由，回放用确切提取网络及显式资源，不运行两个候选各自的ISA核心，因此见证限制于这份共享执行器与选定用例。执行燃料显式固定，环境里的UNISA_MAXSTEPS不作为隐含输入。环境变体包括LANG/TMPDIR/UNISA_ATTRIBUTES/UNISA_CONTAINER/UNISA_MAXSTEPS；不声称已穷尽全部宿主泄漏。

stdout/output.bytes、诊断capture.stderr、rc/status.txt分别保存。原生reference.stderr含本机路径留仓外，只存其哈希和去路径诊断。文件身份见manifest.tsv；没有复用授权，不回填7000、不是完整套件结证。
