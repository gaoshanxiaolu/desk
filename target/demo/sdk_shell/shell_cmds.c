/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
/*
 * Copyright (c) 2013-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
#include "qcom_gpio.h"
#include "qcom/qcom_spi.h"

extern qcom_timer_t shell_timer;

extern int v6_enabled;
extern A_UINT32 config_socket_active_close;
// wmiconfig
A_INT32 cmd_wmi_config(A_INT32 argc, A_CHAR *argv[]);

// iwconfig scan
A_INT32 cmd_iwconfig_scan(A_INT32 argc, A_CHAR *argv[]);

// bench test
A_INT32 cmd_iperf_tx_test(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_bench_tx_test(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_bench_rx_test(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_bench_quit(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_bench_mode(A_INT32 argc, A_CHAR *argv[]);
// ping
A_INT32 cmd_ping(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_ping6(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_ezxml(A_INT32 argc, A_CHAR *argv[]);
A_INT32 cmd_boot_time(A_INT32 argc, A_CHAR *argv[]);

//timer socket
extern int cmd_timer_socket_test(int argc, char *argv[]);


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

extern int coap_client_test(int argc, char **argv);
extern int coap_server_test(int argc, char **argv);

#ifdef ENABLE_MESH
extern int MeshHandler(int argc, char *argv[]);
#endif

#ifdef ENABLE_MODULE 
extern int module_handler(int argc, char *argv[]);
#endif

#ifdef ENABLE_MESH_PERF
extern int bt_user_main(int argc, char *argv[]);
extern int MeshHandler(int argc, char *argv[]);
extern int TestMeshHandler(int argc, char *argv[]);
#endif

extern int iperf_command_parser(int argc, char *argv[]);
int cmd_sleep(int argc, char *argv[]);

int cmd_csr1010_hostboot(int argc, char * argv[]);
int cmd_csr1010_reboot(int argc, char * argv[]);

console_cmd_t cust_cmds[] = {
    {.name = (A_UCHAR *) "bootcsr",
     .description = (A_UCHAR *) "boot csr1010 through spi",
     .execute = cmd_csr1010_hostboot},
     {.name = (A_UCHAR *) "rebootcsr",
     .description = (A_UCHAR *) "reboot csr1010 through spi",
     .execute = cmd_csr1010_reboot},

#ifdef ENABLE_MODULE 
    {.name = (A_UCHAR *) "insmod",
     .description = (A_UCHAR *) "install module",
     .execute = module_handler},
    {.name = (A_UCHAR *) "rmmod",
     .description = (A_UCHAR *) "unload module",
     .execute = module_handler},
    {.name = (A_UCHAR *) "lsmod",
     .description = (A_UCHAR *) "list module information",
     .execute = module_handler},
    {.name = (A_UCHAR *) "runmodcb",
     .description = (A_UCHAR *) "test module function callback",
     .execute = module_handler},
     {.name = (A_UCHAR *) "rmmodcb",
     .description = (A_UCHAR *) "Unregister module function callback",
     .execute = module_handler},
#endif
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
     {.name = (A_UCHAR *) "iperf",
      .description = (A_UCHAR *) "iperf demo",
      .execute = cmd_iperf_tx_test},

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

    {.name = (A_UCHAR *) "timer_socket",
     .description = (A_UCHAR *) "timer_socket_test",
     .execute = cmd_timer_socket_test},


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

#ifdef ENABLE_MESH 
  {.name = (A_UCHAR *) "mesh",
     .description = (A_UCHAR *) "Bluetooth mesh operations",
     .execute = MeshHandler},
#endif

#ifdef ENABLE_MESH_PERF
  {.name = (A_UCHAR *) "bluetooth",
     .description = (A_UCHAR *) "Bluetooth mesh_perf",
     .execute = bt_user_main},
  {.name = (A_UCHAR *) "mesh_perf",
     .description = (A_UCHAR *) "mesh_perf",
     .execute = TestMeshHandler},
  {.name = (A_UCHAR *) "mesh",
     .description = (A_UCHAR *) "mesh_perf",
     .execute = MeshHandler},
#endif

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

A_INT32 cmd_iperf_tx_test(A_INT32 argc, A_CHAR *argv[])
{
    return swat_iperf_handle(argc, argv);
}

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
     }else if(strcmp(argv[1],"sock_active_close")== 0){
        config_socket_active_close = 1;
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
#if 1 //!defined(ENABLE_BLUETOOTH)
    A_STATUS    retVal = 0;
#endif

    if (argc != 2) {
        return 0;
    }

    sleep_seconds = atoi(argv[1]);

#if 1 //!defined(ENABLE_BLUETOOTH)
    retVal = qcom_gpio_apply_configuration_by_pin(GPIO_CONFIG_UART2_RTS_PIN, TRUE);
    if (A_OK != retVal)
    {
        SWAT_PTF("not supported GPIO number!\n");
    }
#endif /* !ENABLE_BLUETOOTH */

    qcom_timer_stop(&shell_timer);
  
    tx_thread_sleep(sleep_seconds * TXQC_TICK_PER_SEC);
   
    qcom_timer_start(&shell_timer);

#if 1 //!defined(ENABLE_BLUETOOTH)
    qcom_gpio_apply_configuration_by_pin(GPIO_CONFIG_UART2_RTS_PIN, FALSE);
#endif    
}


extern void CSR1Kboot(void);

int cmd_csr1010_hostboot(int argc, char * argv[])
{
	SWAT_PTF("Try boot csr1010 through SPI\n");
	CSR1Kboot();

	return 0;
}

extern void CSR1Kreboot(void);
int cmd_csr1010_reboot(int argc, char *argv[])
{
	SWAT_PTF("Try reboot csr1010 through SPI\n");
	CSR1Kreboot();

	return 0;
}

