/* Shared UTF-8 walk, four-byte wchar storage, concatenation and extent. */
int g[]=L"\x20ac\101";
int main(void){
 static int st[]=L"é";
 int a[2]=L"ab";
 int *p=L"😀";
 int *q="a" L"é" "z";
 int *e=L"";
 return !(g[0]==8364 && g[1]==65 && g[2]==0 && st[0]==233 && st[1]==0 && a[1]==98 && p[0]==128512 && p[1]==0 && q[0]==97 && q[1]==233 && q[2]==122 && q[3]==0 && *e==0 && sizeof(L"a\0b")==16);
}
