/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
#include "qcom_i2c_master.h"
#include "qcom_i2c_slave_api.h"

I2CS_REG_CB_ARGS   g_reg_cb_args;
I2CS_FIFO_CB_ARGS   g_fifo_cb_args;
I2CS_CSR_CB_ARGS g_csr_cb_args;
#define TEST_FIFO_SIZE 32
#define I2CM_SWAP32(l) (((l) >> 24) | \
		   (((l) & 0x00ff0000) >> 8)  |\
		   (((l) & 0x0000ff00) << 8)  |\
		   ((l) << 24))
A_UINT8    p_fifo_read[TEST_FIFO_SIZE];
A_UINT8    p_fifo_write[TEST_FIFO_SIZE];
/*i2c slave test api */
void csr_intr_handler(I2CS_CSR_CB_ARGS *cb_args)
{
	A_UINT32 data;
	if(cb_args->intr & I2CS_INTR_CSR){
		/*cb_args->csr_data is assigned value written from master by driver*/
		A_PRINTF("W0x%x\n",cb_args->csr_data);
		/*loop back to read*/
		data = cb_args->csr_data;
		qcom_i2cs_cmd(I2CS_CMD_CSR_DATA_WRITE,&data);
	}
	
}
void reg_intr_handler(I2CS_REG_CB_ARGS *cb_args)
{
	A_UINT32 data;
	if(cb_args->intr & I2CS_INTR_REG_WRITE_FINISH){
		/*cb_args->reg_data is assigned value written from master by driver*/
		A_PRINTF("W0x%x\n",cb_args->reg_data);
		data = cb_args->reg_data;
		qcom_i2cs_cmd(I2CS_CMD_REG_DATA_WRITE,&data);
	}
	else if(cb_args->intr & I2CS_INTR_REG_READ_FINISH){
		A_PRINTF("R0x%x\n",cb_args->reg_data);
	}
}
 void fifo_intr_handler(I2CS_FIFO_CB_ARGS *cb_args)
{
    
	volatile int i=0;
	A_UINT32 data =TEST_FIFO_SIZE;
	static int cnt1=0;
	static int cnt2=0;
	static int cnt3=0;
	if(cb_args->intr & I2CS_INTR_FIFO_WRITE_FINISH){
	/*call this when master finish write operation, can copy data from write FIFO depends on user design*/
		A_PRINTF("%dW\n",++cnt1);
	}
	else if(cb_args->intr & I2CS_INTR_FIFO_READ_FINISH){
		A_PRINTF("%dR\n",++cnt2);
	}
	if(cb_args->intr & I2CS_INTR_FIFO_WRITE_FULL){
	/*loop back test, copy data from write FIFO to read FIFO when write FIFO is full*/

	        
		//if(30==++cnt3){
		for(i=0;i < TEST_FIFO_SIZE; i++){
	         	p_fifo_read[i]=p_fifo_write[i];
	    	}
		A_PRINTF("F\n");
		/*call this cmd funcs only if write data to read FIFO, or may cause assert*/
		qcom_i2cs_cmd(I2CS_CMD_READ_FIFO_WRITE_UPDATE,&data);
		/*call this cmd funcs only if read data from write FIFO, or may cause assert*/
		qcom_i2cs_cmd(I2CS_CMD_WRITE_FIFO_READ_UPDATE,&data);
		cnt3=0;
		//}
	}
}
/*i2c loop test api */
void swat_wmiconfig_i2cs_install(A_UINT8 config)
{
    A_UINT8 pin_pair = I2CS_PIN_PAIR_1;
    if(config)
        pin_pair = config - 1;
	if(qcom_i2cs_control(1,pin_pair)<0)
		A_PRINTF("ERROR! reinit or init fail\n");
	I2CS_REG_PARA    reg_params;
	I2CS_FIFO_PARA   f_params;
	I2CS_CSR_PARA	c_params;
	A_MEMZERO(&reg_params, sizeof(reg_params));
	A_MEMZERO(&f_params, sizeof(f_params));
	A_MEMZERO(&c_params, sizeof(c_params));
	
	c_params.intr |= I2CS_INTR_CSR ;
	c_params.cb = csr_intr_handler;
	c_params.cb_args = &g_csr_cb_args;
	
	/*Initialize reg service params*/
	/*IDs are i2c slave address in 0x7F format, none for default value*/
	//reg_params.id = 0x5A; 
	/*register interrupt only if needed practical operation in user app callback, without define callback is unacceptable*/
	reg_params.intr |= I2CS_INTR_REG_READ_FINISH ;
	reg_params.intr |= I2CS_INTR_REG_WRITE_FINISH ;
	reg_params.cb = reg_intr_handler;
	reg_params.cb_args = &g_reg_cb_args;

	/*Initialize fifo service params*/
	//f_params.id = 0x66;
	f_params.write_base = (A_UINT32)&p_fifo_write[0];
	f_params.read_base = (A_UINT32)&p_fifo_read[0];
	f_params.write_length = I2CS_FIFO_SIZE_32;//I2CS_FIFO_SIZE_32;
	f_params.read_length = I2CS_FIFO_SIZE_32;//I2CS_FIFO_SIZE_32;
	f_params.cb = fifo_intr_handler;
	f_params.cb_args = &g_fifo_cb_args;
	f_params.intr |= I2CS_INTR_FIFO_READ_FINISH; 
	f_params.intr |= I2CS_INTR_FIFO_WRITE_FINISH; 
	f_params.intr |= I2CS_INTR_FIFO_WRITE_FULL; 
	/*init which mode depends on user definination*/
	qcom_i2cs_reg_init(&reg_params);
	qcom_i2cs_fifo_init(&f_params);
	qcom_i2cs_csr_init(&c_params);
}

