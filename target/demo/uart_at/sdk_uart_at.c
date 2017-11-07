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

TX_THREAD host_thread;
#define BYTE_POOL_SIZE (5*1024)
#define PSEUDO_HOST_STACK_SIZE (4 * 1024)   /* small stack for pseudo-Host thread */
TX_BYTE_POOL pool;
extern void at_console_setup();

void qcom_wps_event_process_cb(A_UINT8 ucDeviceID, QCOM_WPS_EVENT_TYPE uEventID, qcom_wps_event_t *pEvtBuffer, void *qcom_wps_event_handler_arg)
{
    switch (uEventID)
    {
        case QCOM_WPS_PROFILE_EVENT:
        {
            qcom_wps_profile_info_t *p_tmp;

            if (pEvtBuffer == NULL) {
                return;
            }

            p_tmp = &pEvtBuffer->event.profile_info;

            if (QCOM_WPS_STATUS_SUCCESS == p_tmp->error_code) {
                A_STATUS ret;

                ret = qcom_wps_set_credentials(ucDeviceID, &p_tmp->credentials);
                if (ret != A_OK) {
                    A_PRINTF("WPS error: Failed to set wps credentials.\n");
                    return;
                }
                else {
                    A_PRINTF("WPS ok.\n");
                }

                qcom_wps_connect(ucDeviceID);
            } else {
                switch (p_tmp->error_code) {
                    case QCOM_WPS_ERROR_INVALID_START_INFO:
                        A_PRINTF("WPS error: Invalid start info\n");
                        break;
                    case QCOM_WPS_ERROR_MULTIPLE_PBC_SESSIONS:
                        A_PRINTF("WPS error: Multiple PBC Sessions\n");
                        break;
                    case QCOM_WPS_ERROR_WALKTIMER_TIMEOUT:
                        A_PRINTF("WPS error: Walktimer Timeout\n");
                        break;
                    case QCOM_WPS_ERROR_M2D_RCVD:
                        A_PRINTF("WPS error: M2D RCVD\n");
                        break;
                    case QCOM_WPS_ERROR_PWD_AUTH_FAIL:
                        A_PRINTF("WPS error: AUTH FAIL\n");
                        break;
                    case QCOM_WPS_ERROR_CANCELLED:
                        A_PRINTF("WPS error: WPS CANCEL\n");
                        break;
                    case QCOM_WPS_ERROR_INVALID_PIN:
                        A_PRINTF("WPS error: INVALID PIN\n");
                        break;
                    default:
                        A_PRINTF("WPS error: UNKNOWN\n");
                       break;
                }
            }
            break;
        }
        default:
            A_PRINTF("Unhandled WPS event id %d\n", uEventID);
            break;
    }

    return;
}


void
shell_host_entry(ULONG which_thread)
{
    extern void user_pre_init(void);
    user_pre_init();
	
    extern void task_execute_at_cmd();

    /* init the uart interface for uart at*/
    /* we use the UART0/HCI UART */
    at_console_setup(); 

    /* Enable WPS and register event handler */
    /* THIS IS NOT THE RIGHT PLACE FOR THIS REGISTRATION. THIS API 
     * SHOULD BE CALLED ALONG WITH qcom_wps_enable() WHEN enable is 
     * set to 1.*/
    qcom_wps_register_event_handler(qcom_wps_event_process_cb, NULL);

    /* main task of uart at */
    task_execute_at_cmd();
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

