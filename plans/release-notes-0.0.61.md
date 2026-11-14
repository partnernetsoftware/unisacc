# unisacc 0.0.61 -- 发布说明

状态:草稿,定稿于公开之前。本文件随提交 Y 入库,并作为发布页正文;因此本文件不能包含 Y 自身的哈希,也不能包含公开之后才发生的读回结果。未实跑的项一律写未跑,未证实的项不写 PASS。本文件不宣称 QUALIFIED;同候选 CI 判定见下:run 43554326355 十个作业全部 success,release-decide 结论为 NOT_QUALIFIED(步 4 s9 UNKNOWN)。

- 冻结提交 `d4b3bf72f7ae999c96c0fa15d8030a7c2d4de073`(标签 v0.0.61-src;构造树等同 main `03c4d5df9d`,只多 release/candidate.tsv 一行)。
- 提交 X `6b07b710eb`(main 上只改 release/candidate.tsv);本版没有 X2;提交 Y 即带入本文件的提交,其哈希不写入本文件,由账本记录。
- 同候选 CI:candidate-native.yml run 43554326355(https://github.com/partnernetsoftware/unisacc/actions/runs/43554326355),对 X 的 `tools/ci-verdict.cx --sha` 结果为 `CI_VERDICT PASS all-jobs-success`,六目标格加 fetch、cell-tools、ffi-lnx、report 共 10 个作业全部 success。
- 候选 APE `unisacc.com` sha256 `c4c47ecb97ee70865edaf39d91e29f5b22b6c8efdbcfb81c4482517ffc0224c3`,7022820 字节,`--version` 实测 `unisacc 0.0.61`。
- git 范围 `v0.0.59..Y`(版本号跳过 0.0.60,见已知缺口)。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.61 的主题是提速与库包补全,范围并入了原定 0.0.60 的内容。上一发布版见 `plans/release-notes-0.0.59.md`。

取证范围:`git log --oneline fbdc99ca..03c4d5df9d`,共 582 个提交(0.0.59 构造源 fbdc99ca 到本版构造树)。以下按类归纳,只写冻结树内已有的内容。

## 行为变化(产品输入)

性能:

(1) 运行库 qsort 改内省排序(堆排序回落,SPEED-10),新增批次 check-qsort-intro;其预算 9 -> 25 秒,依据实测 run_ms=9211。
(2) 运行库 stdout 缓冲(SPEED-17):非终端全缓冲、终端行缓冲、setvbuf、stderr 写前冲刷。
(3) E2 冷编译补丁:exec/pp/directive-scan-byte.tsv 的 P3BLANK 去掉 OUT 32 填充(SPEED-12B)。
(4) 产品摘要大成员走内核 SHA(SPEED-7,摘要逐字相同,742 ms -> 38 ms)。

库包与运行库:

(5) cx.stream 新增 18 个函数实现(CXS-C7:xs_reader_fd/mem、xs_read、xs_read_full、xs_read_line 等)。
(6) cx.os 补全 10 项实现(sy_file_size/abspath/mkstemp/homedir/hostname、sy_tm_t/sy_utc_split 等);cx.sys 11 个新行实现(sy_os_name、sy_arch_name、sy_page_size、sy_errno_name 等)。
(7) cx/sys 新增预派生钩子 sy_atspawn,cx.os 0.1.2 重新 bump。
(8) safedel 白名单新增 `HOME/.cache/unisacc/chains` 严格子路径。

链工具(非产品输入,随本版带出):

(9) entry-build:RT_UNIT/RT_EXTERN 只按对象键选(drive.o 为 RT_UNIT,其余为 RT_EXTERN);whole-front 的 worker.cx 以 RT_UNIT 编译。
(10) whole-front、release-build 的快照与暂存路径改经 chainroot;release-build freeze-product 暂存目录先用 sy_rmdir 清理。
(11) tlib kit:prep 镜像缓存命中后置 0755,修复 place_tlib 复制为 0644 致 tlib-prep 不可执行。
(12) 多个工具默认不再写仓根 out/(迁到 TMPDIR)。

不在本版内:g2/g3 并发化(8d70af5101)经 `git merge-base --is-ancestor` 核对不在 03c4d5df9d 之前,不是本版内容,本版 g2/g3 的 objects 相仍按目标串行。ext.net 与 cx.sys netraw 在冻结树之后,不属于 0.0.61。

## 实测速度(单样本或 3 样本,非 OS 冷)

来源 build/speed-baseline.tsv 2026-11-14 四行(real4 产物 `c55533c7`,构造树 03c4d5df9d;F 重打包产物 `c4c47ecb` 与其同一份切片,仅版本串不同)。

- 冷启动 first_e2e:684/705/714 ms,对 0.0.59 基线 796/815 ms,约 -14%(load1 约 1.2-1.5)。
- 冷编译:ledger-add 6468 ms(旧 9469,-32%)、entry-build 13624 ms(旧 18703,-27%)、whole-front 12846 ms(旧 16863,-24%);均单样本。
- F 重打包产物(c4c47ecb)实测:first_e2e 3 样本 675/676/695 ms(run-startup-bench);冷编译各 1 样本(coldcompile):ledger-add 6515 ms、entry-build 13681 ms、whole-front 12546 ms、coldcompile 6216 ms;均非 OS 冷。
- 整链墙钟 4829 s(real4,rev f94e1969783b,与本版冻结树 F 的差异只有 release/candidate.tsv)。0.0.59 -> 0.0.61 没有对 F 重跑整链,版本仅在打包注入;因此最终 APE 本身未经一次完整整链构造,它的六片取自 real4 整链(F 与 real4 的 SID 同为 a68bf9fc...,候选表不计入 SID)。没有 F 重打包整链证据。

## 已知缺口与未跑范围(如实)

(1) CI 六目标原生作业全部 success(run 43554326355);全批资格未达:release-decide 为 NOT_QUALIFIED,s9 daily/release 档未跑。
(2) 本机已知红:wt-gate(tools/all-waves.cx 的 wave 在 -run 方式下要求同目录原生 run-batch,5/8 断言失败,干净工作树也红);slice-migration-gen(source-missing cx/compiler/frames.cx)。未修。
(3) 若干批次因窗口 DEFERRED 未实跑(prd_mempalace 记 27 + 36 个);osx/win 平台批次需对应主机,本机无证据。约 80 个批次此前红、大半属 FTRIM-bootstrap,装新产品后复验,未验证。
(4) 构造用的 osx cache-manager 借用自 0.0.59。
(5) 版本号跳过 0.0.60,其范围并入 0.0.61。
(6) check-stdio-buffer 不覆盖 tty/pty 路径,也不覆盖 --link-only 与子集编译的缓存 identity 一致性;六目标未跑。
(7) candidate-chain g2 对象相批墙钟约 320 s(real4 323.6 s),超过单次 300 s 预算;整改任务 G2-OBJ-SPLIT,限期 2026-11-30。
(8) 速度数字均为单样本或 3 样本,非 OS 冷;run-startup-bench 与 coldcompile 已对新产物实测,check-bench-gencode 与 speed-probe(生成代码一档)对新产物未跑。
(9) 整链约 77 分钟(4829 s 实测),g2/g3 objects 相串行;并发化未入本版。
(10) 仍写仓根 out/ 的工具:gen-model-decode-material、csih-ape(csih 车道)、chain-run。

## 撤回与回滚

按运行手册:撤回公开用 `gh release edit v0.0.61 --draft=true`,并把 v0.0.59 重设为 Latest;彻底撤回再删 tag;main 上的提交 Y 只用 `git revert`。本机根回退到 0.0.59:`tools/install-ape.cx --rollback`(已装时备份为 unisacc.com.059)。具体命令见 `release/rollback.txt`(RUN_ID 与 X 已填实)。

规则:发布页正文必须与本文件逐字相同,公开后读回核对;读回结果记账本。
