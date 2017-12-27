/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
#include "qcom_gpio.h"
#include "wx_airkiss.h"
#include "wx_heart_package.h"
#include "ble_uart.h"
#include "upgrade_task.h"
#include "csr1k_host_boot.h"
#include "csr1k_host_boot.h"
#include "chair_recv_uart_task.h"
extern qcom_timer_t shell_timer;

extern int v6_enabled;
// wmiconfig
A_INT32 cmd_wmi_config(A_INT32 argc, A_CHAR *argv[]);

// iwconfig scan
A_INT32 cmd_iwconfig_scan(A_INT32 argc, A_CHAR *argv[]);

// bench test
A_INT32 cmd_bench_tx_test(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_bench_rx_test(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_bench_quit(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_bench_mode(A_INT32 argc, A_CHAR *argv[]);
// ping
A_INT32 cmd_ping(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_ping6(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_ezxml(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_boot_time(A_INT32 argc, A_CHAR *argv[]);


extern int cmd_i2c_test(int argc, char *argv[]);
extern int cmd_i2s_test(int argc, char *argv[]);
extern int cmd_gpio_ir_test(int argc, char *argv[]);
extern int cmd_adc_test(int argc, char *argv[]);
extern int cmd_uart_wakeup_set(int argc, char *argv[]);
extern int cmd_uart_wakeup_test(int argc, char *argv[]);
extern int cmd_uart_baudrate_configure(int argc, char *argv[]);
#ifdef ENABLE_BLUETOOTH 
extern int bluetooth_server_handler(int argc, char *argv[]);
#endif

#ifdef ENABLE_AIRKISS 
extern int do_airkiss(int argc, char *argv[]);
#endif
int cmd_csr1010_hostboot(int argc, char * argv[]);

int cmd_sleep(int argc, char *argv[]);

console_cmd_t cust_cmds[] = {
#ifdef SWAT_PING
    {.name = (A_UCHAR *) "ping",
     .description = (A_UCHAR *) "ping",
     .execute = cmd_ping},
    {.name = (A_UCHAR *) "ping6",
     .description = (A_UCHAR *) "ping6",
     .execute = cmd_ping6},
#endif

    {.name = (A_UCHAR *) "ezxml",
     .description = (A_UCHAR *) "ezxml",
     .execute = cmd_ezxml},

    {.name = (A_UCHAR *) "hostless_boot_time",
     .description = (A_UCHAR *) "hostless_boot_time",
     .execute = cmd_boot_time},

    /* Used to Auto Test */
#if defined(SWAT_WMICONFIG)
    {.name = (A_UCHAR *) "wmiconfig",
     .description = (A_UCHAR *) "wmiconfig cmd args",
     .execute = cmd_wmi_config},
    {.name = (A_UCHAR *) "iwconfig",
     .description = (A_UCHAR *) "scan for APs and specified SSID",
     .execute = cmd_iwconfig_scan},
#endif

#if defined(SWAT_BENCH)
    {.name = (A_UCHAR *) "benchtx",
     .description = (A_UCHAR *) "run the traffic transmit test",
     .execute = cmd_bench_tx_test},
    {.name = (A_UCHAR *) "benchrx",
     .description = (A_UCHAR *) "run the traffic receive test",
     .execute = cmd_bench_rx_test},
    {.name = (A_UCHAR *) "benchquit",
     .description = (A_UCHAR *) "quit the bench test",
     .execute = cmd_bench_quit},
     {.name = (A_UCHAR *) "benchmode",
     .description = (A_UCHAR *) "set benchmode v4/v6",
     .execute = cmd_bench_mode},

    /* {.name = (A_UCHAR *) "benchdbg", */
     /* .description = (A_UCHAR *) "dbg the bench test", */
     /* .execute = swat_benchdbg_handle}, */
#endif

#ifdef SWAT_SLEEP
    {.name = (A_UCHAR *) "sleep",
     .description = (A_UCHAR *) "sleep num_seconds",
     .execute = cmd_sleep},
#endif
#ifdef SWAT_SSL
  {.name = (A_UCHAR *) "getcert",
     .description = (A_UCHAR *) "get certificate",
     .execute = ssl_get_cert_handler},
#endif
#if defined(AR6002_REV74)
#ifdef SWAT_I2C
/* i2c */

  {.name = (A_UCHAR *) "i2c",
     .description = (A_UCHAR *) "i2c read/write",
     .execute = cmd_i2c_test},
#endif 
#endif /*if defined(AR6002_REV74)*/
#ifdef SWAT_I2S
  {.name = (A_UCHAR *) "i2s",
     .description = (A_UCHAR *) "i2s test",
     .execute = cmd_i2s_test},
#endif
//#ifdef SWAT_GPIO_IR
  {.name = (A_UCHAR *) "gpioir",
     .description = (A_UCHAR *) "gpio ir test",
     .execute = cmd_gpio_ir_test},
//#endif

#ifdef SWAT_UART_WAKEUP
  {.name = (A_UCHAR *) "uart",
     .description = (A_UCHAR *) "uart wake up",
     .execute = cmd_uart_wakeup_set},
  {.name = (A_UCHAR *) "uartwakeup",
     .description = (A_UCHAR *) "uart wake up test demo",
     .execute = cmd_uart_wakeup_test},
  {.name = (A_UCHAR *) "setbaudrate",
     .description = (A_UCHAR *) "set uart baudrate",
     .execute = cmd_uart_baudrate_configure},
#endif

#ifdef SWAT_ADC
  {.name = (A_UCHAR *) "adc",
     .description = (A_UCHAR *) "adc test",
     .execute = cmd_adc_test},
#endif

#ifdef ENABLE_HTTPS_CLIENT  
  {.name = (A_UCHAR *) "httpsc",
     .description = (A_UCHAR *) "https client",
     .execute = https_client_handler},
#endif
#ifdef ENABLE_HTTPS_SERVER 
  {.name = (A_UCHAR *) "httpss",
     .description = (A_UCHAR *) "https server",
     .execute = https_server_handler},
#endif

#ifdef ENABLE_BLUETOOTH 
  {.name = (A_UCHAR *) "bluetooth",
     .description = (A_UCHAR *) "Bluetooth SPP/SPPLE server",
     .execute = bluetooth_server_handler},
#endif

#ifdef ENABLE_AIRKISS 
  {.name = (A_UCHAR *) "airkiss",
     .description = (A_UCHAR *) "weixin airkiss",
     .execute = do_airkiss},
#endif

#ifdef ENABLE_AIRKISS 
  {.name = (A_UCHAR *) "conwifi",
     .description = (A_UCHAR *) "connect to your wifi",
     .execute = conwifi},
#endif

	{.name = (A_UCHAR *) "upgrade",
	   .description = (A_UCHAR *) "upgrade cmd",
	   .execute = mannual_start_upgrade_task},

	{.name = (A_UCHAR *) "desk_test",
	   .description = (A_UCHAR *) "desk_test cmd",
	   .execute = desk_test},

	{.name = (A_UCHAR *) "set_id",
	   .description = (A_UCHAR *) "set dev id cmd",
	   .execute = set_dev_id},

	{.name = (A_UCHAR *) "clear_net",
	   .description = (A_UCHAR *) "set dev id cmd",
	   .execute = clear_net},

{.name = (A_UCHAR *) "bootcsr", 	
	.description = (A_UCHAR *) "boot csr1010 through spi",	   
	.execute = cmd_csr1010_hostboot},

/*{.name = (A_UCHAR *) "scs", 	
	.description = (A_UCHAR *) "send light on off",	   
	.execute = cmd_update_chair_status},*/

};

A_INT32 cust_cmds_num = sizeof (cust_cmds) / sizeof (console_cmd_t);


#ifdef SWAT_PING
A_INT32 cmd_ping(A_INT32 argc, A_CHAR *argv[])
{
    return swat_ping_handle(argc, argv,0);
}
A_INT32 cmd_ping6(A_INT32 argc, A_CHAR *argv[])
{
    return swat_ping_handle(argc, argv,1);
}

#endif

A_INT32 cmd_boot_time(A_INT32 argc, A_CHAR *argv[])
{
    return swat_boot_time(argc, argv);
}


A_INT32 cmd_ezxml(A_INT32 argc, A_CHAR *argv[])
{
    return swat_ezxml_handle(argc, argv);
}

/* For Auto Test */
#if defined(SWAT_WMICONFIG)
A_INT32 cmd_wmi_config(A_INT32 argc, A_CHAR *argv[])
{
    return swat_wmiconfig_handle(argc, argv);
}

A_INT32 cmd_iwconfig_scan(A_INT32 argc, A_CHAR *argv[])
{
    return swat_iwconfig_scan_handle(argc, argv);
}
#endif

#if defined(SWAT_BENCH)
A_INT32 cmd_bench_tx_test(A_INT32 argc, A_CHAR *argv[])
{
    return swat_benchtx_handle(argc, argv);
}

A_INT32 cmd_bench_rx_test(A_INT32 argc, A_CHAR *argv[])
{
    return swat_benchrx_handle(argc, argv);
}

A_INT32 cmd_bench_quit(A_INT32 argc, A_CHAR *argv[])
{
     return swat_benchquit_handle(argc, argv);
}
A_INT32 cmd_bench_mode(A_INT32 argc, A_CHAR *argv[])
{
     if (argc != 2) {
         goto error;				 
     }

     if(strcmp(argv[1],"v4") == 0){
         v6_enabled = 0;
     }else if(strcmp(argv[1],"v6")== 0){
         v6_enabled = 1;
     }else{
         goto error;
     }

     return 0;
		 
error:
     SWAT_PTF("invalid arguments : enter v4/v6 \n");
     return 1;
}

#endif

int
cmd_sleep(int argc, char *argv[])
{
    int sleep_seconds;
#if !defined(ENABLE_BLUETOOTH)
    A_STATUS    retVal = 0;
#endif

    if (argc != 2) {
        return 0;
    }

    sleep_seconds = atoi(argv[1]);

#if !defined(ENABLE_BLUETOOTH)
    retVal = qcom_gpio_apply_configuration_by_pin(GPIO_CONFIG_UART2_RTS_PIN, TRUE);
    if (A_OK != retVal)
    {
        SWAT_PTF("not supported GPIO number!\n");
    }
#endif /* !ENABLE_BLUETOOTH */

    qcom_timer_stop(&shell_timer);
  
    tx_thread_sleep(sleep_seconds * TXQC_TICK_PER_SEC);
   
    qcom_timer_start(&shell_timer);

#if !defined(ENABLE_BLUETOOTH)
    qcom_gpio_apply_configuration_by_pin(GPIO_CONFIG_UART2_RTS_PIN, FALSE);
#endif    
}

extern void CSR1Kboot(void);
int cmd_csr1010_hostboot(int argc, char * argv[])
	{	SWAT_PTF("Try boot csr1010 through SPI\n");	
	CSR1Kboot();	
	return 0;
	}

