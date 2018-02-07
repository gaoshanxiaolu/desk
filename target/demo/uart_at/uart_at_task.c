/*
 * Copyright (c) 2014 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */
#include "threadx/tx_api.h"
#include "threadx/tx_thread.h"
#include "qcom_common.h"


#define MAX_TASK_NUM 20

#define THR_TASK_STS_NULL     0
#define THR_TASK_STS_CREATED  1
#define THR_TASK_STS_EXITED   2

TX_THREAD* thr[MAX_TASK_NUM];
char* thrStack[MAX_TASK_NUM];
char thrStatus[MAX_TASK_NUM];
char taskName[MAX_TASK_NUM*2];

TX_EVENT_FLAGS_GROUP event_flags_0;
extern void qcom_msleep(int ms);
extern void at_send_packet();
extern void _at_process_char(void);
extern int g_tx_timeout;

TX_MUTEX             mutex_tsk;
#define TSK_SEM_INIT tx_mutex_create(&mutex_tsk, "mutex tsk", TX_NO_INHERIT);
#define TSK_SEM_LOCK {int status =  tx_mutex_get(&mutex_tsk, TX_WAIT_FOREVER); \
  if (status != TX_SUCCESS) {\
    DBG_TSK("lock the task mutex failed !!!\n");\
  }\
}
    
#define TSK_SEM_UNLOCK { int status =  tx_mutex_put(&mutex_tsk);\
  if (status != TX_SUCCESS) {\
    DBG_TSK("unlock the task mutex failed !!!\n");\
  }\
}

#ifdef DEBUG_TASK
  #define DBG_TSK(fmt...) printf("[DBG@%14s:%d]", __FUNCTION__, __LINE__); printf(fmt);
#else
  #define DBG_TSK(fmt...) 
#endif

static int
task_init(void)
{
    static int inited = 0;
    int i;
    if (0 == inited) {
        TSK_SEM_INIT;
        for (i = 0; i < MAX_TASK_NUM; i++) {
            thr[i] = 0;
            thrStatus[i] = THR_TASK_STS_NULL;
            thrStack[i] = 0;
        }
        inited = 1;
    }
}

static int
mark_exit_task(TX_THREAD * thread_ptr)
{
    int i;
    for (i = 0; i < MAX_TASK_NUM; i++) {
        if (0 != thr[i]) {
            if (strcmp(thr[i]->tx_thread_name, thread_ptr->tx_thread_name) == 0) {
                thrStatus[i] = THR_TASK_STS_EXITED;
                DBG_TSK("---> marked exit task, index %d, %s, addr %p %p\n", i,
                        thr[i]->tx_thread_name, thr[i], thread_ptr);
                return i;
            }
        }
    }
}

/* 
  * qcom_task_del - delete the user task 
*/
void
qcom_task_del(void)
{
    int i;

    for (i = 0; i < MAX_TASK_NUM; i++) {
        if ((THR_TASK_STS_EXITED == thrStatus[i])) {
            if ((thr[i]) && (thrStack[i])) {
          tx_thread_delete(thr[i]);
        }
    }
  }

    return;
}


static int
get_empty_task_index(void)
{
    int i;
    for (i = 0; i < MAX_TASK_NUM; i++) {
        if (0 == thr[i]) {
            return i;
        }
    }

    return -1;
}


static int
free_task_mem()
{
    int i;

#ifdef DEBUG_TASK
    extern unsigned int allocram_remaining_bytes;
    extern int mem_free_ret(void *ptr);
    unsigned int mem_free_size = qcom_mem_heap_get_free_size();
    int ret = 0;
#endif
    for (i = 0; i < MAX_TASK_NUM; i++) {
        if ((THR_TASK_STS_EXITED == thrStatus[i])) {
            if ((thr[i]) && (thrStack[i])) {
                DBG_TSK("---> Free %d, thr:%p, stk:%p.\n", i, thr[i], thrStack[i]);
                DBG_TSK("@@1 MEM FREE : %d.\n", mem_free_size);
                //tx_thread_delete(thr[i]);
                mem_free(thr[i]);
          mem_free(thrStack[i]);
                DBG_TSK("@@2 MEM FREE : %d.\n", mem_free_size);
                thr[i] = 0;
                thrStatus[i] = THR_TASK_STS_NULL;
                thrStack[i] = 0;
                taskName[2 * i] = 0;

            } else {
                DBG_TSK("!!!> Free %d Failed, thr[i] %p, thrStack[i] %p\n", i, thr[i], thrStack[i]);
            }
        }
    }
}

