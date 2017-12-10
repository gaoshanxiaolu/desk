/*
  * Copyright (c) 2015 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#ifndef _WX_AIRKISS_H_
#define _WX_AIRKISS_H_

enum AIRKISS_STATE {
	AIRKISS_IDLE,
	AIRKISS_SEARCHING,
	AIRKISS_CONNECTING,
	AIRKISS_RESPONING,
};

typedef struct scan_results{
	A_INT8 channel;
	A_UCHAR rssi;
	unsigned char ssid[32];
	A_INT8 ssid_len;
	A_INT32 count;
}SCAN_RESULTS;

A_INT32 do_airkiss(A_INT32 argc, A_CHAR *argv[]);
A_INT32 conwifi(A_INT32 argc, A_CHAR *argv[]);
A_BOOL is_has_ssid_pwd(void);
void reset_wifi_config(void);
int get_dev_id(char *id);
A_INT32 set_dev_id(A_INT32 argc, A_CHAR *argv[]);
A_INT32 clear_net(A_INT32 argc, A_CHAR *argv[]);
int save_wifi_setting(void);
void save_config_dev_type(enum DEV_TYPE dev_type);
enum DEV_TYPE get_config_dev_type(void);
int ioe_wifi_config_get();

#endif
