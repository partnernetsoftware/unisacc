/* Cooperative private mailbox consumer. Only the unique ACTIVE owner calls
 * this library. Trusted ancestors, no external replacement and all writers
 * sharing this process-record lock are prerequisites (not authentication).
 * No creation, deletion, replay of started, or crash exactly-once promise. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "csih_message_io.h"

typedef struct {
    int lock, dirs[5], count, unfinished, selected_kind;
    char paths[5][4096], ids[1024][33], selected[33];
    char *buf;
    csih_message message;
} cmi_work;

static void cmi_why(char *why,size_t cap,const char *s) {
    if(why && cap) snprintf(why,cap,"%s",s);
}
static int cmi_id(const char *s) {
    size_t i;if(!s || strlen(s)!=32)return 0;
    for(i=0;i<32;i++)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return 0;
    return 1;
}
static int cmi_session(const char *s) {
    size_t i,n;if(!s)return 0;n=strlen(s);if(!n||n>64)return 0;
    for(i=0;i<n;i++)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]==':'||s[i]=='_'||s[i]=='-'))return 0;
    return 1;
}
static int cmi_path(char *dst,const char *parent,const char *name) {
    size_t a=strlen(parent),b=strlen(name);
    if(a+b+1>=4096)return 0;
    memcpy(dst,parent,a);dst[a]='/';memcpy(dst+a+1,name,b+1);return 1;
}
static int cmi_check(int fd,int directory) {
    struct stat sb;if(fstat(fd,&sb)<0)return 0;
    return sb.st_uid==getuid() && (directory?S_ISDIR(sb.st_mode):S_ISREG(sb.st_mode)) && (sb.st_mode&07777)==(directory?0700:0600);
}
static int cmi_read(cmi_work *w,const char *path,const char *id,const char *session,csih_message *msg,char *why,size_t cap) {
    int fd,ok=0,kind=0;size_t n=0;ssize_t nr;struct stat sb;
    fd=open(path,O_RDONLY|O_NOFOLLOW|O_CLOEXEC,0);
    if(fd<0){cmi_why(why,cap,"message open failed");return 0;}
    if(!cmi_check(fd,0)||fstat(fd,&sb)<0||sb.st_size<1||sb.st_size>32768){cmi_why(why,cap,"message kind/owner/permissions/size invalid");goto end;}
    while(n<32769){nr=read(fd,w->buf+n,32769-n);if(nr<0){if(errno==EINTR)continue;cmi_why(why,cap,"message read failed");goto end;}if(nr==0)break;n+=(size_t)nr;}
    if(n!=(size_t)sb.st_size){cmi_why(why,cap,"message length changed or oversized");goto end;}
    kind=csih_message_decode(w->buf,n,session,msg,why,cap);
    if(!kind)goto end;
    if(strcmp(msg->id,id)){cmi_why(why,cap,"message id/filename mismatch");goto end;}
    ok=kind;
end:
    if(close(fd)<0){cmi_why(why,cap,"message close failed");ok=0;}
    return ok;
}
/* Open lock exactly once: another close of this inode in the same process
 * releases POSIX record locks. Child processes must not reuse this lifecycle. */
