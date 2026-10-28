/* Linux x86_64 native process-group ABI. include/unistd.h currently leaves
 * this API to host forwarding; the explicit runtime module owns it here.
 * __NR_setpgid=109 is the Linux x86_64 syscall ABI, not a host libc call. */
int setpgid(int pid,int pgid){
 long result=__syscall6(109,(long)pid,(long)pgid,0,0,0);
 if(result<0){errno=(int)-result;return -1;}return (int)result;
}
