/******************************************************************************
* Copyright 2013, Qualcomm Connected Experiences, Inc.
*
******************************************************************************/
#define AJ_MODULE WIFI

#include "aj_target.h"
#include "aj_debug.h"
#include "aj_util.h"
#include "aj_status.h"
#include "aj_wifi_ctrl.h"
#include "aj_crypto.h"

#include <qcom/qcom_wlan.h>
#include <qcom/qcom_network.h>
#include <qcom/qcom_sec.h>
#include <qcom/qcom_system.h>
#include <qcom/socket_api.h>
#include <qcom/qcom_ap.h>
#include <qcom/qcom_scan.h>
#include <qcom/qcom_sta.h>
#include <qcom/qcom_timer.h>
#include <qcom/qcom_misc.h>
#include <qcom/qcom_internal.h>
#include <qcom/qcom_pwr.h>


#ifndef NDEBUG
AJ_EXPORT uint8_t dbgWIFI = 0;
#endif

#define softApIp      0xC0A8010A
#define softApMask    0xFFFFFF00
#define softApGateway 0x00000000
#define startIP 0xC0A801C3
#define endIP   0xC0A801C7

#define IP_LEASE    (60 * 60 * 1000)

A_UINT8 qcom_DeviceId = 0;

AJ_WiFiSecurityType AJ_WiFisecType;
AJ_WiFiCipherType AJ_WiFicipherType;

static AJ_WiFiConnectState connectState = AJ_WIFI_IDLE;

AJ_WiFiConnectState AJ_GetWifiConnectState(void)
{
    return connectState;
}

#define WIFI_CONNECT_TIMEOUT (10 * 1000)
#define WIFI_CONNECT_SLEEP (200)

#define NO_NETWORK_AVAIL (1)
#define DISCONNECT_CMD   (3)

static void WiFiCallback(A_UINT8 device_id, int val)
{
    AJ_InfoPrintf(("WiFiCallback dev %d, val %d\n", device_id, val));
    if (device_id != qcom_DeviceId) {
        return;
    }

    /* TODO: The TCP sockets used by aj_net.c do not recognize when a wifi
     * connection goes away. Any activity in this callback indicate that for
     * this STA, network socket connections are no longer valid, and should be
     * torn down if they haven't been already. This can be performed in
     * aj_net.c if qcom_select changes to detect socket validity (like linux).
     */
    AJ_Net_Lost();

    if (val == 0) {
        if (connectState == AJ_WIFI_DISCONNECTING || connectState == AJ_WIFI_CONNECT_OK) {
            connectState = AJ_WIFI_IDLE;
            AJ_InfoPrintf(("WiFi Disconnected\n"));
        } else if (connectState != AJ_WIFI_CONNECT_FAILED) {
            AJ_WarnPrintf(("WiFi Connect Failed old state %d\n", connectState));
            connectState = AJ_WIFI_CONNECT_FAILED;
        }
    } else if (val == 1) {
        /*
         * With WEP or no security a callback value == 1 means we are done. In the case of wEP this
         * means there is no way to tell if the association succeeded or failed.
         */
        if ((AJ_WiFisecType == AJ_WIFI_SECURITY_NONE) || (AJ_WiFisecType == AJ_WIFI_SECURITY_WEP)) {
            connectState = AJ_WIFI_CONNECT_OK;
            AJ_InfoPrintf(("\nConnected to AP\n"));
        } else {
            connectState = AJ_WIFI_CONNECTING;
            AJ_InfoPrintf(("Connecting to AP, waiting for auth\n"));
        }
    } else if (val == 10) {
        connectState = AJ_WIFI_AUTH_FAILED;
        AJ_WarnPrintf(("WiFi Auth Failed \n"));
    } else if (val == 16) {
        connectState = AJ_WIFI_CONNECT_OK;
        AJ_InfoPrintf(("Connected to AP\n"));
    }
}

