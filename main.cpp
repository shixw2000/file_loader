#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include"testcase.h"


void usage(int argc, char* argv[]) {
    printf("usage: %s <opt{1|2}>|\n", argv[0]);
}

int main(int argc, char* argv[]) {
    int ret = 0;
    int opt = 0;

    if (2 == argc) {
        opt = atoi(argv[1]);
    } else {
        usage(argc, argv);
        return -1;
    }

    ret = test_main(opt);
    
    return ret;
}

