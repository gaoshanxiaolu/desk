/*

* Copyright (c) 2016 Qualcomm Technologies, Inc.

* All Rights Reserved.

* Confidential and Proprietary - Qualcomm Technologies, Inc.

*/

#include "qcom_common.h"
//#include "qcom_internal.h"
//Workaround- could not include qcom_internal.h
#ifndef IP_CONFIG_QUERY
#define IP_CONFIG_QUERY 0
#endif

int add_params(int p0, int p1, int p2, int p3)
{
    return p0 + p1 + p2 +p3;
}


int get_dev_addr()
{
    A_UINT8 macAddr[6];
    A_MEMSET(&macAddr, 0, sizeof (macAddr));
    A_UINT32 ipaddr;
    A_UINT32 submask;
    A_UINT32 gateway;

    qcom_mac_get(0, (A_UINT8 *) & macAddr);
    qcom_ipconfig(0, IP_CONFIG_QUERY, &ipaddr, &submask, &gateway);

    A_PRINTF("MAC addr= %02x:%02x:%02x:%02x:%02x:%02x\n",
            macAddr[0], macAddr[1], macAddr[2],
            macAddr[3], macAddr[4], macAddr[5]);
    A_PRINTF("IP  addr: %d.%d.%d.%d\n", (ipaddr) >> 24 & 0xFF,
           (ipaddr) >> 16 & 0xFF, (ipaddr) >> 8 & 0xFF, (ipaddr) & 0xFF);

    return 0;
}


void test_funcs()
{
    int sum;


    /*local functions*/
    sum = add_params(0, 1 ,2, 3);
    if (sum == 6) {
        A_PRINTF("Local function call: Success\n");
    } else {
        A_PRINTF("Local function call: FAIL\n");
    }


    /*ROM functions*/
    A_PRINTF("\nBegin of ROM function Test\n");
    get_dev_addr();
    A_PRINTF("End of ROM function Test\n");


}

