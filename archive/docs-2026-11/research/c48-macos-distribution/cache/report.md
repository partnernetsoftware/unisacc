# 历史阶段记录（已被../report.md最终实测结论取代）

正规`release-ape.cx pack OUT P3 SLICES...`现在默认以当前冻结产品分别有界构造Mac x64/ARM两个CX管理器（子进程bound50），嵌入APE；构造收据绑定源码、产品与两个helper实际SHA。显式`pack --cache-manager-osx X64 ARM64 OUT ...`仍可接声明材料，最终收据以BUILT_CURRENT_PRODUCT/EXPLICIT_INPUTS区分，不把显式输入假称当前构造。低层generic `ape-pack.cx`未传manager保持旧合同；新参数`--cache-keep N`默认4，范围1..32。未更新根产品、安装、签名或Release资产。

Darwin launcher仅提取并调嵌入native管理器，retention算法为CX。当前项计入最近N且刷新标记时间，再额外保留活跃租约；mtime相同按key确定排序。传launcher PID，正常exec后PID不变；PID复用宁多保留。仅管理owned私有目录中严格TOOL-16hex的owned regular单link文件，不递归。完整实际SHA与标记不匹配、链接、未知条目不删除；校验失败或非阻塞flock忙则重新从原APE隔离提取执行。正常回退退出删除已知run文件及私有目录。尚未声称恶意同UID并发/ABA安全。

## 旧缓存迁移与dry-run

`cache-retain [--dry-run] [--adopt-legacy ACTIVE_LIST] CACHE TOOL RAW_CURRENT_KEY KEEP LAUNCHER_PID`。必须已有当前raw-key文件；传入PID必须是管理器实际父PID。ACTIVE_LIST首行`CACHE_ACTIVE_V1<TAB>缓存realpath<LF>`，其后每行`TOOL-16hex<TAB>实际活跃PID<LF>`；可同名多PID，禁止重复pair、非法名字、CR/NUL及目录错配，≤64KiB/512条。列表必须由调用方一次有界lsof审计完整生成；本工具不把列表或SHA视为可执行信任。

显式adopt把旧压缩blobSHA键作为原名保留，V2标记绑定name/type/fullSHA，不要求旧filename匹配rawSHA前缀；同一N集合淘汰，并为审计到的各实际PID建租约，避免审计父退出后失保护。没有审计授权时，无标记旧项保守保留，不能声称全缓存已限N；旧launcher日后产生的新无标记文件需要再次审计迁移。dry-run不创建/更新标记、租约、mtime或文件，仅锁目录并读取和输出计划。真实135项未执行迁移或删除。

## 实际证据

- 独立默认检查：3117实际exit0，16/0；原10项全部保留。新增dry-run无marker/mtime变更、压缩键旧项活跃迁移、实际PID租约、同N淘汰、错保护目录和锁争用拒绝。第一次默认检查48844与第二阶段11784均10/0；未删旧断言。
- 默认正规pack：15366、55102实际exit0，均实际构造两helper。55102绑定源码`9990884b3575965ac07e00b03de08cb33f42d2bc3cc95cfc76377a2af1713799`、产品`f069d4c9…`；产物`release-default-v3/unisacc-next.com` SHA `a28c0a2e3462362845036ebe25135839458e73f705ac3e7f8b047f63a24f98d8`。私有XDG真正Darwin hello实际exit7；产生管理标记和租约，证明非静默fallback。
- 独立实跑回退：9628、62317实际exit0；锁忙和坏marker分别触发隔离hello，均返回7，临时目录剩0。探针源码在`out/c48-macos-distribution/fallback-check.cx`，不是默认检查的隐式out依赖。
- 低层旧模式：26554、97543实际pack exit0；62317末尾实际cmp0，before/after都是2,668,832B。无manager旧路径字节一致。
- 最新隔离目录分支修正后，32684实际打包0；`private-v4.com`实际hello7。最终release收据mode字段变更后6242实际已装产品实编并正确usage拒绝2；默认pack端到端证据来自mode字段变更前，未假称最终metadata版本重跑。

私有APE是入口探针，非宿主槽刻意使用占位输入（x64槽复用ARM hello，LinuxARM槽复用x64 ELF），绝非可分发六目标候选；两个manager本身均为正确目标。x64 native执行、异常信号/解包失败时临时目录回收、正式批次登记及实际旧缓存审计迁移仍OPEN。初轮openat未暴露与缺mode参数编译失败均已报告，后续改用sys.posix并实编成功。

README首屏四步固定已发布0.0.47与真实SHA，安全说明采用父实际证据：Safari直连GitHub504；官方同字节经本机HTTP转存由Safari自然添加quarantine，未清除标记/关闭保护；Gatekeeper仍拒绝，CLI成功不等于信任通过。最低macOS13为镜像声明，实测仅26.5.1。
