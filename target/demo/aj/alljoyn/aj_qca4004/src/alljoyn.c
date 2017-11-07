/**
 * @file
 */
/******************************************************************************
 * Copyright 2013-2014, Qualcomm Connected Experiences, Inc.
 *
 ******************************************************************************/

#define AJ_MODULE ALLJOYN

#include <limits.h>
#include "aj_wifi_ctrl.h"
#include "aj_util.h"
#include "aj_debug.h"

#ifndef NDEBUG
AJ_EXPORT uint8_t dbgALLJOYN = 0;
#endif

#define NET_UP_TIMEOUT  15000

extern void AJ_Main();

#pragma weak AJ_ssid
extern const char* AJ_ssid;

#pragma weak AJ_passphrase
extern const char* AJ_passphrase;

extern const AJ_WiFiSecurityType AJ_secType;
extern const AJ_WiFiCipherType AJ_cipherType;

/*
 * This function is provided for testing convenience. Generates a 5 byte WEP hex key from an ascii
 * passphrase. We don't support the 13 byte version which uses an MD5 hash to generate the hex key
 * from the passphrase.
 */
static AJ_Status WEPKey(const char* pwd, uint8_t* hex, uint32_t hexLen)
{
    static const uint32_t WEP_MAGIC1 = 0x343FD;
    static const uint32_t WEP_MAGIC2 = 0x269EC3;
    uint32_t i;
    uint32_t seed;
    uint8_t key[5];

    for (i = 0; *pwd; ++i) {
        seed ^= *pwd++ << (i & 3) << 8;
    }
    for (i = 0; i < (hexLen / 2); ++i) {
        seed = WEP_MAGIC1 * seed + WEP_MAGIC2;
        key[i] = (seed >> 16);
    }
    return AJ_RawToHex(key, sizeof(key), hex, hexLen, FALSE);
}

#pragma weak ConfigureWifi
AJ_Status ConfigureWifi(void)
{
    AJ_Status status = AJ_ERR_CONNECT;
    AJ_WiFiSecurityType secType;
    AJ_WiFiCipherType cipherType;

    if (AJ_passphrase == NULL) {
        secType = AJ_WIFI_SECURITY_NONE;
        cipherType = AJ_WIFI_CIPHER_NONE;
    } else {
        secType = AJ_secType;
        cipherType = AJ_cipherType;
    }

    AJ_AlwaysPrintf(("Trying to connect to AP %s\n", AJ_ssid));
    if (secType == AJ_WIFI_SECURITY_WEP) {
        /* WEPkey does not return the proper key value based on passphrase.
            The passphrase for WEP must be the key value.
        char wepKey[11];
        WEPKey(AJ_passphrase, wepKey, sizeof(wepKey));
        status = AJ_ConnectWiFi(AJ_ssid, AJ_WIFI_SECURITY_WEP, AJ_WIFI_CIPHER_WEP, wepKey);
        */
        status = AJ_ConnectWiFi(AJ_ssid, AJ_WIFI_SECURITY_WEP, AJ_WIFI_CIPHER_WEP, AJ_passphrase);
    } else {
        status = AJ_ConnectWiFi(AJ_ssid, secType, cipherType, AJ_passphrase);
    }

    if (status != AJ_OK) {
        AJ_ErrPrintf(("ConfigureWifi error\n"));
    }

    if (AJ_GetWifiConnectState() == AJ_WIFI_AUTH_FAILED) {
        AJ_ErrPrintf(("ConfigureWifi authentication failed\n"));
        status = AJ_ERR_SECURITY;
    }
    return status;
}

char* AddrStr(uint32_t addr)
{
     static char txt[17];
     sprintf((char*)&txt, "%3lu.%3lu.%3lu.%3lu\0",
             (addr & 0xFF000000) >> 24,
             (addr & 0x00FF0000) >> 16,
             (addr & 0x0000FF00) >>  8,
             (addr & 0x000000FF)
             );

    return txt;
}


#pragma weak ConfigureSoftAP
AJ_Status ConfigureSoftAP(void)
{
    AJ_Status status = AJ_ERR_CONNECT;
    AJ_InfoPrintf(("Configuring soft AP %s\n", AJ_ssid));
    status = AJ_EnableSoftAP(AJ_ssid, FALSE, AJ_passphrase, INT_MAX);
    if (status == AJ_ERR_TIMEOUT) {
        AJ_ErrPrintf(("AJ_EnableSoftAP timeout\n"));
    } else if (status != AJ_OK) {
        AJ_ErrPrintf(("AJ_EnableSoftAP error\n"));
    }
    return status;
}

#pragma weak ScanResult
void ScanResult(void* context, const char* ssid, const uint8_t mac[6], uint8_t rssi, AJ_WiFiSecurityType secType, AJ_WiFiCipherType cipherType)
{
    static const char* const sec[] = { "OPEN", "WEP", "WPA", "WPA2" };
    static const char* const typ[] = { "", ":TKIP", ":CCMP", ":WEP" };

    AJ_InfoPrintf(("SSID %s [%02x:%02X:%02x:%02x:%02x:%02x] RSSI=%d security=%s%s\n", ssid, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], rssi, sec[secType], typ[cipherType]));
}

#pragma weak ConfigureWirelessNetwork
AJ_Status ConfigureWirelessNetwork(void)
{
    AJ_Status status = AJ_OK;

    status = AJ_WiFiScan(NULL, ScanResult, 32);
    if (status != AJ_OK) {
        AJ_ErrPrintf(("WiFi scan failed\n"));
    }

    status = ConfigureSoftAP();
    if (status != AJ_OK) {
        while (1) {
            uint32_t ip, mask, gw;
            status = ConfigureWifi();

            if (status != AJ_OK) {
                AJ_InfoPrintf(("AllJoyn_Start(): ConfigureWifi status=%s", AJ_StatusText(status)));
                continue;
            } else {
                status = AJ_AcquireIPAddress(&ip, &mask, &gw, NET_UP_TIMEOUT);
                AJ_Printf("Got IP %s\n", AddrStr(ip));
                if (status != AJ_OK) {
                    AJ_InfoPrintf(("AllJoyn_Start(): AJ_AcquireIPAddress status=%s", AJ_StatusText(status)));
                }

                break;
            }
        }
    }
    return status;
}


void AllJoyn_Start(unsigned long arg)
{
    AJ_Status status = AJ_OK;

    extern void user_pre_init(void);
    user_pre_init();

    AJ_AlwaysPrintf(("\n******************************************************"));
    AJ_AlwaysPrintf(("\n                AllJoyn Thin-Client"));
    AJ_AlwaysPrintf(("\n******************************************************\n"));

    AJ_PoolAllocInit();

    AJ_PrintFWVersion();

    AJ_InfoPrintf(("Alljoyn Version %s\n", AJ_GetVersion()));

    status = ConfigureWirelessNetwork();

    if (status == AJ_OK) {
        AJ_Main();
    }

    AJ_ErrPrintf(("Quitting\n"));
    while (TRUE) {}
}
