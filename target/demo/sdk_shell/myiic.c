#include "myiic.h"
#include "qcom_common.h"
#include "qcom_gpio.h"


#define IIC_SCL_PIN 26
#define IIC_SDA_PIN 25


//IO��������
#define SDA_IN()  	 qcom_gpio_pin_dir(IIC_SDA_PIN, GPIO_PIN_DIR_INPUT)
#define SDA_OUT() 	 qcom_gpio_pin_dir(IIC_SDA_PIN, GPIO_PIN_DIR_OUTPUT)
#define SCL_OUT() 	 qcom_gpio_pin_dir(IIC_SCL_PIN, GPIO_PIN_DIR_OUTPUT)

//IO����
#define IIC_SCL(value)   qcom_gpio_pin_set(IIC_SCL_PIN, (value))
#define IIC_SDA(value)   qcom_gpio_pin_set(IIC_SDA_PIN, (value))
#define READ_SDA		 qcom_gpio_pin_get(IIC_SDA_PIN)

void delay_us(A_INT32 nus)
{		
	#if 0
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;				//LOAD��??��	    	 
	ticks=nus*100; 						//D����a��??��??��y 
	told=SysTick->VAL;        				//????��?������???��y?��?��
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;	//?a��?���騰a��???SYSTICK��?��???��Y??��???��y?��?��?����?��?.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;			//����??3?1y/�̨�������a?��3����?����??,?����?3?.
		}  
	};
	#else

	A_INT32 i,j;
	for(i=0;i<nus*20;i++)
	{
		j++;
	}
	#endif
}

//?������nms
//nms:��a?��������?ms��y
void delay_ms_(A_INT32 nms)
{
	A_INT32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}

//IIC��ʼ��
void IIC_Init(void)
{
	SDA_OUT();
	SCL_OUT();


    IIC_SDA(1);
    IIC_SCL(1);  
}

//����IIC��ʼ�ź�
void IIC_Start(void)
{
	SDA_OUT();     //sda�����
	IIC_SDA(1);	  	  
	IIC_SCL(1);
	delay_us(4);
 	IIC_SDA(0);//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	IIC_SCL(0);//ǯסI2C���ߣ�׼�����ͻ�������� 
}	  
//����IICֹͣ�ź�
void IIC_Stop(void)
{
	SDA_OUT();//sda�����
	IIC_SCL(0);
	IIC_SDA(0);//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	IIC_SCL(1); 
	IIC_SDA(1);//����I2C���߽����ź�
	delay_us(4);							   	
}
//�ȴ�Ӧ���źŵ���
//����ֵ��1������Ӧ��ʧ��
//        0������Ӧ��ɹ�
unsigned char IIC_Wait_Ack(void)
{
	A_INT32 ucErrTime=0;
	SDA_IN();      //SDA����Ϊ����  
	IIC_SDA(1);delay_us(1);	   
	IIC_SCL(1);delay_us(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL(0);//ʱ�����0 	   
	return 0;  
} 
//����ACKӦ��
void IIC_Ack(void)
{
	IIC_SCL(0);
	SDA_OUT();
	IIC_SDA(0);
	delay_us(2);
	IIC_SCL(1);
	delay_us(2);
	IIC_SCL(0);
}
//������ACKӦ��		    
void IIC_NAck(void)
{
	IIC_SCL(0);
	SDA_OUT();
	IIC_SDA(1);
	delay_us(2);
	IIC_SCL(1);
	delay_us(2);
	IIC_SCL(0);
}					 				     
//IIC����һ���ֽ�
//���شӻ�����Ӧ��
//1����Ӧ��
//0����Ӧ��			  
void IIC_Send_Byte(unsigned char txd)
{                        
    A_INT32 t;   
	SDA_OUT(); 	    
    IIC_SCL(0);//����ʱ�ӿ�ʼ���ݴ���
    for(t=0;t<8;t++)
    {              
        IIC_SDA((txd&0x80)>>7);
        txd<<=1; 	  
		delay_us(2);   //��TEA5767��������ʱ���Ǳ����
		IIC_SCL(1);
		delay_us(2); 
		IIC_SCL(0);	
		delay_us(2);
    }	 
} 	    
//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
unsigned char IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN();//SDA����Ϊ����
    for(i=0;i<8;i++ )
	{
        IIC_SCL(0); 
        delay_us(2);
		IIC_SCL(1);
        receive<<=1;
        if(READ_SDA)receive++;   
		delay_us(1); 
    }					 
    if (!ack)
        IIC_NAck();//����nACK
    else
        IIC_Ack(); //����ACK   
    return receive;
}


unsigned char shidu[4]={'0','0','.','0'};
unsigned char wendu[5]={'-','0','0','.','0'};//��һλ��������
unsigned char dat[5];



//��ʼ��IIC�ӿ�
void DHT12_Init(void)
{
	IIC_Init();
}

void DHT12_ReadByte(unsigned char *dat)
{				  
	unsigned char i;		  	    																 
    IIC_Start();  
		IIC_Send_Byte(0xB8);	   //����д����//����������ַ0xB8,д���� 
		IIC_Wait_Ack();
	
		IIC_Send_Byte(0x00);//���͵�ַ
		IIC_Wait_Ack();

	  IIC_Start();  	 	   
		IIC_Send_Byte(0xB9); //�������ģʽ			   
		IIC_Wait_Ack();	 
	 
	  for(i=0;i<4;i++)
		{
			dat[i]=IIC_Read_Byte(1);//��ǰ�ĸ�������ACK
		}
	  dat[i]=IIC_Read_Byte(0);//����5������NACK

    IIC_Stop();//����һ��ֹͣ����	    
	
}

//��ȡ��ʪ�Ȳ�ת��Ϊ�ַ��� 0ʧ�� 1�ɹ�
/*unsigned char Read_Feel()
{
	//unsigned char i=0;
	unsigned char temp;
	 //ʪ�� 
	DHT12_ReadByte();
	temp=dat[0]+dat[1]+dat[2]+dat[3];
	
	//Uart_Send(dat,4);
	//Uart_Send_Byte('-');
	
	if(dat[4]==temp)
	{
   shidu[0]=dat[0]/10+'0';
   shidu[1]=dat[0]%10+'0';
	 
	 shidu[3]=dat[1]%10+'0';
	
	 //�¶�
	 
   wendu[1]=dat[2]/10+'0';
   wendu[2]=dat[2]%10+'0';
	 

	 if(dat[3]&0x80)
    wendu[0]='-';
	 else
		wendu[0]='0';
	 
   wendu[4]=(dat[3]&0x7f)%10+'0';
	 
	 //Uart_Send(wendu,5);
	 //Uart_Send_Byte('-');
	 //Uart_Send(shidu,4);

	  return 1;
  }
	else
		return 0;//��ȡʧ��
 
 
}

*/
