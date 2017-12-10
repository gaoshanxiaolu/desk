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
#elif defined(AR6002_REV76)
#include "qcom_common.h"
//#include <qcom_i2c.h>
#include <qcom_i2c_master.h>
#include "myiic.h"

extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();

#define I2C_MASTER_MODE

#define I2C_SLAVE_ADDR 1
#define DHT12_SLAVE_READ_ADDR 0XB9
#define DHT12_SLAVE_WRITE_ADDR 0XB8

#define ADPS9300GRD_SLAVE_ADDR 0X52
#define ADPS9300FLOAT_SLAVE_ADDR 0X72
#define ADPS9300VCC_SLAVE_ADDR 0X92

/*
A_INT8 tSensor_byte_read(A_UINT8 addr)
{
         A_INT8 ret;
         A_UCHAR readbuf[10],writebuf[10];       
 
         memset(readbuf,0x00,sizeof(readbuf));
         memset(writebuf,0x00,sizeof(writebuf));
         
         writebuf[0] = I2C_SLAVE_ADDR;
         writebuf[1] = addr;
         writebuf[2] = I2C_SLAVE_ADDR | 0x01;
         ret = qcom_i2c_ctrl(writebuf,3,readbuf,1);
         if(ret != SI_OK)
         {
                   printf("i2c read address %x error\n",addr);
                   return SI_ERR;
         }
         return readbuf[0];
}*/
int failure_cnt = 0;
int need_read = 1;
int dht12_byte_read(A_UCHAR *data)
{
	#ifdef I2C_MASTER_MODE
		 //static A_UCHAR noused = 0,tmp_addr;
		 int ret;
		 //noused = 0;
		 //ret = qcom_i2cm_write(I2CM_PIN_PAIR_2,0x5c,0x00,1,&noused,1);
		 //if(ret<0)
		 //{
		 //	A_PRINTF("qcom_i2cm_write error ret=%d\n\n",ret);
		 //}
		 /*if(noused == 0)
		 {
		 	tmp_addr = 0xb9;
			noused = 1;
		 }
		 else if(noused == 1)
		 {
		 	tmp_addr = 0x5c;
			noused = 2;
		 }
		 else 
		 {
		 	tmp_addr = 0xb8;
			noused = 0;
		 }*/
		if(need_read)
		{
			 ret = qcom_i2cm_read(I2CM_PIN_PAIR_2,0x5c,0x00,1,data,5);
			 if(ret<0)
			 {
				A_PRINTF("qcom_i2cm_read error ret=%d\n\n",ret);
				failure_cnt++;
				if(failure_cnt > 10)
				{
					need_read = 0;
					A_PRINTF("qcom_i2cm_read error continous 10 error,no read again\n");
				}
				return ret;
			 }
			 else
			 {
			 	failure_cnt = 0;
			 }
		}
		else
		{
			return -1;
		}
		 //A_PRINTF("qcom_i2cm_read addr =%d\n\n",tmp_addr);

		 return 0;
	#else
         A_INT8 ret;
         A_UCHAR readbuf[10],writebuf[10];       
 
         memset(readbuf,0x00,sizeof(readbuf));
         memset(writebuf,0x00,sizeof(writebuf));
         
         writebuf[0] = DHT12_SLAVE_ADDR;
         writebuf[1] = addr;
         writebuf[2] = I2C_SLAVE_ADDR | 0x01;
         ret = qcom_i2c_ctrl(writebuf,3,readbuf,1);
         if(ret != SI_OK)
         {
                   printf("dht12 i2c read address %x error\n",addr);
                   return SI_ERR;
         }

		 return readbuf[0];
	#endif
}

/*
A_INT8 tSensor_byte_write(A_UINT8 addr,A_UINT8 value)
{
         A_INT8 ret;
         A_UCHAR readbuf[10],writebuf[10];
 
         memset(readbuf,0x00,sizeof(readbuf));
         memset(writebuf,0x00,sizeof(writebuf));
         
         writebuf[0] = I2C_SLAVE_ADDR;
         writebuf[1] = addr;
         writebuf[2] = value;
         ret = qcom_i2c_ctrl(writebuf,3,NULL,0);
         if(ret != SI_OK)
         {
                   printf("i2c write address %x error\n",addr);
                   return SI_ERR;
         }
         return SI_OK;
 
}
*/

A_INT8 dht12_byte_write(A_UINT8 addr,A_UINT8 value)
{
	#ifdef I2C_MASTER_MODE
		//qcom_i2cm_write(I2CM_PIN_PAIR_2,0xb8,addr,1,&data,1);
		return 0;
	#else
         A_INT8 ret;
         A_UCHAR readbuf[10],writebuf[10];
 
         memset(readbuf,0x00,sizeof(readbuf));
         memset(writebuf,0x00,sizeof(writebuf));
         
         writebuf[0] = DHT12_SLAVE_ADDR;
         writebuf[1] = addr;
         writebuf[2] = value;
         ret = qcom_i2c_ctrl(writebuf,3,NULL,0);
         if(ret != SI_OK)
         {
                   printf("dht12 i2c write address %x error\n",addr);
                   return SI_ERR;
         }
         return SI_OK;
 	#endif
}

