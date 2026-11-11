# 首观察research入仓只读核（cdx；1159）

实际采样：2026-10-11T12:02:45.574084+08:00；依据1159裁定，未代执行。

- 提交 `c1a76e0611ebf534e053eb37fc43059cf5551d5e` 完整差集共24新增文件，全部research/c40-first-observe/，无产品/version/gatedeps/prd/验收修改。现场HEAD `c1a76e0611ebf534e053eb37fc43059cf5551d5e`，status=''。
- HASHES.sha256独立重算全部22条通过；9份回执/裁定/审查md与仓外源文件逐字节一致；state中12份原样副本与状态目录原件逐字节一致。ua二进制未入仓，其ua.sha256与原件全长哈希吻合。
- 1152补证前本哨保存的11项旧原件哈希，现重新计算全部保持，含唯一make日志、candidate日志、timing、前后identity/H1、UA、shim与step日志。make调用仍1行、cand仍仅step-shared.log，无新候选/后续步骤；未见重跑或B.2续构造。
- GitHub只读无0.0.40release/Draft，Latest仍0.0.39。入仓README明确范围闭合、非产品PASS、UNKNOWN保留，未借research入仓放行Draft/queue/B.2/seal。普通push无force由cc回执记，本哨仅核提交与副本事实。

本哨仅落本仓外短记；不写仓/产品、不重跑、不开Draft/B.2、不bump、提交或推送。

PASS research-only差集，旧原件哈希及仓内副本保持，未见重跑/Draft/B.2越权，UNKNOWN与非产品PASS限定保留
