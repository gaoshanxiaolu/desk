/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <fwconfig_AR6006.h>
#include <qcom/qcom_common.h>
#include <qcom/qcom_gpio.h>
#include <qcom/qcom_ir_ctrl.h>

#include "threadx/tx_api.h"
#include "threadx/tx_thread.h"

#include "swat_parse.h"

qcom_gpio_interrupt_info_t	gpio_interrupt;

#define TICK2TIME(t)	(((t)*61)/2)
#define MAX_TS_SIZE		(32*8)
//this timer 200ms should be different if choosing different RC
#define RC_TIMER	200

static int ir_timer_started=0;
qcom_timer_t gpio_IR_timer;
qcom_timer_t gpio_speedctrl_timer;

void gpio_IR_timer_handler(unsigned int alarm, void *arg)
{
	int i=0;
	A_UINT32 cnt;
	A_UINT32 temp_ts;
	A_UINT32 ts[MAX_TS_SIZE]={0};

	ir_timer_started = 0; //clear it

	cnt = qcom_get_ir_intr_ts(ts, sizeof(ts));

	SWAT_PTF("\n=======START cnt=%d========\n", cnt);
	for(i=0;i<cnt;i++)
	{
		

		if(TICK2TIME(ts[i]) == 0)
			qcom_printf("i=%d, ts=%d\n", i, ts[i]);
			//break;
		if((ts[i]!=0) && (temp_ts!=0) ){
			if(((TICK2TIME(ts[i])-TICK2TIME(temp_ts)) > 990) && ((TICK2TIME(ts[i])-TICK2TIME(temp_ts)) < 1500))
				qcom_printf(" 0 ");
			else if(((TICK2TIME(ts[i])-TICK2TIME(temp_ts)) > 1800) && ((TICK2TIME(ts[i])-TICK2TIME(temp_ts)) < 2800))
				qcom_printf(" 1 ");
			else
				qcom_printf("-%d-",(TICK2TIME(ts[i])-TICK2TIME(temp_ts)));
		}
		temp_ts=ts[i];
	}

	
	SWAT_PTF("\n=======END INT=%d========\n", i);
	
}
void test_gpio_ir_handler()
{
	static int ir_cnt=0;
	if(ir_timer_started==0)
		ir_cnt++;
	if(ir_cnt>=1 && ir_timer_started==0){
		/*Timer should be calcaulated based on the real RC signal.*/
		qcom_timer_init(&gpio_IR_timer, gpio_IR_timer_handler, NULL, RC_TIMER, ONESHOT/*PERIODIC*/);
		qcom_timer_start(&gpio_IR_timer);
		ir_timer_started = 1;
		ir_cnt=0;
	}
	
}


A_UINT32 test_gpio_cb(void *arg)
{
	if(atoi((char*)arg)==1){
		qcom_printf(".");
		return CB_ISR_COMPLETE;
		}
	qcom_printf("^");
	return CB_ISR_CONTINUE;
	
}
A_UINT32 test_gpio_pull_low(A_UINT32 pin, A_UINT32 ms);
A_UINT32 test_gpio_pull_high(A_UINT32 pin, A_UINT32 ms);
int us_test=1000;
int time_us = 100;
int pin_state = 0;
int testPin=31;
TX_SEMAPHORE test_isr_sem;
void test_gpio_int(void *arg)
{
	#if 1
	qcom_gpio_pin_set(testPin, pin_state);
	#else
	/* This direct I/O write would be faster in 3~5us */
	{
		if(pin_state)
			(*((volatile A_UINT32 *)(0x14000))) |= (A_UINT32)(0x80000000);
		else
			(*((volatile A_UINT32 *)(0x14000))) &= (A_UINT32)(0x7fffffff);
	}
	#endif
	pin_state ^=0x1;
	qcom_hf_timer_start(test_gpio_int, NULL, time_us, ONESHOT);
}
void gpio_speedctrl_timer_handler(unsigned int alarm, void *arg)
{
	qcom_cust_speed_ctrl(18, us_test, 2);
}

