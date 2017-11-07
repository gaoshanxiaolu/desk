/*

* Copyright (c) 2016 Qualcomm Technologies, Inc.

* All Rights Reserved.

* Confidential and Proprietary - Qualcomm Technologies, Inc.

*/

#include "qcom_common.h"
extern void swat_wmiconfig_devmode_get(A_UINT8 device_id, A_UINT32 *wifiMode);

void test_ram_sym()
{
    A_UINT32 wifiMode;

    wifiMode = -1;

    A_PRINTF("\nBegin of RAM function Test\n");
    swat_wmiconfig_devmode_get(0, &wifiMode);

    if (wifiMode == 0)
        A_PRINTF("WiFi Mode AP\n");
    else if (wifiMode == 1)
        A_PRINTF("WiFi Mode STA\n");
    else
        A_PRINTF("WiFi Mode Unknown\n");
    A_PRINTF("End of RAM function Test\n");
}
