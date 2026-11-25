fn() { local b=$1 n=$2 i=0 k j; while [ $i -lt $n ]; do k=$((b+i)); echo "static int f$k(int a){int r=a; r=r+$k; r=r*2; r=r-1; r=r+3; r=r*3; r=r-2; r=r+5; r=r*2; r=r-4; r=r+7; r=r*3; r=r-6; r=r+9; r=r*2; r=r-8; r=r+11; r=r*3; if(r>100)r=r-50; return r;}"; i=$((i+1)); done; }
fn 0 210 > one.cx; echo 'int main(void){return f1(1)>0?0:0;}' >> one.cx
: > root.cx; for m in 0 1 2 3 4 5 6 7 8; do { echo "#pragma unisa module u$m"; fn $((m*23)) 23; echo "int ex$m(void){return f$((m*23))(3);}"; } > u$m.cx; echo "#pragma unisa use u$m" >> root.cx; done
echo 'int main(void){return ex0()+ex4()+ex8()>0?0:1;}' >> root.cx
wc -l one.cx u0.cx root.cx
