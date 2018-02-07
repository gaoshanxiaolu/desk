#include "qcom_common.h"
#include "qcom_gpio.h"
#include "qcom_uart.h"
#include "select_api.h"
//#include "swat_parse.h"
#include "qcom_cli.h"
#include "adc_layer_wmi.h"
#include "swat_wmiconfig_network.h"

#include "socket_api.h"
#include "wx_airkiss.h"

#include "ble_uart.h"
extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();

#define CONTROL_LOGIC_LEVEL

#define delay_polling_ms (50)
#define timeout_2s       (40)
#define timeout_5s       (100)

#define pin_ir1 13
#define pin_ir2 12
#define pin_ir3 11
//green
#define pin_m1 32
//yellow
#define pin_m2 30
#define pin_gold 33
#define pin_zi 31
#define pin_laptop 6
#define pin_led 27

#define pin_light 10  //pin num 9
//#define pin_in_ir1 21  //pin num 4
//#define pin_in_ir2 22 //pin num 5
#define pin_in_ir1 2  //pin num 16
#define pin_in_ir2 5 //pin num 14


#define PIN_SCL 26 //PIN num 44
#define PIN_SDA 25 //PIN NUM 43

A_UINT16 desk_pos_value = 700;
A_UINT16 get_desk_pos_value(void)
{
	#if 1
    return desk_pos_value;
	#else
	desk_pos_value++;
	if(desk_pos_value > 1000)
		desk_pos_value = 700;
	return desk_pos_value;
	#endif
}

#define need_read_len 1024

unsigned char AscToHex(unsigned char aHex){
    if((aHex<=9))
        aHex += 0x30;
    else if((aHex>=10)&&(aHex<=15))//A-F
        aHex += 0x37;
    else aHex = 0xff;
    return aHex;
}

unsigned char HexToAsc(unsigned char aChar){
    if((aChar>=0x30)&&(aChar<=0x39))
        aChar -= 0x30;
    else if((aChar>=0x41)&&(aChar<=0x46))
        aChar -= 0x37;
    else if((aChar>=0x61)&&(aChar<=0x66))
        aChar -= 0x57;
    else aChar = 0xff;
    return aChar; 
}

void big_little(uint8_t *data)
{
	uint8_t *p,tmp;
	p = (uint8_t *)data;
	tmp = p[0];
	p[0] = p[1];
	p[1]= tmp;
}

uint8_t crc_check(uint8_t std_crc,uint8_t *tmp)
{
	uint8_t k,j,crc;
	
	crc = 0;
	for(k=0;k<9;k++)
	{
		//printf("%x ",tmp[k]);
		crc ^= tmp[k];
		for(j=0;j<8;j++)
		{
			if((crc & 0x01)!=0)
			{
				crc = (crc >> 1)^0x8c;
			}
			else
			{
				crc = crc >> 1;
			}
		}
	}
	//printf(" calu crc = %x\r\n",crc);
	if(crc == std_crc)
		return 1;
	return 0;
}

