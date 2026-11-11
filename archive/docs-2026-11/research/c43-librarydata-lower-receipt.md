# 2c1 lower 最终仓外收据 — 实际模型验证，非产品/平台绿

模型验证以仓外冻结overlay完成，本笔收录模型、参考与自测；未接API、未开整闸。动态C运行器是旧产品观察工具，不能当作本笔产品行为验收。

实际跑过：
- Linux x86_64 Python/C(seed) JSON逐字节同，4310状态；ARM64同样C/P字节同，4712状态。
- 27 canonical夹具每条tbl/net与实时独立参考收拒同。拒绝rc1、stdout空、具名诊断；接受片段与live参考及冻结期望逐字节同。所有UNILOW1核精确长度、非重叠offset/真实0占位、length1、role/reserved0及full ID多重集覆盖ordinary+data。
- 动态C检查实际PASS27 tbl/net及不同cwd/env回放；旧ordinary动态C13例PASS。
- 两种ordinary/data混合顺序各2full/1ordinary/1data，tbl/net完整产物与冻结对照同，普通/数据片段与各自参考同。
- 真实E3完整main body readonly-read与writable-write rc0，完整emit与pretracker字节相同；readonly-write rc1、stdout0、stderr点名symbol256/namehostvalue。
- source.str初值7与.unit2/.gdef object控制各tbl/net rc0，full/data外部span0且live参考同。DEFINED1走旧LBD.owned，不读provider。这未证明库API注入99后实际执行返回7。
- contract_model独立ARM AF/AI固定NEG各tbl/net rc1、stdout0、not-covered含256/name；五合法控制(AF/AI可写、AF/AI无data、RO读)各tbl/net rc0、完整输出等before，共14/14。仅固定模型fusion见证，不称整个ARM平台或六目标绿。

首红：Linux readonly暂存链曾rc0/304B；PF/PL fusion跳过tracker。真实scanner已拆括号，非raw token布局错误。保留原emit，在fusion前dry消费被吞指令后恢复op/na/a0..a6，原同反例现拒，不改优化字节。ARM AF/AI原skip漏洞同法修，原AF误收290B现拒，未换反例/判据。


提交基准为71c05bd2。生产模型源与自测摘要随本笔提交；ABI、reviewed_trees与seedmemory在该HEAD干净导出加本笔限定路径上生成。资源/RSS重测收据在下节追加，未完成之前不得计派生绿。

## lower 构造读集与 RSS（实际跑过）

基准71c05bd2干净导出加本笔模型文件，C以`cc -std=c99 -O2 -Iseed seed/gen.c`构建；每条构造55秒内，串行、strace与直接os.wait4 RSS分开。八route×C/P×trace/RSS共32次全部rc0，逐route四个产物摘要相同。读集保留旧家族并集，只新增八张librarydataspan/librarydataprov表；parse2未增加输入。gen键`a0f1253495dbefa6`→`07496d0240e2c51a`，parse2仍`1292f970a84806f1`。

| route | C峰值KiB | C_RSS MiB（floor；入场另加原有1 MiB） |
|---|---:|---:|
| lower-lnx-x | 244612 | 238 |
| lower-lnx-a | 269580 | 263 |
| lower-osx-x | 237920 | 232 |
| lower-osx-a | 265164 | 258 |
| lower-win-x | 181532 | 177 |
| lower-win-a | 209372 | 204 |
| obj-lower-x | 248740 | 242 |
| obj-lower-a | 273480 | 267 |

量测工具是仓外观察层，sha256 `45edb7707c05ac6848cff6f36ffbf34f775d77e048a0d0b2a985059b923f60cc`；原始32份收据与摘要落仓外，不把宿主路径或环境纳入核心合同。合并收据sha256 `fd7cce8e40be6422c7ed13d7eb4d2dcc9547a81ad8dbf9d0ca3e24b63c9d2efb`。只重测受本笔lower读集影响路线，未删除未重测路线依赖；Python冷构造仍UNKNOWN。

## 入仓检查与身份

实际派生套件：seedmemory rc0、abi-manifest rc0（431条）、reviewed-trees rc0；script-inventory rc0（521脚本，orphan0）、gate-layers rc0。新exec-librarydata-model仅模型自测，root旧产品是动态C观察运行器，不作新产品行为证据。三路线缓存按源码内容键准备并复核产物摘要，准备与执行各自限时。

| 文件 | sha256 |
|---|---|
| exec/lower/librarydataprov-byte.tsv | 8849ee4933cd1adaff077c298aa8f275f2facc111dbcf4d39b19bace93ac35a0 |
| exec/lower/librarydataprov-manifest.tsv | c2c8c059d2bcfdb129f7bcafdcf84ad196d2072dcab7d9850f0efa5572bc9b52 |
| exec/lower/librarydataprov-result.tsv | 8300a72f9a90c2d7c5da13e20b0740715c320df10ec4270bd658aff3ca563dde |
| exec/lower/librarydataprov-template.tsv | 30f8b764b9c91a2f7fb0326a90dfb758e0faf83f17cf02fd0018ac2b662e1c0d |
| exec/lower/librarydataspan-byte.tsv | 950d95605350d1a023986d8c6bba3882f2075c6cff27428783a482b908e6bf8c |
| exec/lower/librarydataspan-manifest.tsv | 1bc6fc1bc184ecf853427edba00ffa186d4f94d8e4aedd7c513e09d65d212924 |
| exec/lower/librarydataspan-result.tsv | afdc05b61d2470263be2bf7d2767cb3aeba2a6316541e1e742d178ed961ae354 |
| exec/lower/librarydataspan-template.tsv | d932d813d3d86729532ca6b5f7620233a962613f912b5f6f4e49304e5a172f83 |

命名闸实际：`exec-librarydata-model rc0`，39例、tbl/net、实时参考、完整span覆盖、旧发射与cwd/env回放，父独立复跑7秒（旧产品只作动态C观察器）；full/arm/object准备各原55秒窗rc0，C/P同字节。既有ordinary模型13例仓外动态C通过。本笔不触发或声称库API验收。

提交闸首轮：命名39例/ABI rc0，派生inventory/seedmemory rc0；reviewed拒绝源于derivedcheck WORKTREE新文件导出未保留执行位（新librarydatacheck.sh规范0755被写成0644），不是模型对拍红。协作方补git_file_mode/chmod后，父按原26路径导出复核reviewed rc0；不调整快照迁就错误导出，原命令待工具提交后重跑。
