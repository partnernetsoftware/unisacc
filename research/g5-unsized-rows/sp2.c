#include <stdio.h>
const char *gp[][2]={"a","b","c"}; char gs[][2][4]={"a","b","c"}; static char g2[][3]={"ab","c",{"d"}};
int main(void){ static const char *lp[][3]={"x"}; char ls[][2][3]={{"a","b"},"c"}; printf("%d %d %d %d %d %s %s %s\n",(int)sizeof gp,(int)sizeof gs,(int)sizeof g2,(int)sizeof lp,(int)sizeof ls,gp[1][0],gs[1][0],ls[1][0]); return 0; }
