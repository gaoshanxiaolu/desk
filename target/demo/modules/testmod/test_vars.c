/*

* Copyright (c) 2016 Qualcomm Technologies, Inc.

* All Rights Reserved.

* Confidential and Proprietary - Qualcomm Technologies, Inc.

*/

#include "qcom_common.h"

/*const*/
const char *cstr = "CONST";

/*Data*/
int g_test_val_a5a5 =  0xa5a5;

/*Uninitialized Data*/
int g_test_val_zero;

void test_vars()
{
    int err = 0;

    /*Static Local*/
    static int l_test_val_1234 = 1234;


    if (g_test_val_a5a5 != 0xa5a5) {
        err ++;
    }

    if (l_test_val_1234 != 1234) {
       err ++;
    }

    if (g_test_val_zero != 0) {
       err ++;
    }


    if (A_STRCMP(cstr, "CONST") != 0) {
        err ++;
    }

    if (err)
       A_PRINTF("Variable Test: FAIL\n");
    else
       A_PRINTF("Variable Test: Success\n");
}