void   desk_uart_task()
{
    A_UINT32 uart_length = need_read_len;
    A_CHAR uart_buff[need_read_len];
	A_CHAR uartBuf2[300];
	A_UINT32 buf_len;
	int printf_cnt;
    q_fd_set fd;
    struct timeval tmo;
    int ret;
    qcom_uart_para com_uart_cfg;
	int uart2_fd;
	
    com_uart_cfg.BaudRate=     9600; /* 1Mbits */
    com_uart_cfg.number=       8;
    com_uart_cfg.StopBits =    1;
    com_uart_cfg.parity =      0;
    com_uart_cfg.FlowControl = 0;

	if (qcom_gpio_pin_dir(3, GPIO_PIN_DIR_INPUT) != A_OK) {
			A_PRINTF("ERROR:set uart2 pin %d dir error\r\n",pin_m1);
   }

	//qcom_gpio_pin_pad(3,100,QCOM_GPIO_PIN_PULLUP,FALSE);

   qcom_single_uart_init((A_CHAR *)"UART2");

    uart2_fd = qcom_uart_open((A_CHAR *)"UART2");
    if (uart2_fd == -1) {
        A_PRINTF("qcom_uart_open uart2 failed...\r\n");
        return;
    }

    qcom_set_uart_config((A_CHAR *)"UART2",&com_uart_cfg);
    tx_thread_sleep(1000);

   printf("enter uart desk data thread \r\n");
   buf_len = 0;
    while (1)
    {
        FD_ZERO(&fd);
        FD_SET(uart2_fd, &fd);
        tmo.tv_sec = 30;
        tmo.tv_usec = 0;

		memset(uart_buff, 0x00,  sizeof(uart_buff));
        ret = qcom_select(2, &fd, NULL, NULL, &tmo);
        if (ret == 0) 
		{
            A_PRINTF("UART receive timeout\r\n");
			//A_CHAR a[6] = {0x7e,0x04,0x41,0x00,0x01,0xef};
			//A_UINT32 len = 6;
			//A_INT32 x = qcom_uart_write(uart2_fd,a,&len);
			//A_PRINTF("first play =%d\r\n",x);
        } 
		else 
        {
            //A_PRINTF("Uart received data\r\n");
            if (FD_ISSET(uart2_fd, &fd)) 
			{
		  		uart_length = need_read_len;
                qcom_uart_read(uart2_fd, uart_buff, &uart_length);
                if (uart_length) 
				{
					int k;
					/*printf("len = %d, ",uart_length);
					
					for(k=0;k<uart_length;k++)
					{
						printf("%2x ",uart_buff[k]);
					}
					printf("\r\n");*/

					k=0;

					memcpy(&(uartBuf2[buf_len]),uart_buff,uart_length);
					buf_len += uart_length;
					
					if(buf_len > 18)
					{
						int get_value;

						get_value = 0;
						while(k <= buf_len)
						{
							if((uartBuf2[k] == 0x01) && (uartBuf2[k+1] == 0x01) && (uartBuf2[k+4] == 0x01) && (uartBuf2[k+5] == 0x01) )
							{
								A_UINT16 tmp;
								tmp = (uartBuf2[k+2] << 8) | uartBuf2[k+3];
								if((tmp <= 1271) && (tmp >= 623))
								{
									desk_pos_value = tmp;
									static A_UINT16 cnt = 100;
									cnt++;

									if(cnt > 10)
									{
										printf("desk pos =%d\r\n",desk_pos_value);
										cnt = 0;
									}
								}
								else
								{
									printf("error pos value %d\r\n",tmp);
								}
								k += 4;
								get_value = 1;
								break;
							}
							else
							{
								k++;
							}
						}

						if(get_value)
						{
							//if(printf_cnt++ > 200)
							{
								printf_cnt = 0;
								//printf("desk pos =%d\r\n",desk_pos_value);
								
							}

						}
						else
						{
							//printf("desk pos failed\r\n");
						}

						//memcpy(uartBuf2,&(uartBuf2[k]),buf_len - k);
						//memset(&(uartBuf2[k]), 0x00,  buf_len - k);
						buf_len = 0;

					}

                }
            }
			else
			{
                 A_PRINTF("UART2 something is error!\n");           		
        	}
        }
    }
}
enum DESK_RUN_STATE desk_run_state;

enum DESK_RUN_STATE get_desk_run_state(void)
{
	return desk_run_state;
}

int desk_is_moved = 0;

void up_desk(void)
{
	desk_is_moved = 1;
#ifdef CONTROL_LOGIC_LEVEL
#if 1
				//(*((volatile A_UINT32 *)0x1402c)) =  0x1;//hw write GPIO32 to up,
				(*((volatile A_UINT32 *)0x14030)) =  0x1;//hw write GPIO32 to down,
				
				(*((volatile A_UINT32 *)0x14004)) =  0x40000000;//hw write GPIO30 to up, 
				//(*((volatile A_UINT32 *)0x14008)) =  0x40000000;//hw write GPIO30 to down,
				
				(*((volatile A_UINT32 *)0x14004)) =  0x80000000;//hw write GPIO31 to up,
				//(*((volatile A_UINT32 *)0x14008)) =  0x80000000;//hw write GPIO31 to down,
				
				(*((volatile A_UINT32 *)0x1402c)) =  0x2;//hw write GPIO33 to up,
				//(*((volatile A_UINT32 *)0x14030)) =  0x2;//hw write GPIO33 to down,
#else

		qcom_gpio_pin_set(pin_m1,!TRUE);
		qcom_gpio_pin_set(pin_m2,!FALSE);
		
		qcom_gpio_pin_set(pin_zi,!FALSE);
		qcom_gpio_pin_set(pin_gold,!FALSE);
#endif
	
	
	
#else
		qcom_gpio_pin_set(pin_m1,TRUE);
		qcom_gpio_pin_set(pin_m2,FALSE);
		qcom_gpio_pin_set(pin_gold,FALSE);
		qcom_gpio_pin_set(pin_zi,FALSE);

	
			
#endif



	desk_run_state = UP_DESK;

}
void down_desk(void)
{
	desk_is_moved = 1;
#ifdef CONTROL_LOGIC_LEVEL
#if 1
				(*((volatile A_UINT32 *)0x1402c)) =  0x1;//hw write GPIO32 to up,
				//(*((volatile A_UINT32 *)0x14030)) =  0x1;//hw write GPIO32 to down,
				
				//(*((volatile A_UINT32 *)0x14004)) =  0x40000000;//hw write GPIO30 to up, 
				(*((volatile A_UINT32 *)0x14008)) =  0x40000000;//hw write GPIO30 to down,
				
				(*((volatile A_UINT32 *)0x14004)) =  0x80000000;//hw write GPIO31 to up,
				//(*((volatile A_UINT32 *)0x14008)) =  0x80000000;//hw write GPIO31 to down,
				
				(*((volatile A_UINT32 *)0x1402c)) =  0x2;//hw write GPIO33 to up,
				//(*((volatile A_UINT32 *)0x14030)) =  0x2;//hw write GPIO33 to down,
#else

		qcom_gpio_pin_set(pin_m1,!FALSE);
		qcom_gpio_pin_set(pin_m2,!TRUE);
		
		qcom_gpio_pin_set(pin_zi,!FALSE);
		qcom_gpio_pin_set(pin_gold,!FALSE);

	#endif
	
	
#else
		qcom_gpio_pin_set(pin_m1,FALSE);
		qcom_gpio_pin_set(pin_m2,TRUE);
		qcom_gpio_pin_set(pin_gold,FALSE);
		qcom_gpio_pin_set(pin_zi,FALSE);

	
			
#endif



	desk_run_state = DOWN_DESK;
}

