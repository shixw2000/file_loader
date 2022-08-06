#ifndef __TESTCASE_H__
#define __TESTCASE_H__
#include"globaltype.h"
#include"ihandler.h"


class ProcHandler : public I_Handler {
public:
    virtual Void process(Void* msg);
};

int test_main(int opt);

#endif

