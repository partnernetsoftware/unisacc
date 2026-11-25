# 待补 unverified 行草案(两行,不入表)

格式对齐 build/state-unverified.tsv 十一列: id/version/kind/state/subject/missing/action/write_domain/owner/evidence/updated。

    E01	0.0.105	no-run	UNVERIFIED	release/decision.txt 与 release/evidence.tsv 可能停在旧版,release-exec 第 4 与第 7 步用同一份旧输入互比恒等	第 4 步未断言两文件含本候选版本号,裁决对应本版未证	第 4 步加"两文件版本号等于本候选"断言并实跑一次释放链	release/decision.txt release/evidence.tsv tools/release-exec.cx	integrator	prd_mempalace.md:128	2026-11-24
    E02	0.0.105	no-run	UNVERIFIED	单元缓存与预构建对象的键是否覆盖所用模块接口摘要未查(结构体扩字段后旧布局对象被复用属同类风险)	键覆盖模块接口摘要的核验	构造"模块接口变更但源码不变"的场景,验证缓存不复用旧布局对象	tools/ cx/compiler/ src/	integrator	prd_mempalace.md:132	2026-11-24

归类:两条都是"对已发布行为的声称缺实跑"(未跑核验),按 R37 第 4 条 (c) 属声称类(no-run),计入上限;若入表,声称类将从 7 行变 9 行,离上限 3 更远。依据:两行的 missing 列都是"未跑/未证",不是已登记的外部缺口或债。
