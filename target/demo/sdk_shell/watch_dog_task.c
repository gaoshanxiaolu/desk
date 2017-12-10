#include "qcom_common.h"


extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();

void watch_dog_task()
{
    printf("\r\n enter watch_dog_task\r\n");

	qcom_watchdog(1,30);
    
    while(TRUE)
    {

    	tx_thread_sleep(2000);

		qcom_watchdog_feed();
    }
}


A_INT32 start_watch_dog_task(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(watch_dog_task, 2, 2048, 80);
}

