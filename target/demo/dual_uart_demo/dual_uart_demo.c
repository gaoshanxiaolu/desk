/*
  * Copyright (c) 2014 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "qcom_p2p_api.h"
#include "qcom_wps.h"
#include "threadx/tx_api.h"
#include "threadxdmn_api.h"
#include "qcom_uart.h"
#include "select_api.h"

TX_THREAD host_thread;
TX_THREAD dual_uart_demo_cmd;

TX_MUTEX mutex_tsk;
#define TSK_SEM_INIT tx_mutex_create(&mutex_tsk, "mutex tsk", TX_NO_INHERIT);
#define TSK_SEM_LOCK {int status =  tx_mutex_get(&mutex_tsk, TX_WAIT_FOREVER); \
  if (status != TX_SUCCESS) {\
    A_PRINTF("lock the task mutex failed !!!\n");\
  }\
}
    
#define TSK_SEM_UNLOCK { int status =  tx_mutex_put(&mutex_tsk);\
  if (status != TX_SUCCESS) {\
    A_PRINTF("unlock the task mutex failed !!!\n");\
  }\
}

#define BYTE_POOL_SIZE (5*1024)
#define PSEUDO_HOST_STACK_SIZE (4 * 1024)   /* small stack for pseudo-Host thread */
#define uart_test_str0 "hello uart0\n"
#define uart_test_str1 "hello uart1\n"
#define uart_test_str2 "hello uart2\n"
#define uart_buffer_size 512
#define DUAL_UART_CMD_BUFFER_LEN 20

#define ATH_CIPHER_TYPE_TKIP 0x04
#define ATH_CIPHER_TYPE_CCMP 0x08
#define ATH_CIPHER_TYPE_WEP  0x02
#define ATH_CIPHER_TYPE_NONE  0x00

#define SECURITY_AUTH_NONE    0x00
#define SECURITY_AUTH_PSK    0x01
#define SECURITY_AUTH_1X     0x02

#define MAX_TASK_NUM 20

#define UART0_TX_BUFFER_DEFAULT_SIZE        2048
#define UART1_TX_BUFFER_DEFAULT_SIZE        2048
#define UART2_TX_BUFFER_DEFAULT_SIZE        (1024 * 12)

TX_THREAD* thr[MAX_TASK_NUM];
char *thrStack[MAX_TASK_NUM];


TX_BYTE_POOL pool;
char *dual_uart_buffer;
int dual_uart_cmd_offset = 0;
int configure_baudrate_value = 0;
int configure_powermode_value = 0;
int configure_deviceid_value = 0;
A_UINT8 configure_baudrate_flag = 0;
A_UINT8 configure_pwrmode_flag = 0;
A_UINT8 configure_deviceid_flag = 0;
A_UINT8 configure_scan_flag = 0;
A_UINT8 configure_scan_all_flag = 0;
A_CHAR ssid_dualuart_demo[DUAL_UART_CMD_BUFFER_LEN];
A_BOOL scan_task_running = FALSE;

/*
* User should configure the appropriate tx/rx pins 
* for uart0 and uart1 based on their HW design. 
* The two uart will be opened for read and write, 
* receive data from each uart and echo the data to user. 
*/

static int Get_Scanning_Uart_Tx_Ringbuffer_Threshold(int phys_uart_id)
{
    int result = 10000;
    int baudrate = 115200;
    qcom_uart_para com_uart_cfg;

    if(phys_uart_id == 0)
    {
        qcom_get_uart_config((A_CHAR *)"UART0",&com_uart_cfg);
    }
    else if(phys_uart_id == 1)
    {
        qcom_get_uart_config((A_CHAR *)"UART1",&com_uart_cfg);
    }
    else if(phys_uart_id == 2)
    {
        qcom_get_uart_config((A_CHAR *)"UART2",&com_uart_cfg);
    }
    baudrate = com_uart_cfg.BaudRate;

    switch(baudrate)
    {
        case 4800: result = 400; break;
        case 9600: result = 800; break;
        case 19200: result = 1200; break;
        case 38400: result = 1600; break;
        case 115200: result = 10000; break;
    }
    return result;
    
}

