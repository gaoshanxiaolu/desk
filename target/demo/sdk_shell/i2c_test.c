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
int Temprature,Humi;//������ʪ�ȱ��� ���˱���Ϊȫ�ֱ���
u8 Sensor_AnswerFlag=0;//���崫������Ӧ��־
u8 Sensor_ErrorFlag;  //�����ȡ�����������־

void DHT12_Init(void)
{					     

	qcom_gpio_apply_peripheral_configuration(QCOM_PERIPHERAL_ID_GPIOn(SDA), 0);
	   
	qcom_gpio_pin_dir(SDA, 0);

}

#define READ_SDA qcom_gpio_pin_get(SDA)

/********************************************\
|* ���ܣ� �����������͵ĵ����ֽ�	        *|
\********************************************/
u8 DHT12_Rdata(void)
{
	u8 i;
	u16 j;
	u8 data=0,bit=0;
	
	for(i=0;i<8;i++)
	{
		while(!READ_SDA)//����ϴε͵�ƽ�Ƿ����
		{
			if(++j>=50000) //��ֹ������ѭ��
			{
				break;
			}
		}
		//��ʱMin=26us Max70us ��������"0" �ĸߵ�ƽ		 
		us_delay(30);

		//�жϴ�������������λ
		bit=0;
		if(READ_SDA)
		{
			bit=1;
		}
		j=0;
		while(READ_SDA)	//�ȴ��ߵ�ƽ����
		{
			if(++j>=50000) //��ֹ������ѭ��
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
|* ���ܣ�DHT12��ȡ��ʪ�Ⱥ���       *|
\********************************************/
//������Humi_H��ʪ�ȸ�λ��Humi_L��ʪ�ȵ�λ��Temp_H���¶ȸ�λ��Temp_L���¶ȵ�λ��Temp_CAL��У��λ
//���ݸ�ʽΪ��ʪ�ȸ�λ��ʪ��������+ʪ�ȵ�λ��ʪ��С����+�¶ȸ�λ���¶�������+�¶ȵ�λ���¶�С����+ У��λ
//У�飺У��λ=ʪ�ȸ�λ+ʪ�ȵ�λ+�¶ȸ�λ+�¶ȵ�λ
u8 DHT12_READ(void)
{
	u32 j;
	u8 Humi_H,Humi_L,Temp_H,Temp_L,Temp_CAL,temp;

	//����������ʼ�ź�
	qcom_gpio_pin_dir(SDA, 0);//��Ϊ���ģʽ
	qcom_gpio_pin_set(SDA, 0);//SEND_SDA=0;	//�������������ߣ�SDA������
	tx_thread_sleep(20);//����һ��ʱ�䣨����18ms���� ֪ͨ������׼������
	qcom_gpio_pin_set(SDA, 1);//SEND_SDA=1;	 //�ͷ�����
	qcom_gpio_pin_dir(SDA, 1);;	//��Ϊ����ģʽ���жϴ�������Ӧ�ź�
	us_delay(30);//��ʱ30us

	Sensor_AnswerFlag=0;	//��������Ӧ��־
	//�жϴӻ��Ƿ��е͵�ƽ��Ӧ�ź� �粻��Ӧ����������Ӧ����������	  
	if(READ_SDA==0)
	{
		Sensor_AnswerFlag=1;	//�յ���ʼ�ź�

		j=0;
		while((!READ_SDA)) //�жϴӻ����� 80us �ĵ͵�ƽ��Ӧ�ź��Ƿ����	
		{
			if(++j>=500) //��ֹ������ѭ��
			{
				Sensor_ErrorFlag=1;
				break;
			}
		}

		j=0;
		while(READ_SDA)//�жϴӻ��Ƿ񷢳� 80us �ĸߵ�ƽ���緢����������ݽ���״̬
		{
			if(++j>=800) //��ֹ������ѭ��
			{
				Sensor_ErrorFlag=1;
				break;
			}		
		}
		//��������
		Humi_H=DHT12_Rdata();
		Humi_L=DHT12_Rdata();
		Temp_H=DHT12_Rdata();	
		Temp_L=DHT12_Rdata();
		Temp_CAL=DHT12_Rdata();

		temp=(u8)(Humi_H+Humi_L+Temp_H+Temp_L);//ֻȡ��8λ

		if(Temp_CAL==temp)//���У��ɹ�����������
		{
			Humi=Humi_H*10+Humi_L; //ʪ��
	
			if(Temp_L&0X80)	//Ϊ���¶�
			{
				Temprature =0-(Temp_H*10+((Temp_L&0x7F)));
			}
			else   //Ϊ���¶�
			{
				Temprature=Temp_H*10+Temp_L;//Ϊ���¶�
			}
			//�ж������Ƿ񳬹����̣��¶ȣ�-20��~60�棬ʪ��20��RH~95��RH��
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
			//Temprature=Temprature/10;//����Ϊ�¶�ֵ
			//Humi=Humi/10; //����Ϊʪ��ֵ
			printf("\r\n: Temprature %d\r\n",Temprature); //��ʾ�¶�
			printf(" Humi %d  %%RH\r\n",Humi);//��ʾʪ��	
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
		Sensor_ErrorFlag=0;  //δ�յ���������Ӧ
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
 
