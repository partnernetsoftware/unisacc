/* Conservative original-span reference; the product route uses prune delta. */
#include "tapescan.h"
/* Schema derived from unisa/tape.py SHAPE/REGS; sha256 e45dd6919efca9a267893b9cc4301294eabafb328b40264328fcfdc64898490a */
#define tp_OP_FRAME 34
#define tp_OP_STORE64 22
#define tp_OP_MOV 1
#define tp_OP_CALL 29
#define tp_OP_JUMP 27
#define tp_OP_JUMPZ 28
#define tp_OP_LEA 25
#define tp_OP_CALLR 30
#define tp_OP_CALLM 68
#define tp_OP_RET 33
#define tp_OP_EXIT 38
#define tp_OP_LOAD64 21
#define tp_OP_LD 23
#define tp_OP_ST 24
#define tp_OP_ZERO 26
#define tp_NOPS 69
#define tp_NREGS 8
char *tp_opnames[tp_NOPS] = {"imm","mov","add64","sub64","mul64","xor64","and64","or64","shl64","shr64","lshr64",".div",".mod",".udiv",".umod","slt64","sle64","ult64","ule64","eq","ne","load64","store64",".ld",".st",".lea",".zero","jump","jumpz","call","callr",".hostcall",".hostaddr","ret",".frame",".arg",".print",".write",".exit",".sys",".sys6",".argc",".argv","nop","fadd64","fsub64","fmul64","fdiv64","flt64","fle64","feq64","fadd32","fsub32","fmul32","fdiv32","flt32","fle32","feq32","cvtid","cvtud","cvtis","cvtus","cvtdi","cvtdu","cvtsd","cvtds","fsqrt64","fsqrt32","callm"};
char *tp_opshapes[tp_NOPS] = {"ri","rr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rri","rir","rrii","riri","rs","rii","L","rL","L","r","rr","ri","","i","ir","r","rr","r","srrr","srrrrrr","r","rr","","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rrr","rr","rr","rr","rr","rr","rr","rr","rr","rr","rr","ri"};
char *tp_regnames[tp_NREGS] = {"r0","r1","r2","r3","r4","r5","r6","r7"};
#define tp_SRCMAX 2097152
#define tp_ROWMAX 32768
#define tp_NAMEMAX 8192
#define tp_EDGEMAX 65536
char *tp_input; int tp_inputn;
int tp_rs[tp_ROWMAX],tp_re[tp_ROWMAX],tp_rk[tp_ROWMAX],tp_rp[tp_ROWMAX],tp_ro[tp_ROWMAX],tp_rn[tp_ROWMAX],tp_owner[tp_ROWMAX];
long tp_av[tp_ROWMAX*8]; int tp_an[tp_ROWMAX*8]; int tp_numeric[tp_ROWMAX*8];
int tp_ns[tp_NAMEMAX],tp_nl[tp_NAMEMAX],tp_lab[tp_NAMEMAX],tp_datum[tp_NAMEMAX],tp_pro[tp_NAMEMAX],tp_anchor[tp_NAMEMAX];int tp_nn;
int tp_firstlab[tp_ROWMAX],tp_startmark[tp_ROWMAX],tp_us[tp_ROWMAX],tp_ue[tp_ROWMAX],tp_alive[tp_ROWMAX],tp_roots[tp_ROWMAX],tp_hascallr[tp_ROWMAX],tp_head[tp_ROWMAX],tp_queue[tp_ROWMAX];int tp_un;
int tp_edge_to[tp_EDGEMAX],tp_edge_next[tp_EDGEMAX],tp_ne;
int tp_bad,tp_nrows,tp_npc;
int tp_tstart[16],tp_tlen[16],tp_nt;
int tp_space(int c){return c==32||c==9||c==10||c==13||c==11||c==12;}
int tp_length(char *s){int n=0;while(s[n])n++;return n;}
int tp_equal(char *a,char *b,int n){int i;for(i=0;i<n;i++)if(a[i]!=b[i])return 0;return 1;}
int tp_same(int at,int n,char *s){return tp_length(s)==n && tp_equal(tp_input+at,s,n);}
int tp_namechar(int c,int first){return (c>=65&&c<=90)||(c>=97&&c<=122)||c==95||(!first&&((c>=48&&c<=57)||c==46||c==36));}
int tp_validname(int at,int n){int i;if(n<1)return 0;for(i=0;i<n;i++)if(!tp_namechar(tp_input[at+i]&255,i==0))return 0;return 1;}
int tp_intern(int at,int n){int i;for(i=0;i<tp_nn;i++)if(tp_nl[i]==n&&tp_equal(tp_input+tp_ns[i],tp_input+at,n))return i;
 if(tp_nn>=tp_NAMEMAX){tp_bad=1;return 0;}i=tp_nn++;tp_ns[i]=at;tp_nl[i]=n;tp_lab[i]=-1;return i;}
