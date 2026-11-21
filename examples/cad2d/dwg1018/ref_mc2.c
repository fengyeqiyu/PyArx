#include <stdio.h>
static unsigned char buf[64]; static size_t rp;
static unsigned char RC(void){ return buf[rp++]; }
static int read_MC(void){
    int i,j,negative=0; unsigned char byte[8]; unsigned int result=0;
    for(i=4,j=0;i>=0;i--,j+=7){
        byte[i]=RC();
        if(!(byte[i]&0x80)){
            if(byte[i]&0x40){ negative=1; byte[i]&=0xbf; }
            result|=((unsigned int)byte[i])<<j;
            return negative? -(int)result:(int)result;
        } else byte[i]&=0x7f;
        result|=((unsigned int)byte[i])<<j;
    } return 0;
}
int main(void){
    struct{ unsigned char b[4]; int n; long expect; } t[]={
        {{0x20,0xa0},2,160},
        {{0x81,0xa0},2,160},
        {{0x84},1,4},
        {{0x80,0xc3},2,67},
    };
    for(int k=0;k<4;k++){
        rp=0; for(int z=0;z<t[k].n;z++) buf[z]=t[k].b[z];
        int v=read_MC(); printf("bytes %02x %02x -> %d (expect %ld) %s\n",
            t[k].b[0],t[k].b[1],v,t[k].expect,v==t[k].expect?"OK":"BAD");
    }
    return 0;
}