void swat_wmiconfig_i2cs_uninstall(A_UINT8 config)
{
    A_UINT8 pin_pair = I2CS_PIN_PAIR_1;
    if(config)
        pin_pair = config - 1;
	if(qcom_i2cs_control(0,pin_pair)<0){
		A_PRINTF("ERROR! redeinit or deinit fail\n");
	}
	
}



/*i2c master test api */
/*multi-slave test, dev_num is sequence number of eeprom on board*/  
int swat_wmiconfig_i2c_eeprom_read(A_UINT8 dev_num,int addr, A_UINT8 *data, A_UINT8 config)
{
	unsigned char dev_addr;
	unsigned char addr_len;
	int ret;
	A_UINT8 pin_pair = I2CM_PIN_PAIR_4;
    if(config)
        pin_pair = config - 1;
	A_PRINTF("\nEEPROM %d I2C byte read",dev_num);
	ret =qcom_i2cm_init(pin_pair,I2C_FREQ_200K,0);
	if(ret<0){
		A_PRINTF("\rFAIL\n");
		return 0;
	}
	switch (dev_num) {
	case 0:
		dev_addr = 0xA0>>1;
#ifdef FPGA
		addr_len =1;
#else
		addr_len =2;
#endif
		break;
	case 1:
		dev_addr = 0xA4>>1;
		addr_len =2;
		break;
	case 2:
		dev_addr = 0xA6>>1;
		addr_len =2;
		break;
    	default:
		ret =-1;
	       break;
    }
	
   	ret =qcom_i2cm_read(pin_pair, dev_addr,addr,addr_len, data, 1);
	if(ret<0)
		A_PRINTF("\rFAIL\n\n");
	else
		A_PRINTF("\r[0x%x]\n\n",*data);
	return 0;
}

int swat_wmiconfig_i2c_eeprom_write(A_UINT8 dev_num,int addr, A_UINT8 *data, A_UINT8 config)
{
	unsigned char dev_addr;
	unsigned char addr_len;
	int ret;
	A_UINT8 pin_pair = I2CM_PIN_PAIR_4;
    if(config)
        pin_pair = config - 1;
	A_PRINTF("\nEEPROM %d I2C byte write",dev_num);
	ret =qcom_i2cm_init(pin_pair,I2C_FREQ_200K,0);
	if(ret<0){
		A_PRINTF("\rFAIL\n");
		return 0;
	}
	switch (dev_num) {
	case 0:
		dev_addr = 0xA0>>1;
#ifdef FPGA
		addr_len =1;
#else
		addr_len =2;
#endif
		break;
	case 1:
		dev_addr = 0xA4>>1;
		addr_len =2;
		break;
	case 2:
		dev_addr = 0xA6>>1;
		addr_len =2;
		break;
    	default:
		ret =-1;
	       break;
    }
   	ret =qcom_i2cm_write(pin_pair, dev_addr,addr,addr_len, data, 1);
	if(ret<0)
		A_PRINTF("\rFAIL\n\n");
	else
		A_PRINTF("\r[0x%x]\n\n",*data);
	return 0;
}

int swat_wmiconfig_i2cm_loop_bread(A_UINT8 *data, A_UINT8 config)
{
	unsigned char dev_addr = 0x72;
	A_UINT8 pin_pair = I2CM_PIN_PAIR_2;
    if(config)
        pin_pair = config - 1;
	A_PRINTF("\nI2C loop byte read\n");
	qcom_i2cm_init(pin_pair,I2C_FREQ_200K,0);
   	return qcom_i2cm_read(pin_pair, dev_addr,0,0, data, 1);
}