int tp_lookup(char *s){int i;for(i=0;i<tp_nn;i++)if(tp_same(tp_ns[i],tp_nl[i],s))return i;return -1;}
int tp_number(int at,int n,long *out){unsigned long value;int neg,leading_zero;
 if(!ts_integer(tp_input+at,n,&value,&neg,&leading_zero))return 0;
 /* Canonical decimal/hex spelling only; ambiguous legacy octal stays whole. */
 if(leading_zero&&value!=0)return 0;
 if(value>(neg?9223372036854775808UL:9223372036854775807UL))return 0;
 *out=neg?(long)(0UL-value):(long)value;return 1;}
int tp_getreg(int at,int n){int i;for(i=0;i<tp_NREGS;i++)if(tp_same(at,n,tp_regnames[i]))return i;return -1;}
int tp_getop(int at,int n){int i;for(i=0;i<tp_NOPS;i++)if(tp_same(at,n,tp_opnames[i]))return i;return -1;}
int tp_token(int at,int n){if(tp_nt>=16){tp_bad=1;return 0;}tp_tstart[tp_nt]=at;tp_tlen[tp_nt]=n;tp_nt++;return 1;}
int tp_tokenize(int p,int end){int s,i,cut;tp_nt=0;while(p<end){while(p<end&&(tp_space(tp_input[p])||tp_input[p]==','||tp_input[p]=='['||tp_input[p]==']'))p++;
 if(p==end)break;s=p;while(p<end&&!tp_space(tp_input[p])&&tp_input[p]!=','&&tp_input[p]!='['&&tp_input[p]!=']')p++;
 cut=-1;if(tp_input[s]=='r')for(i=s+1;i<p;i++)if(tp_input[i]=='+'||tp_input[i]=='-'){cut=i;break;}
 if(cut>=0){tp_token(s,cut-s);tp_token(cut,p-cut);}else tp_token(s,p-s);if(tp_bad)return 0;}return 1;}
