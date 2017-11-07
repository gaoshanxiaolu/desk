/*

* Copyright (c) 2016 Qualcomm Technologies, Inc.

* All Rights Reserved.

* Confidential and Proprietary - Qualcomm Technologies, Inc.

*/

#include "qcom_common.h"
#include "module_api.h"


extern void test_funcs();
extern void test_vars();
extern void test_ram_sym();
extern void test_thread_init();
extern void test_thread_exit();
extern int add_params(int p0, int p1, int p2, int p3);
extern int swat_module_register_cb(void *callback);
extern void swat_add_params();


static int test_init()
{
    A_PRINTF("Start Module Test:\n");
 
    test_vars();
    test_funcs();
    swat_module_register_cb(add_params);
    swat_add_params();
    test_ram_sym();
    test_thread_init();

    return 0;
}


static void test_exit()
{
    swat_module_register_cb(NULL);
    test_thread_exit();
    A_PRINTF("End Module Test\n");

    return;
}

MODULE_INIT(test_init);
MODULE_EXIT(test_exit);

