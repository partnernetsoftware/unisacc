# exec-pp-literals measured leaf seal

实际测量，非fixture。冻结输入源提交56131365；原样命令 `python3 exec/pp/literalcheck.py`，仓外 bound58，strace -ff -yy -s10485760 -e trace=%file,read,write,getdents64。最终原样套件rc0，plain/locations均完成；suite.out为原始两行结果。不是整闸、不是7000回填。

streams.tsv 固定44次模型执行：6接受（每模式3个aggregate源流）、38拒绝（每模式18literal+1macroarity），两个模型。每模式的67literal及14macro选择被包含在aggregate输入/输出中，不伪称134次独立调用。每行按 case/id/verdict/rc 后依次给 input、stdout、stderr、model、显式source-label的文件/长度/SHA256。located保留完整UNIPP1字节，不只抽payload。

resources.tsv固定实际模型侧14次资源响应（两模式各7个头，hdr/name对应语言资源命名空间），body作为字节文件保存。模型和输入/响应/源名是核心输入；输出及拒绝字节构成结果见证。观察PID、物理工具、环境、目录与文件查询均非核心语义身份。

construction/executor provenance: dependencies.tsv 91项由实际read/exec读取收集；Python bytecode读取映射到源.py的构造见证，实际.pyc只在仓外tools.tsv观察记录。closure.expected.tsv独立固定测量清单，漏一个实际pp规则必须拒绝；这不是证明所有未来输入域的完备性或自动白名单资格。共享run.c/core.c等当前整文件绑定是实现见证/待拆债，不能据此宣称宿主适配不可剥离。核心输入ID不包含这些实现/宿主记录。

观测原始 tools/env/目录/元数据查询在仓外归档，入仓只有去路径摘要和原件哈希；见末节。

nothing.h更正：初版误以为模型元数据查询；全文件trace显示它只有oracle侧读取，模型不查此路径，不能假造核心空头依赖。stdarg.h初红是捕获解码器把源码payload中的“) =”当系统调用返回值；已修为尾部结果并有解析回归自测，不曾绕过长度核验。

仓外trace-index.tsv钉74个原始trace文件（原件仅仓外/tmp/c43-pp-literals-filetrace，约83MB，不塞仓库）。原始采证工具是strace，数据提取是ppleafcapture.cx，合同ppleafseal.cx只核验固定字节，不执行generator/oracle/model。该data包由/tmp/c43-pp-literals-frozen只读副本复制。

验证：--check-core rc0；--check-observation只报告NOT_RULED；--negative漏规则与修改规则/实读头文件均红。复用review仍required；尚无完整传递读取白名单授权，没改gatequeue/gatedeps。

## 提交前分层修订（cc 预审）

环境、绝对工具路径、PID/临时目录、原始文件查询、目录列表和 trace-index 原件只保存在仓外 `/tmp/c43-pp-literals-observation-archive`；其文件长度和哈希见 observation-archive.tsv。入仓 observation-summary.tsv 只保留无路径工具名、内容长度、SHA256（版本字串未由原始测量采集，不编造；内容哈希绑定精确版本字节）。这些不参与核心检查。`--check-observation` 仅报告 NOT_RULED，不据观测变化判核心红或绿。先前本页的完整 tools/env/目录核验描述属于采证过程，已退出入仓合同。

两份 net 模型暂保留实际执行字节：目前没有证明构建器在不同宿主/工具环境下能逐字节确定性重建；只留哈希会让合同无法独立核验实际模型字节。可在独立重建对拍证明后再改成可取得的内容寻址资源，不能本刀先删见证。

streams.tsv 只含模型执行的输入、输出、拒绝和显式传入源名；没有 cc -E -P 输出。参照输出留在原始仓外 trace 的观测层。44 次源名流保留真实调用字节，故其中物理临时名称仍是显式输入而不是宿主窥探；不修改历史字节来冒充新测量。未来规范虚拟源名须重新实测。

负例在私有数据/源码镜像做三项：漏实际规则依赖、predefines.tsv 增加一行、被 b_ppif.c 间接使用的 stdarg.h 增加一行，均必须红；不改共享源码。目录/absence/元数据查询没有进入核心合同。

identity.tsv 是固定核心输入与输出见证摘要；核验重算后必须逐字节等于该数据，不能仅打印新摘要作为通过。
