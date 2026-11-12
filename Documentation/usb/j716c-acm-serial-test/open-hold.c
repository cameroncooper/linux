/* SPDX-License-Identifier: MIT */
/* One finite raw serial exchange; every post-open path retains TTY until reset. */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <poll.h>
#include <termios.h>
#include "packet-hash.h"
static uint64_t now(void) {struct timespec t;if(clock_gettime(CLOCK_BOOTTIME,&t))return 0;return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static __attribute__((noreturn)) void held(const char *s) { (void)write(1,s,strlen(s));for(;;)pause(); }
static int transfer(int fd,unsigned char *b,size_t n,int sending,uint64_t end) {
 size_t used=0;
 while(used<n) {
  uint64_t t=now();if(!t||t>=end)return 0;
  struct pollfd p={fd,sending?POLLOUT:POLLIN,0};int r=poll(&p,1,(int)((end-t+999999)/1000000));
  if(r<0||!r||(p.revents&(POLLERR|POLLHUP|POLLNVAL)))return 0;
  ssize_t z=sending?write(fd,b+used,n-used):read(fd,b+used,n-used);
  if(z<0&&(errno==EAGAIN||errno==EWOULDBLOCK))continue;
  if(z<=0)return 0;used+=(size_t)z;
 }
 return 1;
}
static void header(unsigned char h[64],const char *token,unsigned seq,unsigned size,unsigned dir) {
 memset(h,0,64);memcpy(h,"J7ACM1\r\n",8);memcpy(h+8,token,32);
 for(unsigned i=0;i<4;i++){h[40+i]=(unsigned char)(seq>>(8*i));h[44+i]=(unsigned char)(size>>(8*i));}h[48]=(unsigned char)dir;
}
static unsigned char byte(unsigned i,const char *token,unsigned seq,unsigned dir) {return (unsigned char)(i*29+(i>>8)+(unsigned char)token[i%32]+dir*97+seq*13);}
static void digest(const void *data,size_t n,char out[65]) {struct sha s;unsigned char d[32];init(&s);update(&s,data,n);final(&s,d);tohex(d,out);}
static void decimal(unsigned v,char b[16]) {char rev[16];unsigned n=0;do{rev[n++]=(char)('0'+v%10);v/=10;}while(v);for(unsigned i=0;i<n;i++)b[i]=rev[n-1-i];b[n]=0;}
static void receipt(const char *dir,unsigned seq,const unsigned char *b,unsigned n) {
 char hash[65],line[160]="ACM_DATA_",number[16];digest(b,n,hash);strcat(line,dir);strcat(line,"_SEQ=");decimal(seq,number);strcat(line,number);strcat(line," LEN=");decimal(n,number);strcat(line,number);strcat(line," SHA256=");strcat(line,hash);strcat(line,"\n");(void)write(1,line,strlen(line));
}
int main(int argc,char **argv) {
 char boot[38]={0};struct stat before,after;sigset_t mask;
 if(argc!=3||strlen(argv[1])!=36||strlen(argv[2])!=32||getuid()!=0||geteuid()!=0)return 2;
 for(unsigned i=0;i<32;i++)if(!((argv[2][i]>='0'&&argv[2][i]<='9')||(argv[2][i]>='a'&&argv[2][i]<='f')))return 2;
 if(setsid()<0||sigfillset(&mask)||sigprocmask(SIG_SETMASK,&mask,0))return 2;
 int file=open("/proc/sys/kernel/random/boot_id",O_RDONLY|O_CLOEXEC);if(file<0)return 2;
 ssize_t count=read(file,boot,37);int closed=close(file);
 if(count!=37||closed||boot[36]!='\n'||memcmp(boot,argv[1],36))return 2;
 if(lstat("/dev/ttyGS0",&before)||!S_ISCHR(before.st_mode)||before.st_uid)return 2;
 int tty=open("/dev/ttyGS0",O_RDWR|O_NOCTTY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);if(tty<0)return 2;
 if(tty!=3||fstat(tty,&after)||!S_ISCHR(after.st_mode)||after.st_dev!=before.st_dev||after.st_ino!=before.st_ino||after.st_rdev!=before.st_rdev)held("ACM_DATA_HELD_ERROR=inode\n");
 struct termios term;
 if(tcgetattr(tty,&term))held("ACM_DATA_HELD_ERROR=tcgetattr\n");
 term.c_iflag=0;term.c_oflag=0;term.c_lflag=0;term.c_cflag=(term.c_cflag&~(CSIZE|PARENB|CSTOPB|HUPCL|CRTSCTS))|CS8|CREAD|CLOCAL;term.c_cc[VMIN]=1;term.c_cc[VTIME]=0;
 if(tcsetattr(tty,TCSANOW,&term))held("ACM_DATA_HELD_ERROR=tcsetattr\n");
 uint64_t start=now();if(!start)held("ACM_DATA_HELD_ERROR=clock\n");uint64_t end=start+9000000000ULL;
 (void)write(1,"ACM_TTY_HOLD_READY=1 FD=3\n",sizeof("ACM_TTY_HOLD_READY=1 FD=3\n")-1);
 static unsigned char data[65536];unsigned char got[64],expected[64];const unsigned sizes[2]={4096,65536};
 for(unsigned seq=0;seq<2;seq++) {
  unsigned n=sizes[seq];header(expected,argv[2],seq,n,0);
  if(!transfer(tty,got,64,0,end)||memcmp(got,expected,64))held("ACM_DATA_HELD_ERROR=RXheader\n");
  if(!transfer(tty,data,n,0,end))held("ACM_DATA_HELD_ERROR=RXpayload\n");
  for(unsigned i=0;i<n;i++)if(data[i]!=byte(i,argv[2],seq,0))held("ACM_DATA_HELD_ERROR=RXbytes\n");
  receipt("RX",seq,data,n);header(expected,argv[2],seq,n,1);
  for(unsigned i=0;i<n;i++)data[i]=byte(i,argv[2],seq,1);
  if(!transfer(tty,expected,64,1,end)||!transfer(tty,data,n,1,end))held("ACM_DATA_HELD_ERROR=TX\n");
  receipt("TX",seq,data,n);
 }
 held("ACM_DATA_PASS=1 BYTES_EACH_WAY=69632\n");
}