int tp_parse(void){int p=0,end,textend,start,q,quoted,esc,i,op,shape,idx,id;long value;
 for(i=0;i<tp_ROWMAX;i++)tp_firstlab[i]=-1;
 while(p<tp_inputn){if(tp_nrows>=tp_ROWMAX){tp_bad=1;return 0;}idx=tp_nrows++;start=p;while(p<tp_inputn&&tp_input[p]!=10)p++;end=p;if(p<tp_inputn)p++;
  tp_rs[idx]=start;tp_re[idx]=p;tp_rp[idx]=tp_npc;tp_rk[idx]=0;
  textend=end;quoted=0;esc=0;for(q=start;q<end;q++){if(quoted){if(esc)esc=0;else if(tp_input[q]==92)esc=1;else if(tp_input[q]==34)quoted=0;}
   else if(tp_input[q]==34)quoted=1;else if(tp_input[q]==59){textend=q;break;}}
  while(start<textend&&tp_space(tp_input[start]))start++;while(textend>start&&tp_space(tp_input[textend-1]))textend--;
  if(start==textend)continue;
  if(tp_input[textend-1]==58&&tp_validname(start,textend-start-1)){id=tp_intern(start,textend-start-1);if(tp_bad)return 0;
   if(tp_lab[id]>=0){tp_bad=1;return 0;}tp_lab[id]=idx;tp_rn[idx]=id;tp_rk[idx]=2;if(tp_firstlab[tp_npc]<0)tp_firstlab[tp_npc]=idx;continue;}
  q=start;while(q<textend&&!tp_space(tp_input[q]))q++;
  if(tp_same(start,q-start,".bss")||tp_same(start,q-start,".str")){int isbss=tp_same(start,q-start,".bss");
   while(q<textend&&tp_space(tp_input[q]))q++;i=q;while(q<textend&&!tp_space(tp_input[q]))q++;
   if(!tp_validname(i,q-i)){tp_bad=1;return 0;}id=tp_intern(i,q-i);if(tp_bad)return 0;
   while(q<textend&&tp_space(tp_input[q]))q++;
   if(isbss){value=0;if(q==textend){tp_bad=1;return 0;}for(;q<textend;q++){if(tp_input[q]<'0'||tp_input[q]>'9'){tp_bad=1;return 0;}value=value*10+tp_input[q]-'0';if(value>33554432){tp_bad=1;return 0;}}}
   else {int decoded;if(!ts_quoted_length(tp_input+q,textend-q,1048576,&decoded)){tp_bad=1;return 0;}}
   tp_datum[id]=1;tp_rk[idx]=3;tp_rn[idx]=id;continue;}
  op=tp_getop(start,q-start);if(op<0){tp_bad=1;return 0;}tp_ro[idx]=op;
  if(!tp_tokenize(q,textend))return 0;shape=tp_length(tp_opshapes[op]);if(tp_nt!=shape||tp_nt>8){tp_bad=1;return 0;}
  for(i=0;i<tp_nt;i++){char kind=tp_opshapes[op][i];int at=idx*8+i;tp_numeric[at]=0;tp_an[at]=-1;
   if(kind=='r'){id=tp_getreg(tp_tstart[i],tp_tlen[i]);if(id<0){tp_bad=1;return 0;}tp_av[at]=id;}
   else if(kind=='i'){if(!tp_number(tp_tstart[i],tp_tlen[i],&value)){tp_bad=1;return 0;}tp_av[at]=value;tp_numeric[at]=1;}
   else if(tp_number(tp_tstart[i],tp_tlen[i],&value)){tp_numeric[at]=1;tp_av[at]=value;}
   else {if(!tp_validname(tp_tstart[i],tp_tlen[i])){tp_bad=1;return 0;}tp_an[at]=tp_intern(tp_tstart[i],tp_tlen[i]);if(tp_bad)return 0;}}
  if(tp_same(start,q-start,".hostaddr")||tp_same(start,q-start,".hostcall")){tp_bad=1;return 0;}
  if((op==tp_OP_LD||op==tp_OP_ST)&&tp_av[idx*8+3]!=1&&tp_av[idx*8+3]!=2&&tp_av[idx*8+3]!=4&&tp_av[idx*8+3]!=8){tp_bad=1;return 0;}
  tp_rk[idx]=1;tp_npc++;if(tp_npc>=tp_ROWMAX){tp_bad=1;return 0;}
 }
 for(i=0;i<tp_nn;i++)if(tp_lab[i]>=0&&tp_datum[i]){tp_bad=1;return 0;}return 1;}
int tp_isprologue(int row){int found[4],n=0,i;for(i=row+1;i<tp_nrows;i++){if(tp_rk[i]==3)break;if(tp_rk[i]==1)found[n++]=i;if(n==4)break;}
 if(n!=4)return 0;i=found[0];if(tp_ro[i]!=tp_OP_FRAME||tp_av[i*8]!=8)return 0;
 i=found[1];if(tp_ro[i]!=tp_OP_STORE64||tp_av[i*8]!=7||tp_av[i*8+1]!=0||tp_av[i*8+2]!=6)return 0;
 i=found[2];if(tp_ro[i]!=tp_OP_MOV||tp_av[i*8]!=6||tp_av[i*8+1]!=7)return 0;
 i=found[3];return tp_ro[i]==tp_OP_FRAME&&tp_av[i*8]>=0;}