static A_UINT32 DhcpCallback(A_UINT8 * mac, A_UINT32 addr)
{
    AJ_InfoPrintf(("DhcpCallback MAC:%02x:%02x:%02x:%02x:%02x:%02x IP:%d.%d.%d.%d\n",
                            mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
                            (addr >>24) & 0xff, (addr>>16) & 0xff, (addr >>8) & 0xff, addr & 0xff));

    connectState = AJ_WIFI_STATION_OK;
    return A_OK;
}

static void SoftAPCallback(A_UINT8 device_id, int val)
{
    AJ_InfoPrintf(("SoftAPCallback dev %d, val %d\n", device_id, val));
    if (device_id != qcom_DeviceId) {
        return;
    }

    /* TODO: This is a work-around because the Ruby v1.1 firmware does not
     * currently alert us here when a STA drops off of our SoftAP unexpectedly.
     * It does alert us if/when it show back up, so any time this callback
     * occurs, we know that any connections we believe to be valid at this
     * point are not, and will need to be set back up.
     */
    AJ_Net_Lost();

    if (val == 0) {
        if (connectState == AJ_WIFI_DISCONNECTING || connectState == AJ_WIFI_SOFT_AP_UP) {
            connectState = AJ_WIFI_IDLE;
			qcom_power_set_mode(qcom_DeviceId,REC_POWER);
            AJ_InfoPrintf(("Soft AP Down\n"));
        } else if (connectState == AJ_WIFI_STATION_OK) {
            connectState = AJ_WIFI_SOFT_AP_UP;
            AJ_InfoPrintf(("Soft AP Station Disconnected\n"));
        } else {
            connectState = AJ_WIFI_CONNECT_FAILED;
            AJ_InfoPrintf(("Soft AP Connect Failed\n"));
        }
    } else if (val == 1) {
        if (connectState == AJ_WIFI_SOFT_AP_INIT) {
            AJ_InfoPrintf(("Soft AP Initialized\n"));
            connectState = AJ_WIFI_SOFT_AP_UP;
        } else {
            AJ_InfoPrintf(("Soft AP Station Connected\n"));
        }
    }
}


char wepPhrase[33];

