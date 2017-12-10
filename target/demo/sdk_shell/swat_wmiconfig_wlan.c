/*
  * Copyright (c) 2015 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
#include "qcom_event.h"
#include "ble_uart.h"

/* WLAN API */
void
swat_wmiconfig_information(A_UINT8 device_id)
{
    A_UINT8 macAddr[6];
    A_CHAR ssid[33];
    A_UINT32 powermode;
    A_UINT8 chipState;
    A_UINT8 phymode;
    A_UINT16 channel = 0;
    QCOM_WLAN_DEV_MODE devMode = QCOM_WLAN_DEV_MODE_INVALID;
    QCOM_WLAN_DEV_MODE devMode_show = QCOM_WLAN_DEV_MODE_INVALID;
    A_CHAR *modeDesc[] = { "Station", "AP", "AD-HOC", "Unknown" };
    /* bpool_info_t bp_info; */
    A_UINT32 dev_mode;

    A_MEMSET(&macAddr, 0, sizeof (macAddr));
    qcom_mac_get(device_id, (A_UINT8 *) & macAddr);

    /* Mode */
    qcom_op_get_mode(device_id, &dev_mode);
    devMode = dev_mode;

    /* SSID */
    A_MEMSET(ssid, 0, 33);
    qcom_get_state(device_id, &chipState);
    /* if (((ap_conf.bApStart == 1) && (devMode == 0)) */
        /* || ((ap_conf.bApStart == 0) && (devMode == 1))) { */
        if (chipState == 4)     //K_WLAN_LINK_STATE_CONNECTED_STATE
        {
            qcom_get_ssid(device_id, ssid);
        }
        devMode_show = devMode;
    /* } else { */
        /* devMode_show = 1 - ap_conf.bApStart; */
    /* } */
    SWAT_PTF("SSID         =   %s\n", ssid);

    /* PHY MODE */
    qcom_get_phy_mode(device_id, &phymode);
	
    if (phymode == QCOM_11A_MODE) {
        SWAT_PTF("Phy Mode     =   a\n");
    } else if (phymode == QCOM_11B_MODE) {
        SWAT_PTF("Phy Mode     =   b\n");
    } else if (phymode == QCOM_11G_MODE) {
        SWAT_PTF("Phy Mode     =   g\n");
    } else if (phymode == QCOM_11N_MODE) {
        SWAT_PTF("Phy Mode     =   n\n");
    } else if (phymode == QCOM_HT40_MODE) {
        SWAT_PTF("Phy Mode     =   ht40\n");
    } else {
        SWAT_PTF("Phy Mode     =   mixed\n");
    }

    /* Power Mode */
    qcom_power_get_mode(device_id, &powermode); 
    if (powermode == 1)         //REC_POWER
    {
        SWAT_PTF("Power Mode   =   Power Save\n");
    } else if (powermode == 2)  //MAX_PERF_POWER
    {
        SWAT_PTF("Power Mode   =   Max Perf\n");
    } else {
        SWAT_PTF("Power Mode   =   Power Save\n"); //default is in sleep mode
    }
    SWAT_PTF("Mac Addr     =   %x:%x:%x:%x:%x:%x\n",
             macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    SWAT_PTF("Mode         =   %s\n", modeDesc[devMode_show]);

    /* Channel */
    qcom_get_channel(device_id, &channel);
    if (channel >= 2412) {
        channel = (((channel - 2412) / 5) + 1);
    }
    SWAT_PTF("Channel      =   %d\n", channel);
    
    {
      A_UINT32 size;
      A_UINT8 mac_cnt;
      A_UINT8 mac_reap;
      A_UINT8 htc_cnt; 
      A_UINT8 htc_reap;
      A_UINT8 fw_cnt;
      A_UINT8 fw_reap;
      A_UINT8 free_cnt;
      A_UINT32 htc_get;
      A_UINT32 htc_put;
      
      qcom_free_heap_size_get(&size);
      qcom_buffer_info_get(&mac_cnt, &mac_reap, &htc_cnt, &htc_reap, &fw_cnt, 
                           &fw_reap, &free_cnt, &htc_get, &htc_put);
      
      SWAT_PTF("Heapsize     =   %d\n", size);
      SWAT_PTF("Buffer : Mac (%d+%d), HTC (%d+%d), FW (%d+%d), Free %d, tx (alloc %d, free %d)\n",  \
             mac_cnt, mac_reap, htc_cnt, htc_reap, fw_cnt, fw_reap, free_cnt, htc_get, htc_put);
    }
}

void
swat_wmiconfig_channel_set(A_UINT8 device_id, A_UINT32 channel)
{
	int status;
    status =qcom_set_channel(device_id, channel);
	if(status!=A_OK)
		SWAT_PTF("Channel is invalid.\n");
}

void
swat_wmiconfig_tx_power_set(A_UINT8 device_id, A_UINT32 txPower)
{
    /* dbm */
    qcom_set_tx_power(device_id, txPower);
}

void
swat_wmiconfig_lpl_set(A_UINT8 device_id, A_UINT32 enable)
{
    qcom_set_lpl_enable(device_id, enable);
}

void
swat_wmiconfig_gtx_set(A_UINT8 device_id, A_UINT32 enable)
{
    qcom_set_gtx_enable(device_id, enable);
}

void
swat_wmiconfig_rate_set(A_UINT8 device_id, A_UINT32 mcs, A_UINT32 rate)
{
    qcom_set_rate(device_id, mcs, rate);
}

void
swat_wmiconfig_wifi_mode_set(A_UINT8 device_id, A_UINT8 wifiMode)
{
    qcom_set_phy_mode(device_id, wifiMode);
}

extern A_BOOL swat_sock_close_all_status();
void
swat_wmiconfig_connect_disc(A_UINT8 device_id)
{
    A_UINT8 i;
    A_UINT8 chipState = 0;
    A_BOOL need_benchquit = TRUE;

    /* if no other device is connected, benchquit will run */
    for (i = 0; i < gNumOfWlanDevices; i++) {
        if (i != device_id) {
            qcom_get_state(i, &chipState);
            if (chipState == QCOM_WLAN_LINK_STATE_CONNECTED_STATE) {
                need_benchquit = FALSE;
                break;
            }
        }
    }

    if (need_benchquit == TRUE) {
        SWAT_PTF("benchquit\n");
        while (! swat_sock_close_all_status()) {
          SWAT_PTF("\nsocket not closed before disconnecting, waiting\n");
          swat_bench_quit_config();
          qcom_thread_msleep(1000);
        }
    }

    qcom_disconnect(device_id);

    if (qcom_sta_wlan_reconnect_start_flag[device_id] == 1) {
        qcom_sta_reconnect_stop(device_id);
    }

    A_MEMSET(gDeviceContextPtr[device_id]->ssid, 0, sizeof(gDeviceContextPtr[device_id]->ssid));
    A_MEMSET(gDeviceContextPtr[device_id]->pass, 0, sizeof(gDeviceContextPtr[device_id]->pass));
    qcom_ap_hidden_mode_enable(device_id, 0);
    qcom_sec_set_auth_mode(device_id, WLAN_AUTH_NONE);
    qcom_sec_set_encrypt_mode(device_id, WLAN_CRYPT_NONE);
}

/* Fix CR733548, trigger user's CB function with the following code 
*  based on the connection state: 
*  integer value  0 - not connected
*  integer value  1 - connected (for open auth and wep auth access points)
*  integer value 10 - RSNA_AUTH_FAILURE (for WPA/WPA2 auth based access points)
*  integer value 16 - RSNA_AUTH_SUCCESS (for WPA/WPA2 auth based access points)
*/
/* the function is duplicated with _my_wifi_conn_callback, remvoe this can save 
 * memory
 */

#if 0
void wifi_conn_callback(A_UINT8 device_id, int value)
{
  QCOM_BSS_SCAN_INFO *bss_info = NULL;
  A_UINT8 bssid[6];

  if (value == 10)
  {
    A_PRINTF("[CB]disconnected - INVALID_PROFILE \n");
    value = 0;
  }

  if (value)
  {
    A_PRINTF("Connected to %s, ", gDeviceContextPtr[device_id]->ssid);
    bss_info = qcom_get_bss_entry_by_ssid(device_id, (char *)gDeviceContextPtr[device_id]->ssid);
    if (NULL == bss_info)
    {
      A_PRINTF("\nshell> ");
      return;
    }
    A_MEMCPY(bssid, bss_info->bssid, 6);
    A_PRINTF("bssid = %02x-%02x-%02x-%02x-%02x-%02x, ", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    A_PRINTF("channel = %d, rssi = %d\n", bss_info->channel, bss_info->rssi);
    //qcom_sta_reconnect_stop();
  }
  else
  {
    A_PRINTF("Disconnected from %s\n", gDeviceContextPtr[device_id]->ssid);

    bss_info = qcom_get_bss_entry_by_ssid(device_id, (char *)gDeviceContextPtr[device_id]->ssid);
    if (NULL == bss_info)
    {
      A_PRINTF("\nshell> ");
      return;
    }
    A_PRINTF("channel = %d, rssi = %d\n", bss_info->channel, bss_info->rssi);
  }
  A_PRINTF("\nshell> ");
}
#endif
#if 1
extern A_UINT8 conn_route_flag;
void wifi_conn_callback(A_UINT8 device_id, int value)
{
  QCOM_BSS_SCAN_INFO *bss_info = NULL;
  A_UINT8 bssid[6];

  //conn_route_flag = 0;
  if (value == 10)
  {
    A_PRINTF("[CB]disconnected - INVALID_PROFILE \n");
    value = 0;
  }

  if (value)
  {
    A_PRINTF("Connected to %s, ", gDeviceContextPtr[device_id]->ssid);
    bss_info = qcom_get_bss_entry_by_ssid(device_id, (char *)gDeviceContextPtr[device_id]->ssid);
    if (NULL == bss_info)
    {
	  A_PRINTF("NULL == bss_info\r\n ");
      A_PRINTF("\nshell> ");
      return;
    }
    A_MEMCPY(bssid, bss_info->bssid, 6);
    A_PRINTF("bssid = %02x-%02x-%02x-%02x-%02x-%02x, ", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    A_PRINTF("channel = %d, rssi = %d\n", bss_info->channel, bss_info->rssi);
	if(value == 16)
	{
		conn_route_flag = 1;
		A_PRINTF("connect route success !!!,value = %d\r\n",value);
		set_led_state(LED_ON);
	//qcom_sta_reconnect_stop();
	}
  }
  else
  {
    A_PRINTF("Disconnected from %s\n", gDeviceContextPtr[device_id]->ssid);

    bss_info = qcom_get_bss_entry_by_ssid(device_id, (char *)gDeviceContextPtr[device_id]->ssid);
    if (NULL == bss_info)
    {
      A_PRINTF("\nshell> ");
      return;
    }
    A_PRINTF("channel = %d, rssi = %d\n", bss_info->channel, bss_info->rssi);
	//set_led_state(LED_SLOW_BLINK);
  }
  A_PRINTF("\nshell> ");
}
#endif

void
swat_wmiconfig_connect_ssid(A_UINT8 device_id, A_CHAR * ssid)
{
    A_UINT32 devMode;
	
    /*Set callback*/
#if 0
		qcom_set_connect_callback(device_id, QCOM_EVENT_API_FN(_my_wifi_conn_callback));
#else
		qcom_set_connect_callback(device_id, wifi_conn_callback);
#endif

    qcom_op_get_mode(device_id, &devMode);
    if (QCOM_WLAN_DEV_MODE_AP != devMode) {
        qcom_op_set_mode(device_id, QCOM_WLAN_DEV_MODE_STATION); // station
    }
    
    qcom_set_ssid(device_id, ssid);

    qcom_commit(device_id);
	
	SWAT_PTF("Setting SSID to %s \n",(char *)ssid);
}

void
swat_wmiconfig_connect_ssid_2(A_UINT8 device_id, A_CHAR * ssid)
{
    A_UINT32 devMode;

    /*Set callback*/
    qcom_set_connect_callback(device_id, QCOM_EVENT_API_FN(_my_wifi_conn_callback));

    qcom_op_get_mode(device_id, &devMode);
    if (QCOM_WLAN_DEV_MODE_AP != devMode) {
        qcom_op_set_mode(device_id, QCOM_WLAN_DEV_MODE_STATION); // station
    }
    
    qcom_set_ssid(device_id, ssid);

    qcom_commit(device_id);
}

void
swat_wmiconfig_connect_adhoc(A_UINT8 device_id, A_CHAR * ssid)
{
    /*Set callback*/
    qcom_set_connect_callback(device_id, QCOM_EVENT_API_FN(_my_wifi_conn_callback));

    qcom_op_set_mode(device_id, QCOM_WLAN_DEV_MODE_ADHOC); // adhoc
    qcom_set_ssid(device_id, ssid);

    qcom_commit(device_id);


    SWAT_PTF("Try to Connected\r\n");
}

void
swat_wmiconfig_listen_time_set(A_UINT8 device_id, A_UINT32 listentime)
{
    qcom_sta_set_listen_time(device_id, listentime);
}

void 
swat_wmiconfig_scan_param_set(A_UINT8 device_id, qcom_scan_params_t  *pScanParams)
{
    qcom_scan_params_set(device_id, pScanParams);
}


void
swat_wmiconfig_bcon_int_set(A_UINT8 device_id, A_UINT16 time_interval)
{
    qcom_ap_set_beacon_interval(device_id, time_interval);
}

void
swat_wmiconfig_country_code_set(A_UINT8 device_id, A_CHAR *country_code)
{
    qcom_set_ap_country(device_id, country_code);
}

void
swat_wmiconfig_country_ie_enable(A_UINT8 device_id, A_UINT8 enable)
{
    /*mgmt_frm_type =QCOM_FRAME_BEACON or QCOM_FRAME_PROBE_RESP
    will enable/disable the specific information element in beacon and probe response */
    qcom_specific_ie_enable_cmd(device_id,QCOM_FRAME_BEACON,IEEE80211_ELEMID_COUNTRY, enable);
}


void
swat_wmiconfig_inact_set(A_UINT8 device_id, A_UINT16 inacttime)
{
    qcom_ap_set_inact_time(device_id, inacttime);
}

void
swat_wmiconfig_ap_hidden_set(A_UINT8 device_id)
{
    //when ssid is hidden mode, wps should be disable
    if(gDeviceContextPtr[device_id]->wpsFlag)
    {
        gDeviceContextPtr[device_id]->wpsFlag = 0;
        swat_wmiconfig_wps_enable(device_id, 0);
        SWAT_PTF("In AP hidden mode, WPS would be disabled.\n");
    }
    
    qcom_ap_hidden_mode_enable(device_id, 1);
}



void
swat_wmiconfig_devmode_get(A_UINT8 device_id, A_UINT32 * wifiMode)
{
    A_UINT32 mode;

    /* 0: sta */
    /* 1:ap */
    qcom_op_get_mode(device_id, &mode);

    if (mode == 1) {
        *wifiMode = 0;          //ap
    } else if (mode == 0) {
        *wifiMode = 1;          //station
    } else {
        *wifiMode = 2;          //unknown
    }
}

void
swat_wmiconfig_rssi_get(A_UINT8 device_id)
{
    A_UINT8 rssi;
    A_UINT8 chipState;
    QCOM_WLAN_DEV_MODE devMode;

    qcom_op_get_mode(device_id, &devMode);
    if(devMode == QCOM_WLAN_DEV_MODE_STATION){
        qcom_get_state(device_id, &chipState);
        if(chipState == 4){
            qcom_sta_get_rssi(device_id, &rssi);
            SWAT_PTF("indicator = %d dB\n", rssi);
        }else{
            SWAT_PTF("Not associate\n");
        }
    }else{
        SWAT_PTF("RSSI not in AP mode\n");
    }
}

void
swat_wmiconfig_allow_aggr(A_UINT8 device_id, A_UINT16 tx_allow_aggr, A_UINT16 rx_allow_aggr)
{
    qcom_allow_aggr_set_tid(device_id, tx_allow_aggr, rx_allow_aggr);
}
    
    
void swat_wmiconfig_dev_mode_set(A_UINT8 device_id, A_CHAR * devModeName)
{
  QCOM_WLAN_DEV_MODE devMode;
  
  if (!swat_strcmp(devModeName, "ap"))
  {
    devMode = QCOM_WLAN_DEV_MODE_AP;
  }
  else if (!swat_strcmp(devModeName, "station"))
  {
    devMode = QCOM_WLAN_DEV_MODE_STATION;
  }
  else
  {
    SWAT_PTF("unkown mode\n");
    return;
  }
  
  qcom_op_set_mode(device_id, devMode);
  
}

void swat_wmiconfig_sta_info(A_UINT8 device_id)
{
  A_UINT8 num=0, i=0;
  A_UINT8 mac[60];
  A_MEMSET(mac, 0, 60);
  qcom_ap_get_sta_info(device_id, &num, mac);
  if (num == 0)
  {
    SWAT_PTF("there are no stations connected\n");
  }
  else
  {
  	SWAT_PTF("there are %d stations connected :\n", num);
  }
  
  for (i=0; i<num; i++)
  {
    SWAT_PTF("Sta %d MAC address: %x:%x:%x:%x:%x:%x\n", i+1, 
		mac[6*i+0], mac[6*i+1],mac[6*i+2],mac[6*i+3],mac[6*i+4],mac[6*i+5]);
  }
}

void
swat_wmiconfig_sta_country_code_set(A_UINT8 device_id, A_CHAR *country_code)
{
    qcom_set_sta_country_code(device_id, country_code);
}

void
swat_wmiconfig_roaming_enable(A_UINT8 device_id, A_BOOL enable)
{
	qcom_roam_mode_enable(device_id, enable);
}

void 
swat_wmiconfig_app_ie(A_UINT8 device_id,A_BOOL enable)
{
    A_INT8 i;

    #define IEEE80211_APPLE_IE_LEN     60
    /*
     * Frame Types
     */
    typedef enum {
        WMI_FRAME_BEACON        =   0,
        WMI_FRAME_PROBE_REQ,
        WMI_FRAME_PROBE_RESP,
        WMI_FRAME_ASSOC_REQ
    } WMI_MGMT_FRAME_TYPE;
 
    struct ieee80211_beacon_apple_ie {
        A_UINT8 ie_id;         /* IEEE80211_ELEMID_SPECIFIC 221*/
        A_UINT8 len;        
        A_UINT8 oui[3];          /*00-a0-40 AppleCom */
        A_UINT8 oui_type;        /* 0 */
        A_UINT8 data[IEEE80211_APPLE_IE_LEN];
    } __attribute__ ((packed)) ie_info={
            221,
            64,
            {0x0,0xa0,0x40},
            0   
        };
    for(i=0; i < IEEE80211_APPLE_IE_LEN; i++)
    {
        ie_info.data[i]= 0xAB;
    }

    if(enable)
    {
        qcom_set_vendor_specific_ie_cmd(device_id,WMI_FRAME_BEACON ,(A_UINT8*)&ie_info, sizeof(ie_info));
        qcom_set_vendor_specific_ie_cmd(device_id,WMI_FRAME_PROBE_REQ ,(A_UINT8*)&ie_info, sizeof(ie_info));
        qcom_set_vendor_specific_ie_cmd(device_id,WMI_FRAME_PROBE_RESP ,(A_UINT8*)&ie_info, sizeof(ie_info));
        qcom_set_vendor_specific_ie_cmd(device_id,WMI_FRAME_ASSOC_REQ ,(A_UINT8*)&ie_info, sizeof(ie_info));
    }
    else
    {      
        qcom_set_vendor_specific_ie_cmd(device_id,WMI_FRAME_BEACON ,NULL, 0);
        qcom_set_vendor_specific_ie_cmd(device_id,WMI_FRAME_PROBE_REQ ,NULL, 0);
        qcom_set_vendor_specific_ie_cmd(device_id,WMI_FRAME_PROBE_RESP ,NULL, 0);
        qcom_set_vendor_specific_ie_cmd(device_id,WMI_FRAME_ASSOC_REQ ,NULL, 0);
    }
        
}

#ifdef DIVERSITY_ENABLE 
void swat_ant_div_set(int div, int adjust)
{
	qcom_ant_div_set(div, adjust);	
}

void swat_ant_div_get(void)
{
	qcom_ant_div_get();
}
#endif