void laptop_on(void)
{
	qcom_gpio_pin_set(pin_laptop,TRUE);
	A_PRINTF("lamp on\r\n");
}

void laptop_off(void)
{
	qcom_gpio_pin_set(pin_laptop,FALSE);
	A_PRINTF("lamp off\r\n");
}
A_INT8 laptop_state;
void laptop_toggle(void)
{
	if(laptop_state == 0)
	{
		laptop_on();
		laptop_state = 1;
	}
	else
	{
		laptop_off();
		laptop_state = 0;
	}
}

void init_desk(void)
{

	
#ifdef CONTROL_LOGIC_LEVEL
	qcom_gpio_pin_set(pin_m1,TRUE);
	qcom_gpio_pin_set(pin_m2,TRUE);	
	qcom_gpio_pin_set(pin_gold,TRUE);
	qcom_gpio_pin_set(pin_zi,TRUE);
	qcom_gpio_pin_set(pin_laptop,FALSE);
#else
qcom_gpio_pin_set(pin_m1,FALSE);
qcom_gpio_pin_set(pin_m2,FALSE);	
qcom_gpio_pin_set(pin_gold,FALSE);
qcom_gpio_pin_set(pin_zi,FALSE);
qcom_gpio_pin_set(pin_laptop,FALSE);

#endif
laptop_state = 0;

}

void stop_desk(void)
{
	desk_is_moved = 0;
	
	#ifdef CONTROL_LOGIC_LEVEL
	#if 1
(*((volatile A_UINT32 *)0x1402c)) =  0x1;//hw write GPIO32 to up,
//(*((volatile A_UINT32 *)0x14030)) =  0x1;//hw write GPIO32 to down,

(*((volatile A_UINT32 *)0x14004)) =  0x40000000;//hw write GPIO30 to up, 
//(*((volatile A_UINT32 *)0x14008)) =  0x40000000;//hw write GPIO30 to down,

(*((volatile A_UINT32 *)0x14004)) =  0x80000000;//hw write GPIO31 to up,
//(*((volatile A_UINT32 *)0x14008)) =  0x80000000;//hw write GPIO31 to down,

(*((volatile A_UINT32 *)0x1402c)) =  0x2;//hw write GPIO33 to up,
//(*((volatile A_UINT32 *)0x14030)) =  0x2;//hw write GPIO33 to down,

#else	
	qcom_gpio_pin_set(pin_m1,TRUE);
	qcom_gpio_pin_set(pin_m2,TRUE);	
	qcom_gpio_pin_set(pin_gold,TRUE);
	qcom_gpio_pin_set(pin_zi,TRUE);
#endif
	#else
	qcom_gpio_pin_set(pin_m1,FALSE);
	qcom_gpio_pin_set(pin_m2,FALSE);	
	qcom_gpio_pin_set(pin_gold,FALSE);
	qcom_gpio_pin_set(pin_zi,FALSE);
	#endif
	desk_run_state = STOP;

}

