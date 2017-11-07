/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
#include "qcom_wps.h"
#include "qcom_wps_priv.h"

void qcom_wps_event_process_cb(A_UINT8 ucDeviceID, QCOM_WPS_EVENT_TYPE uEventID, qcom_wps_event_t *pEvtBuffer, void *qcom_wps_event_handler_arg)
{
    switch (uEventID)
    {
        case QCOM_WPS_PROFILE_EVENT:
        {
            qcom_wps_profile_info_t *p_tmp;

            if (pEvtBuffer == NULL) {
                return;
            }

            p_tmp = &pEvtBuffer->event.profile_info;

            if (QCOM_WPS_STATUS_SUCCESS == p_tmp->error_code) {
                A_STATUS ret;

                ret = qcom_wps_set_credentials(ucDeviceID, &p_tmp->credentials);
                if (ret != A_OK) {
                    SWAT_PTF("WPS error: Failed to set wps credentials.\n");
                    return;
                }
                else {
                    SWAT_PTF("WPS ok.\n");
                }

                qcom_wps_connect(ucDeviceID);
            } else {
                switch (p_tmp->error_code) {
                    case QCOM_WPS_ERROR_INVALID_START_INFO:
                        SWAT_PTF("WPS error: Invalid start info\n");
                        break;
                    case QCOM_WPS_ERROR_MULTIPLE_PBC_SESSIONS:
                        SWAT_PTF("WPS error: Multiple PBC Sessions\n");
                        break;
                    case QCOM_WPS_ERROR_WALKTIMER_TIMEOUT:
                        SWAT_PTF("WPS error: Walktimer Timeout\n");
                        break;
                    case QCOM_WPS_ERROR_M2D_RCVD:
                        SWAT_PTF("WPS error: M2D RCVD\n");
                        break;
                    case QCOM_WPS_ERROR_PWD_AUTH_FAIL:
                        SWAT_PTF("WPS error: AUTH FAIL\n");
                        break;
                    case QCOM_WPS_ERROR_CANCELLED:
                        SWAT_PTF("WPS error: WPS CANCEL\n");
                        break;
                    case QCOM_WPS_ERROR_INVALID_PIN:
                        SWAT_PTF("WPS error: INVALID PIN\n");
                        break;
                    default:
                        SWAT_PTF("WPS error: UNKNOWN\n");
                       break;
                }
            }
            break;
        }
        default:
            SWAT_PTF("Unhandled WPS event id %d\n", uEventID);
            break;
    }

    return;
}


void
swat_wmiconfig_wps_enable(A_UINT8 device_id, A_UINT32 enable)
{
    /* 1 : enable */
    /* 0 : disable */
//    extern void qcom_wps_enable(A_UINT8 device_id, int enable);
    
    if (enable)
    {
        qcom_wps_register_event_handler(qcom_wps_event_process_cb, NULL);
    }
    else
    {
        qcom_wps_register_event_handler(NULL, NULL);
    }
    qcom_wps_enable(device_id, enable);
}

void
swat_wmiconfig_wps_start(A_UINT8 device_id, A_UINT32 connect, A_UINT32 mode, A_INT8 * pPin)
{
     if(gDeviceContextPtr[device_id]->hiddenFlag == 1)
     {
        SWAT_PTF("In AP hidden mode, WPS is disabled.\n");
        return;
     }
     
  //  extern void qcom_wps_start(A_UINT8 device_id, int connect, int mode, char *pin);
     qcom_wps_start(device_id, connect, mode, (char *)pPin);
}

void
swat_wmiconfig_wps_start_without_scan(A_UINT8 device_id, A_UINT32 connect, A_UINT32 mode, A_INT8* pPin,A_INT8* pssid, A_UINT32 channel, A_UINT8* pmac)
{
    A_UINT32 dev_mode,role;
    int hw_channel;

    if (channel < 27)
    {
        hw_channel = (channel - 1) * 5 + 2412;
    }
    else
    {
        hw_channel = 5000 + channel * 5;
    }

    qcom_op_get_mode(device_id, &dev_mode);
    role = (QCOM_WLAN_DEV_MODE_AP == dev_mode) ? (WPS_REGISTRAR_ROLE):(WPS_ENROLLEE_ROLE);
    qcom_wps_start_without_scan(device_id,role,mode,hw_channel,connect,120,(char*)pssid,(char*)pPin,(unsigned char*)pmac);
}