void test_isr_timer_handler(void *arg)
{

	int i=0;
	for(i=0;i<1;i++)
	{
		qcom_gpio_pin_set(18, pin_state);
		pin_state ^=0x1;
		qcom_tx_semaphore_put(&test_isr_sem);
		//qcom_gpio_pin_set(18, 0);	
	}
}

TX_EVENT_FLAGS_GROUP test_timer_event; 

void gpio_test_timer_handler(unsigned int alarm, void *arg)
{
	int i;
	for(i=0;i<1;i++)
	{
		qcom_gpio_pin_set(32, 1);
		//us_delay(timer_debug1);
		qcom_gpio_pin_set(32, 0);	

        tx_event_flags_set(&test_timer_event, 0x1, TX_OR); 
	}
}

void normal_timer_task_test()
{
	int timer_ret;
	ULONG flags;
	while(1)
	{
		if( TX_SUCCESS == (timer_ret = tx_event_flags_get(&test_timer_event, 0x1, TX_OR_CLEAR, &flags, TX_WAIT_FOREVER))) {
			qcom_printf("#");
		} 
		else
		{
			qcom_printf("timer Failed\n");
		}
		
	}
} 

qcom_gpio_isr_info_t gpio_isr={0};
qcom_timer_t gpio_test_timer;

extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
void isr_timer_task_test()
{
	int isr_ret;
	while(1)
	{
		if( TX_SUCCESS == (isr_ret = qcom_tx_semaphore_get(&test_isr_sem,TX_WAIT_FOREVER))) {
            qcom_printf(".");
		} 
		
		qcom_thread_msleep(1);
	}
}

