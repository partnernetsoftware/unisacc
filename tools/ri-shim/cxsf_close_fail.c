/* 来源: git show 9d221111f2:cx/runtime/source_frame.cx 第 1218–1225 行 */
int cxsf_close_fail(long rc, void (*failfn)(const char *)) {
    if (!failfn) return 4;
    if (rc < 0) {
        failfn("close failed");
        return 1;
    }
    return 0;
}
