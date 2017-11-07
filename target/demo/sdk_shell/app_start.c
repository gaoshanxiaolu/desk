/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_system.h"
#include "base.h"
#include "targaddrs.h"

void user_main(void);

void
app_start(void)
{
    QCOM_SYS_START_PARA_t sysStartPara;

    sysStartPara.isMccEnabled = 0;
    sysStartPara.numOfVdev = 1;

#if defined(FPGA)
    #if defined(ENABLED_MCC_MODE)
    sysStartPara.isMccEnabled = ENABLED_MCC_MODE;
    #endif

    #if defined(NUM_OF_VDEVS)
    sysStartPara.numOfVdev = NUM_OF_VDEVS;
    #endif
#else
    /*
    * For asic version, the MCC and device number are set in sdk_proxy.
    */
    if (HOST_INTEREST->hi_option_flag2 & HI_OPTION_MCC_ENABLE) {
        sysStartPara.isMccEnabled = 1;
    }
    #if defined(NUM_OF_VDEVS)
    sysStartPara.numOfVdev = NUM_OF_VDEVS;
    #else
    sysStartPara.numOfVdev = ((HOST_INTEREST->hi_option_flag >> HI_OPTION_NUM_DEV_SHIFT) & HI_OPTION_NUM_DEV_MASK);
    #endif
#endif

#if defined(WLAN_BTCOEX_ENABLED)
    /* 
     * assign patch function pointer to init BTCOEX after GPIOs are initialized.
     * (call to BTCOEX_Init was not compiled into ROM wlan_main_part1 fn)
     */
    extern void (*wlan_init_done_fn)(void);
    extern void btcoex_wlan_init_done(void);
    wlan_init_done_fn = btcoex_wlan_init_done; 

    /* ROM was built with stub fns for coex. Need to reference all the RAM fns here to be linked in */
    extern void BTCOEX_Attach_patch(void);
    BTCOEX_Attach_patch();
#endif /* WLAN_BTCOEX_ENABLED */

    qcom_sys_start(user_main, &sysStartPara);

    return;
}
