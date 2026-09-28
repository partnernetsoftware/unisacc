# R9-5 Linux 目录枚举切片（设计先记，2026-09-28）

只实现 Linux getdents64 三参数 intrinsic，系统号来自 catalog：x86_64 217，arm64 61。macOS/Windows 缺事实必须拒绝，不用其他调用猜替代，不引入 fork/exec。

arm64到通用号表的官方包含链另见[arm64 unistd v6.10](https://raw.githubusercontent.com/torvalds/linux/v6.10/arch/arm64/include/uapi/asm/unistd.h)。

官方事实来源：[Linux x86 syscall表](https://raw.githubusercontent.com/torvalds/linux/master/arch/x86/entry/syscalls/syscall_64.tbl)、[asm-generic syscall表](https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/asm-generic/unistd.h)、[getdents64实现](https://raw.githubusercontent.com/torvalds/linux/master/fs/readdir.c)。记录采用 Linux dirent64：ino@0、off@8、reclen@16、type@18、name@19；变长记录按8字节对齐。

新增最小 dirent.h：opendir/readdir/closedir，4096字节多批读取，errno区分EOF和错误；记录剩余长度、reclen下界/对齐/上界及名字NUL有界验证，坏包EIO且不发布残缺记录。参数与局部用__u_前缀，避免头内自动局部同名覆盖调用者。非Linux调用intrinsic由后端拒绝。

procview枚举/proc数字目录，再读status。数值合法且范围不靠PROCMAX；目录失败、不可读/消失status、容量上限分别说明。扫描期间消失和权限不可读无法仅靠现有fopen精确区分，合并计数并如实命名。RSS字段缺失不假称权限拒绝。可用编译期PROCVIEW_PROC_ROOT测试私有目录，不修改默认/proc入口。

测试计划：host C shim直接调用实际头，合成坏record/多批/error；私有/proc形状含>20000 PID、无status与非数字目录；Linux实际目录多批及长名留客机验证。生成物和正式模型由父统一重建；本片不把两条拒绝相等当正例。

## 实际验证与边界

实际跑过：macOS cc shim多批/8类错误、Linux Lima default arm64 cc shim真实701目录项多批与245字节长名、900001高PID、状态文件先创建后删除、实际chmod目录权限拒绝及缺路径；客机树由管道送入私有临时目录并清理，未改共享UA/rootcom。

首轮真实arm64实验因O_DIRECTORY误用x86值而失败，保留诊断：arm64为16384、x86_64为65536；修正后相同测试通过。官方来源：[arm64 fcntl](https://raw.githubusercontent.com/torvalds/linux/master/arch/arm64/include/uapi/asm/fcntl.h)、[generic fcntl](https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/asm-generic/fcntl.h)、[dirent64布局](https://raw.githubusercontent.com/torvalds/linux/master/include/linux/dirent.h)。

Python gold路径实际验证：getdents64正例在Linux两ISA生成tape并lower；macOS两ISA及Windows两ISA明确拒绝。完整procview两Linux目标的Python前端/lower均成功（26549条tape）。这是规则参考和host-shim执行证据，不是当前正式模型包的网络/原生产物验证。C前端新增分派、通用Windows none-import守卫与实际构造模型待父统一生成/冻结门禁；Linux x86_64实跑未做，macOS/Windows目录API不提供。