AJ_Status AJ_ConnectWiFi(const char* ssid, AJ_WiFiSecurityType secType, AJ_WiFiCipherType cipherType, const char* passphrase)
{
    AJ_Status status = AJ_ERR_CONNECT;
    uint8_t connected_status;
    int32_t timeout = WIFI_CONNECT_TIMEOUT;
    uint32_t passLen;

    /*
     * Clear the old connection state
     */
    status = AJ_DisconnectWiFi();
    if (status != AJ_OK) {
        return status;
    }

    /* Clear out stale IP addresses */
    AJ_SetIPAddress(0, 0, 0);

    /* set the discovery selection timeout to the default period for STA mode */
    AJ_SetSelectionTimeout(5000);

    if (passphrase == NULL) {
        passphrase = "";
    }

    passLen = strlen(passphrase);

    AJ_InfoPrintf(("Connecting to %s with password %s, passlength=%d\n", ssid, passphrase, passLen));
    // security and encryption will be inferred from the scan results
    qcom_set_connect_callback(qcom_DeviceId, WiFiCallback);
    qcom_op_set_mode(qcom_DeviceId, QCOM_WLAN_DEV_MODE_STATION); // station

    /* assign the global values so WiFiCallback can tell if security is needed */
    AJ_WiFisecType = secType;
    AJ_WiFicipherType = cipherType;

    switch (secType) {
    case AJ_WIFI_SECURITY_WEP: {
        memset(wepPhrase, 0, sizeof(wepPhrase));
        strncpy(wepPhrase, passphrase, min(passLen, sizeof(wepPhrase)-1));
        /* Workaround. From AP mode to STA mode, qcom_commit disconnect automatically which will clean the
           WEP info. So it should be disconnected first, after which we could set WEP info and commit.
           This change forces a call to disconnect regardless of our view of WiFi connection state.
        */
        qcom_disconnect(qcom_DeviceId);
        qcom_sec_set_auth_mode(qcom_DeviceId, WLAN_AUTH_NONE);
        qcom_sec_set_wepkey(qcom_DeviceId, 1, wepPhrase);
        qcom_sec_set_wepkey_index(qcom_DeviceId, 1);
        qcom_sec_set_wep_mode(qcom_DeviceId, 2);
        break;
    }
    case AJ_WIFI_SECURITY_WPA2:
        qcom_sec_set_auth_mode(qcom_DeviceId, WLAN_AUTH_WPA2_PSK);
        qcom_sec_set_passphrase(qcom_DeviceId, (char*) passphrase);
        break;

    case AJ_WIFI_SECURITY_WPA:
        qcom_sec_set_auth_mode(qcom_DeviceId, WLAN_AUTH_WPA_PSK);
        qcom_sec_set_passphrase(qcom_DeviceId, (char*) passphrase);
        break;

    default:
        qcom_sec_set_auth_mode(qcom_DeviceId, WLAN_AUTH_NONE);
        break;
    }


    switch (cipherType) {
    case AJ_WIFI_CIPHER_NONE:
        qcom_sec_set_encrypt_mode(qcom_DeviceId, WLAN_CRYPT_NONE);
        break;

    case AJ_WIFI_CIPHER_WEP:
        qcom_sec_set_encrypt_mode(qcom_DeviceId, WLAN_CRYPT_WEP_CRYPT);
        break;

    case AJ_WIFI_CIPHER_CCMP:
        qcom_sec_set_encrypt_mode(qcom_DeviceId, WLAN_CRYPT_AES_CRYPT);
        break;

    case AJ_WIFI_CIPHER_TKIP:
        qcom_sec_set_encrypt_mode(qcom_DeviceId, WLAN_CRYPT_TKIP_CRYPT);
        break;

    default:
        break;
    }


    qcom_set_ssid(qcom_DeviceId, (char*) ssid);

    qcom_commit(qcom_DeviceId);

    // TODO: no way of knowing why we failed!
    // no way of failing early; must rely on timeout
    do {
        qcom_get_state(qcom_DeviceId, &connected_status);
        AJ_InfoPrintf(("AJ_ConnectWiFi State: %u \n", connected_status));
        if (connected_status == QCOM_WLAN_LINK_STATE_CONNECTED_STATE) {
            break;
        }
        AJ_Sleep(WIFI_CONNECT_SLEEP);
        timeout -= WIFI_CONNECT_SLEEP;
    } while ((connected_status != QCOM_WLAN_LINK_STATE_CONNECTED_STATE) && (timeout > 0));

    AJ_InfoPrintf(("AJ_ConnectWiFi State: %u \n", connected_status));
    if (connected_status == QCOM_WLAN_LINK_STATE_CONNECTED_STATE) {
        connectState = AJ_WIFI_CONNECT_OK;
        qcom_dhcpc_enable(qcom_DeviceId, 1);  /* turn on dhcp client */
        return AJ_OK;
    } else {
        connectState = AJ_WIFI_CONNECT_FAILED;
        return AJ_ERR_CONNECT;
    }

    return status;
}

AJ_Status AJ_DisconnectWiFi(void)
{
    AJ_Status status = AJ_OK;
    A_STATUS astatus = A_OK;
    AJ_WiFiConnectState oldState = AJ_GetWifiConnectState();

    if (connectState != AJ_WIFI_DISCONNECTING) {
        connectState = AJ_WIFI_DISCONNECTING;
        astatus = qcom_disconnect(qcom_DeviceId);
        if (astatus != A_OK) {
            connectState = oldState;
            status = AJ_ERR_DRIVER;
        }
    }
    return status;
}

