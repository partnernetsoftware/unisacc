#!/usr/bin/env python3
"""Private host checks for the Linux directory slice; every child bounded."""
import os, pathlib, platform, subprocess, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
def run(args, expected=0):
    r = subprocess.run(list(map(str,args)), capture_output=True, timeout=15)
    if r.returncode != expected:
        raise RuntimeError(f'{args}: rc={r.returncode}: {r.stderr.decode(errors="replace")}')
    return r
SHIM = r'''#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifdef __linux__
#include <sys/syscall.h>
static long __open(char *p,int flags,int mode) { long n=open(p,flags,mode); return n<0?-errno:n; }
static long __close(int fd) { long n=close(fd); return n<0?-errno:n; }
static long __lseek(int fd,long off,int whence) { long n=lseek(fd,off,whence); return n<0?-errno:n; }
static long __getdents64(int fd,void *p,int n) { long r=syscall(SYS_getdents64,fd,p,n);return r<0?-errno:r; }
#endif
'''
CONTROL = r'''#include <stdio.h>
#include <string.h>
static int mode, calls;
static long __open(char *p,int flags,int perms) { return mode==9?-13:7; }
static long __close(int fd) { return 0; }
static long __getdents64(int fd,void *p,int n) {
 unsigned char *r=p; int len=24; memset(p,0,n); calls++;
 if(mode==8)return -13;
 if(mode==7)return 4097;
 if(calls>2)return 0;
 r[0]=42;r[16]=24;r[18]=8;memcpy(r+19,"test",5);
 if(mode==1)r[16]=0;
 if(mode==2)r[16]=16;
 if(mode==3)r[16]=25;
 if(mode==4)r[16]=32;
 if(mode==5)memset(r+19,'x',5);
 if(mode==6)len=19;
 return len;
}
static long __lseek(int fd,long off,int whence) { return 0; }
#include <stdlib.h>
#include <errno.h>
#undef __APPLE__   /* the mock writes Linux records (0.0.18 gave macOS its own layout) */
#include "dirent.h"
int main(void) {
 DIR *d; struct dirent *e; int k;
 d=opendir("ignored");if(!d)return 1;
 for(k=0;k<2;k++){e=readdir(d);if(!e||strcmp(e->d_name,"test")||e->d_ino!=42||e->d_type!=8)return 2;}
 if(readdir(d)||errno||calls!=3)return 3;closedir(d);
 for(mode=1;mode<=8;mode++){calls=0;d=opendir("ignored");if(!d)return 4;
 if(readdir(d)||errno!=(mode==8?13:5))return 5;
 if(readdir(d)||calls!=1)return 6;closedir(d);}
 mode=9;if(opendir("denied")||errno!=13)return 7;
 puts("dirent controls: multi-batch, malformed records and permission error passed");return 0;
}
'''
LIVE = r'''#include <stdio.h>
#include "dirent.h"
int main(int argc,char **argv){DIR *d;struct dirent *e;int n=0;
 d=opendir(argv[1]);if(!d)return 1;while((e=readdir(d))!=0){puts(e->d_name);n++;}
 if(errno||closedir(d))return 2;return n<3?3:0;}
'''
with tempfile.TemporaryDirectory(prefix='procenum-private-') as temp:
    d=pathlib.Path(temp)
    (d/'dirent.h').write_bytes((ROOT/'include/dirent.h').read_bytes())
    (d/'sys').mkdir(); (d/'sys'/'_win.h').write_bytes((ROOT/'include/sys/_win.h').read_bytes())   # dirent.h includes it (0.0.21)
    (d/'control.c').write_text(CONTROL)
    run(['cc','-std=c99','-I',d,d/'control.c','-o',d/'control'])
    print(run([d/'control']).stdout.decode().strip())
    source=(ROOT/'examples/apps/procview.c').read_text()
    if 'PROCMAX' in source or 'for (pid =' in source:
        raise RuntimeError('PID range scanning remains')
    if platform.system() == 'Linux':
        (d/'shim.h').write_text(SHIM)
        (d/'live.c').write_text(LIVE)
        run(['cc','-D_GNU_SOURCE','-std=c99','-include',d/'shim.h','-I',d,d/'live.c','-o',d/'live'])
        names=d/'names';names.mkdir()
        expected={f'entry-{i:04d}' for i in range(700)}|{'long-'+('x'*240)}
        for name in expected:(names/name).touch()
        got=set(run([d/'live',names]).stdout.decode().splitlines())- {'.','..'}
        if got!=expected:raise RuntimeError('real multi-batch directory/long name mismatch')
        fake=d/'proc';fake.mkdir()
        for name in ('1','900001','900002','notpid','12bad'):(fake/name).mkdir()
        for name in ('1','900001'):
            (fake/name/'status').write_text('Name:\tprobe-'+name+'\nPPid:\t0\nVmRSS:\t12 kB\n')
        (fake/'900002'/'status').write_text('Name:\tvanished\n')
        (fake/'900002'/'status').unlink()
        proc=d/'procview.c';proc.write_text(source)
        run(['cc','-D_GNU_SOURCE','-std=c99','-include',d/'shim.h','-I',d,
             '-DPROCVIEW_PROC_ROOT="'+str(fake)+'"',proc,'-o',d/'procview'])
        r=run([d/'procview','--capture'])
        pids={int(line.split()[0]) for line in r.stdout.splitlines()}
        if pids!={1,900001} or b'1 status files inaccessible or disappeared' not in r.stderr:
            raise RuntimeError('high PID / disappeared status regression')
        # Denied directory through a nonexistent path gives an actual open
        # failure even when this test runs as root. EACCES is tested above.
        (d/'deny.c').write_text(LIVE)
        run([d/'live',d/'missing'],expected=1)
        if os.geteuid() != 0:
            names.chmod(0)
            try:run([d/'live',names],expected=1)
            finally:names.chmod(0o700)
            print('Linux actual directory permission denial passed')
        else:print('Linux actual chmod permission denial not checked as root')
        print('Linux real directory: 701 entries, long name, high PID, missing status/path passed')
    else:
        print('Linux live directory execution not run on this host')