static int cmi_open(cmi_work *w,const char *dir,char *why,size_t cap) {
    static const char *names[]={"ready","started","done"};struct flock lk;char lockpath[4096];int i;
    if(!dir||dir[0]!='/'||strlen(dir)>=4096){cmi_why(why,cap,"absolute session directory required");return -1;}
    strcpy(w->paths[0],dir);
    if(!cmi_path(w->paths[1],dir,"inbox")){cmi_why(why,cap,"path too long");return -1;}
    for(i=0;i<2;i++){w->dirs[i]=open(w->paths[i],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC,0);if(w->dirs[i]<0||!cmi_check(w->dirs[i],1)){cmi_why(why,cap,"directory kind/owner/permissions invalid");return -1;}}
    if(!cmi_path(lockpath,w->paths[1],"mailbox.lock")){cmi_why(why,cap,"path too long");return -1;}
    w->lock=open(lockpath,O_RDWR|O_NOFOLLOW|O_CLOEXEC,0);
    if(w->lock<0||!cmi_check(w->lock,0)){cmi_why(why,cap,"lock kind/owner/permissions invalid");return -1;}
    memset(&lk,0,sizeof lk);lk.l_type=F_WRLCK;lk.l_whence=SEEK_SET;
    if(fcntl(w->lock,F_SETLK,(long)&lk)<0){if(errno==EAGAIN||errno==EACCES){cmi_why(why,cap,"mailbox busy");return 0;}cmi_why(why,cap,"record lock failed");return -1;}
    for(i=0;i<3;i++){if(!cmi_path(w->paths[i+2],w->paths[1],names[i])){cmi_why(why,cap,"path too long");return -1;}w->dirs[i+2]=open(w->paths[i+2],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC,0);if(w->dirs[i+2]<0||!cmi_check(w->dirs[i+2],1)){cmi_why(why,cap,"state directory kind/owner/permissions invalid");return -1;}}
    return 1;
}
static int cmi_scan(cmi_work *w,const char *session,int tasks,char *why,size_t cap) {
    DIR *d;struct dirent *e;int i,j,k,ok,seen=0;char path[4096],id[33];csih_message *msg;
    msg=malloc(sizeof *msg);if(!msg){cmi_why(why,cap,"allocation failed");return 0;}
    d=opendir(w->paths[1]);if(!d){free(msg);cmi_why(why,cap,"inbox scan open failed");return 0;}
    ok=1;
    while(1){errno=0;e=readdir(d);if(!e)break;if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;seen++;if(seen>4|| (strcmp(e->d_name,"ready")&&strcmp(e->d_name,"started")&&strcmp(e->d_name,"done")&&strcmp(e->d_name,"mailbox.lock"))){ok=0;cmi_why(why,cap,"unknown inbox entry");break;}}
    if(ok&&errno){ok=0;cmi_why(why,cap,"inbox scan read failed");}
    if(closedir(d)<0){ok=0;cmi_why(why,cap,"inbox scan close failed");}
    if(seen!=4&&ok){ok=0;cmi_why(why,cap,"inbox entries missing");}
    for(i=2;i<5&&ok;i++){
        d=opendir(w->paths[i]);if(!d){ok=0;cmi_why(why,cap,"state scan open failed");break;}
        while(1){errno=0;e=readdir(d);if(!e)break;
            if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
            if(strlen(e->d_name)!=37||strcmp(e->d_name+32,".json")){ok=0;cmi_why(why,cap,"unknown state entry");break;}
            memcpy(id,e->d_name,32);id[32]=0;
            if(!cmi_id(id)||w->count>=1024){ok=0;cmi_why(why,cap,"invalid id or capacity1024 exceeded");break;}
            for(j=0;j<w->count;j++)if(!strcmp(w->ids[j],id))break;
            if(j!=w->count){ok=0;cmi_why(why,cap,"duplicate id across states");break;}
            strcpy(w->ids[w->count++],id);
            if(i!=4 && ++w->unfinished>32){ok=0;cmi_why(why,cap,"capacity32 exceeded");break;}
            if(!cmi_path(path,w->paths[i],e->d_name)){ok=0;cmi_why(why,cap,"path too long");break;}
            k=cmi_read(w,path,id,session,msg,why,cap);if(!k){ok=0;break;}
            if(i==2&&(tasks||k==2)&&(!w->selected[0]||strcmp(id,w->selected)<0)){strcpy(w->selected,id);w->selected_kind=k;memcpy(&w->message,msg,sizeof *msg);}
        }
        if(ok&&errno){ok=0;cmi_why(why,cap,"state scan read failed");}
        if(closedir(d)<0){ok=0;cmi_why(why,cap,"state scan close failed");}
    }
    free(msg);return ok;
}
static int cmi_cleanup(cmi_work *w) {
    int i,ok=1;for(i=4;i>=0;i--)if(w->dirs[i]>=0&&close(w->dirs[i])<0)ok=0;
    if(w->lock>=0&&close(w->lock)<0)ok=0;
    free(w->buf);return ok;
}
static int cmi_move(cmi_work *w,int src,int dst,const char *id,char *why,size_t cap,int *committed) {
    char name[38],a[4096],b[4096];struct stat sb;
    snprintf(name,sizeof name,"%s.json",id);
    if(!cmi_path(a,w->paths[src],name)||!cmi_path(b,w->paths[dst],name)){cmi_why(why,cap,"path too long");return 0;}
    if(lstat(b,&sb)==0||errno!=ENOENT){cmi_why(why,cap,"destination exists or cannot be checked");return 0;}
    if(rename(a,b)<0){cmi_why(why,cap,"rename failed");return 0;}
    *committed=1;
    if(fsync(w->dirs[src])<0||fsync(w->dirs[dst])<0){cmi_why(why,cap,"converted; directory persistence unconfirmed");return 0;}
    return 1;
}
static int cmi_operation(const char *dir,const char *session,int tasks,const char *finish,csih_message *out,char *why,size_t cap) {
    cmi_work *w;int i,opened,rc=-1,committed=0;char name[38],path[4096];
    if(out)memset(out,0,sizeof *out);
    if(!cmi_session(session)||(!finish&&(!out||(tasks!=0&&tasks!=1)))||(finish&&!cmi_id(finish))){cmi_why(why,cap,"invalid arguments");return -1;}
    w=calloc(1,sizeof *w);if(!w){cmi_why(why,cap,"allocation failed");return -1;}
    w->lock=-1;for(i=0;i<5;i++)w->dirs[i]=-1;
    w->buf=malloc(32769);if(!w->buf){cmi_why(why,cap,"allocation failed");goto end;}
    opened=cmi_open(w,dir,why,cap);if(opened<=0){if(!opened&&!finish)rc=0;goto end;}
    if(!cmi_scan(w,session,tasks,why,cap))goto end;
    if(finish){
        snprintf(name,sizeof name,"%s.json",finish);
        if(!cmi_path(path,w->paths[3],name)){cmi_why(why,cap,"path too long");goto end;}
        if(!cmi_read(w,path,finish,session,&w->message,why,cap))goto end;
        if(cmi_move(w,3,4,finish,why,cap,&committed))rc=0;
    }else if(!w->selected_kind){rc=0;cmi_why(why,cap,"no eligible message");}
    else if(cmi_move(w,2,3,w->selected,why,cap,&committed))rc=w->selected_kind;
end:
    if(!cmi_cleanup(w)){rc=-1;cmi_why(why,cap,committed?"converted; close unconfirmed":"close failed");}
    if(committed&&rc<0)rc=-2;
    if(rc==1||rc==2)memcpy(out,&w->message,sizeof *out);
    else if(out)memset(out,0,sizeof *out);
    if(rc>=0&&!(rc==0&&!finish))cmi_why(why,cap,"ok");
    free(w);return rc;
}
int csih_message_take(const char *dir,const char *session,int tasks,csih_message *out,char *why,size_t cap) {
    return cmi_operation(dir,session,tasks,NULL,out,why,cap);
}
int csih_message_finish(const char *dir,const char *session,const char *id,char *why,size_t cap) {
    if(!id){cmi_why(why,cap,"invalid id");return -1;}
    return cmi_operation(dir,session,0,id,NULL,why,cap);
}
