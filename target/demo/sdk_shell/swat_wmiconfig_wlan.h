/*
  * Copyright (c) 2015 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#ifndef _SWAT_WMICONFIG_WLAN_H_
#define _SWAT_WMICONFIG_WLAN_H_

void swat_wmiconfig_country_code_get(A_UINT8 device_id, A_CHAR *acountry_code);

void swat_wmiconfig_information(A_UINT8 device_id);

void swat_wmiconfig_channel_set(A_UINT8 device_id, A_UINT32 channel);

void swat_wmiconfig_dev_mode_set(A_UINT8 device_id, A_CHAR* devModeName);

void swat_wmiconfig_tx_power_set(A_UINT8 device_id, A_UINT32 txPower);

void swat_wmiconfig_lpl_set(A_UINT8 device_id, A_UINT32 enable);

void swat_wmiconfig_gtx_set(A_UINT8 device_id, A_UINT32 enable);

void swat_wmiconfig_rate_set(A_UINT8 device_id, A_UINT32 mcs, A_UINT32 rate);

void swat_wmiconfig_wifi_mode_set(A_UINT8 device_id, A_UINT8 wifiMode);

void swat_wmiconfig_11n_mode_enable(A_UINT8 device_id, A_UINT32 flg);

void swat_wmiconfig_connect_disc(A_UINT8 device_id);

void swat_wmiconfig_radio_onoff(A_UINT8 device_id, A_UINT32 state);

void swat_wmiconfig_all_bss_scan(A_UINT8 device_id);

void swat_wmiconfig_spec_bss_scan(A_UINT8 device_id, A_CHAR* pSsid);

void swat_wmiconfig_connect_ssid(A_UINT8 device_id, A_CHAR* pSsid);

void swat_wmiconfig_connect_ssid_2(A_UINT8 device_id, A_CHAR * ssid);

void swat_wmiconfig_connect_adhoc(A_UINT8 device_id, A_CHAR * ssid);

void swat_wmiconfig_listen_time_set(A_UINT8 device_id, A_UINT32 time);

void swat_wmiconfig_scan_param_set(A_UINT8 device_id, qcom_scan_params_t *pScanParams);

void swat_wmiconfig_bcon_int_set(A_UINT8 device_id, A_UINT16 time_interval);

void swat_wmiconfig_country_code_set(A_UINT8 device_id, char* country_code);

void swat_wmiconfig_country_ie_enable(A_UINT8 device_id, A_UINT8 enable);

void swat_wmiconfig_inact_set(A_UINT8 device_id, A_UINT16 inacttime);

void swat_wmiconfig_ap_hidden_set(A_UINT8 device_id);

void swat_wmiconfig_rssi_get(A_UINT8 device_id);

void swat_wmiconfig_devmode_get(A_UINT8 device_id, A_UINT32 *wifiMode);


void swat_wmiconfig_allow_aggr(A_UINT8 device_id, A_UINT16 tx_allow_aggr, A_UINT16 rx_allow_aggr);

void swat_wmiconfig_sta_info(A_UINT8 device_id);

void swat_wmiconfig_sta_country_code_set(A_UINT8 device_id, A_CHAR *country_code);

void swat_wmiconfig_roaming_enable(A_UINT8 device_id, A_BOOL enable);

void swat_wmiconfig_app_ie(A_UINT8 device_id,A_BOOL enable);


#endif