void dual_uart_demo_subtask_exit()
{
    if ((thr[0]) && (thrStack[0]))
    {
        tx_thread_delete(thr[0]);
    }
    TSK_SEM_LOCK;
    if ((thr[0]) && (thrStack[0]))
    {
        mem_free(thr[0]);
        mem_free(thrStack[0]);
        thr[0] = 0;
        thrStack[0] = 0;
    }
    TSK_SEM_UNLOCK;
    scan_task_running = FALSE;
    tx_thread_terminate(&dual_uart_demo_cmd);
    
}

A_INT32 process_scan_cmd(A_INT8 scan_all, A_CHAR * ssid)
{
    A_CHAR * pssid;
    QCOM_BSS_SCAN_INFO* pOut;
    A_UINT16 count = 0;
    A_INT32 result = 0;
    A_CHAR saved_ssid[WMI_MAX_SSID_LEN+1];

    A_MEMSET(saved_ssid, 0, WMI_MAX_SSID_LEN+1);
    qcom_get_ssid(configure_deviceid_value, saved_ssid);

    if(scan_all == 1)
    {
        pssid = "";
    }
    else
    {
        pssid = ssid;
    }

    qcom_set_ssid(configure_deviceid_value, pssid);

    qcom_start_scan_params_t scanParams;
    scanParams.forceFgScan  = 1;
    scanParams.scanType     = WMI_LONG_SCAN;
    scanParams.numChannels  = 0;
    scanParams.forceScanIntervalInMs = 1;
    scanParams.homeDwellTimeInMs = 0;       
    qcom_set_scan(configure_deviceid_value, &scanParams);

    if(qcom_get_scan(configure_deviceid_value, &pOut, &count) == A_OK)
    {        
        result = 1;
        A_PRINTF("scan complete, found %d APs!\n", count);
    }

    qcom_set_ssid(configure_deviceid_value, saved_ssid);
    memset(ssid_dualuart_demo, 0, DUAL_UART_CMD_BUFFER_LEN);
    return result;
}


void task_process_scan_cmd(A_UINT32 scan_all)
{
    process_scan_cmd(scan_all, ssid_dualuart_demo);
    /* Thread Delete */
    dual_uart_demo_subtask_exit();
}

void create_uart_cmd_task(A_UINT32 scan_all)
{
    void *stk = NULL;
    void *pthr = NULL;
    int ret = 0;

    stk = mem_alloc(1024);
    if (stk == NULL) 
    {
        A_PRINTF("malloc stack failed.\n");
        ret = -1;
        return;
    }
    pthr = mem_alloc(sizeof (TX_THREAD));
    if (NULL == pthr) 
    {
        A_PRINTF("malloc thr failed.\n");
        ret = -1;
        return;
    }
    
    TSK_SEM_LOCK;
    thrStack[0] = stk;
    thr[0] = pthr;
    TSK_SEM_UNLOCK;

    ret = tx_thread_create(&dual_uart_demo_cmd, "dual_uart_demo_cmd thread", (void (*)(unsigned long))task_process_scan_cmd,
                     scan_all, stk, 1024, 16, 16, 4, TX_AUTO_START);

    if(ret != 0)
    {
        if (pthr != NULL)
        {
            mem_free(pthr);
        }
        if (stk != NULL) 
        {
            mem_free(stk);
        }
        TSK_SEM_LOCK;
        thrStack[0] = NULL;
        thr[0] = NULL;
        TSK_SEM_UNLOCK;
    }
    
}