void free_desk_pin(void)
{
	#ifdef CONTROL_LOGIC_LEVEL
#if 1
	(*((volatile A_UINT32 *)0x1402c)) =  0x1;//hw write GPIO32 to up,
	//(*((volatile A_UINT32 *)0x14030)) =  0x1;//hw write GPIO32 to down,
	
	(*((volatile A_UINT32 *)0x14004)) =  0x40000000;//hw write GPIO30 to up, 
	//(*((volatile A_UINT32 *)0x14008)) =  0x40000000;//hw write GPIO30 to down,
	
	(*((volatile A_UINT32 *)0x14004)) =  0x80000000;//hw write GPIO31 to up,
	//(*((volatile A_UINT32 *)0x14008)) =  0x80000000;//hw write GPIO31 to down,
	
	(*((volatile A_UINT32 *)0x1402c)) =  0x2;//hw write GPIO33 to up,
	//(*((volatile A_UINT32 *)0x14030)) =  0x2;//hw write GPIO33 to down,
	
#else

	qcom_gpio_pin_set(pin_m1,TRUE);
	qcom_gpio_pin_set(pin_m2,TRUE);	
	qcom_gpio_pin_set(pin_gold,TRUE);
	qcom_gpio_pin_set(pin_zi,TRUE);
#endif
	#else
	qcom_gpio_pin_set(pin_m1,FALSE);
	qcom_gpio_pin_set(pin_m2,FALSE);	
	qcom_gpio_pin_set(pin_gold,FALSE);
	qcom_gpio_pin_set(pin_zi,FALSE);
	#endif
}
void gen_m_signal(void)
{

#ifdef CONTROL_LOGIC_LEVEL

#if 1
(*((volatile A_UINT32 *)0x1402c)) =  0x1;//hw write GPIO32 to up,
//(*((volatile A_UINT32 *)0x14030)) =  0x1;//hw write GPIO32 to down,

(*((volatile A_UINT32 *)0x14004)) =  0x40000000;//hw write GPIO30 to up, 
//(*((volatile A_UINT32 *)0x14008)) =  0x40000000;//hw write GPIO30 to down,

(*((volatile A_UINT32 *)0x14004)) =  0x80000000;//hw write GPIO31 to up,
//(*((volatile A_UINT32 *)0x14008)) =  0x80000000;//hw write GPIO31 to down,

//(*((volatile A_UINT32 *)0x1402c)) =  0x2;//hw write GPIO33 to up,
(*((volatile A_UINT32 *)0x14030)) =  0x2;//hw write GPIO33 to down,

#else
	qcom_gpio_pin_set(pin_m1,!FALSE);
	qcom_gpio_pin_set(pin_m2,!FALSE);	
	
	qcom_gpio_pin_set(pin_zi,!FALSE);
	qcom_gpio_pin_set(pin_gold,!TRUE);
#endif

#else
	qcom_gpio_pin_set(pin_m1,FALSE);
	qcom_gpio_pin_set(pin_m2,FALSE);	
	qcom_gpio_pin_set(pin_gold,TRUE);
	qcom_gpio_pin_set(pin_zi,FALSE);

#endif

	//us_delay(100000);

	//stop_desk();

	

	A_PRINTF(" m value signal\r\n");
}
void gen_1234_signal(A_UINT8 num)
{
	if(num == 1)
	{
#ifdef CONTROL_LOGIC_LEVEL

#if 1
//	(*((volatile A_UINT32 *)0x1402c)) =  0x1;//hw write GPIO32 to up,
	(*((volatile A_UINT32 *)0x14030)) =  0x1;//hw write GPIO32 to down,
	
//	(*((volatile A_UINT32 *)0x14004)) =  0x40000000;//hw write GPIO30 to up, 
	(*((volatile A_UINT32 *)0x14008)) =  0x40000000;//hw write GPIO30 to down,
	
	(*((volatile A_UINT32 *)0x14004)) =  0x80000000;//hw write GPIO31 to up,
//	(*((volatile A_UINT32 *)0x14008)) =  0x80000000;//hw write GPIO31 to down,
	
	(*((volatile A_UINT32 *)0x1402c)) =  0x2;//hw write GPIO33 to up,
//	(*((volatile A_UINT32 *)0x14030)) =  0x2;//hw write GPIO33 to down,
	
#else

	qcom_gpio_pin_set(pin_m1,!TRUE);
	qcom_gpio_pin_set(pin_m2,!TRUE); 
	
	qcom_gpio_pin_set(pin_zi,!FALSE);
	qcom_gpio_pin_set(pin_gold,!FALSE);

#endif

#else
	qcom_gpio_pin_set(pin_m1,TRUE);
	qcom_gpio_pin_set(pin_m2,TRUE); 
	qcom_gpio_pin_set(pin_gold,FALSE);
	qcom_gpio_pin_set(pin_zi,FALSE);

		
#endif



		A_PRINTF(" num 1 value\r\n");
	}
	else if(num == 2)
	{
#ifdef CONTROL_LOGIC_LEVEL

#if 1
		(*((volatile A_UINT32 *)0x1402c)) =  0x1;//hw write GPIO32 to up,
		//(*((volatile A_UINT32 *)0x14030)) =  0x1;//hw write GPIO32 to down,
		
		(*((volatile A_UINT32 *)0x14004)) =  0x40000000;//hw write GPIO30 to up, 
		//(*((volatile A_UINT32 *)0x14008)) =  0x40000000;//hw write GPIO30 to down,
		
		//(*((volatile A_UINT32 *)0x14004)) =  0x80000000;//hw write GPIO31 to up,
		(*((volatile A_UINT32 *)0x14008)) =  0x80000000;//hw write GPIO31 to down,
		
		(*((volatile A_UINT32 *)0x1402c)) =  0x2;//hw write GPIO33 to up,
		//(*((volatile A_UINT32 *)0x14030)) =  0x2;//hw write GPIO33 to down,
#else
		qcom_gpio_pin_set(pin_m1,!FALSE);
		qcom_gpio_pin_set(pin_m2,!FALSE);	
		
		qcom_gpio_pin_set(pin_zi,!TRUE);
		qcom_gpio_pin_set(pin_gold,!FALSE);

#endif
	
	
#else
		qcom_gpio_pin_set(pin_m1,FALSE);
		qcom_gpio_pin_set(pin_m2,FALSE);	
		
		qcom_gpio_pin_set(pin_zi,TRUE);
		qcom_gpio_pin_set(pin_gold,FALSE);

		
				
#endif



		A_PRINTF(" num 2 value\r\n");
	}
	else if(num == 3)
	{
#ifdef CONTROL_LOGIC_LEVEL
#if 1
				(*((volatile A_UINT32 *)0x1402c)) =  0x1;//hw write GPIO32 to up,
				//(*((volatile A_UINT32 *)0x14030)) =  0x1;//hw write GPIO32 to down,
				
				//(*((volatile A_UINT32 *)0x14004)) =  0x40000000;//hw write GPIO30 to up, 
				(*((volatile A_UINT32 *)0x14008)) =  0x40000000;//hw write GPIO30 to down,
				
				//(*((volatile A_UINT32 *)0x14004)) =  0x80000000;//hw write GPIO31 to up,
				(*((volatile A_UINT32 *)0x14008)) =  0x80000000;//hw write GPIO31 to down,
				
				(*((volatile A_UINT32 *)0x1402c)) =  0x2;//hw write GPIO33 to up,
				//(*((volatile A_UINT32 *)0x14030)) =  0x2;//hw write GPIO33 to down,
#else

		qcom_gpio_pin_set(pin_m1,!FALSE);
		qcom_gpio_pin_set(pin_m2,!TRUE); 
		
		qcom_gpio_pin_set(pin_zi,!TRUE);
		qcom_gpio_pin_set(pin_gold,!FALSE);

#endif

	
	
#else
		qcom_gpio_pin_set(pin_m1,FALSE);
		qcom_gpio_pin_set(pin_m2,TRUE); 
		qcom_gpio_pin_set(pin_gold,FALSE);
		qcom_gpio_pin_set(pin_zi,TRUE);

		
				
						
#endif

		A_PRINTF(" num 3 value\r\n");
	}
	else if(num == 4)
	{
#ifdef CONTROL_LOGIC_LEVEL
#if 1
		//(*((volatile A_UINT32 *)0x1402c)) =  0x1;//hw write GPIO32 to up,
		(*((volatile A_UINT32 *)0x14030)) =  0x1;//hw write GPIO32 to down,
		
		(*((volatile A_UINT32 *)0x14004)) =  0x40000000;//hw write GPIO30 to up, 
		//(*((volatile A_UINT32 *)0x14008)) =  0x40000000;//hw write GPIO30 to down,
		
		//(*((volatile A_UINT32 *)0x14004)) =  0x80000000;//hw write GPIO31 to up,
		(*((volatile A_UINT32 *)0x14008)) =  0x80000000;//hw write GPIO31 to down,
		
		(*((volatile A_UINT32 *)0x1402c)) =  0x2;//hw write GPIO33 to up,
		//(*((volatile A_UINT32 *)0x14030)) =  0x2;//hw write GPIO33 to down,
#else				
		qcom_gpio_pin_set(pin_m1,!TRUE);
		qcom_gpio_pin_set(pin_m2,!FALSE);	
		
		qcom_gpio_pin_set(pin_zi,!TRUE);
		qcom_gpio_pin_set(pin_gold,!FALSE);
#endif			
#else
				
		qcom_gpio_pin_set(pin_m1,TRUE);
		qcom_gpio_pin_set(pin_m2,FALSE);	
		qcom_gpio_pin_set(pin_gold,FALSE);
		qcom_gpio_pin_set(pin_zi,TRUE);
							
#endif

		A_PRINTF(" num 4 value\r\n");
	}
	else
	{
		A_PRINTF("undef num value\r\n");
	}
	//us_delay(100000);
	//stop_desk();
}

