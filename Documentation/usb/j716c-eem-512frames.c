/* SPDX-License-Identifier: GPL-2.0-only */
/* One finite synthetic SSH stdio exchange; no device or network control. */
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "j716c-eem-512frames.h"
static void digest(const void *p,size_t n,char out[65]) {
    struct sha s;unsigned char h[32];init(&s);update(&s,p,n);final(&s,h);tohex(h,out);
}
static void sequence_text(unsigned n,char out[4]) {
    char reversed[3];unsigned used=0;
    do{reversed[used++]=(char)('0'+n%10);n/=10;}while(n);
    for(unsigned i=0;i<used;i++)out[i]=reversed[used-1-i];
    out[used]=0;
}
static unsigned char buf[65536], expected[65536];
static int io(int fd, void *p, size_t n, int writing) {
    unsigned char *b=p;
    while(n) {
        ssize_t k=writing?write(fd,b,n):read(fd,b,n);
        if(k<0&&errno==EINTR)continue;
        if(k<=0)return 0;
        b+=k;n-=(size_t)k;
    }
    return 1;
}
static int line(char *out,size_t cap) {
    size_t n=0;
    while(n+1<cap) {
        if(!io(0,out+n,1,0)||!out[n])return 0;
        if(out[n++]=='\n'){out[n]=0;return 1;}
    }
    return 0;
}
static void fill(unsigned char *p,size_t n,const char *token,int direction,unsigned seq) {
    for(size_t i=0;i<n;i++)p[i]=(unsigned char)((unsigned char)token[i%32]+17*i+31*direction+7*seq+(i>>8));
}
int main(int argc,char **argv) {
    if(argc!=2||strlen(argv[1])!=32||strspn(argv[1],"0123456789abcdef")!=32)return 2;
    const char *token=argv[1];char got[256],header[256],hash[65];
    for(unsigned seq=0;seq<512;seq++) {
        size_t n=(seq&1)?65536:4096;char sequence[4];sequence_text(seq,sequence);
        fill(expected,n,token,0,seq);digest(expected,n,hash);
        strcpy(header,"EEM4 ");strcat(header,token);strcat(header," H2T ");strcat(header,sequence);strcat(header,(seq&1)?" 65536 ":" 4096 ");strcat(header,hash);strcat(header,"\n");
        size_t z=strlen(header);
        if(z>=sizeof header||!line(got,sizeof got)||strcmp(header,got)||!io(0,buf,n,0)||memcmp(buf,expected,n))return 3;
        digest(buf,n,hash);
        strcpy(header,"RECEIVED ");strcat(header,token);strcat(header," H2T ");strcat(header,sequence);strcat(header,(seq&1)?" 65536 ":" 4096 ");strcat(header,hash);strcat(header,"\n");z=strlen(header);
        if(z>=sizeof header||!io(1,header,(size_t)z,1))return 4;
        fill(buf,n,token,1,seq);digest(buf,n,hash);
        strcpy(header,"EEM4 ");strcat(header,token);strcat(header," T2H ");strcat(header,sequence);strcat(header,(seq&1)?" 65536 ":" 4096 ");strcat(header,hash);strcat(header,"\n");z=strlen(header);
        if(z>=sizeof header||!io(1,header,(size_t)z,1)||!io(1,buf,n,1))return 5;
    }
    unsigned char extra;ssize_t k;
    do{k=read(0,&extra,1);}while(k<0&&errno==EINTR);
    if(k!=0)return 6;
    strcpy(header,"EEM_DATA_PASS TOKEN=");strcat(header,token);strcat(header," BYTES_EACH_DIRECTION=17825792 FRAMES_EACH_DIRECTION=512\n");size_t z=strlen(header);
    return z>0&&(size_t)z<sizeof header&&io(1,header,(size_t)z,1)?0:7;
}