/*
A_INT32 tSensor_init()
{
         //A_UCHAR ret;
 
         qcom_i2c_init(I2C_SCK_4,I2C_SDA_4,I2C_FREQ_50K);
 
         ret = tSensor_byte_read(SENSOR_WHO_AM_I);
         if(ret == SI_ERR)
         {
                   printf("i2c read SENSOR_WHO_AM_I error\n");
                   return SI_ERR;
         }
         if(ret != SENSOR_ID)
         {
                   printf("Unkown device,please check the chips\n");
                   return SI_ERR;
         }
 
         ret = tSensor_byte_write(SENSOR_CTRL_REG1,0x00);
         if(ret != SI_OK)
         {
                   printf("i2c write SENSOR_CTRL_REG1 error\n");
                   return SI_ERR;
         }
 
         ret = tSensor_byte_write(SENSOR_XYZ_DATA_CFG,MODE_2G);
         if(ret != SI_OK)
         {
                   printf("i2c write SENSOR_XYZ_DATA_CFG error\n");
                   return SI_ERR;
         }
 
         ret = tSensor_byte_write(SENSOR_CTRL_REG1,0x01);
         if(ret != SI_OK)
         {
                   printf("i2c write SENSOR_CTRL_REG1 error\n");
                   return SI_ERR;
         }
         qcom_thread_msleep(MODE_CHANGE_DELAY_MS);
         
         return SI_OK;
}

A_INT32 tSensor_update(short*x, short*y,short *z)
{
         A_INT32 result;
         A_UCHAR readbuf[10],writebuf[10];
         A_UCHAR ret;
         if(!x||!y||!z)
         {
                   printf("NULL address in\r\n");
                   return SI_ERR;
         }
         do{
                   do{
                                     result = tSensor_byte_read(SENSOR_STATUS);
                   }while(!(result & 0x08));
 
                   memset(readbuf, 0x00,sizeof(readbuf));
                   memset(writebuf,0x00,sizeof(writebuf));
 
                   writebuf[0] = I2C_SLAVE_ADDR;
                   writebuf[1] = SENSOR_OUT_X_MSB;
                   writebuf[2] = I2C_SLAVE_ADDR | 0x01;
                   ret = qcom_i2c_ctrl(writebuf,3,readbuf,6);
                  if(ret != SI_OK)
                   {
                            printf("i2c read SENSOR_OUT_X_MSB error\n");
                            return SI_ERR;
                   }
                   *x = (short)(((readbuf[0] << 8) & 0xff00) | readbuf[1]);
                   *y = (short)(((readbuf[2] << 8) & 0xff00) | readbuf[3]);
                   *z = (short)(((readbuf[4] << 8) & 0xff00) | readbuf[5]);
                   *x = (short)(*x) >> 2;
                   *y = (short)(*y) >> 2;
                   *z = (short)(*z) >> 2;
         }while(0);
         return SI_OK;
}
*/
A_INT32 temperatue,humidity;

A_INT32 get_temperatue(void)
{
	#if 0
	static A_INT16 flag1=0;
	if(flag1 == 0)
	{
		temperatue = -102;
		flag1 = 1;
	}
	else
	{
		temperatue = 205;
		flag1 = 0;
	}
	#endif
	return temperatue;

	
}

A_INT32 get_humidity(void)
{
#if 0
	static A_INT16 flag1=0;
	if(flag1 == 0)
	{
		humidity = 562;
		flag1 = 1;
	}
	else
	{
		humidity = 678;
		flag1 = 0;
	}
#endif

	return humidity;
}

void   dht12_read_task()
{
	A_UINT8 data[5];
	int ret;
	//float temperatue,humidity;
	
	//qcom_i2c_init(I2C_SCK_4,I2C_SDA_4,I2C_FREQ_50K);
	qcom_i2cm_init(I2CM_PIN_PAIR_2,I2C_FREQ_50K,0);
	//DHT12_Init();

	printf("enter dht12_read_task\r\n");

	while(TRUE)
	{
		ret = dht12_byte_read(data);
		if(ret < 0)
		{
			tx_thread_sleep(3000);
			continue;
		}
		//DHT12_ReadByte(data);
		
		A_UINT8 sum_crc;
		sum_crc = data[0]+data[1]+data[2]+data[3];
		if(sum_crc == data[4])
		{
			printf("dht12 humidity = %d.%d\r\n",data[0],data[1]);
			humidity = data[0]*10 + data[1];
			if(data[3] & 0x80)
			{
				printf("dht12 temperatue = -%d.%d\r\n",data[2],data[3]&0x7f);
				temperatue = (data[2]*10 + (data[3]&0x7f))* -1;
			}
			else
			{
				printf("dht12 temperatue = +%d.%d\r\n",data[2],data[3]);
				temperatue = (data[2]*10 + (data[3]&0x7f));
			}
		}
		else
		{
			printf("dht12 sum crc check error,d0=%2x,d1=%2x,d2=%2x,d3=%2x,d4=%2x,\r\n",data[0],data[1],data[2],data[3],data[4]);
		}
		tx_thread_sleep(3000);
	}
}

A_INT32 start_dht12_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(dht12_read_task, 2, 2048, 80);
}

#endif/*if defined(AR6002_REV74)*/
 