enum LED_STATE led_state;
int need_check_slow_blink_timeout_flag;
int tcnt;

void set_led_state(enum LED_STATE state)
{
	if(state == LED_SLOW_BLINK)
	{
		tcnt =0;
		need_check_slow_blink_timeout_flag = 1;
	}
	else
	{
		need_check_slow_blink_timeout_flag = 0;
		tcnt = 0;
		
	}
    led_state = state;
}
void   desk_led_indicate_task()
{
    int cnt;
    A_BOOL value;
    
	 if (qcom_gpio_pin_dir(pin_led, GPIO_PIN_DIR_OUTPUT) != A_OK) {
			 A_PRINTF("ERROR:set pin_led %d dir error\r\n",pin_m1);
	}

	A_PRINTF("pin_led task ok\r\n");
    cnt = 0;
	tcnt = 0;
	while(1)
	{
        if(led_state == LED_OFF)
        {
            qcom_gpio_pin_set(pin_led,FALSE);
            value = FALSE;
        }
        else if(led_state == LED_ON)
        {
            qcom_gpio_pin_set(pin_led,TRUE);
            value = TRUE;
        }
        else if(led_state == LED_SLOW_BLINK)
        {
            cnt++;
            if(cnt > 5)
            {
                cnt = 0;
                value = !value;
                qcom_gpio_pin_set(pin_led,value);
				if(need_check_slow_blink_timeout_flag)
				{
					tcnt++;
					if(tcnt > 10*12)//60s
					{
						tcnt = 0;
						reset_wifi_config();
						printf("connect route timeout:reset wifi config\r\n");
						//conwifi(1,NULL);
						qcom_sys_reset();

					}
				}
            }

        }
        else if(led_state == LED_FAST_BLINK)
        {
            cnt++;
            if(cnt > 1)
            {
                cnt = 0;
                value = !value;
                qcom_gpio_pin_set(pin_led,value);
            }
            
        }

		tx_thread_sleep(100);
	}
	
	 

}
void   desk_ir_and_light_task()
{
    #if 0
	 if (qcom_gpio_pin_dir(pin_light, GPIO_PIN_DIR_OUTPUT) != A_OK) {
			 A_PRINTF("ERROR:set pin_light %d dir error\r\n",pin_light);
	}

	 if (qcom_gpio_pin_dir(pin_in_ir1, GPIO_PIN_DIR_OUTPUT) != A_OK) {
			 A_PRINTF("ERROR:set pin_in_ir1 %d dir error\r\n",pin_in_ir1);
	}

	 if (qcom_gpio_pin_dir(pin_in_ir2, GPIO_PIN_DIR_OUTPUT) != A_OK) {
			 A_PRINTF("ERROR:set pin_in_ir2 %d dir error\r\n",pin_in_ir2);
	}

	 if (qcom_gpio_pin_dir(PIN_SCL, GPIO_PIN_DIR_OUTPUT) != A_OK) {
			 A_PRINTF("ERROR:set pin_in_ir1 %d dir error\r\n",pin_in_ir1);
	}

	 if (qcom_gpio_pin_dir(PIN_SDA, GPIO_PIN_DIR_OUTPUT) != A_OK) {
			 A_PRINTF("ERROR:set pin_in_ir2 %d dir error\r\n",pin_in_ir2);
	}
#else
 if (qcom_gpio_pin_dir(pin_light, GPIO_PIN_DIR_INPUT) != A_OK) {
		 A_PRINTF("ERROR:set pin_light %d dir error\r\n",pin_light);
}

 if (qcom_gpio_pin_dir(pin_in_ir1, GPIO_PIN_DIR_INPUT) != A_OK) {
		 A_PRINTF("ERROR:set pin_in_ir1 %d dir error\r\n",pin_in_ir1);
}

 if (qcom_gpio_pin_dir(pin_in_ir2, GPIO_PIN_DIR_INPUT) != A_OK) {
		 A_PRINTF("ERROR:set pin_in_ir2 %d dir error\r\n",pin_in_ir2);
}

 if (qcom_gpio_pin_dir(PIN_SCL, GPIO_PIN_DIR_INPUT) != A_OK) {
		 A_PRINTF("ERROR:set pin_in_ir1 %d dir error\r\n",pin_in_ir1);
}

 if (qcom_gpio_pin_dir(PIN_SDA, GPIO_PIN_DIR_INPUT) != A_OK) {
		 A_PRINTF("ERROR:set pin_in_ir2 %d dir error\r\n",pin_in_ir2);
}

#endif

	A_PRINTF("desk_ir_and_light_task task ok\r\n");

	while(1)
	{
		#if 0
		qcom_gpio_pin_set(pin_light,FALSE);
		qcom_gpio_pin_set(pin_in_ir1,FALSE);
		qcom_gpio_pin_set(pin_in_ir2,FALSE);
		qcom_gpio_pin_set(PIN_SCL,FALSE);
		qcom_gpio_pin_set(PIN_SDA,FALSE);
		A_PRINTF("+++++++++++++++++++++++++++++++++++\r\n");
		tx_thread_sleep(2000);
		
		qcom_gpio_pin_set(pin_light,TRUE);
		qcom_gpio_pin_set(pin_in_ir1,TRUE);
		qcom_gpio_pin_set(pin_in_ir2,TRUE);
		qcom_gpio_pin_set(PIN_SCL,TRUE);
		qcom_gpio_pin_set(PIN_SDA,TRUE);
		A_PRINTF("----------------------------------\r\n");
		tx_thread_sleep(2000);

		#else
		A_UINT8 light,in_ir1,in_ir2,scl,sda;
		light = qcom_gpio_pin_get(pin_light);
		in_ir1 = qcom_gpio_pin_get(pin_in_ir1);
		in_ir2= qcom_gpio_pin_get(pin_in_ir2);
		scl = qcom_gpio_pin_get(PIN_SCL);
		sda = qcom_gpio_pin_get(PIN_SDA);
		A_PRINTF("++++++light=%d,in_ir1=%d,in_ir2=%d,scl=%d,sda=%d\r\n",light,in_ir1,in_ir2,scl,sda);
		tx_thread_sleep(2000);

		#endif
		

	}
	
	 

}