void
swat_wmiconfig_ap_wps_set(A_UINT8 device_id)
{
     if(gDeviceContextPtr[device_id]->hiddenFlag == 1)
     {
        SWAT_PTF("In AP hidden mode, WPS should not be enabled.\n");
        return;
     }
     swat_wmiconfig_wps_enable(device_id, 1);
     gDeviceContextPtr[currentDeviceId]->wpsFlag = 1;
}

void
swat_wmiconfig_wps_config_state_set(A_UINT8 device_id, A_INT32 mode)
{
    qcom_wps_config_state_set(device_id, mode);
    return;
}

void
swat_wmiconfig_wep_key_set(A_UINT8 device_id, A_CHAR * pKey, A_UINT8 key_index)
{
    qcom_sec_set_wepkey(device_id, key_index, pKey);
}

void
swat_wmiconfig_wep_key_index_set(A_UINT8 device_id, A_UINT8 key_index, A_UINT8 mode)
{
    /* confirm with guangde */
    qcom_sec_set_wepkey_index(device_id, key_index);

	qcom_sec_set_wep_mode(device_id, mode);

    if(gDeviceContextPtr[currentDeviceId]->wpsFlag == 1)
    {
        //Disable WSC if the APUT is manually configured for WEP.
        gDeviceContextPtr[currentDeviceId]->wpsFlag = 0;
        swat_wmiconfig_wps_enable(device_id, 0);
        SWAT_PTF("Disable WSC if the APUT is manually configured for WEP.\n");
    }
    
}

void swat_wmiconfig_wpa_set(A_UINT8 device_id, A_CHAR* version, A_CHAR* ucipher, A_CHAR* mcipher)
{
    int authmode, encyptmode;

    authmode = 0;
    encyptmode = 0;
    if (!A_STRCMP(version, "1"))
    {
      authmode = WLAN_AUTH_WPA_PSK;
    }
    else if (!A_STRCMP(version, "2"))
    {
      authmode = WLAN_AUTH_WPA2_PSK;
    }
    else
    {
      SWAT_PTF("invaid version, should be 1 or 2\n");

      return;
    }

    if (!A_STRCMP(ucipher, mcipher))
    {
      if (!A_STRCMP(ucipher, "TKIP"))
      {
      	encyptmode = WLAN_CRYPT_TKIP_CRYPT;
      }
      else if (!A_STRCMP(ucipher, "CCMP"))
      {
        encyptmode = WLAN_CRYPT_AES_CRYPT;
      }
      else
      {
        SWAT_PTF("invaid uchipher mcipher, should be TKIP or CCMP\n");

    	return;
      }
    }
    else
    {
      SWAT_PTF("invaid uchipher mcipher, should be same\n");

      return;
    }
    
    if(gDeviceContextPtr[currentDeviceId]->wpsFlag == 1 && authmode == WLAN_AUTH_WPA_PSK && encyptmode == WLAN_CRYPT_TKIP_CRYPT)
    {
        //Disable WSC if the APUT is manually configured for WPA/TKIP only.
        gDeviceContextPtr[currentDeviceId]->wpsFlag = 0;
        swat_wmiconfig_wps_enable(device_id, 0);
        SWAT_PTF("Disable WSC when the APUT is manually configured for WPA/TKIP only.\n");
    }
    qcom_sec_set_auth_mode(device_id, authmode);
    qcom_sec_set_encrypt_mode(device_id, encyptmode);

}


void
swat_wmiconfig_wep_passowrd_set(A_UINT8 device_id, A_CHAR * passwd)
{
    /* qcom_sec_set_passphrase(passwd, A_STRLEN(passwd)); */
    qcom_sec_set_passphrase(device_id, passwd);
}
