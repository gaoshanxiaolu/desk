/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
 

#include <qcom/qcom_common.h>
#include <qcom/qcom_utils.h>
#include "uart_serphw_api.h"
#include <qcom/qcom_uart.h>
#include <qcom/qcom_cli.h>
#include <qcom/priv_uart.h>
#include "qcom/qcom_gpio.h"

#define UART_WAKEUP_TEST
//#define DISABLE_HSUART_WAKEUP

#ifdef UART_WAKEUP_TEST

extern A_UINT8 sdk_shell_flag;
extern A_UINT8 uart_wakeup_flag;
extern unsigned int sleep_disable_refcnt;
#if defined(DISABLE_HSUART_WAKEUP)
char *uart_wakeup_help = "uart { uart0 } { rx | rts } { enable | disable }";
char *uart_configure_baudrate_help = "setbaudrate { uart0 } { value }";
#else
char *uart_wakeup_help = "uart { uart0 | uart1 | uart2 } { rx | rts } { enable | disable }";
char *uart_configure_baudrate_help = "setbaudrate { uart0 | uart1 | uart2 } { value }";
#endif
//#define UART0_RX_PIN_NUM 28

int cmd_uart_wakeup_test(int argc, char *argv[])
{    
    if(uart_wakeup_flag == 0)
    {
        qcom_printf("Uart wakeup test abort, please enable uart rx/rts first.\n");
        return -1;
    }
    
    if(sleep_disable_refcnt != 0)
    {
        qcom_printf("It is max performance currently, please change into recycle power mode first.\n");
        return -1;
    }

    uart_wakeup_test();
    return 0;
}

int cmd_uart_wakeup_set(int argc, char *argv[])
{
#if !defined(DISABLE_HSUART_WAKEUP)
    int uart1_connect_status = 0;
    int uart2_connect_status = 0;
    //int uart0_connect_status = 0;
    //int debug_uart = -1;
    int delay_flag = 0;
#endif
    
    if(argc < 4)
    {
        qcom_printf("%s", uart_wakeup_help);    
        return -1;
    }

    if(!A_STRCMP(argv[1],"uart0") && !A_STRCMP(argv[2],"rx"))
    {
        //the case to identity whether uart0 is connected is different with uart1 and uart2. Althogh it can be fixed by 
        //(1)modifying tunable value(GPIO28_ACTIVE_CONFIG 0x90002808);
        //(2)adding a delay(about 10secs) when identifying uart0 connection status.
        //but both two methods are not proper for this issue, so we will not check whether uart0 is connected at present.
        #if 0
        debug_uart = Console_Uart_Num();
        
        if((debug_uart == 1) || (debug_uart == 2))
        {
            qcom_gpio_pin_dir(UART0_RX_PIN_NUM,1);//input
            delay_flag = 1;
            uart0_connect_status = uart_check_connection_status(0, delay_flag);
            if(uart0_connect_status == 0)
            {
                qcom_printf("uart0 wakeup function failed, connect uart0 first!\n");
                return -1;
            }
        }
        #endif
        if(!A_STRCMP(argv[3],"enable"))
        {
            qcom_uart_wakeup_config((A_CHAR *)"UART0", 0, 1);
        }
        else if(!A_STRCMP(argv[3],"disable"))
        {
            qcom_uart_wakeup_config((A_CHAR *)"UART0", 0, 0);
        }
        else 
        {
            qcom_printf("not supportted options\n");
            return -1;  
        }
                
    }
#if !defined(DISABLE_HSUART_WAKEUP)
    else if(!A_STRCMP(argv[1],"uart1") && !A_STRCMP(argv[2],"rx"))
    {
        if(!A_STRCMP(argv[3],"enable"))
        {
            qcom_uart_wakeup_config((A_CHAR *)"UART1", 0, 1);
        }
        else if(!A_STRCMP(argv[3],"disable"))
        {
            qcom_uart_wakeup_config((A_CHAR *)"UART1", 0, 0);
        }
        else 
        {
            qcom_printf("not supportted options\n");
            return -1;  
        }
        uart1_connect_status = uart_check_connection_status(1, delay_flag);
        if(uart1_connect_status == 0)                                                      
        {                                                                                      
            qcom_printf("uart1 wakeup function failed, connect uart1 first!\n");
            qcom_uart_wakeup_config((A_CHAR *)"UART1", 0, 0);
            return -1;                                                                 
        }
                
    }
    
    else if(!A_STRCMP(argv[1],"uart2") && !A_STRCMP(argv[2],"rx"))
    {
        uart2_connect_status = uart_check_connection_status(2, delay_flag);
        
        if(uart2_connect_status == 0)
        {
            qcom_printf("uart2 wakeup function failed, connect uart2 first!\n");
            return -1;
        }
        if(!A_STRCMP(argv[3],"enable"))
        {
            qcom_uart_wakeup_config((A_CHAR *)"UART2", 0, 1);
        }
        else if(!A_STRCMP(argv[3],"disable"))
        {
            qcom_uart_wakeup_config((A_CHAR *)"UART2", 0, 0);
        }
        else 
        {
            qcom_printf("not supportted options\n");
            return -1;  
        }
                
    }
#endif
    else
    {
        qcom_printf("%s", uart_wakeup_help);    
        return -1;
    }    
}

#if 0
int cmd_uart1_rx_disable(int argc, char *argv[])
{
    uart1_wakeup_enable_rx = 0; 
    return;
}
#endif
int cmd_uart_baudrate_configure(int argc, char *argv[])
{
    A_UINT32 uart_baudrate_value = 0;
    qcom_uart_para com_uart_cfg;
    int uart_no = -1;

    if(argc != 3)
    {
        qcom_printf("%s", uart_configure_baudrate_help);    
        return -1;
    }
    
    if(!A_STRCMP(argv[1],"uart0"))
    {
        uart_no = 0;
    }
    else if(!A_STRCMP(argv[1],"uart1"))
    {
        uart_no = 1;
    }
    else
    {
        qcom_printf("unsupported uart number!");
        return -1;
    }

    uart_baudrate_value = atoi(argv[2]);

    switch(uart_baudrate_value)
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
            qcom_printf("unsupported baudrate!");
            return -1;
        }
    }
    
    com_uart_cfg.BaudRate = uart_baudrate_value;
    com_uart_cfg.number = 8;
    com_uart_cfg.StopBits = 1;
    com_uart_cfg.parity = 0;
    com_uart_cfg.FlowControl = 0;

    if(uart_no == 0)
    {
        sdk_shell_flag = 1;
        qcom_set_uart_config((A_CHAR *)"UART0",&com_uart_cfg);
    }    
    else if(uart_no == 1)
    {
        sdk_shell_flag = 1;
        qcom_set_uart_config((A_CHAR *)"UART1",&com_uart_cfg);
    }
    else if(uart_no == 2)
    {
        sdk_shell_flag = 1;
        qcom_set_uart_config((A_CHAR *)"UART2",&com_uart_cfg);
    }
    else
    {     
        qcom_printf("unsupported uart number!");
        return -1;
    }
    
}
#endif //UART_WAKEUP_TEST