void   desk_motor_task()
{
	A_UINT32 state;
	A_INT32 timeout_cnt;
	A_UINT8 ir1,ir2,ir3;
	A_UINT8 last_ir1,last_ir2,last_ir3;
	 if (qcom_gpio_pin_dir(pin_m1, GPIO_PIN_DIR_OUTPUT) != A_OK) {
			 A_PRINTF("ERROR:set pin %d dir error\r\n",pin_m1);
	}
	  if (qcom_gpio_pin_dir(pin_m2, GPIO_PIN_DIR_OUTPUT) != A_OK) {
			  A_PRINTF("ERROR:set pin %d dir error\r\n",pin_m2);
	 }
	   if (qcom_gpio_pin_dir(pin_gold, GPIO_PIN_DIR_OUTPUT) != A_OK) {
			   A_PRINTF("ERROR:set pin %d dir error\r\n",pin_gold);
	  }
		if (qcom_gpio_pin_dir(pin_zi, GPIO_PIN_DIR_OUTPUT) != A_OK) {
				A_PRINTF("ERROR:set pin %d dir error\r\n",pin_zi);
	   }
		 if (qcom_gpio_pin_dir(pin_laptop, GPIO_PIN_DIR_OUTPUT) != A_OK) {
				 A_PRINTF("ERROR:set pin %d dir error\r\n",pin_laptop);
		}

	   if (qcom_gpio_pin_dir(pin_ir1, GPIO_PIN_DIR_INPUT) != A_OK) {
			   A_PRINTF("ERROR:set pin %d dir error\r\n",pin_ir1);
	  }
		if (qcom_gpio_pin_dir(pin_ir2, GPIO_PIN_DIR_INPUT) != A_OK) {
				A_PRINTF("ERROR:set pin %d dir error\r\n",pin_ir2);
	   }
		 if (qcom_gpio_pin_dir(pin_ir3, GPIO_PIN_DIR_INPUT) != A_OK) {
				 A_PRINTF("ERROR:set pin %d dir error\r\n",pin_ir3);
		}

	init_desk();

	A_PRINTF("gpio init ok\r\n");
	timeout_cnt = 0;
	state = 0;
	last_ir1 = FALSE;last_ir2 = FALSE;last_ir3 = FALSE;
	while(1)
	{
		tx_thread_sleep(delay_polling_ms);
		
		ir1 = qcom_gpio_pin_get(pin_ir1);
		
		ir2 = qcom_gpio_pin_get(pin_ir2);
		
		ir3 = qcom_gpio_pin_get(pin_ir3);

		#ifdef CONTROL_LOGIC_LEVEL
		ir1 = !ir1;
		ir2 = !ir2;
		ir3 = !ir3;
		#endif
		
		switch(state)
		{
			case 0:
				if(ir1 && ir3)
				{
					continue;
				}
				
				if(ir1)
				{
					timeout_cnt = timeout_2s;
					state = 1;
					last_ir1 = ir1;
					A_PRINTF("ir1 pin high\r\n");
				}
				if(ir3)
				{
					timeout_cnt = timeout_2s;
					state = 1;
					last_ir3 = ir3;
					A_PRINTF("ir3 pin high\r\n");
				}
				if(ir2)
				{
					if(laptop_state == 1)
					{
						timeout_cnt = timeout_2s;
					}
					else
					{
						timeout_cnt = timeout_5s;
					}
					state = 8;
					last_ir2 = ir2;
					A_PRINTF("ir2 pin high\r\n");

				}
				break;

			case 1:
				if(last_ir1)
				{
					if(ir1 == FALSE)
					{
						last_ir1 = ir1;
						timeout_cnt = timeout_2s;
						state = 2;
						A_PRINTF("ir1 pin low\r\n");
						continue;
					}
				}

				if(last_ir3)
				{
					if(ir3 == FALSE)
					{
						last_ir3 = ir3;
						timeout_cnt = timeout_2s;
						state = 2;
						A_PRINTF("ir3 pin low\r\n");
						continue;
					}
				}

				timeout_cnt--;
				if(timeout_cnt <= 0)
				{
					state = 0;
					last_ir1 = FALSE;last_ir2 = FALSE;last_ir3 = FALSE;
					A_PRINTF("ir1 ir3 pin low check timeoout\r\n");
				}
				break;

			case 2:
				if(ir2)
				{
					last_ir2 = ir2;
					state = 3;
					timeout_cnt = timeout_2s;
					A_PRINTF("ir2 pin high\r\n");
					continue;
				}
				
				timeout_cnt--;
				if(timeout_cnt <= 0)
				{
					state = 0;
					last_ir1 = FALSE;last_ir2 = FALSE;last_ir3 = FALSE;
					A_PRINTF("ir2 pin high check timeoout\r\n");
				}

				break;
			case 3:
				if(last_ir2)
				{
					if(ir2 == FALSE)
					{
						last_ir2 = ir2;
						timeout_cnt = timeout_2s;
						state = 4;
						A_PRINTF("ir2 pin low\r\n");
						continue;
					}
				}

				timeout_cnt--;
				if(timeout_cnt <= 0)
				{
					state = 0;
					last_ir1 = FALSE;last_ir2 = FALSE;last_ir3 = FALSE;
					A_PRINTF("ir2 pin low check timeoout\r\n");
				}

				break;
				
			case 4:

				if(ir1)
				{
					last_ir1 = ir1;
					//down_desk();
					up_desk();
					state= 5;
					A_PRINTF("ir1 pin high, up desk\r\n");
					continue;
				}

				if(ir3)
				{
					last_ir3 = ir3;
					//up_desk();
					down_desk();
					state = 5;
					A_PRINTF("ir3 pin high, down desk\r\n");
					continue;
				}
				
				timeout_cnt--;
				if(timeout_cnt <= 0)
				{
					state = 0;
					last_ir1 = FALSE;last_ir2 = FALSE;last_ir3 = FALSE;
					A_PRINTF("ir1 ir3 pin high check timeoout\r\n");
				}

				break;
			case 5 :

				if(last_ir1)
				{
					if(ir1 == FALSE)
					{
						last_ir1 = ir1;
						stop_desk();
						state = 0;
						A_PRINTF("ir1 pin low, stop desk\r\n");
					}
				}

				if(last_ir3)
				{
					if(ir3 == FALSE)
					{
						last_ir3 = ir3;
						stop_desk();
						state = 0;
						A_PRINTF("ir3 pin low, stop desk\r\n");
					}
				}
				break;
			case 8 : //for laptop on
				if(last_ir2 && ir2)
				{
					timeout_cnt--;
				}
				else
				{
					last_ir2 = ir2;
					state = 0;
					A_PRINTF("lamp on or off need ir2 keep high 2 s\r\n");
				}

				if(timeout_cnt == 0)
				{
					laptop_toggle();
					state = 0;
				}
				break;
			default:
				A_PRINTF("undef state\r\n");
				break;
		}
		
	}
	
	 

}

