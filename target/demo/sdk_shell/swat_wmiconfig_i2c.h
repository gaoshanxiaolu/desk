/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#ifndef _SWAT_WMICONFIG_I2C_H_
#define _SWAT_WMICONFIG_I2C_H_

int
swat_wmiconfig_i2c_eeprom_read(A_UINT8 dev_num,int addr, A_UINT8 *data, A_UINT8 config);

int
swat_wmiconfig_i2c_eeprom_write(A_UINT8 dev_num,int addr, A_UINT8 *data, A_UINT8 config);

int
swat_wmiconfig_i2cm_loop_bread(A_UINT8 *data, A_UINT8 config);

int
swat_wmiconfig_i2cm_loop_bwrite(A_UINT8 *data, A_UINT8 config);
int
swat_wmiconfig_i2cm_loop_rwrite(A_UINT8 *data, A_UINT8 config);
int
swat_wmiconfig_i2cm_loop_rread(A_UINT8 *data, A_UINT8 config);
int
swat_wmiconfig_i2cm_loop_fwrite(A_UINT32 *data, A_UINT8 config);
int
swat_wmiconfig_i2cm_loop_fread(A_UINT32 *data, A_UINT8 config);
/*i2c loop test api */
void swat_wmiconfig_i2cs_install(A_UINT8 config);
void swat_wmiconfig_i2cs_uninstall(A_UINT8 config);


#endif

