/* Minimal C handle-map decoder, transcribed from decode_latest.c 2592-2670
 * and bits_ref.c bit_read_UMC/MC. Reads hdl_full.bin, prints first entries. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *dat;
static size_t byte;

static unsigned char RC(void){ return dat[byte++]; }

static unsigned int UMC(void){
    unsigned int result=0; int j=0;
    for(int i=7;i>=0;i--){
        unsigned char b=RC();
        if(!(b&0x80)){ result |= b<<j; return result; }
        result |= (b&0x7f)<<j; j+=7;
    }
    return 0;
}

static int MC(void){
    int negative=0; unsigned int result=0; int j=0;
    for(int i=4;i>=0;i--){
        unsigned char b=RC();
        if(!(b&0x80)){
            if(b&0x40){ negative=1; b&=0xbf; }
            result |= b<<j;
            return negative? -(int)result : (int)result;
        }
        b&=0x7f; result |= b<<j; j+=7;
    }
    return 0;
}

static unsigned int RS_BE(void){
    unsigned char b1=RC(),b2=RC();
    return ((unsigned)b1<<8)|b2;
}

int main(int argc,char**argv){
    FILE*f=fopen(argv[1],"rb");
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    dat=malloc(n); fread(dat,1,n,f); fclose(f);
    size_t pos=0; int page=0;
    while(pos+2<(size_t)n && page<22){
        byte=pos;
        unsigned int ss=RS_BE();
        size_t start=byte;
        if(ss>2040){ fprintf(stderr,"page %d bad size %u @%zu\n",page,ss,pos); break; }
        size_t last_off=0,last_h=0; int cnt=0;
        while(byte-start<ss){
            size_t old=byte;
            unsigned int ho=UMC();
            if(!ho){ byte=old+1; ho=UMC(); }
            int off=MC();
            last_off+=off; last_h+=ho;
            if(cnt<12) printf("page%d #%d h=%llu ho=%u off=%d loc=%llu\n",
                page,cnt,(unsigned long long)last_h,ho,off,(unsigned long long)last_off);
            if(byte==old) break;
            cnt++;
        }
        fprintf(stderr,"page %d size=%u entries=%d end=%zu\n",page,ss,cnt,byte);
        pos=byte+2; page++;
    }
    return 0;
}
