/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#ifndef _SWAT_WMICONFIG_SECURITY_H_
#define _SWAT_WMICONFIG_SECURITY_H_

void swat_wmiconfig_wps_enable(A_UINT8 device_id, A_UINT32 enable);

void swat_wmiconfig_wps_start(A_UINT8 device_id, A_UINT32 connect, A_UINT32 mode, A_INT8* pPin);

void swat_wmiconfig_wps_start_without_scan(A_UINT8 device_id, A_UINT32 connect, A_UINT32 mode, A_INT8* pPin,A_INT8* pssid, A_UINT32 channel, A_UINT8* pmac);

void swat_wmiconfig_wep_key_set(A_UINT8 device_id, A_CHAR* pKey, A_UINT8 key);

void swat_wmiconfig_wep_key_index_set(A_UINT8 device_id, A_UINT8 key_index, A_UINT8 mode);

void swat_wmiconfig_wep_passowrd_set(A_UINT8 device_id, A_CHAR* pPassowrd);

void swat_wmiconfig_wpa_set(A_UINT8 device_id, A_CHAR* version, A_CHAR* ucipher, A_CHAR* mcipher);

void swat_wmiconfig_ap_wps_set(A_UINT8 device_id);

void swat_wmiconfig_wps_config_state_set(A_UINT8 device_id, A_INT32 mode);
#endif

