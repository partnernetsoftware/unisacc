#ifdef CXM_HOST_REALPATH
    char ra[512], rb[512];
    extern char *realpath(const char *, char *);
#endif
    if (cxm_eq(a, b)) return 1;
#ifdef CXM_HOST_REALPATH
    if (realpath(a, ra) && realpath(b, rb) && cxm_eq(ra, rb)) return 1;
#endif
    return 0;
