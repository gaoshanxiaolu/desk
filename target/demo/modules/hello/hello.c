/*

* Copyright (c) 2016 Qualcomm Technologies, Inc.

* All Rights Reserved.

* Confidential and Proprietary - Qualcomm Technologies, Inc.

*/

#include "qcom_common.h"
#include "module_api.h"

static int hello_init()
{
    A_PRINTF("hello world\n");
    return 0;
}


static void hello_exit()
{
    A_PRINTF("hello exit\n");
    return;
}

MODULE_INIT(hello_init);
MODULE_EXIT(hello_exit);