AJ_Status AJ_EnableSoftAP(const char* ssid, uint8_t hidden, const char* passphrase, uint32_t timeout)
{
    AJ_Status status = AJ_OK;
    uint8_t connected_status;
    WLAN_AUTH_MODE authMode = WLAN_AUTH_NONE;
    WLAN_CRYPT_TYPE encryptMode = WLAN_CRYPT_NONE;
    const char* pKey = "dummyKey";
    uint32_t time2 = 0;

    AJ_InfoPrintf(("AJ_EnableSoftAP\n"));

    /*
     * Clear the current connection
     */
    status = AJ_DisconnectWiFi();
    if (status != AJ_OK) {
        return status;
    }

    /* set the discovery selection timeout to a much shorter period for SoftAP mode */
    AJ_SetSelectionTimeout(200);

    /* assign the global values so WiFiCallback can tell if security is needed */
    AJ_WiFisecType = AJ_WIFI_SECURITY_NONE;
    AJ_WiFicipherType = AJ_WIFI_CIPHER_NONE;

    if (passphrase == NULL) {
        passphrase = "";
    }

    if (passphrase && strlen(passphrase) > 0) {
        authMode = WLAN_AUTH_WPA2_PSK;
        encryptMode = WLAN_CRYPT_AES_CRYPT;
        pKey = passphrase;
        AJ_WiFisecType = AJ_WIFI_SECURITY_WPA2;
        AJ_WiFicipherType = AJ_WIFI_CIPHER_CCMP;
    }

    AJ_InfoPrintf(("AJ_EnableSoftAP: ssid=%s, key=%s, auth=%d, encrypt=%d\n", ssid, pKey, authMode, encryptMode));
    AJ_SetIPAddress(softApIp, softApMask, softApGateway);
    connectState = AJ_WIFI_SOFT_AP_INIT;

    qcom_set_connect_callback(qcom_DeviceId, SoftAPCallback);

	qcom_power_set_mode(qcom_DeviceId,MAX_PERF_POWER);

    qcom_op_set_mode(qcom_DeviceId, QCOM_WLAN_DEV_MODE_AP);

    if (hidden) {
        qcom_ap_hidden_mode_enable(qcom_DeviceId, hidden);
    }

    qcom_sec_set_auth_mode(qcom_DeviceId, authMode);
    if (authMode != WLAN_AUTH_NONE) {
        qcom_sec_set_encrypt_mode(qcom_DeviceId, encryptMode);
        qcom_sec_set_passphrase(qcom_DeviceId, (char*) pKey);
    }

    /*
     * Set the IP range for DHCP
     */
    qcom_dhcps_set_pool(qcom_DeviceId, startIP, endIP, IP_LEASE);
    qcom_dhcps_register_cb(qcom_DeviceId, DhcpCallback);

    qcom_ap_start(qcom_DeviceId, (char *)ssid);

    do {
        AJ_Sleep(100);
        time2 += 100;
    } while (AJ_GetWifiConnectState() != AJ_WIFI_STATION_OK && (timeout == 0 || time2 < timeout));

    return (AJ_GetWifiConnectState() == AJ_WIFI_STATION_OK) ? AJ_OK : AJ_ERR_TIMEOUT;
}