int swat_wmiconfig_i2cm_loop_bwrite(A_UINT8 *data,A_UINT8 config)
{
	unsigned char dev_addr = 0x72;
	A_UINT8 pin_pair = I2CM_PIN_PAIR_2;
    if(config)
	   	pin_pair = config - 1;
	A_PRINTF("\nI2C loop byte write\n");
	qcom_i2cm_init(pin_pair,I2C_FREQ_200K,0);
		
   	return qcom_i2cm_write(pin_pair, dev_addr,0,0, data, 1);
}
int swat_wmiconfig_i2cm_loop_rwrite(A_UINT8 *data,A_UINT8 config)
{
	unsigned char dev_addr = 0x76;
	A_UINT8 pin_pair = I2CM_PIN_PAIR_2;
    if(config)
	    pin_pair = config - 1;
	A_PRINTF("\nI2C loop reg write\n");
	qcom_i2cm_init(pin_pair,I2C_FREQ_800K,0);
	int ret;
	A_UINT32 temp=(I2CM_SWAP32(*(A_UINT32 *)&data[0]));
	ret=qcom_i2cm_write(pin_pair, dev_addr,0,0, (A_UINT8 *)&temp, 4);
   	return ret;
}
int swat_wmiconfig_i2cm_loop_rread(A_UINT8 *data,A_UINT8 config)
{
	unsigned char dev_addr = 0x76;
	int ret;
	A_UINT8 pin_pair = I2CM_PIN_PAIR_2;
    if(config)
	    pin_pair = config - 1;
	A_PRINTF("\nI2C loop reg read\n");
	qcom_i2cm_init(pin_pair,I2C_FREQ_800K,0);
	ret =qcom_i2cm_read(pin_pair, dev_addr,0,0, data, 4);
   	return ret;
}
static int is_fifo_written=0;
#if 1
int swat_wmiconfig_i2cm_loop_fwrite(A_UINT32 *data,A_UINT8 config)
{
	unsigned char dev_addr = 0x78;
	int ret,i;
	A_UINT8 pin_pair = I2CM_PIN_PAIR_2;
    if(config)
        pin_pair = config - 1;
	A_PRINTF("\nI2C loop fifo write\n");
	qcom_i2cm_init(pin_pair,I2C_FREQ_200K,35000);
	for(i=0;i<8;i++){
		A_UINT32 temp=(I2CM_SWAP32(data[i]));
		ret=qcom_i2cm_write(pin_pair, dev_addr,0,0, (A_UINT8 *)&temp, 4);

		if(ret<0){
			A_PRINTF("\r FAIL in %d time\n",i);
			break;
		}
	}
	is_fifo_written=(0==ret)?1:0;
   	return ret;
}
#else /*for debug*/
int swat_wmiconfig_i2cm_loop_fread(A_UINT32 *data);
int swat_wmiconfig_i2cm_loop_fwrite(A_UINT32 *data)
{
	unsigned char dev_addr = 0x78;
	int ret,i;
	A_PRINTF("\nI2C loop fifo write\n");
		qcom_i2cm_init(2,7,35000);
	for(i=0;i<8;i++){
		//A_UINT32 temp=(I2CM_SWAP32(data[i]));
		if(7==i){
			A_UINT8 test_char[7]={0x5,0x5,0x15,0x15,0x55,0x55,0x50};
			ret=qcom_i2cm_write(2, dev_addr,0,0, test_char, 7);
			break;
		}
		A_UINT32 temp=0x0;
		ret=qcom_i2cm_write(2, dev_addr,0,0, (A_UINT8 *)&temp, 4);

		if(ret<0){
			A_PRINTF("\r FAIL in %d time\n",i);
			break;
		}
	}
	//A_UINT32 temp1= 0xffffffff;
	//ret=qcom_i2cm_write(2, dev_addr,0,0, (A_UINT8 *)&temp1, 4);
	is_fifo_written=(0==ret)?1:0;
	//ret=swat_wmiconfig_i2cm_loop_fread(data);
   	return ret;
}

#endif
int swat_wmiconfig_i2cm_loop_fread(A_UINT32 *data, A_UINT8 config)
{
	unsigned char dev_addr = 0x78;
	int ret,i;
	A_UINT8 pin_pair = I2CM_PIN_PAIR_2;
    if(config)
        pin_pair = config - 1;
	A_PRINTF("\nI2C loop fifo read\n");
	if(!is_fifo_written){
		A_PRINTF("\r FAIL\n write first!\n");
		return -1;
	}
	qcom_i2cm_init(pin_pair,I2C_FREQ_200K,35000);
	for(i=0;i<8;i++){
		ret=qcom_i2cm_read(pin_pair, dev_addr,0,0, (A_UINT8 *)&data[i], 4);
		if(ret<0){
			A_PRINTF("ERROR! \n");
			break;
		}
	}
	is_fifo_written=(0==ret)?0:1;
   	return ret;
}

