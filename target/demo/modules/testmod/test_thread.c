/*

* Copyright (c) 2016 Qualcomm Technologies, Inc.

* All Rights Reserved.

* Confidential and Proprietary - Qualcomm Technologies, Inc.

*/

#include "qcom_common.h"
#include "malloc_api.h"

#define THREAD_STACK_SIZE (4 * 1024 )
TX_THREAD testmod_thread;
CHAR *testmod_stack;
int task_running;

void mod_task(unsigned long arg)
{
    A_PRINTF("mod task start\n");

    while(task_running) {
        tx_thread_sleep(60000);
        A_PRINTF("mod task running\n");
    }

    A_PRINTF("mod task exit\n");
    return;
}



void test_thread_init()
{

    A_PRINTF("\nCreate mod task\n");

    testmod_stack = (CHAR *)malloc(THREAD_STACK_SIZE);
    if (!testmod_stack) {
    	A_PRINTF("Failed to create task\n");
        return;
    }

    task_running = 1;
    tx_thread_create(&testmod_thread, "testmod thread", mod_task,
                     0, testmod_stack, THREAD_STACK_SIZE, 16, 16, 4, TX_AUTO_START);


}

void test_thread_exit()
{

    if (!task_running)
        return;

    task_running = 0;

    /*wait task to finish elegantly*/
    tx_thread_sleep(1000);

    tx_thread_terminate(&testmod_thread);
    tx_thread_delete(&testmod_thread);

    free(testmod_stack);
    A_PRINTF("Destroy mod task\n");
}