#define AJ_WIFI_SCAN_TIMEOUT 8 // seconds
AJ_Status AJ_WiFiScan(void* context, AJ_WiFiScanResult callback, uint8_t maxAPs)
{
    A_STATUS result;
    AJ_Status status;
    qcom_start_scan_params_t scanParams;
    QCOM_BSS_SCAN_INFO* scanList = NULL;
    uint16_t count;
    uint8_t bssid[6];
    char ssid[33];
    uint8_t rssi;
    int i = 0;
    int j = 0;
    int* apIndex;

    result = qcom_set_ssid(qcom_DeviceId, "");
    if (result != A_OK) {
        AJ_ErrPrintf(("AJ_WiFiScan(): qcom_set_ssid failed: %d\n", result));
        return AJ_ERR_FAILURE;
    }

    scanParams.forceFgScan  = 1;
    scanParams.scanType     = WMI_LONG_SCAN;
    scanParams.numChannels  = 0;
    scanParams.forceScanIntervalInMs = 1;
    scanParams.homeDwellTimeInMs = 0;
    result = qcom_set_scan(qcom_DeviceId, &scanParams);
    if (result != A_OK) {
        AJ_ErrPrintf(("AJ_WiFiScan(): qcom_set_scan failed: %d\n", result));
        return AJ_ERR_FAILURE;
    }

    /* qcom_get_scan will block until a scan is finished */
    result = qcom_get_scan(qcom_DeviceId, &scanList, &count);
    if ((result != A_OK)) {
        AJ_ErrPrintf(("AJ_WiFiScan(): qcom_get_scan failed %d\n", result));
        return AJ_ERR_FAILURE;
    }
    if (count <= 0) {
        AJ_ErrPrintf(("AJ_WiFiScan(): no APs found %d\n", count));
        return AJ_ERR_FAILURE;
    }
    AJ_InfoPrintf(("Number of APs found = %d\n", count));

    apIndex = AJ_Malloc(sizeof(int) * count);
    if (apIndex == NULL) {
        AJ_ErrPrintf(("AJ_WiFiScan() AJ_Malloc failed\n"));
        return AJ_ERR_RESOURCES;
    }

    for (i = 0; i < count; ++i) {
        apIndex[i] = i;
    }

    // simple bubble sort
    for (i = 0; i < maxAPs; ++i) {
        for (j = i + 1; j < count; ++j) {
            if (scanList[apIndex[j]].rssi > scanList[apIndex[i]].rssi) {
                int tmp = apIndex[j];
                apIndex[j] = apIndex[i];
                apIndex[i] = tmp;
            }
        }
    }

    maxAPs = min(count, maxAPs);
    for (i = 0; i < maxAPs; ++i) {
        int ind = apIndex[i];
        AJ_WiFiSecurityType secType = AJ_WIFI_SECURITY_NONE;
        AJ_WiFiCipherType cipherType = AJ_WIFI_CIPHER_NONE;
        memcpy(ssid, (char*) scanList[ind].ssid, scanList[ind].ssid_len);
        ssid[scanList[ind].ssid_len] = '\0';
        memcpy(bssid, scanList[ind].bssid, sizeof(bssid));
        rssi = scanList[ind].rssi;
        if (scanList[ind].security_enabled) {
            if (scanList[ind].rsn_auth) {
                secType = AJ_WIFI_SECURITY_WPA2;
                if (scanList[ind].rsn_cipher & ATH_CIPHER_TYPE_CCMP) {
                    cipherType = AJ_WIFI_CIPHER_CCMP;
                } else if (scanList[ind].rsn_cipher & ATH_CIPHER_TYPE_TKIP) {
                    cipherType = AJ_WIFI_CIPHER_TKIP;
                }
            } else if (scanList[ind].wpa_auth) {
                secType = AJ_WIFI_SECURITY_WPA;
                if (scanList[ind].wpa_cipher & ATH_CIPHER_TYPE_CCMP) {
                    cipherType = AJ_WIFI_CIPHER_CCMP;
                } else if (scanList[ind].wpa_cipher & ATH_CIPHER_TYPE_TKIP) {
                    cipherType = AJ_WIFI_CIPHER_TKIP;
                }
            } else {
                secType = AJ_WIFI_SECURITY_WEP;
                cipherType = AJ_WIFI_CIPHER_WEP;
            }
        }
        callback(context, ssid, bssid, rssi, secType, cipherType);
    }

    AJ_Free(apIndex);
    return AJ_OK;
}

AJ_Status AJ_ResetWiFi(void)
{
    return AJ_OK;
}

AJ_Status AJ_GetIPAddress(uint32_t* ip, uint32_t* mask, uint32_t* gateway)
{
    A_INT32 ret;
    // set to zero first
    *ip = *mask = *gateway = 0;

    ret = qcom_ip_address_get(qcom_DeviceId, ip, mask, gateway);
    return (ret == A_OK ? AJ_OK : AJ_ERR_DHCP);
}


#define DHCP_WAIT       100

