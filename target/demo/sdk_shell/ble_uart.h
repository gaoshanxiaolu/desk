//
/*
  * Copyright (c) 2015 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#ifndef _BLE_UART_H_
#define _BLE_UART_H_

enum MX_SIGNEL{
    M1,
    M2,
    M3,
    M4,
    X1,
    X2,
    X3,
    X4,
    INVALID_XM
};

enum LED_STATE {
    LED_OFF,
    LED_ON,
    LED_SLOW_BLINK,
    LED_FAST_BLINK
};

enum DESK_RUN_STATE {
	STOP,
	UP_DESK,
	DOWN_DESK
};
	
void set_led_state(enum LED_STATE state);
enum DESK_RUN_STATE get_desk_run_state(void);
void up_desk(void);
void down_desk(void);
void stop_desk(void);

void set_mx_sig_val(enum MX_SIGNEL sig);


A_INT32 start_desk_uart_app(A_INT32 argc, A_CHAR *argv[]);
A_INT32 start_desk_motor_app(A_INT32 argc, A_CHAR *argv[]);
A_INT32 start_desk_MorX_signel_app(A_INT32 argc, A_CHAR *argv[]);
A_UINT16 get_desk_pos_value(void);
A_INT32 start_desk_led_disp_app(A_INT32 argc, A_CHAR *argv[]);
A_INT32 start_desk_light_and_inir_app(A_INT32 argc, A_CHAR *argv[]);


#endif


