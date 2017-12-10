/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

//#include "qcom_system.h"
#include "qcom_common.h"
#include "threadx/tx_api.h"
#include "qcom_cli.h"
#include "threadxdmn_api.h"
#include "qcom_wps.h"
#include "qcom_gpio.h"
#include "qcom_gpio_interrupts.h"
#include "ad_key.h"
#include "ble_uart.h"
#include "wx_heart_package.h"
#include "i2c_test.h"
#include "watch_dog_task.h"
#include "upgrade_task.h"
#include "chair_recv_uart_task.h"
#include "wx_airkiss.h"

//#include "os/misc_api.h"

TX_THREAD host_thread;
#ifdef REV74_TEST_ENV4

#define BYTE_POOL_SIZE (2*1024 + 128 )
#define PSEUDO_HOST_STACK_SIZE (2 * 1024 )   /* small stack for pseudo-Host thread */

#else

#define BYTE_POOL_SIZE (4*1024 + 256 )
#define PSEUDO_HOST_STACK_SIZE (4 * 1024 )   /* small stack for pseudo-Host thread */

#endif
TX_BYTE_POOL pool;

void qcom_wps_event_process_cb(A_UINT8 ucDeviceID, QCOM_WPS_EVENT_TYPE uEventID, 
                    qcom_wps_event_t *pEvtBuffer, void *qcom_wps_event_handler_arg);

typedef struct shell_gpio_info_s {
    unsigned int pin_num;
    unsigned int num_configs;
    gpio_pin_peripheral_config_t    *configs;
} shell_gpio_info_t;

A_UINT32    pin_number;

void shell_add_gpio_configurations(void)
{
    A_UINT32    i = 0;

    /* Currently only one dynamic configuration is good enough as GPIO pin 2 
     * is used by both UART and I2S. UART configuration is already provided 
     * as part of tunable input text and only I2S configuration needs to be 
     * added here. None of GPIO configurations are added here as this demo 
     * doesnt use GPIO configurations. If customer applications requires it, 
     * it can be added here */
    shell_gpio_info_t    pin_configs[] = {
        { 2, 1, (gpio_pin_peripheral_config_t [1]) { { 5, 0x80001800 } } },
    };

    for (i = 0; i < sizeof(pin_configs)/sizeof(shell_gpio_info_t); i++)
    {
        if (A_OK != qcom_gpio_add_alternate_configurations(pin_configs[i].pin_num, 
                        pin_configs[i].num_configs, pin_configs[i].configs))
        {
            A_PRINTF("qcom_add_config failed for pin %d\n", pin_configs[i].pin_num);
        }
    }

    return;
}


void
shell_host_entry(ULONG which_thread)
{
    #define PRINTF_ENBALE 1
    extern void user_pre_init(void);
    user_pre_init();
    qcom_enable_print(PRINTF_ENBALE);

    extern console_cmd_t cust_cmds[];

    extern int cust_cmds_num;

    extern void task_execute_cli_cmd();

	#if defined (AR6002_REV74)

	extern void install_patches_iot_forapp();

	install_patches_iot_forapp();

	#endif

    console_setup();

    console_reg_cmds(cust_cmds, cust_cmds_num);

    //A_PRINTF("cli started ---------------\n");

    /* Enable WPS and register event handler */
    /* THIS IS NOT THE RIGHT PLACE FOR THIS REGISTRATION. THIS API
     * SHOULD BE CALLED ALONG WITH qcom_wps_enable() WHEN enable is
     * set to 1.*/
    #if defined(AR6002_REV74)
    qcom_wps_register_event_handler(qcom_wps_event_process_cb, NULL);
    #endif
		ioe_wifi_config_get();
		ad_key_app();
		//conwifi(1,NULL);
#if defined(BLE_UART)
		start_smart_chair_socket_tx_app(1,NULL);
		start_smart_chair_gw_uart_app(1,NULL);
		start_desk_motor_app(1,NULL);
		start_desk_uart_app(1,NULL);
        start_desk_MorX_signel_app(1,NULL);
        start_desk_led_disp_app(1,NULL);
        start_desk_socket_app(1,NULL);
		//start_desk_light_and_inir_app(1,NULL);
		start_dht12_app(1,NULL);
		start_watch_dog_task(1,NULL);
		start_upgrade_task(1,NULL);
		start_upgrade_ble_task(1,NULL);
		printf(">>>>>>>>desk version v%d.%d<<<<<<<<<<<<\r\n",SMART_DESK_MAIN_V,SMART_DESK_SECOND_V);
#endif

    /* Add all the alternate configurations for each GPIO pin */
    //shell_add_gpio_configurations();
    
    task_execute_cli_cmd();
    /* Never returns */
}


void user_main(void)
{   
    extern void task_execute_cli_cmd();
    tx_byte_pool_create(&pool, "cdrtest pool", TX_POOL_CREATE_DYNAMIC, BYTE_POOL_SIZE);

    {
        CHAR *pointer;
        tx_byte_allocate(&pool, (VOID **) & pointer, PSEUDO_HOST_STACK_SIZE, TX_NO_WAIT);

        tx_thread_create(&host_thread, "cdrtest thread", shell_host_entry,
                         0, pointer, PSEUDO_HOST_STACK_SIZE, 16, 16, 4, TX_AUTO_START);
    }

    cdr_threadx_thread_init();
}

