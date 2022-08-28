#include "ccbase.hpp"
void dump_buffer(unsigned char *input,int size,char *output,int size1){
    char * pp;
    pp=output;
    for(int i=0;i<size;i++){
        int n=2;

        sprintf(pp,"%02x",(unsigned int )(input[i]));
        pp=pp+n;
        if(pp-output>=size1-n){
            break;
        }
    }
}