A_BOOL process_dual_uart_demo_character(A_CHAR character)
{
    A_BOOL result  = TRUE;
    if(!dual_uart_buffer)
    {
        dual_uart_buffer = mem_alloc(DUAL_UART_CMD_BUFFER_LEN);
        memset(dual_uart_buffer,0,DUAL_UART_CMD_BUFFER_LEN);
    }

    if((character =='\n')
        || (character =='\r')
        || (dual_uart_cmd_offset >= (DUAL_UART_CMD_BUFFER_LEN -1)))
    {
        if(dual_uart_cmd_offset == 0)
        {
            if(configure_scan_flag == 1)
            {
                configure_scan_all_flag = 1;
            }
            else
            {
                A_PRINTF("\nno value input!\n");
                result = FALSE;
            }
        }
        else
        {
            //terminate command string with NULL character
            *( (char*)dual_uart_buffer + dual_uart_cmd_offset) ='\0';

            //process baudate set
            if(configure_baudrate_flag == 1)
            {
                configure_baudrate_value = atoi(dual_uart_buffer);
                switch(configure_baudrate_value)
                {
                    case 600: break;
                    case 1200: break;
                    case 2400: break;
                    case 4800: break;
                    case 9600: break;
                    case 19200: break;
                    case 38400: break;
                    case 57600: break;
                    case 115200: break;
                    case 230400: break;
                    case 380400: break;
                    case 460800: break;
                    case 921600: break;
                    case 1843200: break;
                    case 2000000: break;
                    case 3000000: break;
                    default:
                    {
                        configure_baudrate_value = 0;
                        A_PRINTF("\nplease input a valid baudrate!\n");
                        result = FALSE;
                    }
                }                

                if(configure_baudrate_value != 0)
                {
                    A_PRINTF("\nconfigure uart baudrate %d!\n", configure_baudrate_value);
                }                
            }

            //process powermode set
            else if(configure_pwrmode_flag == 1)
            {
                if(!A_STRCMP(dual_uart_buffer, "0"))
                {
                    configure_powermode_value = 2;
                    A_PRINTF("\nconfigure max performance power mode!\n");
                }
                else if(!A_STRCMP(dual_uart_buffer, "1"))
                {
                    configure_powermode_value = 1;
                    A_PRINTF("\nconfigure REC power mode!\n");
                }
                else
                {
                    A_PRINTF("\nplease input a valid power mode!\n");
                    result = FALSE;
                }                
            }

            //process device_id set
            else if(configure_deviceid_flag == 1)
            {
                if(!A_STRCMP(dual_uart_buffer, "0"))
                {
                    configure_deviceid_value = 0;
                }
                else if(!A_STRCMP(dual_uart_buffer, "1"))
                {
                    configure_deviceid_value = 1;
                }
                else
                {
                    A_PRINTF("\nplease input a valid device_id!\n");
                    result = FALSE;
                }                
            }

            //process scan command
            else if(configure_scan_flag == 1)
            {
                memset(ssid_dualuart_demo, 0, DUAL_UART_CMD_BUFFER_LEN);
                memcpy(ssid_dualuart_demo, dual_uart_buffer, DUAL_UART_CMD_BUFFER_LEN);
            }
        }
        
        memset(dual_uart_buffer,0,DUAL_UART_CMD_BUFFER_LEN);
        dual_uart_cmd_offset = 0;
        configure_baudrate_flag = 0;
        configure_pwrmode_flag = 0;
        configure_deviceid_flag = 0;
        configure_scan_flag = 0;

        return result;
    }
    else
    {
        if(dual_uart_cmd_offset < DUAL_UART_CMD_BUFFER_LEN-1)
        {
            *((char *)dual_uart_buffer + dual_uart_cmd_offset) = character;
            dual_uart_cmd_offset++;
        }
        return FALSE;
    }
}