int tp_addroot(char *name){int id=tp_lookup(name);if(id>=0&&tp_lab[id]>=0)tp_roots[tp_owner[tp_lab[id]]]=1;return id;}
int tp_edge(int from,int to){if(tp_ne>=tp_EDGEMAX){tp_bad=1;return 0;}tp_edge_to[tp_ne]=to;tp_edge_next[tp_ne]=tp_head[from];tp_head[from]=tp_ne;tp_ne++;return 1;}
int tp_analyze(void){int i,j,k,id,op,u,last,target,tail=0,pos=0,entry;
 for(i=0;i<tp_nn;i++)if(tp_lab[i]>=0&&tp_isprologue(tp_lab[i])){tp_pro[i]=1;tp_anchor[i]=1;}
 for(i=0;i<4;i++){char *name=i==0?"_start":i==1?"main":i==2?"__init":"__main_ret";id=tp_lookup(name);if(id>=0&&tp_lab[id]>=0)tp_anchor[id]=1;}
 for(i=0;i<tp_nrows;i++)if(tp_rk[i]==1&&tp_ro[i]==tp_OP_CALL){id=tp_an[i*8];if(id<0||tp_lab[id]<0){tp_bad=1;return 0;}tp_anchor[id]=1;}
 tp_startmark[0]=1;for(i=0;i<tp_nn;i++)if(tp_anchor[i])tp_startmark[tp_firstlab[tp_rp[tp_lab[i]]]]=1;
 for(i=0;i<tp_nrows;i++)if(tp_startmark[i]){if(tp_un>=tp_ROWMAX){tp_bad=1;return 0;}tp_us[tp_un]=i;tp_head[tp_un]=-1;tp_un++;}
 if(tp_un==0){tp_un=1;tp_us[0]=0;tp_head[0]=-1;}
 for(u=0;u<tp_un;u++){tp_ue[u]=u+1<tp_un?tp_us[u+1]:tp_nrows;for(i=tp_us[u];i<tp_ue[u];i++)tp_owner[i]=u;}
 entry=tp_addroot("_start");if(entry<0||tp_lab[entry]<0)for(i=0;i<tp_nrows;i++)if(tp_rk[i]==1){tp_roots[tp_owner[i]]=1;break;}
 tp_addroot("main");tp_addroot("__init");tp_addroot("__main_ret");
 for(i=0;i<tp_nrows;i++)if(tp_rk[i]==1){op=tp_ro[i];u=tp_owner[i];
  if(op==tp_OP_CALL||op==tp_OP_JUMP||op==tp_OP_JUMPZ){k=tp_length(tp_opshapes[op])-1;id=tp_an[i*8+k];if(id<0||tp_lab[id]<0){tp_bad=1;return 0;}
   if(!tp_edge(u,tp_owner[tp_lab[id]]))return 0;}
  else if(op==tp_OP_LEA){id=tp_an[i*8+1];if(id>=0&&tp_lab[id]>=0)tp_roots[tp_owner[tp_lab[id]]]=1;else if(id>=0&&!tp_datum[id]){tp_bad=1;return 0;}}
  if(op==tp_OP_CALLR||op==tp_OP_CALLM)tp_hascallr[u]=1;
 }
 for(u=0;u+1<tp_un;u++){last=-1;for(i=tp_us[u];i<tp_ue[u];i++)if(tp_rk[i]==1)last=tp_ro[i];
  if(last!=tp_OP_RET&&last!=tp_OP_JUMP&&last!=tp_OP_EXIT)if(!tp_edge(u,u+1))return 0;}
 for(u=0;u<tp_un;u++)if(tp_roots[u]){tp_alive[u]=1;tp_queue[tail++]=u;}
 while(pos<tail){u=tp_queue[pos++];if(tp_hascallr[u]){tp_bad=1;return 0;}
  for(j=tp_head[u];j>=0;j=tp_edge_next[j]){target=tp_edge_to[j];if(!tp_alive[target]){tp_alive[target]=1;tp_queue[tail++]=target;if(tail>tp_ROWMAX){tp_bad=1;return 0;}}}}
 return 1;}

char tp_output[tp_SRCMAX];
int tp_prune_length;
/* Returns the original buffer on unsupported/capacity input. Not reentrant. */
char *tp_prune(char *text, int length) {
 int i, k, size, written;
 tp_prune_length=length;
 if(length<0||length>tp_SRCMAX)return text;
 for(i=0;i<length;i++)if(text[i]==13||text[i]==11||text[i]==12)return text;
 tp_nn=0;tp_un=0;tp_ne=0;tp_bad=0;tp_nrows=0;tp_npc=0;tp_nt=0;
 for(i=0;i<tp_NAMEMAX;i++){tp_datum[i]=0;tp_pro[i]=0;tp_anchor[i]=0;}
 for(i=0;i<tp_ROWMAX;i++){tp_startmark[i]=0;tp_alive[i]=0;tp_roots[i]=0;tp_hascallr[i]=0;}
 tp_input=text;tp_inputn=length;
 if(!tp_parse()||!tp_analyze()||tp_bad)return text;
 size=0;
 for(i=0;i<tp_nrows;i++)if(tp_rk[i]==3||tp_alive[tp_owner[i]]){
  written=tp_re[i]-tp_rs[i];for(k=0;k<written;k++)tp_output[size+k]=tp_input[tp_rs[i]+k];size+=written;
 }
 tp_prune_length=size;return tp_output;
}
