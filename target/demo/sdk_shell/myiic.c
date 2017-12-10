#include "myiic.h"
#include "qcom_common.h"
#include "qcom_gpio.h"


#define IIC_SCL_PIN 26
#define IIC_SDA_PIN 25


//IO方向设置
#define SDA_IN()  	 qcom_gpio_pin_dir(IIC_SDA_PIN, GPIO_PIN_DIR_INPUT)
#define SDA_OUT() 	 qcom_gpio_pin_dir(IIC_SDA_PIN, GPIO_PIN_DIR_OUTPUT)
#define SCL_OUT() 	 qcom_gpio_pin_dir(IIC_SCL_PIN, GPIO_PIN_DIR_OUTPUT)

//IO操作
#define IIC_SCL(value)   qcom_gpio_pin_set(IIC_SCL_PIN, (value))
#define IIC_SDA(value)   qcom_gpio_pin_set(IIC_SDA_PIN, (value))
#define READ_SDA		 qcom_gpio_pin_get(IIC_SDA_PIN)

void delay_us(A_INT32 nus)
{		
	#if 0
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;				//LOADμ??μ	    	 
	ticks=nus*100; 						//Dèòaμ??ú??êy 
	told=SysTick->VAL;        				//????è?ê±μ???êy?÷?μ
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;	//?aà?×￠òaò???SYSTICKê?ò???μY??μ???êy?÷?í?éò?á?.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;			//ê±??3?1y/μèóúòa?ó3ùμ?ê±??,?òí?3?.
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

//?óê±nms
//nms:òa?óê±μ?msêy
void delay_ms_(A_INT32 nms)
{
	A_INT32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}

//IIC初始化
void IIC_Init(void)
{
	SDA_OUT();
	SCL_OUT();


    IIC_SDA(1);
    IIC_SCL(1);  
}

//产生IIC起始信号
void IIC_Start(void)
{
	SDA_OUT();     //sda线输出
	IIC_SDA(1);	  	  
	IIC_SCL(1);
	delay_us(4);
 	IIC_SDA(0);//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	IIC_SCL(0);//钳住I2C总线，准备发送或接收数据 
}	  
//产生IIC停止信号
void IIC_Stop(void)
{
	SDA_OUT();//sda线输出
	IIC_SCL(0);
	IIC_SDA(0);//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	IIC_SCL(1); 
	IIC_SDA(1);//发送I2C总线结束信号
	delay_us(4);							   	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
unsigned char IIC_Wait_Ack(void)
{
	A_INT32 ucErrTime=0;
	SDA_IN();      //SDA设置为输入  
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
	IIC_SCL(0);//时钟输出0 	   
	return 0;  
} 
//产生ACK应答
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
//不产生ACK应答		    
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
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void IIC_Send_Byte(unsigned char txd)
{                        
    A_INT32 t;   
	SDA_OUT(); 	    
    IIC_SCL(0);//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {              
        IIC_SDA((txd&0x80)>>7);
        txd<<=1; 	  
		delay_us(2);   //对TEA5767这三个延时都是必须的
		IIC_SCL(1);
		delay_us(2); 
		IIC_SCL(0);	
		delay_us(2);
    }	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
unsigned char IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	SDA_IN();//SDA设置为输入
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
        IIC_NAck();//发送nACK
    else
        IIC_Ack(); //发送ACK   
    return receive;
}


unsigned char shidu[4]={'0','0','.','0'};
unsigned char wendu[5]={'-','0','0','.','0'};//第一位是正负号
unsigned char dat[5];



//初始化IIC接口
void DHT12_Init(void)
{
	IIC_Init();
}

void DHT12_ReadByte(unsigned char *dat)
{				  
	unsigned char i;		  	    																 
    IIC_Start();  
		IIC_Send_Byte(0xB8);	   //发送写命令//发送器件地址0xB8,写数据 
		IIC_Wait_Ack();
	
		IIC_Send_Byte(0x00);//发送地址
		IIC_Wait_Ack();

	  IIC_Start();  	 	   
		IIC_Send_Byte(0xB9); //进入接收模式			   
		IIC_Wait_Ack();	 
	 
	  for(i=0;i<4;i++)
		{
			dat[i]=IIC_Read_Byte(1);//读前四个，发送ACK
		}
	  dat[i]=IIC_Read_Byte(0);//读第5个发送NACK

    IIC_Stop();//产生一个停止条件	    
	
}

//读取温湿度并转换为字符串 0失败 1成功
/*unsigned char Read_Feel()
{
	//unsigned char i=0;
	unsigned char temp;
	 //湿度 
	DHT12_ReadByte();
	temp=dat[0]+dat[1]+dat[2]+dat[3];
	
	//Uart_Send(dat,4);
	//Uart_Send_Byte('-');
	
	if(dat[4]==temp)
	{
   shidu[0]=dat[0]/10+'0';
   shidu[1]=dat[0]%10+'0';
	 
	 shidu[3]=dat[1]%10+'0';
	
	 //温度
	 
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
		return 0;//读取失败
 
 
}

*/
