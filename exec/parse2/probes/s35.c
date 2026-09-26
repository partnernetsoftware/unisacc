/* equal: global pointer strings are pooled by source position, around a function body's string */
char *gp = "ptr";
int x = 3;
char *gq = "q";
int main(void) { char *l = "ab"; if (x) x = 2; return gp[0] + l[0] + gq[0]; }