void uart_demo(void)
{
    A_UINT32 uart_length;
    A_CHAR uart_buff[uart_buffer_size];
    q_fd_set fd;
    struct timeval tmo;
    A_INT32 uart0_fd, uart1_fd, uart2_fd;
    int ret;
    qcom_uart_para com_uart0_cfg, com_uart1_cfg, com_uart2_cfg;
    A_CHAR date[20];
    A_CHAR time[20];
    A_CHAR ver[20];
    A_CHAR cl[20];
    A_BOOL process_result;
    A_INT32 uart1_tx_ringbuffer_remain, uart1_tx_ringbuffer_threshold;
    A_INT32 uart2_tx_ringbuffer_remain, uart2_tx_ringbuffer_threshold;
    A_INT32 uart1_tx_ringbuffer_configure = UART1_TX_BUFFER_DEFAULT_SIZE;
    A_INT32 uart2_tx_ringbuffer_configure = UART2_TX_BUFFER_DEFAULT_SIZE;
    A_CHAR *null_buffer = NULL;
    A_UINT32 uart_length_default = 1;

    A_PRINTF("uart_demo in...\n");
    qcom_single_uart_init((A_CHAR *)"UART0");
    qcom_single_uart_init((A_CHAR *)"UART1");
    qcom_uart_set_buffer_size((A_CHAR *)"UART2", uart2_tx_ringbuffer_configure, uart2_tx_ringbuffer_configure);
    qcom_single_uart_init((A_CHAR *)"UART2");    
    uart0_fd = qcom_uart_open((A_CHAR *)"UART0");
    if (uart0_fd == -1) {
        A_PRINTF("qcom_uart_open uart0 failed...\n");
        return;
    }
    uart1_fd = qcom_uart_open((A_CHAR *)"UART1");
    if (uart1_fd == -1) {
        A_PRINTF("qcom_uart_open uart1 failed...\n");
        return;
    }
    uart2_fd = qcom_uart_open((A_CHAR *)"UART2"); 
    if (uart2_fd == -1) {
        A_PRINTF("qcom_uart_open uart2 failed...\n");
        return;
    }
    A_MEMSET(ver, 0, 20);
    qcom_firmware_version_get(date, time, ver, cl);
    tx_thread_sleep(1000);
    A_PRINTF("uart0_fd %d uart1_fd %d uart2_fd %d\n", uart0_fd, uart1_fd, uart2_fd);
    A_PRINTF("Firmware version    : %s\n", ver);
    tx_thread_sleep(10000);

    uart_length = strlen((A_CHAR *)uart_test_str0);
    qcom_uart_write(uart0_fd, (A_CHAR *)uart_test_str0, &uart_length);
    uart_length = strlen((A_CHAR *)uart_test_str1);
    qcom_uart_write(uart1_fd, (A_CHAR *)uart_test_str1, &uart_length);
    uart_length = strlen((A_CHAR *)uart_test_str2);
    qcom_uart_write(uart2_fd, (A_CHAR *)uart_test_str2, &uart_length);    
    
    while (1)
    {
        FD_ZERO(&fd);
        FD_SET(uart0_fd, &fd);
        FD_SET(uart1_fd, &fd);
        FD_SET(uart2_fd, &fd);
        tmo.tv_sec = 30;
        tmo.tv_usec = 0;

        ret = qcom_select(uart2_fd + 1, &fd, NULL, NULL, &tmo);
        if (ret == 0) 
        {
            A_PRINTF("UART receive timeout\n");
        }
        else
        {
            if (FD_ISSET(uart0_fd, &fd)) 
            {
                uart_length = uart_buffer_size;
                qcom_uart_read(uart0_fd, uart_buff, &uart_length);
                //tx_thread_sleep(100);
                if (uart_length)
                {                    
                    if((configure_baudrate_flag == 0) 
                        && (configure_pwrmode_flag == 0)
                        && (configure_deviceid_flag == 0)
                        && (configure_scan_flag == 0))
                    {
                        if(uart_buff[0] == '$')
                        {
                            configure_baudrate_flag = 1;                            
                            A_PRINTF("\nstart to configure uart0 baudrate!");
                            qcom_uart_write(uart0_fd, uart_buff, &uart_length);
                        }
                        else if(uart_buff[0] == 'P')
                        {
                            configure_pwrmode_flag = 1;                            
                            A_PRINTF("\nstart to configure uart0 power mode!");
                            qcom_uart_write(uart0_fd, uart_buff, &uart_length);
                        }
                        else if(uart_buff[0] == '#')
                        {
                            configure_deviceid_flag = 1;                            
                            A_PRINTF("\nstart to configure device_id!");
                            qcom_uart_write(uart0_fd, uart_buff, &uart_length);
                        }
                        else if(uart_buff[0] == '~')
                        {
                            //to get current uart ring buffer size
                            uart1_tx_ringbuffer_remain= qcom_uart_write(uart1_fd, null_buffer, &uart_length_default);
                            uart2_tx_ringbuffer_remain = qcom_uart_write(uart2_fd, null_buffer, &uart_length_default);

                            //to get uart ringbuffer least requirment
                            uart1_tx_ringbuffer_threshold = Get_Scanning_Uart_Tx_Ringbuffer_Threshold(1);
                            uart2_tx_ringbuffer_threshold = Get_Scanning_Uart_Tx_Ringbuffer_Threshold(2);
                            int min_uart1_tx_buffer = (uart1_tx_ringbuffer_configure < uart1_tx_ringbuffer_threshold)? uart1_tx_ringbuffer_configure : uart1_tx_ringbuffer_threshold;
                            
                            if((uart1_tx_ringbuffer_remain >= min_uart1_tx_buffer) 
                                && (uart2_tx_ringbuffer_remain >= uart2_tx_ringbuffer_threshold))
                            {
                                configure_scan_flag = 1;
                                A_PRINTF("\nscan command!");
                                qcom_uart_write(uart0_fd, uart_buff, &uart_length);
                            }
                            else
                            {
                                if(uart1_tx_ringbuffer_remain < min_uart1_tx_buffer)
                                {
                                    A_PRINTF("uart1 tx buffer insufficient, scan aborted!\n");
                                }
                                else if(uart2_tx_ringbuffer_remain < uart2_tx_ringbuffer_threshold)
                                {
                                    A_PRINTF("uart2 tx buffer insufficient, scan aborted!\n");
                                }
                            }
                        }
                        else
                        {
                            //A_PRINTF("receive length:%d\n", uart_length);
                            qcom_uart_write(uart0_fd, uart_buff, &uart_length);
                            //tx_thread_sleep(100);
                        }                        
                    }
                    else
                    {
                        if(configure_baudrate_flag == 1)
                        {
                            process_dual_uart_demo_character(uart_buff[0]);
                            qcom_uart_write(uart0_fd, uart_buff, &uart_length);

                            //configure uart baud rate
                            if(configure_baudrate_value != 0)
                            {
                                com_uart0_cfg.BaudRate=     configure_baudrate_value;
                                com_uart0_cfg.number=       8;
                                com_uart0_cfg.StopBits =    1;
                                com_uart0_cfg.parity =      0;
                                com_uart0_cfg.FlowControl = 0;
                                
                                qcom_set_uart_config((A_CHAR *)"UART0",&com_uart0_cfg);
                                
                                configure_baudrate_value =0;
                            }
                        }
                        else if(configure_pwrmode_flag == 1)
                        {
                            process_dual_uart_demo_character(uart_buff[0]);
                            qcom_uart_write(uart0_fd, uart_buff, &uart_length);

                            //configure power mode
                            if(configure_powermode_value != 0)
                            {                               
                                qcom_power_set_mode(0, configure_powermode_value);
                                
                                configure_powermode_value = 0;
                            }
                        }
                        else if(configure_deviceid_flag == 1)
                        {
                            process_result = process_dual_uart_demo_character(uart_buff[0]);
                            qcom_uart_write(uart0_fd, uart_buff, &uart_length);

                            //configure device id
                            if(process_result == TRUE)
                            {
                                qcom_sys_set_current_devid(configure_deviceid_value);
                            }                            
                        }
                        else if(configure_scan_flag == 1)
                        {
                            process_result = process_dual_uart_demo_character(uart_buff[0]);
                            qcom_uart_write(uart0_fd, uart_buff, &uart_length);

                            //configure scan mode
                            if(configure_scan_all_flag == 1)
                            {
                                if(scan_task_running == FALSE)
                                {
                                    scan_task_running = TRUE;
                                    create_uart_cmd_task(1);                                
                                    configure_scan_all_flag = 0;
                                }
                                else
                                {
                                    configure_scan_all_flag = 0;
                                    A_PRINTF("Another scan task is running!\n");
                                }
                            }
                            else if(process_result == TRUE)
                            {
                                if(scan_task_running == FALSE)
                                {
                                    scan_task_running = TRUE;
                                    create_uart_cmd_task(0);
                                }
                                else
                                {
                                    A_PRINTF("Another scan task is running!\n");
                                }
                            }
                        }
                        
                    }
                }                
            }
            if(FD_ISSET(uart1_fd, &fd))
            {
                uart_length = uart_buffer_size;
                qcom_uart_read(uart1_fd, uart_buff, &uart_length);
                // tx_thread_sleep(100);
                if (uart_length)
                {                    
                    if((configure_baudrate_flag == 0) 
                        && (configure_pwrmode_flag == 0))
                    {
                        if(uart_buff[0] == '$')
                        {
                            configure_baudrate_flag = 1;
                            A_PRINTF("\nstart to configure uart1 baudrate!\n");
                            qcom_uart_write(uart1_fd, uart_buff, &uart_length);
                        }
                        else if(uart_buff[0] == 'P')
                        {
                            configure_pwrmode_flag = 1;                            
                            A_PRINTF("\nstart to configure uart1 power mode!");
                            qcom_uart_write(uart1_fd, uart_buff, &uart_length);
                        }
                        else
                        {
                            //A_PRINTF("receive length:%d\n", uart_length);
                            qcom_uart_write(uart1_fd, uart_buff, &uart_length);
                            //tx_thread_sleep(100);
                        }                        
                    }
                    else
                    {
                        if(configure_baudrate_flag == 1)
                        {
                            process_dual_uart_demo_character(uart_buff[0]);
                            qcom_uart_write(uart1_fd, uart_buff, &uart_length);

                            //configure uart baud rate
                            if(configure_baudrate_value != 0)
                            {
                                com_uart1_cfg.BaudRate=     configure_baudrate_value;
                                com_uart1_cfg.number=       8;
                                com_uart1_cfg.StopBits =    1;
                                com_uart1_cfg.parity =      0;
                                com_uart1_cfg.FlowControl = 0;
                                
                                qcom_set_uart_config((A_CHAR *)"UART1",&com_uart1_cfg);
                                
                                configure_baudrate_value =0;
                            }
                        }
                        else if(configure_pwrmode_flag == 1)
                        {
                            process_dual_uart_demo_character(uart_buff[0]);
                            qcom_uart_write(uart1_fd, uart_buff, &uart_length);

                            //configure power mode
                            if(configure_powermode_value != 0)
                            {                               
                                qcom_power_set_mode(0, configure_powermode_value);
                                
                                configure_powermode_value = 0;
                            }
                        }
                    }                    
                }
            }
	    if(FD_ISSET(uart2_fd, &fd))
	    {
                uart_length = uart_buffer_size;
                qcom_uart_read(uart2_fd, uart_buff, &uart_length);
                // tx_thread_sleep(100);
                if (uart_length)
                {
                    if((configure_baudrate_flag == 0) 
                        && (configure_pwrmode_flag == 0))
                    {
                        if(uart_buff[0] == '$')
                        {
                            configure_baudrate_flag = 1;
                            A_PRINTF("\nstart to configure uart2 baudrate!\n");
                            qcom_uart_write(uart2_fd, uart_buff, &uart_length);
                        }
                        else if(uart_buff[0] == 'P')
                        {
                            configure_pwrmode_flag = 1;                            
                            A_PRINTF("\nstart to configure uart2 power mode!");
                            qcom_uart_write(uart2_fd, uart_buff, &uart_length);
                        }
                        else
                        {
                            //A_PRINTF("receive length:%d\n", uart_length);
                            qcom_uart_write(uart2_fd, uart_buff, &uart_length);
                            //tx_thread_sleep(100);
                        }                        
                    }
                    else
                    {
                        if(configure_baudrate_flag == 1)
                        {
                            process_dual_uart_demo_character(uart_buff[0]);
                            qcom_uart_write(uart2_fd, uart_buff, &uart_length);

                            //configure uart baud rate
                            if(configure_baudrate_value != 0)
                            {
                                com_uart2_cfg.BaudRate=     configure_baudrate_value;
                                com_uart2_cfg.number=       8;
                                com_uart2_cfg.StopBits =    1;
                                com_uart2_cfg.parity =      0;
                                com_uart2_cfg.FlowControl = 0;
                                
                                qcom_set_uart_config((A_CHAR *)"UART2",&com_uart2_cfg);
                                
                                configure_baudrate_value =0;
                            }
                        }
                        else if(configure_pwrmode_flag == 1)
                        {
                            process_dual_uart_demo_character(uart_buff[0]);
                            qcom_uart_write(uart2_fd, uart_buff, &uart_length);

                            //configure power mode
                            if(configure_powermode_value != 0)
                            {                               
                                qcom_power_set_mode(0, configure_powermode_value);
                                
                                configure_powermode_value = 0;
                            }
                        }
                    }                    
                }
            }
         
        }
    }
}