int cmd_gpio_ir_test(int argc, char *argv[])
{
	
	int enableFlag=0;
	if(argc <2)
    	{
	        qcom_printf("GPIO IR test for Media\n");
	        return -1;
   	}
	if (!A_STRCMP(argv[1], "--hftimer"))
	{
		if(argv[2])
			enableFlag = atoi(argv[2]);
		qcom_printf("test hf-time timer\n");
		if(enableFlag){
			qcom_gpio_pin_dir(testPin, 0);//output
			qcom_gpio_pin_mux(testPin, 0);
			qcom_hf_timer_install();
			if(qcom_hf_timer_start(test_gpio_int, NULL, time_us, ONESHOT) == 0)
				return SWAT_OK;
			else
				qcom_printf("hftimer set timeout >1 second. it should be <=1000*1000 microSeconds\n");
		}
		else{
			qcom_hf_timer_stop();
			qcom_hf_timer_uninstall();
			}
	}
	if (!A_STRCMP(argv[1], "--isrtimer"))
	{
		
		if(argv[2])
			enableFlag = atoi(argv[2]);
		qcom_printf("test real-time timer 8\n");
		if(enableFlag){
			qcom_task_start(isr_timer_task_test, 0, 2048, 10);

            tx_event_flags_create(&test_timer_event, "timer_event");
			qcom_task_start(normal_timer_task_test, 0, 1024, 10); 

			tx_semaphore_create(&test_isr_sem, "isr_sem", 0);
			
			qcom_gpio_pin_dir(18, 0);//output
			qcom_gpio_pin_mux(18, 0);
			qcom_isr_timer_start(test_isr_timer_handler, NULL, time_us, PERIODIC);

			qcom_gpio_pin_dir(32, 0);//output
			qcom_gpio_pin_mux(32, 0);
			qcom_timer_init(&gpio_test_timer, gpio_test_timer_handler, NULL, 20, PERIODIC/*PERIODIC*/);
			qcom_timer_start(&gpio_test_timer);
			}
		else{
			qcom_isr_timer_stop();
			qcom_timer_stop(&gpio_test_timer);
			}

	}
	if (!A_STRCMP(argv[1], "--exit"))
	{
		qcom_printf("exit ir config\n");
		qcom_ir_ctrl_uninstall();
		qcom_gpio_interrupt_deregister(&gpio_interrupt);
	}
	if (!A_STRCMP(argv[1], "--isrcb"))
	{
		char *arg_ret=argv[2];
		unsigned long u_time=0;
		u_time = qcom_time_us();
		qcom_printf("utime=%ld, mtime=%ld\n", u_time, time_ms());
		
		if(argv[3])
			gpio_isr.pin = atoi(argv[3]);
		else
			gpio_isr.pin = 18;
		gpio_isr.gpio_isr_handler_fn = test_gpio_cb;
		gpio_isr.arg = arg_ret;
		qcom_printf("isr cb handler\n");
		qcom_isr_handler_install(&gpio_isr);


		if (qcom_gpio_pin_dir(gpio_isr.pin, TRUE) != A_OK) {
			SWAT_PTF("ERROR:set pin dir error\n");
		}
	
		gpio_interrupt.pin = gpio_isr.pin;
		gpio_interrupt.gpio_pin_int_handler_fn = test_gpio_int;
		gpio_interrupt.arg = NULL;
		if (qcom_gpio_interrupt_register(&gpio_interrupt) != A_OK) {
			SWAT_PTF("ERROR:gpio interrupt register error\n");
		}

		if (qcom_gpio_interrupt_mode(&gpio_interrupt, QCOM_GPIO_PIN_INT_FALLING_EDGE) != A_OK) {
			SWAT_PTF("ERROR:gpio interrupt mode error\n");
		}
		
		qcom_gpio_interrupt_start(&gpio_interrupt);
		
        
        return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--delay"))
	{
	/*extern void qcom_time_delay_us(unsigned int us);
		unsigned long u_time=0;
		unsigned long m_time=0;*/
		
		if(argv[2]){
			if (!A_STRCMP(argv[2], "exit"))
			{
				qcom_timer_stop(&gpio_speedctrl_timer);
				qcom_hf_timer_uninstall();
				return SWAT_OK;
			}
			if(atoi(argv[2]) == 1)
				us_test=100;
			if(atoi(argv[2]) == 2)
				us_test=200;
			if(atoi(argv[2]) == 3)
				us_test=10;
		}
		qcom_gpio_pin_mux(18, 0);
		qcom_gpio_pin_dir(18, 0);
		
		qcom_hf_timer_install();

		qcom_timer_init(&gpio_speedctrl_timer, gpio_speedctrl_timer_handler, NULL, 10, PERIODIC/*PERIODIC*/);
		qcom_timer_start(&gpio_speedctrl_timer);
		
	}
	if (!A_STRCMP(argv[1], "--config")){
		int gpio_pin;

		if (argc>2){
			gpio_pin = atoi(argv[2]);
			if(gpio_pin<0 || gpio_pin>40)
			{
				SWAT_PTF("gpio pin is not correct\n");
				return SWAT_OK;
			}			
		}
		else{
			SWAT_PTF("gpioir need correct parameters\n");
			return SWAT_OK;
		}

		qcom_ir_ctrl_install();
			
		SWAT_PTF("Pin[%d] \n", gpio_pin);

		qcom_config_ir(gpio_pin);
		if (qcom_gpio_pin_dir(gpio_pin, TRUE) != A_OK) {
			SWAT_PTF("ERROR:set pin dir error\n");
		}
	
		gpio_interrupt.pin = gpio_pin;
		gpio_interrupt.gpio_pin_int_handler_fn = test_gpio_ir_handler;
		gpio_interrupt.arg = NULL;
		if (qcom_gpio_interrupt_register(&gpio_interrupt) != A_OK) {
			SWAT_PTF("ERROR:gpio interrupt register error\n");
		}

		if (qcom_gpio_interrupt_mode(&gpio_interrupt, QCOM_GPIO_PIN_INT_FALLING_EDGE) != A_OK) {
			SWAT_PTF("ERROR:gpio interrupt mode error\n");
		}
		
		qcom_gpio_interrupt_start(&gpio_interrupt);
		
        
        return SWAT_OK;
    }
	return;
}