enum MX_SIGNEL mx_sig = INVALID_XM;

#define M_INTERVAL_N_SIG_TIMER (1000)

void set_mx_sig_val(enum MX_SIGNEL sig)
{
    mx_sig = sig;
}
void MorX_signel_task()
{
    printf("enter MorX_signel_task\r\n");
    
    while(TRUE)
    {
    	if(mx_sig == M1)
    	{
    		gen_m_signal();
			tx_thread_sleep(100);
			free_desk_pin();
    		tx_thread_sleep(M_INTERVAL_N_SIG_TIMER);
			//gen_m_signal();
    		gen_1234_signal(1);
			tx_thread_sleep(100);
			free_desk_pin();

            mx_sig = INVALID_XM;
    	}
    	else if(mx_sig == M2)
    	{
    		gen_m_signal();
			tx_thread_sleep(100);
			free_desk_pin();

    		tx_thread_sleep(M_INTERVAL_N_SIG_TIMER);
    		gen_1234_signal(2);
			tx_thread_sleep(100);
			free_desk_pin();

            mx_sig = INVALID_XM;
    	}
    	else if(mx_sig == M3)
    	{
    		gen_m_signal();
			tx_thread_sleep(100);
			free_desk_pin();

    		tx_thread_sleep(M_INTERVAL_N_SIG_TIMER);
    		gen_1234_signal(3);
			tx_thread_sleep(100);
			free_desk_pin();

            mx_sig = INVALID_XM;
    	}
    	else if(mx_sig == M4)
    	{
    		gen_m_signal();
			tx_thread_sleep(100);
			free_desk_pin();

    		tx_thread_sleep(M_INTERVAL_N_SIG_TIMER);
    		gen_1234_signal(4);

			tx_thread_sleep(100);
			free_desk_pin();
            mx_sig = INVALID_XM;
    	}
    	else if(mx_sig == X1)
    	{
    		gen_1234_signal(1);
			tx_thread_sleep(100);
			free_desk_pin();

            mx_sig = INVALID_XM;
    	}
    	else if(mx_sig == X2)
    	{
    		gen_1234_signal(2);
			tx_thread_sleep(100);
			free_desk_pin();

            mx_sig = INVALID_XM;
    	}
    	else if(mx_sig == X3)
    	{
    		gen_1234_signal(3);
			tx_thread_sleep(100);
			free_desk_pin();

            mx_sig = INVALID_XM;
    	}
    	else if(mx_sig == X4)
    	{
    		gen_1234_signal(4);
			tx_thread_sleep(100);
			free_desk_pin();

            mx_sig = INVALID_XM;
    	}
    	else
    	{
    		tx_thread_sleep(100);
    	}
    }
}

A_INT32 start_desk_uart_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(desk_uart_task, 2, 2048, 80);
}

A_INT32 start_desk_motor_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(desk_motor_task, 2, 2048, 80);
}

A_INT32 start_desk_MorX_signel_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(MorX_signel_task, 2, 2048, 80);//999 最高优先级
}

A_INT32 start_desk_led_disp_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(desk_led_indicate_task, 2, 2048, 80);
}

A_INT32 start_desk_light_and_inir_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(desk_ir_and_light_task, 2, 2048, 80);
}