/* 
  * qcom_task_start - create the user task 
*/
int
qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms)
{
    static int taskId = 0;
    int ret = 0;
    int taskIdx = 0;
    void *stk = NULL;
    void *pthr = NULL;

    task_init();
    qcom_task_del();
    TSK_SEM_LOCK;
    free_task_mem();
    taskIdx = get_empty_task_index();
    TSK_SEM_UNLOCK;
    if (-1 == taskIdx) {
        qcom_printf("task index is full.\n");
        ret = -1;
        goto errout;

    }

    stk = mem_alloc(stk_size);
    if (stk == NULL) {
        qcom_printf("malloc stack failed.\n");
        ret = -1;
        goto errout;

    }

    pthr = mem_alloc(sizeof (TX_THREAD));
    if (NULL == pthr) {
        qcom_printf("malloc thr failed.\n");
        ret = -1;
        goto errout;
    }

    TSK_SEM_LOCK;
    thrStack[taskIdx] = stk;
    thr[taskIdx] = pthr;

    DBG_TSK("---> malloc %d, thr:%p, stk:%p.\n", taskIdx, thr[taskIdx], thrStack[taskIdx]);

    /* default priority is 4 */
    /* set the task name */
    taskName[2 * taskIdx] = '0' + taskId % 255;
    taskName[2 * taskIdx + 1] = 0;
    DBG_TSK("thr[%d]:%p stk:%p.\n", taskIdx, thr[taskIdx], stk);
    TSK_SEM_UNLOCK;
#if 0
    ret =
        tx_thread_create(thr[taskIdx], (char *) &taskName[2 * taskIdx],
                         (void (*)(unsigned long)) fn, 0, stk, stk_size, 4, 4, TX_NO_TIME_SLICE,
                         TX_AUTO_START);
#else

	if(tk_ms == 999)
	{
		ret = tx_thread_create(pthr, (char *) &taskName[2 * taskIdx], (void (*)(unsigned long)) fn, arg,
							   stk, stk_size, 0, 16, 4, TX_AUTO_START);

	}
	else
	{
    	ret = tx_thread_create(pthr, (char *) &taskName[2 * taskIdx], (void (*)(unsigned long)) fn, arg,
                           stk, stk_size, 16, 16, 4, TX_AUTO_START);
	}
#endif

    if (0 != ret) {
        qcom_printf("thread create failed, return value : 0x%x.\n", ret);
        goto errout;
    } else {
        TSK_SEM_LOCK;
        thrStatus[taskIdx] = THR_TASK_STS_CREATED;
        taskId++;
        TSK_SEM_UNLOCK;
    }
    //extern unsigned int allocram_remaining_bytes;
errout:
    if (0 != ret){
        if (pthr != NULL){
            mem_free(pthr);
        }

        if (stk != NULL) {
            mem_free(stk);
        }

        if ((taskIdx > 0) && (taskIdx < MAX_TASK_NUM)) {
            TSK_SEM_LOCK;
            thrStack[taskIdx] = NULL;
            thr[taskIdx] = NULL;
            TSK_SEM_UNLOCK;
        }
    }
    qcom_printf("### MEM FREE : %d.\n", qcom_mem_heap_get_free_size());
    return ret;
}


/* 
  * quit and delete the user task.  It is called in user task
*/
void
qcom_task_exit()
{
    TX_THREAD *thread_ptr;

    TX_THREAD_GET_CURRENT(thread_ptr)
        DBG_TSK("--->tsk exit : %s.\n", thread_ptr->tx_thread_name);
    qcom_task_del();
    TSK_SEM_LOCK;
    free_task_mem();            /*free old task */
    mark_exit_task(thread_ptr);
    TSK_SEM_UNLOCK;
    tx_thread_terminate(thread_ptr);

}



void
task_execute_at_cmd()
{
    void _console_process_char(void);
    UINT status;
    ULONG actual_flags;
    
    status = tx_event_flags_create(&event_flags_0, "event flags 0");
    A_PRINTF("create event 0x%x.\n", status);

    while (1) {
	/* check if we got a character*/
	/* timeout is g_tx_timeout    */
        status = tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR,
                                    &actual_flags, g_tx_timeout);
        /* Check status.  */
	/* if there is no char*/
        if (status != TX_SUCCESS) {
	    /*call this func to send the data remaining in the tx buffer*/
	    /*send every g_tx_timeout*/
            at_send_packet();
            qcom_thread_msleep(500);
            continue;
        }

	/*if we got a char, call this func to process this char*/
        _at_process_char();
    }
}

/* after receiving a character, call this function to wake up main at task*/
void
wakeup_task_execute_at_cmd()
{
    UINT status;

    status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);
    /* Check status.  */
    if (status != TX_SUCCESS) {
        A_PRINTF("wake up at failed.\n");
    }

}

