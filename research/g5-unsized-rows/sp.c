int main(void){ const char *p[][2]={"a","b"}; char s[][2][4]={"a","b"}; return sizeof(p)!=2*sizeof(void*) || sizeof(s)!=8; }
