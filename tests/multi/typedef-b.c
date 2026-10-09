

int f(unsigned code){
  code >>= 1;
  return (int)code;
}
int main(void){ return f(4) != 2; }
