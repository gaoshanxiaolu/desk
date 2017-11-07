/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "threadx/tx_api.h"
#include "qcom_cli.h"
#include "threadxdmn_api.h"
#include <aj_target.h>
#include "aj_status.h"
#include "aj_wifi_ctrl.h"

TX_THREAD host_thread;
#ifdef REV74_TEST_ENV4

#define BYTE_POOL_SIZE (2*1024 + 128 )
#define PSEUDO_HOST_STACK_SIZE (2 * 1024 )   /* small stack for pseudo-Host thread */

#else

#define BYTE_POOL_SIZE (3*1024 + 256 )
#define PSEUDO_HOST_STACK_SIZE (3 * 1024 )   /* small stack for pseudo-Host thread */

#endif
TX_BYTE_POOL pool;

extern void AllJoyn_Start(unsigned long arg);

void user_main(void)
{
    extern void task_execute_cli_cmd();
    tx_byte_pool_create(&pool, "AllJoyn pool", TX_POOL_CREATE_DYNAMIC, BYTE_POOL_SIZE);

    {
        CHAR *pointer;
        tx_byte_allocate(&pool, (VOID **) & pointer, PSEUDO_HOST_STACK_SIZE, TX_NO_WAIT);

        tx_thread_create(&host_thread, "AllJoyn thread", AllJoyn_Start,
                         0, pointer, PSEUDO_HOST_STACK_SIZE, 16, 16, 4, TX_AUTO_START);
    }

    cdr_threadx_thread_init();
}


/**
 * Wifi Configuration
 * This section of code enables an application developer to choose at build
 * time which WiFi configuration method to use. The choices are
 *  - connect to a known access point via SSID, passphrase, and security type
 *  - create an access point with a specific SSID and passphrase
 *  - or do all of the configuration work in the application code (OnBoarding does this)
 *
 * The options that are _not_ chosen are given empty implementations which override
 * the default implementations in the target-specific library
 */
#ifndef AJ_CONFIGURE_WIFI_UPON_START
/* supply a stubbed implementations when WiFi should not be automatically configured. */
AJ_Status ConfigureWirelessNetwork(void)
{
    return AJ_OK;
}
#endif


#ifdef WIFI_SSID
const char* AJ_ssid = WIFI_SSID;

/* supply a stubbed implementation of this function when connecting to a predefined access point. */
AJ_Status ConfigureSoftAP()
{
    return AJ_ERR_INVALID;
}

#ifdef WIFI_PASSPHRASE
const char* AJ_passphrase = WIFI_PASSPHRASE;
#else
const char* AJ_passphrase = NULL;
#endif

#ifdef WIFI_SECURITY_TYPE
const AJ_WiFiSecurityType AJ_secType = WIFI_SECURITY_TYPE;
const AJ_WiFiCipherType AJ_cipherType = WIFI_CIPHER_TYPE;
#else
const AJ_WiFiSecurityType AJ_secType = AJ_WIFI_SECURITY_WPA2;
const AJ_WiFiCipherType AJ_cipherType = AJ_WIFI_CIPHER_CCMP;
#endif

#endif  /* ifdef WIFI_SSID */



#ifdef SOFTAP_SSID
const char* AJ_ssid = SOFTAP_SSID;

/* supply a stubbed implementation of this function when creating an access point. */
AJ_Status ConfigureWifi()
{
    return AJ_OK;
}

#ifdef SOFTAP_PASSPHRASE
const char* AJ_passphrase = SOFTAP_PASSPHRASE;
#else
const char* AJ_passphrase = NULL;
#endif

#ifdef SOFTAP_SECURITY_TYPE
const AJ_WiFiSecurityType AJ_secType = SOFTAP_SECURITY_TYPE;
const AJ_WiFiCipherType AJ_cipherType = SOFTAP_CIPHER_TYPE;
#else
const AJ_WiFiSecurityType AJ_secType = AJ_WIFI_SECURITY_WPA2;
const AJ_WiFiCipherType AJ_cipherType = AJ_WIFI_CIPHER_CCMP;
#endif


#endif  /* ifdef SOFTAP_SSID */
/**
 *  End of wifi configuration section
 */


