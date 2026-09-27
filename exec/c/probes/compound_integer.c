/* Mixed widths must preserve destination values and evaluate both sides once. */
unsigned char a[2]; int hits; int index_calls;
unsigned rhs(void){hits++;return 0x1234u;}
int getindex(void){index_calls++;return 0;}
int main(void){
 unsigned char b=255; unsigned short s=65535; unsigned u=0xffffffffu;
 unsigned long ul=0xffffffffffffffffUL; char c=127;
 unsigned r=(a[getindex()] |= rhs());
 unsigned br=(b+=2u); unsigned sr=(s*=2u); unsigned ur=(u>>=1);
 unsigned long lr=(ul/=2u); int cr=(c+=1u);
 return r!=52 || a[0]!=52 || hits!=1 || index_calls!=1
     || br!=1 || b!=1 || sr!=65534 || s!=65534
     || ur!=2147483647u || u!=2147483647u
     || lr!=9223372036854775807UL || ul!=9223372036854775807UL
     || cr!=-128 || c!=-128;
}
