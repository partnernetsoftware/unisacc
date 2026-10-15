#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "resourceledger.h"
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL %s\n", m); fails++; } else printf("ok   %s\n", m); } while (0)
int main(int argc, char **argv) {
    const char *root = argv[1];
    char dir[512], err[512]; RLBlob b[2]; RLLedger L; const unsigned char input[] = "lower-tape-input"; const unsigned char k1[] = "\0cli/defines", k2[] = "\0attr";
    const unsigned char v1[] = "530-bytes-or-so", e[1] = {0};
    snprintf(dir, sizeof dir, "%s/snap-ok", root);
    b[0].key = k1; b[0].kn = (int)sizeof(k1) - 1; b[0].data = v1; b[0].len = (int)strlen((char *)v1);
    b[1].key = k2; b[1].kn = (int)sizeof(k2) - 1; b[1].data = e; b[1].len = 0;   /* ATTR-like key, PRESENT_EMPTY */
    CHECK(rl_snapshot_write(dir, "lower:lnx/x86_64/run/O0", input, (int)strlen((char *)input), b, 2, err, sizeof err) == 0, "write positive");
    { int r = rl_snapshot_load(dir, "lower:lnx/x86_64/run/O0", &L, 0, err, sizeof err); if (r) printf("load err=%s\n", err); CHECK(r == 0, "load positive (same stage, same input)"); }
    CHECK(rl_snapshot_load(dir, "e3:lnx/x86_64/tape/O0", &L, 0, err, sizeof err) != 0 && strstr(err, "mismatch") != NULL, "wrong stage refused by name");
    { int r = rl_snapshot_write(dir, "lower:lnx/x86_64/run/O0", input, (int)strlen((char *)input), b, 2, err, sizeof err); printf("rewrite rc=%d err=%s\n", r, err); }
    snprintf(dir, sizeof dir, "%s/snap-half", root);
    { char p[512]; FILE *f; snprintf(p, sizeof p, "%s/resources.tsv", root); (void)p; (void)f; }
    CHECK(rl_snapshot_write(dir, "lower:lnx/x86_64/run/O0", input, (int)strlen((char *)input), b, 2, err, sizeof err) == 0, "write second snapshot");
    { char p[1024]; snprintf(p, sizeof p, "%s/COMMIT", dir); CHECK(remove(p) == 0, "simulate interrupted commit: COMMIT removed"); }
    CHECK(rl_snapshot_load(dir, "lower:lnx/x86_64/run/O0", &L, 0, err, sizeof err) != 0 && strstr(err, "no COMMIT") != NULL, "no COMMIT: incomplete snapshot not recognised");

    /* capacity: 1000 small blobs overflow the COMMIT buffer: named refusal, nothing published */
    { static RLBlob many[999]; static unsigned char keys[999][512]; static unsigned char one[1] = {7}; int q; char d2[512];
      for (q = 0; q < 999; q++) { int z; for (z = 0; z < 512; z++) keys[q][z] = (unsigned char)('a' + (z + q) % 26); keys[q][0] = 0; keys[q][1] = (unsigned char)(q >> 8); keys[q][2] = (unsigned char)(q & 255);
        many[q].key = keys[q]; many[q].kn = 512; many[q].data = one; many[q].len = 1; }
      snprintf(d2, sizeof d2, "%s/snap-cap", root);
      { int r = rl_snapshot_write(d2, "lower:lnx/x86_64/run/O0", input, (int)strlen((char *)input), many, 999, err, sizeof err);
        printf("capacity rc=%d err=%s\n", r, err); CHECK(r != 0 && strstr(err, "snapshot-capacity-exceeded") != NULL, "capacity overflow refused by name");
        { char p[1024]; snprintf(p, sizeof p, "%s/COMMIT", d2); FILE *f = fopen(p, "rb"); CHECK(f == NULL, "no COMMIT published on overflow"); if (f) fclose(f); }
        { FILE *f2; char p2[1024]; snprintf(p2, sizeof p2, "%s/resources.tsv", d2); f2 = fopen(p2, "rb"); CHECK(f2 == NULL, "no final snapshot files left on overflow"); if (f2) fclose(f2); } } }
    /* write failure: the parent directory is read-only, so the staging directory cannot be created */
    { char ro[512], d3[512]; snprintf(ro, sizeof ro, "%s/ro", root); mkdir(ro, 0500); snprintf(d3, sizeof d3, "%s/snap-x", ro);
      { int r = rl_snapshot_write(d3, "lower:lnx/x86_64/run/O0", input, (int)strlen((char *)input), b, 2, err, sizeof err);
        printf("readonly rc=%d err=%s\n", r, err); CHECK(r != 0 && strstr(err, "resource-snapshot-write-failed") != NULL, "read-only target refused by name (write failure)"); } }
    /* writer refusals and the callable-address rule */
    { RLBlob z[1]; unsigned char zb[2] = {1, 2}; char d5[512];
      snprintf(d5, sizeof d5, "%s/neg-emptykey", root); z[0].key = zb; z[0].kn = 0; z[0].data = zb; z[0].len = 1;
      { int r = rl_snapshot_write(d5, "lower:lnx/x86_64/run/O0", input, (int)strlen((char *)input), z, 1, err, sizeof err);
        CHECK(r != 0 && strstr(err, "snapshot-empty-key") != NULL, "writer refuses a zero-length key by name"); }
      snprintf(d5, sizeof d5, "%s/neg-badstage", root);
      { int r = rl_snapshot_write(d5, "lower:lnx/x86_64/run/O0x", input, (int)strlen((char *)input), b, 2, err, sizeof err);
        CHECK(r != 0 && strstr(err, "snapshot-stage-label") != NULL, "writer refuses an invalid full stage label"); }
      snprintf(d5, sizeof d5, "%s/neg-stageprefix", root);
      { int r = rl_snapshot_write(d5, "lowerx:lnx/x86_64/run/O0", input, (int)strlen((char *)input), b, 2, err, sizeof err);
        CHECK(r != 0 && strstr(err, "snapshot-stage-label") != NULL, "writer refuses a stage name outside the registered table"); } }
    { static RLBlob cb[2]; static unsigned char ck1[21] = {0}, cv[3] = {1,2,3}; char d6[512]; int r1, r2;
      memcpy(ck1 + 1, "library/callablemake", 20);
      cb[0].key = ck1; cb[0].kn = 21; cb[0].data = cv; cb[0].len = 3; cb[1].key = k1; cb[1].kn = (int)sizeof k1 - 1; cb[1].data = v1; cb[1].len = (int)strlen((char *)v1);
      snprintf(d6, sizeof d6, "%s/cb-addr", root);
      r1 = rl_snapshot_write(d6, "lower:lnx/x86_64/run/O0", input, (int)strlen((char *)input), cb, 2, err, sizeof err);
      CHECK(r1 == 0, "snapshot with a callable address key is written (observation data)");
      { L.n = 0; r2 = rl_snapshot_load(d6, "lower:lnx/x86_64/run/O0", &L, 0, err, sizeof err);
        CHECK(r2 != 0 && strstr(err, "callable-address-not-portable") != NULL, "callable address refused for execution by default"); }
      { r2 = rl_snapshot_load(d6, "lower:lnx/x86_64/run/O0", &L, 1, err, sizeof err);
        CHECK(r2 == 0, "callable address loads with --observe-callables"); } }
    /* a legal blob listed in COMMIT but referenced by no ledger row, and a ledger row that names input.bin: each refused by name */
    { char dU[512], dI[512], pth[1024], lab[64], cm[1024]; unsigned char zz[2] = {'x', 'y'}, idh[65], line2[512], *tab; FILE *f; int rr;
      size_t tn; char tabbuf[1024], cmbuf[1024];
      snprintf(lab, sizeof lab, "lower:lnx/x86_64/run/O0");
      unisa_sha256_hex(input, strlen((char *)input), idh);
      snprintf(dU, sizeof dU, "%s/neg-unref", root); mkdir(dU, 0700);
      snprintf(pth, sizeof pth, "%s/input.bin", dU); f = fopen(pth, "wb"); fwrite(input, 1, strlen((char *)input), f); fclose(f);
      snprintf(pth, sizeof pth, "%s/resource-000.bytes", dU); f = fopen(pth, "wb"); fwrite(v1, 1, strlen((char *)v1), f); fclose(f);
      snprintf(pth, sizeof pth, "%s/resource-001.bytes", dU); f = fopen(pth, "wb"); fwrite(zz, 1, 2, f); fclose(f);
      { char hx[65]; char row[1200]; char keyhex[64]; size_t j; unisa_sha256_hex(v1, strlen((char *)v1), hx);
        for (j = 0; j < sizeof k1 - 1; j++) snprintf(keyhex + 2*j, 3, "%02x", k1[j]);
        rl_identity_line(line2, sizeof line2, lab, (char *)idh); tn = strlen(line2); memcpy(tabbuf, line2, tn); tabbuf[tn] = 0;
        snprintf(row, sizeof row, "%s\t1\tresource-000.bytes\t%d\t%s\n", keyhex, (int)strlen((char *)v1), hx); strcat(tabbuf, row);
        snprintf(pth, sizeof pth, "%s/resources.tsv", dU); f = fopen(pth, "wb"); fwrite(tabbuf, 1, strlen(tabbuf), f); fclose(f);
        { char th[65], l1[256], l2[256], l3[256], l4[256]; unisa_sha256_hex(tabbuf, strlen(tabbuf), th);
          snprintf(l1, sizeof l1, "%s  input.bin\n", idh); snprintf(l2, sizeof l2, "%s  resource-000.bytes\n", hx);
          { char hz[65]; unisa_sha256_hex(zz, 2, hz); snprintf(l3, sizeof l3, "%s  resource-001.bytes\n", hz); }
          snprintf(l4, sizeof l4, "%s  resources.tsv\n", th);
          snprintf(cmbuf, sizeof cmbuf, "%s%s%s%s", l1, l2, l3, l4); } }
      snprintf(pth, sizeof pth, "%s/COMMIT", dU); f = fopen(pth, "wb"); fwrite(cmbuf, 1, strlen(cmbuf), f); fclose(f);
      rr = rl_snapshot_load(dU, lab, &L, 0, err, sizeof err);
      CHECK(rr != 0 && strstr(err, "snapshot-commit-unreferenced") != NULL, "legal blob listed in COMMIT but unreferenced by the ledger: refused by name");
      snprintf(dI, sizeof dI, "%s/neg-inputref", root); mkdir(dI, 0700);
      snprintf(pth, sizeof pth, "%s/input.bin", dI); f = fopen(pth, "wb"); fwrite(input, 1, strlen((char *)input), f); fclose(f);
      { char keyhex[64]; char row[1200]; char tb[1024], th[65], l1[256], l2[256]; size_t j;
        for (j = 0; j < sizeof k1 - 1; j++) snprintf(keyhex + 2*j, 3, "%02x", k1[j]);
        rl_identity_line(line2, sizeof line2, lab, (char *)idh); snprintf(tb, sizeof tb, "%s", line2);
        snprintf(row, sizeof row, "%s\t1\tinput.bin\t%d\t%s\n", keyhex, (int)strlen((char *)input), idh); strcat(tb, row);
        snprintf(pth, sizeof pth, "%s/resources.tsv", dI); f = fopen(pth, "wb"); fwrite(tb, 1, strlen(tb), f); fclose(f);
        unisa_sha256_hex(tb, strlen(tb), th);
        snprintf(l1, sizeof l1, "%s  input.bin\n", idh); snprintf(l2, sizeof l2, "%s  resources.tsv\n", th);
        snprintf(cm, sizeof cm, "%s%s", l1, l2); }
      snprintf(pth, sizeof pth, "%s/COMMIT", dI); f = fopen(pth, "wb"); fwrite(cm, 1, strlen(cm), f); fclose(f);
      rr = rl_snapshot_load(dI, lab, &L, 0, err, sizeof err);
      CHECK(rr != 0 && strstr(err, "snapshot-ledger-bad-blob-name") != NULL, "ledger row naming input.bin is refused by name (blob names are resource-NNN only)");
    }
    /* COMMIT integrity: each damaged snapshot is refused by name (fresh snapshot per case) */
    { static const char *cases[3] = {"commit-empty", "commit-dup", "extra-file"}; int c;
      for (c = 0; c < 3; c++) {
        char d4[512], pth[1024], cm[8192]; FILE *f; int r; size_t cn = 0;
        snprintf(d4, sizeof d4, "%s/neg-%s", root, cases[c]);
        r = rl_snapshot_write(d4, "lower:lnx/x86_64/run/O0", input, (int)strlen((char *)input), b, 2, err, sizeof err);
        if (r) { printf("FAIL neg-%s setup: %s\n", cases[c], err); fails++; continue; }
        snprintf(pth, sizeof pth, "%s/COMMIT", d4); f = fopen(pth, "rb"); cn = f ? fread(cm, 1, sizeof cm - 1, f) : 0; if (f) fclose(f); cm[cn] = 0;
        if (c == 0) { f = fopen(pth, "wb"); if (f) fclose(f); }
        else if (c == 1) { char dup[8192]; size_t first = 0; while (first < cn && cm[first] != '\n') first++; first++;
            snprintf(dup, sizeof dup, "%s%s", cm, cm + first); f = fopen(pth, "wb"); if (f) { fputs(dup, f); fclose(f); } }
        else { snprintf(pth, sizeof pth, "%s/stray.txt", d4); f = fopen(pth, "wb"); if (f) { fputs("x", f); fclose(f); } }
        r = rl_snapshot_load(d4, "lower:lnx/x86_64/run/O0", &L, 0, err, sizeof err);
        {   const char *want = c == 0 ? "snapshot-commit-incomplete" : c == 1 ? "snapshot-commit-duplicate" : "snapshot-extra-file";
            int okc = r != 0 && strstr(err, want) != NULL;
            printf("%s neg-%s: %s\n", okc ? "ok  " : "FAIL", cases[c], okc ? want : err); if (!okc) fails++; } }
    }
    printf("%s\n", fails ? "SNAPTEST FAIL" : "SNAPTEST PASS");
    return fails;
}
