/*
  * Copyright (c) 2015 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#ifndef _SWAT_WMICONFIG_BTCOEX_H_
#define _SWAT_WMICONFIG_BTCOEX_H_

int swat_wmiconfig_btcoex_handle(int argc, char *argv[]);
A_STATUS ath_set_btcoex_fe_ant(int devId, int argc, char* argv[]);
A_STATUS ath_set_btcoex_colocated_bt_dev(int devId, int argc, char* argv[]);
A_STATUS ath_set_btcoex_sco_config(int devId, int argc, char* argv[]);
A_STATUS ath_set_btcoex_a2dp_config(int devId, int argc, char* argv[]);
A_STATUS ath_set_btcoex_hid_config(int devId, int argc, char* argv[]);
A_STATUS ath_set_btcoex_aclcoex_config(int devId, int argc, char* argv[]);
A_STATUS ath_set_btcoex_btinquiry_page_config(int devId, int argc, char* argv[]);
A_STATUS ath_set_btcoex_debug(int devId, int argc, char* argv[]);
A_STATUS ath_set_btcoex_bt_operating_status(int devId, int argc, char* argv[]);
A_STATUS ath_get_btcoex_config(int devId, int argc, char* argv[]);
A_STATUS ath_get_btcoex_stats(int devId, int argc, char* argv[]);
A_STATUS ath_set_btcoex_scheme(int devId, int argc, char* argv[]);

#endif /* _SWAT_WMICONFIG_BTCOEX_H_ */