AJ_Status AJ_AcquireIPAddress(uint32_t* ip, uint32_t* mask, uint32_t* gateway, int32_t timeout)
{
    A_INT32 ret;
    AJ_Status status;
    AJ_WiFiConnectState wifi_state = AJ_GetWifiConnectState();

    switch (wifi_state) {
    case AJ_WIFI_CONNECT_OK:
        break;

    // no need to do anything in Soft-AP mode
    case AJ_WIFI_SOFT_AP_INIT:
    case AJ_WIFI_SOFT_AP_UP:
    case AJ_WIFI_STATION_OK:
        return AJ_OK;

    // shouldn't call this function unless already connected!
    case AJ_WIFI_IDLE:
    case AJ_WIFI_CONNECTING:
    case AJ_WIFI_CONNECT_FAILED:
    case AJ_WIFI_AUTH_FAILED:
    case AJ_WIFI_DISCONNECTING:
        return AJ_ERR_DHCP;
    }


    /*
     * zero out the assigned IP address
     */
    AJ_SetIPAddress(0, 0, 0);
    *ip = *mask = *gateway = 0;

    /*
     * This call kicks off DHCP but we need to poll until the values are populated
     */
    AJ_InfoPrintf(("Sending DHCP request\n"));
    qcom_ipconfig(qcom_DeviceId, IP_CONFIG_DHCP, ip, mask, gateway);
    while (0 == *ip) {
        if (timeout < 0) {
            AJ_ErrPrintf(("AJ_AcquireIPAddress(): DHCP Timeout\n"));
            return AJ_ERR_TIMEOUT;
        }


        AJ_Sleep(DHCP_WAIT);
        status = AJ_GetIPAddress(ip, mask, gateway);
        if (status != AJ_OK) {
            return status;
        }
        timeout -= DHCP_WAIT;
    }

    if (status == AJ_OK) {
        AJ_InfoPrintf(("*********** DHCP succeeded %s\n", AddrStr(*ip)));
    }

    return status;
}

AJ_Status AJ_PrintFWVersion()
{
    char date[20];
    char time[20];
    char ver[20];
    char cl[20];
    qcom_firmware_version_get(date, time, ver, cl);

    AJ_AlwaysPrintf(("Host version        :  Hostless\n"));
    AJ_AlwaysPrintf(("Target version      :  \n"));
    AJ_AlwaysPrintf(("Firmware version    :  %s\n", &ver));
    AJ_AlwaysPrintf(("Firmware changelist :  %s\n", &cl));
    AJ_AlwaysPrintf(("Built on: %s %s\n", &date, &time));
#ifdef USE_MAC_FOR_GUID
    {
        char buffer[(ATH_MAC_LEN*2)+1];
        uint8_t macAddr[ATH_MAC_LEN];
        if (A_OK == qcom_mac_get(qcom_DeviceId, macAddr)) {
            AJ_RawToHex(macAddr, sizeof(macAddr), buffer, sizeof(buffer), TRUE);
            AJ_AlwaysPrintf(("WiFi MAC address   :  0x%s\n", buffer));
        }
    }
#endif
}


AJ_Status AJ_SetIPAddress(uint32_t ip, uint32_t mask, uint32_t gateway)
{
    qcom_ipconfig(qcom_DeviceId, IP_CONFIG_SET, &ip, &mask, &gateway);
    return A_OK;
}

#ifdef USE_MAC_FOR_GUID
void AJ_CreateNewGUIDFromMAC(uint8_t* rand, uint32_t len)
{
    uint8_t macAddr[ATH_MAC_LEN];
    A_STATUS status = qcom_mac_get(qcom_DeviceId, macAddr);
    if (A_OK == status) {
        uint8_t i = len;
        uint8_t j = sizeof(macAddr);
        /* copy the MAC address into the GUID so that the least significant bytes match */
        while (i > 0) {
            rand[--i] = macAddr[--j];
            if (j == 0) {
                j = sizeof(macAddr);
            }
        }
    } else {
        AJ_WarnPrintf(("AJ_CreateNewGUIDFromMAC(): Unable to get MAC address (%d), falling back to AJ_RandBytes\n", status));
        AJ_RandBytes(rand, len);
    }
}
#endif
