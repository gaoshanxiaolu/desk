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
#error "i2c version error"
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
//#include "myiic.h"
#include "swat_parse.h"
#include "qcom_time.h"
#include "qcom_gpio.h"

extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();

#define I2C_MASTER_MODE

#define I2C_SLAVE_ADDR 1
#define DHT12_SLAVE_READ_ADDR 0XB9
#define DHT12_SLAVE_WRITE_ADDR 0XB8

#define ADPS9300GRD_SLAVE_ADDR 0X52
#define ADPS9300FLOAT_SLAVE_ADDR 0X72
#define ADPS9300VCC_SLAVE_ADDR 0X92

//for bxue test I2C on GPIO14/15
#define SDA	14


#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
int Temprature,Humi;//定义温湿度变量 ，此变量为全局变量
u8 Sensor_AnswerFlag=0;//定义传感器响应标志
u8 Sensor_ErrorFlag;  //定义读取传感器错误标志

void DHT12_Init(void)
{					     

	qcom_gpio_apply_peripheral_configuration(QCOM_PERIPHERAL_ID_GPIOn(SDA), 0);
	   
	qcom_gpio_pin_dir(SDA, 0);

}

#define READ_SDA qcom_gpio_pin_get(SDA)

/********************************************\
|* 功能： 读传感器发送的单个字节	        *|
\********************************************/
u8 DHT12_Rdata(void)
{
	u8 i;
	u16 j;
	u8 data=0,bit=0;
	
	for(i=0;i<8;i++)
	{
		while(!READ_SDA)//检测上次低电平是否结束
		{
			if(++j>=50000) //防止进入死循环
			{
				break;
			}
		}
		//延时Min=26us Max70us 跳过数据"0" 的高电平		 
		us_delay(30);

		//判断传感器发送数据位
		bit=0;
		if(READ_SDA)
		{
			bit=1;
		}
		j=0;
		while(READ_SDA)	//等待高电平结束
		{
			if(++j>=50000) //防止进入死循环
			{
				break;
			}		
		}
		data<<=1;
		data|=bit;
	}
	return data;
}


/********************************************\
|* 功能：DHT12读取温湿度函数       *|
\********************************************/
//变量：Humi_H：湿度高位；Humi_L：湿度低位；Temp_H：温度高位；Temp_L：温度低位；Temp_CAL：校验位
//数据格式为：湿度高位（湿度整数）+湿度低位（湿度小数）+温度高位（温度整数）+温度低位（温度小数）+ 校验位
//校验：校验位=湿度高位+湿度低位+温度高位+温度低位
u8 DHT12_READ(void)
{
	u32 j;
	u8 Humi_H,Humi_L,Temp_H,Temp_L,Temp_CAL,temp;

	//主机发送起始信号
	qcom_gpio_pin_dir(SDA, 0);//设为输出模式
	qcom_gpio_pin_set(SDA, 0);//SEND_SDA=0;	//主机把数据总线（SDA）拉低
	tx_thread_sleep(20);//拉低一段时间（至少18ms）， 通知传感器准备数据
	qcom_gpio_pin_set(SDA, 1);//SEND_SDA=1;	 //释放总线
	qcom_gpio_pin_dir(SDA, 1);;	//设为输入模式，判断传感器响应信号
	us_delay(30);//延时30us

	Sensor_AnswerFlag=0;	//传感器响应标志
	//判断从机是否有低电平响应信号 如不响应则跳出，响应则向下运行	  
	if(READ_SDA==0)
	{
		Sensor_AnswerFlag=1;	//收到起始信号

		j=0;
		while((!READ_SDA)) //判断从机发出 80us 的低电平响应信号是否结束	
		{
			if(++j>=500) //防止进入死循环
			{
				Sensor_ErrorFlag=1;
				break;
			}
		}

		j=0;
		while(READ_SDA)//判断从机是否发出 80us 的高电平，如发出则进入数据接收状态
		{
			if(++j>=800) //防止进入死循环
			{
				Sensor_ErrorFlag=1;
				break;
			}		
		}
		//接收数据
		Humi_H=DHT12_Rdata();
		Humi_L=DHT12_Rdata();
		Temp_H=DHT12_Rdata();	
		Temp_L=DHT12_Rdata();
		Temp_CAL=DHT12_Rdata();

		temp=(u8)(Humi_H+Humi_L+Temp_H+Temp_L);//只取低8位

		if(Temp_CAL==temp)//如果校验成功，往下运行
		{
			Humi=Humi_H*10+Humi_L; //湿度
	
			if(Temp_L&0X80)	//为负温度
			{
				Temprature =0-(Temp_H*10+((Temp_L&0x7F)));
			}
			else   //为正温度
			{
				Temprature=Temp_H*10+Temp_L;//为正温度
			}
			//判断数据是否超过量程（温度：-20℃~60℃，湿度20％RH~95％RH）
			if(Humi>950) 
			{
			  Humi=950;
			}
			if(Humi<200)
			{
				Humi =200;
			}
			if(Temprature>600)
			{
			  Temprature=600;
			}
			if(Temprature<-200)
			{
				Temprature = -200;
			}
			//Temprature=Temprature/10;//计算为温度值
			//Humi=Humi/10; //计算为湿度值
			printf("\r\n: Temprature %d\r\n",Temprature); //显示温度
			printf(" Humi %d  %%RH\r\n",Humi);//显示湿度	
		}
		
		else
		{
		 	printf("CAL Error!!\r\n");
			printf("%d \r%d \r%d \r%d \r%d \r%d \r\n",Humi_H,Humi_L,Temp_H,Temp_L,Temp_CAL,temp);
			return 1;
		}
	}
	else
	{
		Sensor_ErrorFlag=0;  //未收到传感器响应
		printf("Sensor Error!!\r\n");
		return 1;
	}

	return 0;
}

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
			 ret = qcom_i2cm_read(I2CM_PIN_PAIR_4,0x5c,0x00,1,data,5);
			 if(ret<0)
			 {
				A_PRINTF("4 qcom_i2cm_read error ret=%d\n\n",ret);
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

	//return temperatue;
	return Temprature;

	
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

	//return humidity;
	return Humi;
}

void   dht12_read_task()
{
	//A_UINT8 data[5];
	int ret;
	//float temperatue,humidity;

	#if 0
	//qcom_i2c_init(I2C_SCK_4,I2C_SDA_4,I2C_FREQ_50K);
	qcom_i2cm_init(I2CM_PIN_PAIR_4,I2C_FREQ_50K,0);
	//DHT12_Init();
	#else
	DHT12_Init();
	#endif
	printf("enter dht12_read_task,signal 1-wire\r\n");

	while(TRUE)
	{
		#if 0
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

		#else
		if(need_read)
		{
			 ret = DHT12_READ();
			 if(ret>0)
			 {
				A_PRINTF("DHT12_READ failed ret=%d\n\n",ret);
				failure_cnt++;
				if(failure_cnt > 10)
				{
					need_read = 0;
					A_PRINTF("DHT12_READ error continous 10 error,no read again\n");
				}
			 }
			 else
			 {
			 	failure_cnt = 0;
			 }
		}
		
		#endif
		tx_thread_sleep(3000);
	}
}

A_INT32 start_dht12_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(dht12_read_task, 2, 2048, 80);
}

#endif/*if defined(AR6002_REV74)*/
 
