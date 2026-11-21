/* Verify bit_write_MC vs bit_read_MC roundtrip, transcribed from bits.c. */
#include <stdio.h>
#include <string.h>

static unsigned char buf[64]; static size_t wp, rp;

static void WR(unsigned char b){ buf[wp++]=b; }
static unsigned char RC(void){ return buf[rp++]; }

static void write_MC(long val){
    int i,j,negative=0;
    unsigned char byte[5]={0,0,0,0,0};
    unsigned int value=(unsigned int)val;
    if(val<0){ negative=1; value=(unsigned int)-val; }
    for(i=4,j=0;i>=0;i--,j+=7){
        byte[i]=(unsigned char)((value & 0x7f)>>j);
        byte[i]|=0x80;
    }
    for(i=0;i<4;i++) if(byte[i]&0x7f) break;
    if(byte[i]&0x40 && i>0) i--;
    byte[i]&=0x7f;
    if(negative) byte[i]|=0x40;
    for(j=4;j>=i;j--) WR(byte[j]);
}

static int read_MC(void){
    int i,j,negative=0;
    unsigned char byte[8];
    unsigned int result=0;
    for(i=4,j=0;i>=0;i--,j+=7){
        byte[i]=RC();
        if(!(byte[i]&0x80)){
            if(byte[i]&0x40){ negative=1; byte[i]&=0xbf; }
            result|=((unsigned int)byte[i])<<j;
            return negative? -(int)result : (int)result;
        }
        else byte[i]&=0x7f;
        result|=((unsigned int)byte[i])<<j;
    }
    return 0;
}

int main(void){
    long vals[]={4,160,227,63,38,1073,12345,-63,38,0};
    int n=sizeof(vals)/sizeof(vals[0]);
    wp=0;
    for(int k=0;k<n;k++) write_MC(vals[k]);
    printf("encoded bytes: "); for(size_t z=0;z<wp;z++) printf("%02x ",buf[z]); printf("\n");
    rp=0;
    for(int k=0;k<n;k++){ int v=read_MC(); printf("in=%ld read=%d %s\n",vals[k],v,v==vals[k]?"OK":"BAD"); }
    return 0;
}
