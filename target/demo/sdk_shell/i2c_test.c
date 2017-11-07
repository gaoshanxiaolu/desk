/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */
#if defined(AR6002_REV74)
#include "qcom_common.h"
#include <qcom_i2c.h>

extern int atoul(char *buf);


#define TMP106_OK       1
#define TMP106_ERR      0




int tmp106_reg_write(unsigned char addr, unsigned short val)
{
  int ret;
  
  ret = qcom_i2c_write(0x49, addr, 1, (unsigned char*)&val, 2);
  
  if (ret != SI_OK)
  {
    QCOM_DEBUG_PRINTF("Call tmp106_reg_write failed!\n");
    return TMP106_ERR;
  }
  
  return TMP106_OK;
}

int tmp106_reg_read(unsigned char addr, unsigned short * val)
{
  int ret;
  unsigned char data[2];

  data[0] = data[1] = 0;
  
  ret = qcom_i2c_read(0x49, addr, 1, data, 2);
  if (ret != SI_OK)
  {
    QCOM_DEBUG_PRINTF("Call tmp106_reg_read failed!\n");
    return TMP106_ERR;
  }

  *(unsigned short * )val = (data[0]<< 8) | data[1];
  
  return TMP106_OK;
}


#define AT24_ADDR 0x50
int at24_read(A_INT32 addr, A_UINT8 *val)
{
   qcom_i2c_read(AT24_ADDR, addr, 2, (A_UCHAR *)val, 1);
   return TMP106_OK;
}

int at24_write(A_INT32 addr, A_UINT8 val)
{
   qcom_i2c_write(AT24_ADDR, addr, 2, (A_UCHAR *)&val, 1);
   return TMP106_OK;
}

//#ifdef SWAT_I2S_TEST
#define UDA1380_ADDR 0x18
A_INT32 uda1380_read(A_UINT8 addr, A_UINT16 * val)
{
	A_INT32 ret;
	A_UINT16 temp;

	ret = qcom_i2c_read(UDA1380_ADDR, addr, 1, (A_UCHAR *)&temp, 2);

	*(A_UINT16 *) val =(( temp>>8 )& 0xff) | ((temp & 0xff) <<8);

	return ret;

}

A_INT32 uda1380_write(A_UINT8 addr, A_UINT16 val)
{
	A_INT32 ret;
	ret = qcom_i2c_write(UDA1380_ADDR, addr, 1, (A_UCHAR *)&val, 2);

	return ret;
}
//#endif

 int cmd_i2c_test(int argc, char *argv[])
 {
       int i=0, addr,ret;
       static char i2c_init_flag=0;
 
       if(argc ==1)
       {
               qcom_printf("i2c {read <addr> | write <addr> <val>}\n"); 
               return -1;
       }
       if (!A_STRCMP(argv[1], "read")){
               i=3;
       }
       else if (!A_STRCMP(argv[1], "write")){
               i=4;
       } 
 
       if((!i)  || (argc != i))
       {
               qcom_printf("i2c {read <addr> | write <addr> <val>}\n"); 
               return -1;
       }
 
       if(!i2c_init_flag)
       {
#if 0//def AR6002_REV74
       	qcom_i2c_init(8,9,9);
#else
       	qcom_i2c_init(12,6,9);
#endif
       }


       unsigned short val;
	   
       addr = atoul(argv[2]);
       if(i == 3){

#if 0//def AR6002_REV74
               ret = at24_read(addr, (A_UINT8 *)&val);
               val &= 0xff;
#else
               ret = tmp106_reg_read(addr, &val);
#endif
               if(ret)
                       qcom_printf("read : addr 0x%08x, val 0x%08x\n", addr, val);
               else
                       qcom_printf("read : addr 0x%08x error\n", addr);
       }
       else 
       {
               val = atoul(argv[3]);

#if 0//def AR6002_REV74
               val &= 0xff;
               ret = at24_write(addr, (A_UINT8 )val);
#else
               ret = tmp106_reg_write(addr, val);
#endif
               if(ret)
                       qcom_printf("write: addr 0x%08x, val 0x%08x\n", addr, val);
               else
                       qcom_printf("write: addr 0x%08x, val 0x%08x failed!\n", addr, val);
 
 
       }

 #if 0
       addr = atoul(argv[2]);
       if(i == 3){
               ret = uda1380_read(addr, &val);
               if(ret)
                       printf("read : addr 0x%08x, val 0x%08x\n", addr, val);
               else
                       printf("read : addr 0x%08x error\n", addr);
       }
       else 
       {
               val = atoul(argv[3]);
               ret = uda1380_write(addr, val);
               if(ret)
                       printf("write: addr 0x%08x, val 0x%08x\n", addr, val);
               else
                       printf("write: addr 0x%08x, val 0x%08x failed!\n", addr, val);
 
 
               ret = uda1380_read(addr, &val);
               if(ret)
                       printf("write: check val%08x\n", val);
               else
                       printf("read : addr 0x%08x error\n", addr);
       }

#endif
       return 0;
 }

#endif/*if defined(AR6002_REV74)*/
 