void uart_demo_921600(void)
{
    A_UINT32 uart_length;
    A_UINT32 uart0_length;
    A_CHAR uart_buff[uart_buffer_size];
    A_CHAR uart0_buff[6] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
    q_fd_set fd;
    struct timeval tmo;
    A_INT32 uart0_fd, uart1_fd, uart2_fd;
    int ret;

    qcom_uart_para com_uart_cfg;

    //com_uart_cfg.BaudRate=     115200;
    com_uart_cfg.BaudRate=     921600; /* 1Mbits */
    com_uart_cfg.number=       8;
    com_uart_cfg.StopBits =    1;
    com_uart_cfg.parity =      0;
    com_uart_cfg.FlowControl = 0;

    A_PRINTF("uart_demo in...\n");
    qcom_single_uart_init((A_CHAR *)"UART0");
    qcom_single_uart_init((A_CHAR *)"UART1");
    qcom_single_uart_init((A_CHAR *)"UART2");
    uart0_fd = qcom_uart_open((A_CHAR *)"UART0");
    if (uart0_fd == -1) {
        A_PRINTF("qcom_uart_open uart0 failed...\n");
        return;
    }
    uart1_fd = qcom_uart_open((A_CHAR *)"UART1");
    if (uart1_fd == -1) {
        A_PRINTF("qcom_uart_open uart1 failed...\n");
        return;
    }
    uart2_fd = qcom_uart_open((A_CHAR *)"UART2");
    if (uart1_fd == -1) {
        A_PRINTF("qcom_uar2_open uart2 failed...\n");
        return;
    }

    int i;
    for(i=0; i<15; i++) {
        A_PRINTF("sleep %d seconds...\n", i);
        tx_thread_sleep(1000);
    }
    qcom_set_uart_config((A_CHAR *)"UART0",&com_uart_cfg);

    tx_thread_sleep(1000);
    A_PRINTF("uart0_fd %d uart1_fd %d, uart2_fd %d\n", uart0_fd, uart1_fd, uart2_fd);
    tx_thread_sleep(10000);

    uart_length = strlen((A_CHAR *)uart_test_str0);
    qcom_uart_write(uart0_fd, (A_CHAR *)uart_test_str0, &uart_length);
    uart_length = strlen((A_CHAR *)uart_test_str1);
    qcom_uart_write(uart1_fd, (A_CHAR *)uart_test_str1, &uart_length);
    uart_length = strlen((A_CHAR *)uart_test_str2);
    qcom_uart_write(uart2_fd, (A_CHAR *)uart_test_str2, &uart_length);

    while (1)
    {
        FD_ZERO(&fd);
        FD_SET(uart0_fd, &fd);
        FD_SET(uart1_fd, &fd);
        FD_SET(uart2_fd, &fd);
        tmo.tv_sec = 30;
        tmo.tv_usec = 0;

        ret = qcom_select(uart2_fd + 1, &fd, NULL, NULL, &tmo);
        if (ret == 0) {
            A_PRINTF("UART receive timeout\n");
        } else {
            if (FD_ISSET(uart0_fd, &fd)) {
                qcom_uart_read(uart0_fd, uart_buff, &uart_length);
                tx_thread_sleep(100);
                if (uart_length) {
                    A_PRINTF("uart0 receive length:%d\n", uart_length);
                    qcom_uart_write(uart0_fd, uart_buff, &uart_length);
                    tx_thread_sleep(100);
                }
            } else if (FD_ISSET(uart1_fd, &fd)) {
                qcom_uart_read(uart1_fd, uart_buff, &uart_length);
                tx_thread_sleep(100);
                if (uart_length) {
                    A_PRINTF("uart1 receive length:%d\n", uart_length);
                    qcom_uart_write(uart1_fd, uart_buff, &uart_length);
                    A_PRINTF("Send to UART1: {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa}\n");
                    uart0_length = 6;
                    qcom_uart_write(uart0_fd, uart0_buff, &uart_length);
                    tx_thread_sleep(100);
                }
            }  else if (FD_ISSET(uart2_fd, &fd)) {
                qcom_uart_read(uart2_fd, uart_buff, &uart_length);
                tx_thread_sleep(100);
                if (uart_length) {
                    A_PRINTF("uart2 receive length:%d\n", uart_length);
                    qcom_uart_write(uart2_fd, uart_buff, &uart_length);
                    A_PRINTF("Send to UART0: {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa}\n");
                    uart0_length = 6;
                    qcom_uart_write(uart0_fd, uart0_buff, &uart_length);
                    tx_thread_sleep(100);
                }
            } else {
                A_PRINTF("UART something is error!\n");
            }
        }
    }
}

void
uart_demo_entry(ULONG which_thread)
{
    extern void user_pre_init(void);
    user_pre_init();
    //qcom_enable_print(1);
    A_PRINTF("uart_demo_entry in...\n");
    uart_demo();
}

void user_main(void)
{    
    extern void task_execute_cli_cmd();
    tx_byte_pool_create(&pool, "dual_uart_demo pool", TX_POOL_CREATE_DYNAMIC, BYTE_POOL_SIZE);

    {
        CHAR *pointer;
        tx_byte_allocate(&pool, (VOID **) & pointer, PSEUDO_HOST_STACK_SIZE, TX_NO_WAIT);

        tx_thread_create(&host_thread, "dual_uart_demo thread", uart_demo_entry,
                         0, pointer, PSEUDO_HOST_STACK_SIZE, 16, 16, 4, TX_AUTO_START);
    }

    cdr_threadx_thread_init();
}

