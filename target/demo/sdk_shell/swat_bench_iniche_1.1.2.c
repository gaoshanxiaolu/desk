/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "swat_parse.h"
#include "qcom/tx_alloc_api.h"
#include "swat_bench_core.h"
#include "qcom/socket_api.h"
#include "qcom/select_api.h"
#include "qcom/qcom_ssl.h"
#include <qcom/qcom_timer.h>
#include "swat_wmiconfig_network.h"
//extern void qcom_thread_msleep(unsigned long ms);
extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();
extern A_UINT8 currentDeviceId;

static A_UINT32 savedPowerMode0 = REC_POWER;
static A_UINT32 savedPowerMode1 = REC_POWER;
static A_INT32 savedPowerModeCount = 0;
A_UINT32 config_socket_active_close = 0;


/*Flag controls IPv4 - v6 switch*/
int v6_enabled = 0;

SSL_ROLE_T ssl_role;

#if defined(KF_ROM_1_1_2_INICHE)

static qcom_timer_t benchTimer;
static A_INT8 benchTcpTxTimerStop[CALC_STREAM_NUMBER];
static A_INT8 benchUdpTxTimerStop[CALC_STREAM_NUMBER];
#ifdef SWAT_BENCH_RAW
static A_INT8 benchRawTxTimerStop[CALC_STREAM_NUMBER];
#endif
static void swat_bench_loop(unsigned int alarm, void *data)
{
    int index, i;
    STREAM_CXT_t * pStreamCxt;
    unsigned long cur_time = swat_time_get_ms();
    unsigned long totalInterval = 0;

    for (index = 0; index < CALC_STREAM_NUMBER; index++) {
        for (i = 0; i < 3; i++) {
            int used = 0;
            if(!i){
                pStreamCxt = &cxtTcpTxPara[index];
                used = tcpTxIndex[index].isUsed;
	        }else if(1==i){
	            pStreamCxt = &cxtUdpTxPara[index];
		        used = udpTxIndex[index].isUsed;
	        }
#ifdef SWAT_BENCH_RAW
	        else{
	            pStreamCxt = &cxtRawTxPara[index];
		        used      = rawTxIndex[index].isUsed;
	        }
#endif
            
            if (used && (TEST_MODE_TIME == pStreamCxt->param.mode)) 
            {
                totalInterval =  cur_time - pStreamCxt->calc[DEFAULT_FD_INDEX].firstTime.milliseconds;
                if (totalInterval  > pStreamCxt->param.seconds*1000) {
                    if (!i)   {
                        benchTcpTxTimerStop[index] = 1; 
                    } else if(1==i){
                        benchUdpTxTimerStop[index] = 1; 
                    }
#ifdef SWAT_BENCH_RAW
		            else {
                        benchRawTxTimerStop[index] = 1; 
                    }
#endif
                }
            }
        }
    }        
}
void swat_bench_pwrmode_maxperf()
{
   savedPowerModeCount++;
   if (savedPowerModeCount == 1){
    	qcom_power_get_mode(0, &savedPowerMode0);
    	if(savedPowerMode0!=MAX_PERF_POWER)
        	qcom_power_set_mode(0, MAX_PERF_POWER);
      
       if (gNumOfWlanDevices>1){
        	qcom_power_get_mode(1, &savedPowerMode1);   
        	if(savedPowerMode1!=MAX_PERF_POWER)
            	qcom_power_set_mode(1, MAX_PERF_POWER);
       }  
   }      
}

void swat_bench_restore_pwrmode()
{
   if (savedPowerModeCount<=0)
      return;
   
   savedPowerModeCount--;
   if (savedPowerModeCount == 0){
    	if(savedPowerMode0!=MAX_PERF_POWER)
        	qcom_power_set_mode(0, REC_POWER);

      if (gNumOfWlanDevices>1){
        	if(savedPowerMode1!=MAX_PERF_POWER)
            	qcom_power_set_mode(1, REC_POWER);
       }  
   }    
}
void
swat_bench_timer_init(A_UINT32 seconds, A_UINT32 protocol, A_UINT32 index)
{
    static int timer_inited = 0;
    if (0 == timer_inited) {
        timer_inited = 1;
        swat_time_init(&benchTimer, swat_bench_loop,  NULL,  1 * 1000, PERIODIC); //PERIODIC        
        swat_time_start(&benchTimer);    
    }
    if (TEST_PROT_TCP == protocol || TEST_PROT_SSL == protocol) {
        benchTcpTxTimerStop[index] = 0;
    }
    if (TEST_PROT_UDP == protocol || TEST_PROT_DTLS == protocol) {
        benchUdpTxTimerStop[index] = 0;
    }
#ifdef SWAT_BENCH_RAW
    if (TEST_PROT_RAW == protocol)    
    {   
        benchRawTxTimerStop[index] = 0;   
    }
#endif
}

void
swat_tcp_tx_handle(STREAM_CXT_t * pStreamCxt)
{
    A_INT32 sendBytes = 0 ;
    A_UINT32 sumBytes = 0;
    A_UINT32 currentPackets = 0;
    A_UINT8 *pDataBuffer = NULL;
    A_UINT8 pattern;
#ifdef SWAT_SSL
    A_INT32 result = 0;
    SSL_INST *ssl = NULL;
#endif

    A_UINT32 iperf_display_interval;
    A_UINT32 iperf_display_last;
    A_UINT32 iperf_display_next;
    A_UINT32 index;
    A_UINT32 *index_ptr;
    A_UINT32 *time_ptr;
    if (pStreamCxt->param.iperf_mode) {
        iperf_display_interval = pStreamCxt->param.iperf_display_interval * 1000;
        iperf_display_last = swat_time_get_ms();
        iperf_display_next = iperf_display_last + iperf_display_interval;
    }
    SWAT_PTR_NULL_CHK(pStreamCxt);

    /* Initial Bench Value */
    swat_bench_quit_init();

    /* Initial Calc & Time */
    pStreamCxt->calc[DEFAULT_FD_INDEX].firstTime.milliseconds = CALC_TIME_DEF;
    pStreamCxt->calc[DEFAULT_FD_INDEX].lastTime.milliseconds = CALC_TIME_DEF;
    pStreamCxt->calc[DEFAULT_FD_INDEX].bytes = CALC_BYTES_DEF;
    pStreamCxt->calc[DEFAULT_FD_INDEX].kbytes = CALC_KBYTES_DEF;
    if (pStreamCxt->param.iperf_mode) {

    if (pStreamCxt->param.pktSize < sizeof(A_UINT32)) {
    	pStreamCxt->param.pktSize = sizeof(A_UINT32);
    }
    }
	
    /* Malloc Packet Buffer Size */
    pDataBuffer = swat_mem_malloc(pStreamCxt->param.pktSize);
    if (NULL == pDataBuffer) {
        SWAT_PTF("TCP TX data buffer malloc error\r\n");
        /* Close Socket */
        swat_socket_close(pStreamCxt);
        return;
    }
    pStreamCxt->param.pktBuff = pDataBuffer;

    if (pStreamCxt->param.iperf_mode) {

   	pattern = '0';
   	index_ptr = (A_UINT32 *)pDataBuffer; /* Running index on offset 0 */
   	time_ptr = ((A_UINT32 *)pDataBuffer) + 2; /* Timer on offset 7 */
   	memset(pDataBuffer, 0, pStreamCxt->param.pktSize);

   	/* iperf fills running numbers 0-9 from offset 36 */
   	for(index = 35; index < pStreamCxt->param.pktSize; index++) {
   		pDataBuffer[index] = pattern;
   		pattern++;

   		if(pattern > '9') {
   			pattern = '0';
   		}
   	}

   	index = 0;
   	*index_ptr = htonl(index);
   	*time_ptr = (A_UINT32)swat_time_get_ms();
}
    if (!pStreamCxt->param.iperf_mode) {

    	/* Initial Packet */
    	SWAT_PTF("Sending...\r\n");
    }

    /* Get First Time */
    swat_get_first_time(pStreamCxt,DEFAULT_FD_INDEX);
    if (TEST_MODE_TIME == pStreamCxt->param.mode) {
        swat_bench_timer_init(pStreamCxt->param.seconds, pStreamCxt->param.protocol,
                              pStreamCxt->index);
    }
#ifdef SWAT_SSL
    if (pStreamCxt->param.protocol ==TEST_PROT_SSL)
    {
        if(NULL == (ssl = swat_find_ssl_inst(pStreamCxt->param.ssl_inst_index))){
            printf(" No SSL ctx found\n");
            goto ERROR;
        }

        if (ssl->ssl == NULL)
        {
            // Create SSL connection object
            ssl->ssl = qcom_SSL_new(ssl->sslCtx);
            if (ssl->ssl == NULL)
            {
                printf("ERROR: Unable to create SSL context\n");
                goto ERROR;
            }
#if 0   //If set configure before connection established, qcom_SSL_context_configure will be used. 
            // configure the SSL connection
            if (ssl->config_set)
            {
                result = qcom_SSL_configure(ssl->ssl, &ssl->config);
                if (result < A_OK)
                {
                    printf("ERROR: SSL configure failed (%d)\n", result);
                   goto ERROR;
                }
            }
  #endif
        }

        // Add socket handle to SSL connection
        result = qcom_SSL_set_fd(ssl->ssl, pStreamCxt->socketLocal);
        if (result < 0)
        {
            printf("ERROR: Unable to add socket handle to SSL (%d)\n", result);
           goto ERROR;
        }
   
        // SSL handshake with server
        result = qcom_SSL_connect(ssl->ssl);
        if (result < 0)
        {
            if (result == ESSL_TRUST_CertCnTime)
            {
                /** The peer's SSL certificate is trusted, CN matches the host name, time is valid */
                printf("The certificate is trusted\n");
            }
            else if (result == ESSL_TRUST_CertCn)
            {
                /** The peer's SSL certificate is trusted, CN matches the host name, time is expired */
                printf("ERROR: The certificate is expired\n");
                goto ERROR;
            }
            else if (result == ESSL_TRUST_CertTime)
            {
                /** The peer's SSL certificate is trusted, CN does NOT match the host name, time is valid */
                printf("ERROR: The certificate is trusted, but the host name is not valid\n");
                goto ERROR;
            }
            else if (result == ESSL_TRUST_Cert)
            {
                /** The peer's SSL certificate is trusted, CN does NOT match host name, time is expired */
                printf("ERROR: The certificate is expired and the host name is not valid\n");
                goto ERROR;
            }
            else if (result == ESSL_TRUST_None)
            {
                /** The peer's SSL certificate is NOT trusted */
                printf("ERROR: The certificate is NOT trusted\n");
                goto ERROR;
            }
            else
		    {
		        printf("ERROR: SSL connect failed (%d)\n", result);
		        goto ERROR;
		    }
        }
    }
#endif 
    while (1) {
        if (swat_bench_quit()) {
            /* Get Last Time For Pressure */
            //SWAT_PTF("Warning Bench Quit!!\n");
            swat_get_last_time(pStreamCxt,DEFAULT_FD_INDEX);
            break;
        }
#ifdef SWAT_SSL
        if ((ssl != NULL) && (ssl->role == SSL_CLIENT) &&  (ssl->ssl != NULL)){
            if(ssl->state == SSL_SHUTDOWN){
                /* Get Last Time For Pressure */
                swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
                break;
            }
        }
#endif

    if (pStreamCxt->param.iperf_mode) {

        if (sendBytes > 0){
        	if ((currentPackets + 1 == pStreamCxt->param.numOfPkts) ||
        	    benchTcpTxTimerStop[DEFAULT_FD_INDEX] ){
        		index = ~index; /* iperf last packet */
        	}

        	*index_ptr = htonl(index);
        	index++;
        	*time_ptr = (A_UINT32)swat_time_get_ms();
        }
}
#ifdef SWAT_SSL
           if (pStreamCxt->param.protocol ==TEST_PROT_SSL && NULL != ssl)
           {
               sendBytes = qcom_SSL_write(ssl->ssl, (char *)pDataBuffer, pStreamCxt->param.pktSize);
           }
           else
#endif
           {
               sendBytes =
            swat_send(pStreamCxt->socketLocal, (char *) pDataBuffer,
                      pStreamCxt->param.pktSize, 0);
           }

	    if(!pStreamCxt->param.iperf_mode) { /* Don't believe in delays */
            /* delay */
            if (gNumOfWlanDevices > 1)
            {
                A_UINT32 uidelay;

                /* 
                * For concurrency, delay 100ms. 
                * So the other device's chops have oppotunity to be schedulered. 
                */
                uidelay = (pStreamCxt->param.delay > 100) ? pStreamCxt->param.delay : 100;
                qcom_thread_msleep(pStreamCxt->param.delay);
            }
            else if (pStreamCxt->param.delay) {
                qcom_thread_msleep(pStreamCxt->param.delay);
            }
	   	}
      
        if (sendBytes < 0) {
            if (TX_BUFF_FAIL == sendBytes) {
            	if(!pStreamCxt->param.iperf_mode) {
                /* buff full will send fail, it should be normal */
                SWAT_PTF("[bench id %d, port %d]buffer full\r\n", pStreamCxt->index,
                         (pStreamCxt->param.port));
            	}
            } else {
            	if (!pStreamCxt->param.iperf_mode) {
                    SWAT_PTF
                    ("[bench id %d, port %d]TCP Socket send is error %d sumBytes = %d\r\n",
                     pStreamCxt->index, (pStreamCxt->param.port), sendBytes, sumBytes);
            	} else {
            		app_printf("send error\n");
            	}
				swat_get_last_time(pStreamCxt,DEFAULT_FD_INDEX);
                break;
            }
        } else {
            /* bytes & kbytes */
            sumBytes += sendBytes;
        }

		if (sendBytes >0){
			if (!pStreamCxt->param.iperf_mode) {
			    swat_bytes_calc(pStreamCxt, sendBytes,DEFAULT_FD_INDEX);
			}
			else {
				pStreamCxt->calc[DEFAULT_FD_INDEX].bytes += sendBytes;
			}
		}

		if (pStreamCxt->param.iperf_mode && pStreamCxt->param.iperf_display_interval) {
			A_UINT32 cur_time = swat_time_get_ms();

			if(cur_time >= iperf_display_next) {
				swat_test_iperf_result_print(pStreamCxt, DEFAULT_FD_INDEX, iperf_display_last, cur_time);
				iperf_display_last = cur_time;
				iperf_display_next = cur_time + iperf_display_interval;
			}
		}

		if (sendBytes > 0){
        /* Packets Mode */
        if (TEST_MODE_PACKETS == pStreamCxt->param.mode) {
            currentPackets++;
            if (0 != (sumBytes / pStreamCxt->param.pktSize)) {
                //currentPackets += (sumBytes / pStreamCxt->param.pktSize);
                sumBytes = sumBytes % pStreamCxt->param.pktSize;
            }

            if (currentPackets >= pStreamCxt->param.numOfPkts) {
				swat_get_last_time(pStreamCxt,DEFAULT_FD_INDEX);
                break;
            }
        }
        }

        /* Time Mode */
        if (TEST_MODE_TIME == pStreamCxt->param.mode) {

            if (0 != benchTcpTxTimerStop[pStreamCxt->index]) {
                swat_get_last_time(pStreamCxt,DEFAULT_FD_INDEX);
                break;
            }
        }
    }

    if (!pStreamCxt->param.iperf_mode) {
	    swat_test_result_print(pStreamCxt,DEFAULT_FD_INDEX);

        SWAT_PTF("IOT Throughput Test Completed.\n");
	} else {
		swat_test_iperf_result_print(pStreamCxt, DEFAULT_FD_INDEX, 0, 0);
	}

#ifdef SWAT_SSL
ERROR: 
    if ((ssl != NULL) && (ssl->role == SSL_CLIENT) &&  (ssl->ssl != NULL))
    {
        qcom_SSL_shutdown(ssl->ssl);
        ssl->ssl = NULL;
        if((ssl->state == SSL_SHUTDOWN)&&(ssl->role == SSL_CLIENT))
        {
            if (ssl->sslCtx)
            {
                qcom_SSL_ctx_free(ssl->sslCtx);
                ssl->sslCtx = NULL;
            }              
                
            swat_free_ssl_inst(pStreamCxt->param.ssl_inst_index);
            printf("SSL client stopped: Index %d\n",pStreamCxt->param.ssl_inst_index);
        } 
        else
        {
            ssl->state = SSL_FREE;
        }
    }
#endif
       
    /* Free Buffer */
    swat_buffer_free(&(pStreamCxt->param.pktBuff));
    /* Close Socket */
    swat_socket_close(pStreamCxt);
}
void
swat_tcp_rx_handle(STREAM_CXT_t * pStreamCxt)
{
    A_INT32 ret = 0;
    A_UINT32 clientIp;
    A_UINT16 clientPort;
    A_UINT32 cfd_cnt = 0;
    A_BOOL is_soclose = 0;	
    A_INT32 i = 0;
    struct sockaddr_in  clientAddr;
    struct sockaddr_in6 client6Addr;
    A_INT32 len;
    struct timeval tmo;
    A_INT32 fdAct = 0;
    q_fd_set sockSet,master;
    char ip_str[48];
    A_UINT8 *pDataBuffer = NULL;
    A_INT32 recvBytes = 0;
    A_INT32  is_client_used[MAX_SOCLIENT]={0};
#ifdef SWAT_SSL    
    SSL_INST *ssl = NULL;
#endif
    A_UINT32 iperf_display_interval = pStreamCxt->param.iperf_display_interval * 1000;
    A_UINT32 iperf_display_last = swat_time_get_ms();
    A_UINT32 iperf_display_next = iperf_display_last + iperf_display_interval;
    A_UINT32 active_sock_close = 1;

    SWAT_PTR_NULL_CHK(pStreamCxt);
#ifdef SWAT_SSL
    if (pStreamCxt->param.protocol == TEST_PROT_SSL){
        if(NULL == (ssl = swat_find_ssl_inst(pStreamCxt->param.ssl_inst_index))){
           goto QUIT;
        }
    }
#endif
    pDataBuffer = swat_mem_malloc(pStreamCxt->param.pktSize);
    if (NULL == pDataBuffer) {
        SWAT_PTF("TCP RX data buffer malloc error\r\n");
        return;
    }
    swat_mem_set(pDataBuffer, 0, sizeof (A_UINT8) * pStreamCxt->param.pktSize);
    /* Initial Bench Value */
    swat_bench_quit_init();
    pStreamCxt->pfd_set = (void *) &sockSet;
    tmo.tv_sec = 2;
    tmo.tv_usec = 0;
    /*Init all the client fds*/   
    for(i=0;i<MAX_SOCLIENT;i++){
    	pStreamCxt->clientFd[i] = -1;
    }
    if(pStreamCxt->param.is_v6_enabled){
        len = sizeof(struct sockaddr_in6);
    }else{
        len = sizeof(struct sockaddr_in);
    }
    ret = swat_listen(pStreamCxt->socketLocal, 10);
#ifdef SWAT_SSL
    if (pStreamCxt->param.protocol == TEST_PROT_TCP){
        A_INT32 enable = 1; 
        swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, SO_LOWLATENCY, (char *)&enable, sizeof(enable));
    }
#endif
    if (!pStreamCxt->param.iperf_mode) {
        SWAT_PTF("Listening on socket %d.\n", pStreamCxt->socketLocal);
	}
    while (1) {
	 /*Start select*/
        while (1) {
            if (swat_bench_quit()) {
            /*won't close local so until all the client sos close*/
                for (i=0; i<MAX_SOCLIENT; i++) {
                    if(pStreamCxt->clientFd[i] != -1){
		                swat_get_last_time(pStreamCxt,i);
                    }
                }
                goto QUIT;
            }
            /* Find the master list everytime before calling select() because select
             * modifies sockSet */
            swat_fd_zero(&master);
            swat_fd_set(pStreamCxt->socketLocal, &master);
            for (i=0; i<MAX_SOCLIENT; i++) {
                if(pStreamCxt->clientFd[i] != -1){
				/*if sockets have been closed in fw, clear the clientfd*/
		            if(swat_fd_isset(pStreamCxt->clientFd[i], &sockSet) == -1){
		            	if (!pStreamCxt->param.iperf_mode) {
                            swat_get_last_time(pStreamCxt,i);
			                swat_test_result_print(pStreamCxt,i);
		            	}
#ifdef SWAT_SSL
                        if (pStreamCxt->param.protocol ==TEST_PROT_SSL &&
                            ssl != NULL && ssl->role == SSL_SERVER && ssl->ssl != NULL)
                        {
                            qcom_SSL_shutdown(ssl->ssl);
                            ssl->ssl = NULL;
                        }
#endif
                        /* Free Buffer */
                        //swat_buffer_free(&(pDataBuffer));
                        swat_close(pStreamCxt->clientFd[i]);
                        pStreamCxt->clientFd[i] = -1; 
                        if (!pStreamCxt->param.iperf_mode) {
                            SWAT_PTF("Waiting..\n");
			    	    }
                    }else{
		                swat_fd_set(pStreamCxt->clientFd[i], &master);
		            }
                }
            }
            sockSet = master;
            fdAct = swat_select(pStreamCxt->socketLocal, &sockSet, NULL, NULL, &tmo);   //k_select()
            if (fdAct != 0) {
                break;
            }else{
			    qcom_thread_msleep(10);
			}
        }
		/*client socket select*/
	    /*receive data first in each client fd*/
	    for (i=0; i<MAX_SOCLIENT; i++) {
            if((pStreamCxt->clientFd[i] != -1)&&swat_fd_isset(pStreamCxt->clientFd[i], &sockSet)){
#ifdef SWAT_SSL                
                if (pStreamCxt->param.protocol == TEST_PROT_SSL && NULL != ssl){
                    recvBytes = qcom_SSL_read(ssl->ssl, (char*) pDataBuffer, pStreamCxt->param.pktSize);
                }
                else
#endif                    
                {
                    recvBytes =
                    swat_recv(pStreamCxt->clientFd[i], (char *) pDataBuffer, pStreamCxt->param.pktSize,0);
                }
                if (recvBytes <= 0) {
                	if (!pStreamCxt->param.iperf_mode) {
                        swat_get_last_time(pStreamCxt,i);
			swat_test_result_print(pStreamCxt,i);
                	} else {
                		pStreamCxt->param.iperf_stream_id++;
                		pStreamCxt->param.iperf_time_sec = 0;
                	}
#ifdef SWAT_SSL
                    if (pStreamCxt->param.protocol ==TEST_PROT_SSL && ssl != NULL &&
                        ssl->role == SSL_SERVER && ssl->ssl != NULL)
                    {
                        qcom_SSL_shutdown(ssl->ssl);
                        ssl->ssl = NULL;
                    }
#endif
                    swat_close(pStreamCxt->clientFd[i]);
                    pStreamCxt->clientFd[i] = -1; 
                    if (!pStreamCxt->param.iperf_mode) {
                        SWAT_PTF("Waiting..\n");
			        }
                } else {
        			if (!pStreamCxt->param.iperf_mode) {
                        swat_bytes_calc(pStreamCxt, recvBytes,i);
        			}
        			else {
        				pStreamCxt->calc[i].bytes += recvBytes;
        			}
                }

        		if (pStreamCxt->param.iperf_mode && pStreamCxt->param.iperf_display_interval) {
        			A_UINT32 cur_time = swat_time_get_ms();


        			if (cur_time >= iperf_display_next) {
        				swat_test_iperf_result_print(pStreamCxt, DEFAULT_FD_INDEX, iperf_display_last, cur_time);
        				iperf_display_last = cur_time;
        				iperf_display_next = cur_time + iperf_display_interval;
        			}
                }
		        swat_fd_clr(pStreamCxt->clientFd[i], &sockSet);
            }
        }	
    	
		/*Local socket select*/
		if (swat_fd_isset(pStreamCxt->socketLocal, &sockSet)) {
			
            /*find available socket*/
            for (i=0; i<MAX_SOCLIENT; i++){
                if(-1==pStreamCxt->clientFd[i])
                   break;
            }
            if(MAX_SOCLIENT==i){
                /*Wait till there is an available socket */
                SWAT_PTF("Failed to accept more than %d socket.\n", MAX_SOCLIENT);
                is_soclose = 0;	
                goto QUIT1;
            }
            cfd_cnt = i;

            if(pStreamCxt->param.is_v6_enabled){
                if((pStreamCxt->clientFd[cfd_cnt] = swat_accept(pStreamCxt->socketLocal, (struct sockaddr *) &client6Addr, &len)) < 0){
                    /* Close Socket */
                    SWAT_PTF("Failed to accept socket %d.\n", pStreamCxt->clientFd[cfd_cnt]);
                    is_soclose=1;
                    goto QUIT1;
                }
                //clientIp = client6Addr.sin6_addr;
                clientPort = ntohs(client6Addr.sin6_port);
                SWAT_PTF("Receiving from %s:%d\n",inet6_ntoa((char*)&client6Addr.sin6_addr,(char *)ip_str),clientPort);
            }else{

                if((pStreamCxt->clientFd[cfd_cnt] = swat_accept(pStreamCxt->socketLocal, (struct sockaddr *) &clientAddr, &len)) < 0){
                    /* Close Socket */
                    SWAT_PTF("Failed to accept socket %d.\n", pStreamCxt->clientFd[cfd_cnt]);
                    is_soclose=1;
                    goto QUIT1;
                }
                clientIp = ntohl(clientAddr.sin_addr.s_addr);
                clientPort = ntohs(clientAddr.sin_port);
                if (config_socket_active_close)
                    swat_setsockopt(pStreamCxt->clientFd[cfd_cnt], SOL_SOCKET, SO_ACTIVE_CLOSE, (char *)&active_sock_close, 
                                sizeof(active_sock_close));
                if (!pStreamCxt->param.iperf_mode) {
                    SWAT_PTF("Receiving from 0x%x Remote port:%d \r\n",
                     clientIp, clientPort);
                }
                else {
                    A_UINT32 ipv4_addr;

                	if((swat_getsockopt(pStreamCxt->socketLocal, SOL_SOCKET, SO_MYADDR, (A_UINT8*) &ipv4_addr,
                              sizeof (ipv4_addr))) < 0) {
                		ipv4_addr = 0;
                	}
                	ipv4_addr = htonl(ipv4_addr);
                	app_printf("[%3d] local %d.%d.%d.%d port %d connected with %d.%d.%d.%d port %d\n",
                			pStreamCxt->param.iperf_stream_id,
                            (ipv4_addr) >> 24 & 0xFF, (ipv4_addr) >> 16 & 0xFF,
                            (ipv4_addr) >> 8 & 0xFF, (ipv4_addr) & 0xFF,
                            pStreamCxt->param.port,
            				(clientIp) >> 24 & 0xFF, (clientIp) >> 16 & 0xFF,
            				(clientIp) >> 8 & 0xFF, (clientIp) & 0xFF,
            				clientPort);
            		app_printf("[ ID] Interval       Transfer     Bandwidth\n");
			        iperf_display_last = swat_time_get_ms();
			        iperf_display_next = iperf_display_last + iperf_display_interval;
                }
            }
			
#ifdef SWAT_SSL
			if (0 != cfd_cnt && pStreamCxt->param.protocol == TEST_PROT_SSL)
			{
				// SSL support only 1 link currently
				is_soclose = 1;
				goto QUIT1;
			}
#endif
	     	/* flag used client index for final test result*/
            is_client_used[cfd_cnt]=1;
            /* Initial Calc & Time */
            pStreamCxt->calc[cfd_cnt].firstTime.milliseconds = CALC_TIME_DEF;
            pStreamCxt->calc[cfd_cnt].lastTime.milliseconds  = CALC_TIME_DEF;
            pStreamCxt->calc[cfd_cnt].bytes  = CALC_BYTES_DEF;
            pStreamCxt->calc[cfd_cnt].kbytes = CALC_KBYTES_DEF;
#ifdef SWAT_SSL
            if (pStreamCxt->param.protocol == TEST_PROT_SSL && ssl != NULL)
            {
                if (ssl->ssl == NULL)
                {
                    // Create SSL connection object
                    ssl->ssl = qcom_SSL_new(ssl->sslCtx);
                    if (ssl->ssl == NULL)
                    {
                        printf("ERROR: Unable to create SSL context\n");
                        goto QUIT;
                    }
#if 0   //If set configure before connection established, qcom_SSL_context_configure will be used. 
                    // configure the SSL connection
                    if (ssl->config_set)
                    {
                        ret = qcom_SSL_configure(ssl->ssl, &ssl->config);
                        if (ret < A_OK)
                        {
                            printf("ERROR: SSL configure failed (%d)\n", ret);
                            goto QUIT;
                        }
                    }
#endif                    
                }

                // Add socket handle to SSL connection
                ret = qcom_SSL_set_fd(ssl->ssl, pStreamCxt->clientFd[cfd_cnt]);
                if (ret < A_OK)
                {
                    printf("ERROR: Unable to add socket handle to SSL\n");
                    goto QUIT;
                }

                // SSL handshake with server
                ret = qcom_SSL_accept(ssl->ssl);
                if (ret < 0)
                {
                    printf("ERROR: SSL accept failed (%d)\n", ret);
                    goto QUIT;
                }
            }
#endif
QUIT1:
	        swat_fd_clr(pStreamCxt->socketLocal, &sockSet);     
            if(is_soclose){
                swat_close(pStreamCxt->clientFd[cfd_cnt]);
                pStreamCxt->clientFd[cfd_cnt] = -1; 
                is_soclose=0;
            }else{
                swat_get_first_time(pStreamCxt,cfd_cnt);
            }
        }
    }
QUIT:
    for (i=0; i<MAX_SOCLIENT; i++) {
        if (pStreamCxt->clientFd[i] != -1){
        	if (!pStreamCxt->param.iperf_mode) {
                swat_test_result_print(pStreamCxt,i);
			}
                swat_close(pStreamCxt->clientFd[i]);
                pStreamCxt->clientFd[i] = -1;

        }else if(is_client_used[i]){
        /* print final test result for auto test case*/
        	if (!pStreamCxt->param.iperf_mode) {
                swat_test_result_print(pStreamCxt,i);
        	}
        }
    }
    swat_buffer_free(&(pDataBuffer));
    if (!pStreamCxt->param.iperf_mode) {
        SWAT_PTF("*************IOT Throughput Test Completed **************\n");
    } else {
		swat_test_iperf_result_print(pStreamCxt, DEFAULT_FD_INDEX, 0, 0);
	}
    SWAT_PTF("Shell> ");
    /* Init fd_set */
    swat_fd_zero(&sockSet);
    /* Close Client Socket */

#ifdef SWAT_SSL

    if ((ssl != NULL) && (ssl->role == SSL_SERVER) && (ssl->ssl != NULL))
    {
       qcom_SSL_shutdown(ssl->ssl);
       ssl->ssl = NULL;
    }
#endif

    pStreamCxt->pfd_set = NULL;
    /* Close Socket */
    swat_socket_close(pStreamCxt);
}

extern void iperf_swat_udp_tx_handle(STREAM_CXT_t * pStreamCxt);
extern A_UINT64 qcom_time_us();
void
swat_udp_tx_handle(STREAM_CXT_t * pStreamCxt)
{
    A_INT32 sendBytes = 0;
    A_INT32 fromSize = 0;
    A_UINT32 sumBytes = 0;
    A_UINT32 currentPackets = 0;
    A_UINT8 *pDataBuffer = NULL;
    A_UINT8 pattern;
    struct sockaddr_in remoteAddr;
    struct sockaddr_in6 remote6Addr;
    EOT_PACKET_t eotPacket;
    A_UINT8 ip6_str[48];
    A_UINT8 iperf_term_pkt[12];
    A_UINT32 iperfPktCnt = 0;

    A_INT32 sendTerminalCount = 0;

    A_UINT32 iperf_display_interval = pStreamCxt->param.iperf_display_interval * 1000;
    A_UINT32 iperf_display_last = swat_time_get_ms();
    A_UINT32 iperf_display_next = iperf_display_last + iperf_display_interval;
    A_UINT32 index;
    A_UINT32 *index_ptr;
    A_UINT32 *time_ptr;

    int iperf_udp_delay_target = 0; 
    int iperf_udp_delay = 0; 
    int iperf_udp_adj= 0; 
    A_UINT32 iperf_last_pkt_utime = 0;
    A_UINT32 iperf_curr_pkt_utime = 0;
#ifdef SWAT_SSL
    A_INT32 result = 0;
    SSL_INST *ssl = NULL;
#endif

    SWAT_PTR_NULL_CHK(pStreamCxt);

    /* Initial Bench Value */
    swat_bench_quit_init();

    /* Initial Calc & Time */
    pStreamCxt->calc[DEFAULT_FD_INDEX].firstTime.milliseconds = CALC_TIME_DEF;
    pStreamCxt->calc[DEFAULT_FD_INDEX].lastTime.milliseconds = CALC_TIME_DEF;
    pStreamCxt->calc[DEFAULT_FD_INDEX].bytes = CALC_BYTES_DEF;
    pStreamCxt->calc[DEFAULT_FD_INDEX].kbytes = CALC_KBYTES_DEF;
    if (pStreamCxt->param.pktSize < sizeof(A_UINT32)*3) {
    	pStreamCxt->param.pktSize = sizeof(A_UINT32)*3;
    }

    /* Malloc Packet Buffer Size */
    pDataBuffer = swat_mem_malloc(pStreamCxt->param.pktSize);
    if (NULL == pDataBuffer) {
        SWAT_PTF("UDP TX data buffer malloc error\r\n");
        /* Close Socket */
        swat_socket_close(pStreamCxt);
        return;
    }
    pStreamCxt->param.pktBuff = pDataBuffer;
    /* Prepare IP address & port */
    memset(&remoteAddr, 0, sizeof (struct sockaddr_in));
    memset(&remote6Addr, 0, sizeof (struct sockaddr_in6));

    if(pStreamCxt->param.is_v6_enabled){
        remote6Addr.sin6_port = HTONS(pStreamCxt->param.port);
    	remote6Addr.sin6_family = AF_INET6;	
        memcpy(&remote6Addr.sin6_addr,pStreamCxt->param.ip6Address.addr, sizeof(IP6_ADDR_T));
        fromSize = sizeof (struct sockaddr_in6);
    }else{
        remoteAddr.sin_addr.s_addr= HTONL(pStreamCxt->param.ipAddress);
        remoteAddr.sin_port = HTONS(pStreamCxt->param.port);
        remoteAddr.sin_family = AF_INET;
        fromSize = sizeof (struct sockaddr_in);
    }
    if (!pStreamCxt->param.iperf_mode) {
    /* Initial Packet */
    if (!v6_enabled)  {
        SWAT_PTF("UDP IP %d.%d.%d.%d prepare OK\r\n",
             (pStreamCxt->param.ipAddress) >> 24 & 0xFF, (pStreamCxt->param.ipAddress) >> 16 & 0xFF,
             (pStreamCxt->param.ipAddress) >> 8 & 0xFF, (pStreamCxt->param.ipAddress) & 0xFF);
    } else {
        inet6_ntoa((char *)&pStreamCxt->param.ip6Address,(char *)ip6_str);
        SWAT_PTF("UDP IPv6 %s prepare OK\r\n", ip6_str);
    }
#ifdef SWAT_SSL
    if (pStreamCxt->param.protocol == TEST_PROT_DTLS){
        
        A_INT32 ret = 0;
        if (pStreamCxt->param.is_v6_enabled){
            ret = swat_connect(pStreamCxt->socketLocal, (struct sockaddr *) &remote6Addr,
                         sizeof (struct  sockaddr_in6));
        } else {
            ret = swat_connect(pStreamCxt->socketLocal, (struct sockaddr *) &remoteAddr,
                     sizeof (struct  sockaddr_in));
        } 

        if (ret  < 0) {
            printf(" swat connect fail\n");
            goto ERROR;            
        }
      
        if(NULL == (ssl = swat_find_ssl_inst(pStreamCxt->param.ssl_inst_index))){
            printf(" No SSL ctx found\n");
            goto ERROR;
        }

        if (ssl->ssl == NULL)
        {
            // Create SSL connection object
            ssl->ssl = qcom_DTLS_new(ssl->sslCtx);
            if (ssl->ssl == NULL)
            {
                printf("ERROR: Unable to create SSL context\n");
                goto ERROR;
            }
        }

        // Add socket handle to SSL connection
        result = qcom_SSL_set_fd(ssl->ssl, pStreamCxt->socketLocal);
        if (result < 0)
        {
            printf("ERROR: Unable to add socket handle to SSL (%d)\n", result);
           goto ERROR;
        }

        // SSL handshake with server
        result = qcom_SSL_connect(ssl->ssl);
        if (result < 0)
        {
            if (result == ESSL_TRUST_CertCnTime)
            {
                /** The peer's SSL certificate is trusted, CN matches the host name, time is valid */
                printf("The certificate is trusted\n");
            }
            else if (result == ESSL_TRUST_CertCn)
            {
                /** The peer's SSL certificate is trusted, CN matches the host name, time is expired */
                printf("ERROR: The certificate is expired\n");
                goto ERROR;
            }
            else if (result == ESSL_TRUST_CertTime)
            {
                /** The peer's SSL certificate is trusted, CN does NOT match the host name, time is valid */
                printf("ERROR: The certificate is trusted, but the host name is not valid\n");
                goto ERROR;
            }
            else if (result == ESSL_TRUST_Cert)
            {
                /** The peer's SSL certificate is trusted, CN does NOT match host name, time is expired */
                printf("ERROR: The certificate is expired and the host name is not valid\n");
                goto ERROR;
            }
            else if (result == ESSL_TRUST_None)
            {
                /** The peer's SSL certificate is NOT trusted */
                printf("ERROR: The certificate is NOT trusted\n");
                goto ERROR;
            }
            else
	     {
		    printf("ERROR: SSL connect failed (%d)\n", result);
		    goto ERROR;
		}
        }       
    }
#endif    
    SWAT_PTF("Sending...\r\n");
    } else {
        A_UINT32 ipv4_addr;

    	if((swat_getsockopt(pStreamCxt->socketLocal, SOL_SOCKET, SO_MYADDR, (A_UINT8*) &ipv4_addr,
                  sizeof (ipv4_addr))) < 0) {
    		ipv4_addr = 0;
    	}

    	ipv4_addr = htonl(ipv4_addr);

    	app_printf("[%3d] local %d.%d.%d.%d port %d connected with %d.%d.%d.%d port %d\n",
    			pStreamCxt->param.iperf_stream_id,
                (ipv4_addr) >> 24 & 0xFF, (ipv4_addr) >> 16 & 0xFF,
                (ipv4_addr) >> 8 & 0xFF, (ipv4_addr) & 0xFF,
    			0,
				(pStreamCxt->param.ipAddress) >> 24 & 0xFF, (pStreamCxt->param.ipAddress) >> 16 & 0xFF,
				(pStreamCxt->param.ipAddress) >> 8 & 0xFF, (pStreamCxt->param.ipAddress) & 0xFF,
				pStreamCxt->param.port);
		 app_printf("[ ID] Interval       Transfer     Bandwidth\n");
		
	
          // compute delay for bandwidth restriction, constrained to [0,1] seconds 
          iperf_udp_delay_target = (int) ( pStreamCxt->param.pktSize* ((1000*1000 * 8) 
                                                     / pStreamCxt->param.iperf_udp_rate)); 
          if (iperf_udp_delay_target < 0  || 
              iperf_udp_delay_target > (int) (1 * 1000*1000) ) {
              iperf_udp_delay_target = (int) (1000*1000 * 1); 	
          }	
    }

    /* Get First Time */
    swat_get_first_time(pStreamCxt, DEFAULT_FD_INDEX);
    if (TEST_MODE_TIME == pStreamCxt->param.mode) {
        swat_bench_timer_init(pStreamCxt->param.seconds, pStreamCxt->param.protocol,
                              pStreamCxt->index);
    }

   	/* Create a pattern */
   	pattern = '0';
   	index_ptr = (A_UINT32 *)pDataBuffer; /* Running index on offset 0 */
   	time_ptr = ((A_UINT32 *)pDataBuffer) + 2; /* Timer on offset 7 */
   	memset(pDataBuffer, 0, pStreamCxt->param.pktSize);
   	/* iperf fills running numbers 0-9 from offset 36 */
   	for (index = 35; index < pStreamCxt->param.pktSize; index++) {
   		pDataBuffer[index] = pattern;
   		pattern++;

   		if(pattern > '9') {
   			pattern = '0';
   		}
   	}
   	index = 1;
   	*index_ptr = htonl(index);
   	*time_ptr = (A_UINT32)swat_time_get_ms();
	if (pStreamCxt->param.iperf_mode) {
	    iperf_last_pkt_utime = qcom_time_us();
	}
    while (1) {
        if (swat_bench_quit()) {
            /* Get Last Time For Pressure */
            //SWAT_PTF("Bench quit!!\r\n");
            swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
            break;
        }

#ifdef SWAT_SSL
        if ((ssl != NULL) && (ssl->role == SSL_CLIENT) &&  (ssl->ssl != NULL)){
            if(ssl->state == SSL_SHUTDOWN){
                /* Get Last Time For Pressure */
                swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
                break;
            }
        }
#endif

        if (sendBytes > 0){
        	*index_ptr = htonl(index);
			
        	index++;
        	*time_ptr = (A_UINT32)swat_time_get_ms();
        }
	    if (pStreamCxt->param.iperf_mode) {
	 	    iperf_curr_pkt_utime = qcom_time_us();
		    iperf_udp_adj = iperf_udp_delay_target + (iperf_last_pkt_utime-iperf_curr_pkt_utime);
		    iperf_last_pkt_utime = iperf_curr_pkt_utime;
	        if (iperf_udp_adj> 0  ||  iperf_udp_delay > 0 ) {
                iperf_udp_delay += iperf_udp_adj; 	
            } 
        }
#ifdef SWAT_SSL
        if (pStreamCxt->param.protocol == TEST_PROT_DTLS){
            if(pStreamCxt->param.is_v6_enabled){
                sendBytes = qcom_DTLS_writeto(ssl->ssl, (char*) pDataBuffer,
                                    pStreamCxt->param.pktSize, 0,
                                     (struct sockaddr*)&remote6Addr, sizeof (struct sockaddr_in6));
            }else{
                sendBytes = qcom_DTLS_writeto(ssl->ssl, (char*) pDataBuffer,
                                    pStreamCxt->param.pktSize, 0,
                                     (struct sockaddr*)&remoteAddr, sizeof (struct sockaddr_in));
            }

        } 
        else 
#endif
        {
            if(pStreamCxt->param.is_v6_enabled){
                sendBytes = swat_sendto(pStreamCxt->socketLocal, (char*) pDataBuffer,
                                    pStreamCxt->param.pktSize, 0,
                                     (struct sockaddr*)&remote6Addr, sizeof (struct sockaddr_in6));
            }else{
                sendBytes = swat_sendto(pStreamCxt->socketLocal, (char*) pDataBuffer,
                                    pStreamCxt->param.pktSize, 0,
                                     (struct sockaddr*)&remoteAddr, sizeof (struct sockaddr_in));
            }
        }   
		/*delay */
		if (pStreamCxt->param.delay){
			qcom_thread_msleep(pStreamCxt->param.delay);
		}
		if (pStreamCxt->param.iperf_mode &&(iperf_udp_delay/1000 > 0)){
			qcom_thread_msleep(iperf_udp_delay/1000);
		}		

        if (sendBytes < 0) {
        	if (!pStreamCxt->param.iperf_mode) {
                SWAT_PTF("UDP Socket send is error %d sumBytes = %d\r\n", sendBytes, sumBytes);
        	}
            /* Free Buffer */
            //swat_buffer_free(&(pStreamCxt->param.pktBuff));
            qcom_thread_msleep(100);
            //break;
        } else {
            /* bytes & kbytes */
            sumBytes += sendBytes;
        }

		if (sendBytes > 0){
			if (!pStreamCxt->param.iperf_mode) {
			    swat_bytes_calc(pStreamCxt, sendBytes, DEFAULT_FD_INDEX);
            }
			else {
				pStreamCxt->calc[DEFAULT_FD_INDEX].bytes += sendBytes;
			}
		}

		if (pStreamCxt->param.iperf_mode && pStreamCxt->param.iperf_display_interval) {
			A_UINT32 cur_time = swat_time_get_ms();

			if(cur_time >= iperf_display_next) {
				swat_test_iperf_result_print(pStreamCxt, DEFAULT_FD_INDEX, iperf_display_last, cur_time);
				iperf_display_last = cur_time;
				iperf_display_next = cur_time + iperf_display_interval;
			}
		}
		if (sendBytes > 0){
           /* Packets Mode */
           if (TEST_MODE_PACKETS == pStreamCxt->param.mode) {
               currentPackets++;
               if (0 != (sumBytes / pStreamCxt->param.pktSize)) {
                   //currentPackets += (sumBytes / pStreamCxt->param.pktSize);
                   sumBytes = sumBytes % pStreamCxt->param.pktSize;
               }

               if (currentPackets >= pStreamCxt->param.numOfPkts) {
				   swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
                   break;
               }
            }

            /* Time Mode */
            if (TEST_MODE_TIME == pStreamCxt->param.mode) {
			    /* Get Last Time */
                if (0 != benchUdpTxTimerStop[pStreamCxt->index]) {
                    swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
                    break;
                }
            }
		}       
    }

	iperfPktCnt =( index +1);
	index = ~index; /* iperf last packet */

    int send_fail = 0;
    /* Tell ath_console TX is complete end mark is AABBCCDD */
    while (send_fail <= 10) {
		if(!pStreamCxt->param.iperf_mode) {
        eotPacket.code = 0xAABBCCDD;
        eotPacket.packetCount = currentPackets;
#ifdef SWAT_SSL
        if (pStreamCxt->param.protocol == TEST_PROT_DTLS){           
            if(pStreamCxt->param.is_v6_enabled){
                sendBytes = qcom_DTLS_writeto(ssl->ssl, (char *) & eotPacket,
                                    sizeof (EOT_PACKET_t), 0, (struct sockaddr *) &remote6Addr,
                                    sizeof (struct sockaddr_in6));
            }else{
                sendBytes = qcom_DTLS_writeto(ssl->ssl, (char *) & eotPacket,
                                    sizeof (EOT_PACKET_t), 0, (struct sockaddr *) &remoteAddr,
                                    sizeof (struct sockaddr_in));
               }
        } 
        else 
#endif
        {
            if(pStreamCxt->param.is_v6_enabled){
                sendBytes = swat_sendto(pStreamCxt->socketLocal, (char *) & eotPacket,
                                    sizeof (EOT_PACKET_t), 0, (struct sockaddr *) &remote6Addr,
                                    sizeof (struct sockaddr_in6));
            }else{
                sendBytes = swat_sendto(pStreamCxt->socketLocal, (char *) & eotPacket,
                                    sizeof (EOT_PACKET_t), 0, (struct sockaddr *) &remoteAddr,
                                    sizeof (struct sockaddr_in));
               }
         }
		} else {
            *((A_UINT32*)iperf_term_pkt) =  htonl(index);
        	*((A_UINT32*)iperf_term_pkt + 2) = (A_UINT32)swat_time_get_ms();
			//*index_ptr = htonl(index);
			//*time_ptr = (A_UINT32)swat_time_get_ms();

			if(pStreamCxt->param.is_v6_enabled){
				sendBytes = swat_sendto(pStreamCxt->socketLocal, (char *) & iperf_term_pkt,
                        /*pStreamCxt->param.pktSize*/sizeof (iperf_term_pkt), 0, (struct sockaddr *) &remote6Addr,
									sizeof (struct sockaddr_in6));
			} else{
				sendBytes = swat_sendto(pStreamCxt->socketLocal, (char *) & iperf_term_pkt,
                        /*pStreamCxt->param.pktSize*/sizeof (iperf_term_pkt), 0, (struct sockaddr *) &remoteAddr,
									sizeof (struct sockaddr_in));
			   //dump_hex((A_UINT8*)&iperf_term_pkt, sizeof (iperf_term_pkt));
			}
        }
        if (sendBytes < 0) {
            SWAT_PTF("UDP send terminate packet error %d , retry %d \r\n", sendBytes,
                     sendTerminalCount);

            send_fail ++;
            qcom_thread_msleep(100);
        } else {
            qcom_thread_msleep(100);

            sendTerminalCount++;
            if(sendTerminalCount > 2)
                break;
        }
    }

	if (!pStreamCxt->param.iperf_mode) {
	    swat_test_result_print(pStreamCxt, DEFAULT_FD_INDEX);
        SWAT_PTF("*************IOT Throughput Test Completed **************\n");
    } else {
        SWAT_PTF("[%3d] Sent %d Datagrams\n",pStreamCxt->param.iperf_stream_id, iperfPktCnt);
		swat_test_iperf_result_print(pStreamCxt, DEFAULT_FD_INDEX, 0, 0);
	}
#ifdef SWAT_SSL
ERROR:
        if ((ssl != NULL) && (ssl->role == SSL_CLIENT) &&  (ssl->ssl != NULL))
        {
            qcom_SSL_shutdown(ssl->ssl);
            ssl->ssl = NULL;
            if((ssl->state == SSL_SHUTDOWN)&&(ssl->role == SSL_CLIENT))
            {
                if (ssl->sslCtx)
                {
                    qcom_SSL_ctx_free(ssl->sslCtx);
                    ssl->sslCtx = NULL;
                }              
                    
                swat_free_ssl_inst(pStreamCxt->param.ssl_inst_index);
                printf("SSL client stopped: Index %d\n",pStreamCxt->param.ssl_inst_index);
            }
            else 
            {
                ssl->state = SSL_FREE;
            }
        }
#endif
    /* Free Buffer */
    swat_buffer_free(&(pStreamCxt->param.pktBuff));

    /* Close Socket */
    swat_socket_close(pStreamCxt);
}

void
swat_udp_rx_data(STREAM_CXT_t * pStreamCxt)
{
    A_INT32 recvBytes = 0;
    A_INT32 lastRecvBytes = 0;
    A_INT32 fromSize = 0;
    A_UINT32 sumBytes = 0;
    A_UINT8 *pDataBuffer = NULL;
    struct sockaddr_in fromAddr;
    struct sockaddr_in6 from6Addr;
    q_fd_set sockSet, master;
    struct timeval tmo;
    A_INT32 fdAct = 0;
    A_UINT32 isFirst = 1;
    static A_UINT32 streamTestEnd = 0;
    A_UINT32 clientIp = 0;
    A_UINT16 clientPort = 0;
    A_UINT32 totalInterval = 0;
    A_INT32 sendBytes = 0;
    STAT_PACKET_t StatPacket;
    A_UINT8 ip6_str[48];

    A_INT32 sendTerminalCount = 0;
    A_UINT32 iperf_display_interval = pStreamCxt->param.iperf_display_interval * 1000;
    A_UINT32 iperf_display_last = swat_time_get_ms();
    A_UINT32 iperf_display_next = iperf_display_last + iperf_display_interval;
    A_INT32 packet_index;

    SWAT_PTR_NULL_CHK(pStreamCxt);
#ifdef SWAT_SSL    
    SSL_INST *ssl = NULL;
    if (pStreamCxt->param.protocol == TEST_PROT_DTLS){
        if(NULL == (ssl = swat_find_ssl_inst(pStreamCxt->param.ssl_inst_index))){
            return;
        }
    }   
#endif    
    /* Initial Bench Value */
    swat_bench_quit_init();

    /* Malloc Packet Buffer Size */
    pDataBuffer = swat_mem_malloc(pStreamCxt->param.pktSize);
    if (NULL == pDataBuffer) {
        SWAT_PTF("UDP RX data buffer malloc error\r\n");
        return;
    }
    pStreamCxt->param.pktBuff = pDataBuffer;

    memset(&StatPacket, 0, sizeof (StatPacket));

    /* Prepare IP address & port */
    if(pStreamCxt->param.is_v6_enabled){
        memset(&from6Addr, 0, sizeof (struct sockaddr_in6));
        fromSize = sizeof (struct sockaddr_in6);
    }else{
        memset(&fromAddr, 0, sizeof (struct sockaddr_in));
        fromSize = sizeof (struct sockaddr_in);
    }

    /* Init fd_set */
    swat_fd_zero(&master);
    swat_fd_set(pStreamCxt->socketLocal, &master);
    pStreamCxt->pfd_set = (void *)&master;
    tmo.tv_sec = 1;
    tmo.tv_usec = 0;

    /* Get First Time */
	//removed for EV138076 by zhengang
    //swat_get_first_time(pStreamCxt);

    while (1) {
        if (swat_bench_quit()) {
            /* Get Last Time For Pressure */
			if(!isFirst)
				swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
            break;
        }

    /* Copy the master list everytime before calling select() because select
         * modifies sockSet */
        sockSet = master;        /* Wait for Input */
        fdAct = swat_select(pStreamCxt->socketLocal + 1, &sockSet, NULL, NULL, &tmo);   //k_select()
        if (0 != fdAct) {
            if (swat_fd_isset(pStreamCxt->socketLocal, &sockSet)) {
#ifdef SWAT_SSL
                if (pStreamCxt->param.protocol == TEST_PROT_DTLS){           
                     if(pStreamCxt->param.is_v6_enabled){
                         recvBytes = qcom_DTLS_readfrom(ssl->ssl, (char*) pDataBuffer,
                                               pStreamCxt->param.pktSize, 0,
                                               (struct sockaddr *) &from6Addr, &fromSize);
                    
                     }else{
                         recvBytes = qcom_DTLS_readfrom(ssl->ssl, (char*) pDataBuffer,
                                               pStreamCxt->param.pktSize, 0,
                                               (struct sockaddr *) &fromAddr, &fromSize);
                     }
                }   
                else 
#endif
               {
                    if(pStreamCxt->param.is_v6_enabled){
                        recvBytes = swat_recvfrom(pStreamCxt->socketLocal, (char*) pDataBuffer,
                                              pStreamCxt->param.pktSize, 0,
                                              (struct sockaddr *) &from6Addr, &fromSize);

                    }else{
                        recvBytes = swat_recvfrom(pStreamCxt->socketLocal, (char*) pDataBuffer,
                                              pStreamCxt->param.pktSize, 0,
                                              (struct sockaddr *) &fromAddr, &fromSize);
                    }
                }
                if (recvBytes <= 0) {
                    SWAT_PTF("UDP Socket receive is error %d, sumBytes = %d\r\n", recvBytes,
                             sumBytes);
                    break;
                
                }
		        packet_index = ntohl(*(A_UINT32*)pDataBuffer);		
                if (((!pStreamCxt->param.iperf_mode)&&(recvBytes >= sizeof (EOT_PACKET_t) )) ||
				    (pStreamCxt->param.iperf_mode && (packet_index >=0))){
                    if (isFirst) {
                        if (recvBytes > sizeof (EOT_PACKET_t)) {
                            clientPort = ntohs(fromAddr.sin_port);
                            if(pStreamCxt->param.is_v6_enabled){
                                clientPort = ntohs(from6Addr.sin6_port);
                                inet6_ntoa((char *)&from6Addr.sin6_addr,(char *)ip6_str);
                                if (!pStreamCxt->param.iperf_mode) {
                                    SWAT_PTF("UDP receving from %s port:%d \r\n",ip6_str,clientPort);
                                }
                            }else{
                                clientIp = ntohl(fromAddr.sin_addr.s_addr);

                            	if (!pStreamCxt->param.iperf_mode) {
                                    SWAT_PTF("UDP receving from 0x%x port:%d \r\n",
                                     clientIp, clientPort);
                            	}
                            }

                        	if (pStreamCxt->param.iperf_mode) {
                        		A_UINT32 ipv4_addr;

    							if((swat_getsockopt(pStreamCxt->socketLocal, SOL_SOCKET, SO_MYADDR, (A_UINT8*) &ipv4_addr,
    									  sizeof (ipv4_addr))) < 0) {
    								ipv4_addr = 0;
    							}
    							ipv4_addr = htonl(ipv4_addr);
    							app_printf("[%3d] local %d.%d.%d.%d port %d connected with %d.%d.%d.%d port %d\n",
    									pStreamCxt->param.iperf_stream_id,
    									(ipv4_addr) >> 24 & 0xFF, (ipv4_addr) >> 16 & 0xFF,
    									(ipv4_addr) >> 8 & 0xF, (ipv4_addr) & 0xFF,
    									pStreamCxt->param.port,
    									(clientIp) >> 24 & 0xFF, (clientIp) >> 16 & 0xFF,
    									(clientIp) >> 8 & 0xFF, (clientIp) & 0xFF,
    									clientPort);
    							app_printf("[ ID] Interval       Transfer     Bandwidth\n");
							
						        iperf_display_last = swat_time_get_ms();
                					iperf_display_next = iperf_display_last + iperf_display_interval;
    						}
                            isFirst = 0;
							/* Initial Calc & Time */
							//for EV 138076, only init the data when it is a new udp connection
							pStreamCxt->calc[DEFAULT_FD_INDEX].firstTime.milliseconds = CALC_TIME_DEF;
							pStreamCxt->calc[DEFAULT_FD_INDEX].lastTime.milliseconds = CALC_TIME_DEF;
							pStreamCxt->calc[DEFAULT_FD_INDEX].bytes	= CALC_BYTES_DEF;
							pStreamCxt->calc[DEFAULT_FD_INDEX].kbytes = CALC_KBYTES_DEF;

                            swat_get_first_time(pStreamCxt, DEFAULT_FD_INDEX);
                        }
            			if (!pStreamCxt->param.iperf_mode) {
                            swat_bytes_calc(pStreamCxt, recvBytes, DEFAULT_FD_INDEX);
            			}
            			else {
            				pStreamCxt->calc[DEFAULT_FD_INDEX].bytes += recvBytes;
            			}
                    } else {
                		if(pStreamCxt->param.iperf_mode && pStreamCxt->param.iperf_display_interval) {
                			A_UINT32 cur_time = swat_time_get_ms();

                			if(cur_time >= iperf_display_next) {
                				swat_test_iperf_result_print(pStreamCxt, DEFAULT_FD_INDEX, iperf_display_last, cur_time);
                				iperf_display_last = cur_time;
                				iperf_display_next = cur_time + iperf_display_interval;
                			}
                		}
                    	/*End packet is not count*/
                    	if (recvBytes > sizeof (EOT_PACKET_t)) {
                    	    /*record the last packets length*/
                            lastRecvBytes = recvBytes;
                			if (!pStreamCxt->param.iperf_mode) {
                               swat_bytes_calc(pStreamCxt, recvBytes, DEFAULT_FD_INDEX);
                			}
                			else {
                				pStreamCxt->calc[DEFAULT_FD_INDEX].bytes += recvBytes;
                			}
                    	}
						else {
							/* Update Port */
                            if(pStreamCxt->param.is_v6_enabled){
                                from6Addr.sin6_port = HTONS(clientPort);
                                from6Addr.sin6_family = AF_INET6;	/* End Of Transfer */
                            }else{
                                fromAddr.sin_port = HTONS(clientPort);
                                fromAddr.sin_family = AF_INET;	/* End Of Transfer */
                            }
                            swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);

                            totalInterval =
                                (pStreamCxt->calc[DEFAULT_FD_INDEX].lastTime.milliseconds -
                                 pStreamCxt->calc[DEFAULT_FD_INDEX].firstTime.milliseconds);
							
							if (totalInterval == 0
								&& (pStreamCxt->calc[DEFAULT_FD_INDEX].kbytes || pStreamCxt->calc[DEFAULT_FD_INDEX].bytes >= 512))
							{
								// time deviation as unit is ms.
								totalInterval = 1;
								pStreamCxt->calc[DEFAULT_FD_INDEX].lastTime.milliseconds += 1;
							}
							
                            StatPacket.kbytes = pStreamCxt->calc[DEFAULT_FD_INDEX].kbytes;
                            StatPacket.bytes = pStreamCxt->calc[DEFAULT_FD_INDEX].bytes;
                            StatPacket.msec = totalInterval;
                            /* Tell ath_console TX received end mark AABBCCDD with throughput value */
                            while (sendTerminalCount <= 10) {
#ifdef SWAT_SSL
                            if (pStreamCxt->param.protocol == TEST_PROT_DTLS){           
                                if(pStreamCxt->param.is_v6_enabled){
                                    sendBytes = qcom_DTLS_writeto(ssl->ssl,
                                                            (char *) (&StatPacket),
                                                            sizeof (STAT_PACKET_t), 0,
                                                            (struct sockaddr*)&from6Addr,
                                                            sizeof (struct sockaddr_in6));

                                }else{
                                    sendBytes = qcom_DTLS_writeto(ssl->ssl,
                                                            (char *) (&StatPacket),
                                                            sizeof (STAT_PACKET_t), 0,
                                                            (struct sockaddr*)&fromAddr,
                                                            sizeof (struct sockaddr_in));
                                }
                             }   
                             else 
#endif
                            {
                                if(pStreamCxt->param.is_v6_enabled){
                                    sendBytes = swat_sendto(pStreamCxt->socketLocal,
                                                            (char *) (&StatPacket),
                                                            sizeof (STAT_PACKET_t), 0,
                                                            (struct sockaddr*)&from6Addr,
                                                            sizeof (struct sockaddr_in6));

                                }else{
                                    sendBytes = swat_sendto(pStreamCxt->socketLocal,
                                                            (char *) (&StatPacket),
                                                            sizeof (STAT_PACKET_t), 0,
                                                            (struct sockaddr*)&fromAddr,
                                                            sizeof (struct sockaddr_in));
                                }
                            }    
                                if (sendBytes < 0) {
                                    SWAT_PTF("UDP send throughput info packet error %d , retry %d \r\n",
                                                 sendBytes, sendTerminalCount);
                                    qcom_thread_msleep(100);
                                } else {
                                    /* Clean */
                                    tmo.tv_sec = 2;
                                    tmo.tv_usec = 0;
                                    /*if packets are the retransmition packets, consume them*/
                                    while ((recvBytes == sizeof (EOT_PACKET_t)) ||(lastRecvBytes == recvBytes)) {
                                        /* Copy the master list everytime before calling select() because select
                                         * modifies sockSet */
                                        sockSet = master;        /* Wait for Input */

                                        fdAct = swat_select(pStreamCxt->socketLocal + 1, &sockSet, NULL, NULL, &tmo);   //k_select()
                                        if (0 == fdAct) {
                                            SWAT_PTF("UDP break\n");
                                            break;
                                        }
                                        /*retransmission to prevent peer losing packets*/
                                        if (swat_fd_isset(pStreamCxt->socketLocal, &sockSet)) {
#ifdef SWAT_SSL
                                        if (pStreamCxt->param.protocol == TEST_PROT_DTLS){           
                                             if(pStreamCxt->param.is_v6_enabled){
                                                 recvBytes = qcom_DTLS_readfrom(ssl->ssl, (char*) pDataBuffer,
                                                                           pStreamCxt->param.pktSize, 0,
                                                                           (struct sockaddr *) &from6Addr, &fromSize);
                                                // from6Addr.sin6_port = HTONS(pStreamCxt->param.port);
                                                 from6Addr.sin6_family = AF_INET6;   /* End Of Transfer */
                                                 if (recvBytes == sizeof (EOT_PACKET_t)) {
                                                         sendBytes = qcom_DTLS_writeto(ssl->ssl,
                                                             (char *) (&StatPacket),
                                                             sizeof (STAT_PACKET_t), 0,
                                                             (struct sockaddr*)&from6Addr,
                                                             sizeof (struct sockaddr_in6));
                                                 }
                                             }else{
                                                 recvBytes = qcom_DTLS_readfrom(ssl->ssl, (char*) pDataBuffer,
                                                                           pStreamCxt->param.pktSize, 0,
                                                                           (struct sockaddr *)&fromAddr, &fromSize);
                                                 //fromAddr.sin_port = HTONS(pStreamCxt->param.port);
                                                 fromAddr.sin_family = AF_INET;  /* End Of Transfer */
                                                 if (recvBytes == sizeof (EOT_PACKET_t)) {
                                                         sendBytes = qcom_DTLS_writeto(ssl->ssl,
                                                             (char *) (&StatPacket),
                                                             sizeof (STAT_PACKET_t), 0,
                                                             (struct sockaddr*)&fromAddr,
                                                             sizeof (struct sockaddr_in));
                                                }
                                            }
                                        } 
                                        else 
#endif
                                        {
                                            if(pStreamCxt->param.is_v6_enabled){
                                                recvBytes = swat_recvfrom(pStreamCxt->socketLocal, (char*) pDataBuffer,
                                                                          pStreamCxt->param.pktSize, 0,
                                                                          (struct sockaddr *) &from6Addr, &fromSize);
                                                from6Addr.sin6_port = HTONS(pStreamCxt->param.port);  /*ath_console will bind same port with server to accept status packet*/
                                                from6Addr.sin6_family = AF_INET6;	/* End Of Transfer */
                                                if (recvBytes == sizeof (EOT_PACKET_t)) {
						                                sendBytes = swat_sendto(pStreamCxt->socketLocal,
                                                            (char *) (&StatPacket),
                                                            sizeof (STAT_PACKET_t), 0,
                                                            (struct sockaddr*)&from6Addr,
                                                            sizeof (struct sockaddr_in6));
                                                }
                                            }else{
                                                recvBytes = swat_recvfrom(pStreamCxt->socketLocal, (char*) pDataBuffer,
                                                                          pStreamCxt->param.pktSize, 0,
                                                                          (struct sockaddr *)&fromAddr, &fromSize);
                                                fromAddr.sin_port = HTONS(pStreamCxt->param.port);   /*ath_console will bind same port with server to accept status packet*/
                                                fromAddr.sin_family = AF_INET;	/* End Of Transfer */
                                                if (recvBytes == sizeof (EOT_PACKET_t)) {
						                                sendBytes = swat_sendto(pStreamCxt->socketLocal,
                                                            (char *) (&StatPacket),
                                                            sizeof (STAT_PACKET_t), 0,
                                                            (struct sockaddr*)&fromAddr,
                                                            sizeof (struct sockaddr_in));
                                               }
                                           }
                                       }
                                     }
                                   }
                                   break;
                               }
                               sendTerminalCount++;
                           }
                           break;
                       }
                   }
               } else if ((pStreamCxt->param.iperf_mode && (packet_index < 0))){
               	   streamTestEnd = 1;
			       break;
               }
            }
        }
    }

#ifdef SWAT_SSL    
    if (pStreamCxt->param.protocol == TEST_PROT_DTLS){       
        if(pStreamCxt->param.is_v6_enabled){
            qcom_DTLS_reset(ssl->ssl,(struct sockaddr *)&from6Addr);
        }
        else{
            qcom_DTLS_reset(ssl->ssl,(struct sockaddr *)&fromAddr);
        }
    }
#endif


    if (!pStreamCxt->param.iperf_mode) {
        swat_test_result_print(pStreamCxt, DEFAULT_FD_INDEX);
        SWAT_PTF("Waiting.\n");
    } else {
        if (streamTestEnd && !isFirst) {
		    swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
		    swat_test_iperf_result_print(pStreamCxt, DEFAULT_FD_INDEX, 0, 0);
		    pStreamCxt->param.iperf_time_sec = 0;
		    streamTestEnd =0;
    	}
	}
    /* Free Buffer */
    swat_buffer_free(&(pStreamCxt->param.pktBuff));

}
#ifdef SWAT_BENCH_RAW
void swat_raw_rx_data(STREAM_CXT_t* pStreamCxt)
{
    A_INT32  recvBytes = 0;
    A_INT32  fromSize = 0;
    A_UINT32 sumBytes = 0;
    A_UINT8* pDataBuffer = NULL;
    struct sockaddr_in fromAddr;
    q_fd_set master;
    q_fd_set sockSet;
    struct timeval tmo;
    A_INT32  fdAct = 0;
    A_UINT32 isFirst = 1;
    A_UINT32 clientIp = 0;
    A_UINT16 clientPort = 0;
    A_UINT32 totalInterval = 0;
    A_UINT32 sendBytes = 0;
    STAT_PACKET_t StatPacket;
    ip_header *iphdr;
    unsigned short int ip_flags[4];
    int size;
    A_UINT8 ip_str[16];
    A_INT32 sendTerminalCount = 0;

    SWAT_PTR_NULL_CHK(pStreamCxt);

    /* Initial Bench Value */
    swat_bench_quit_init();

    /* Malloc Packet Buffer Size */
    pDataBuffer = swat_mem_malloc(pStreamCxt->param.pktSize);
    if (NULL == pDataBuffer)
    {
        SWAT_PTF("RAW RX data buffer malloc error\r\n");
        return;
    }
    pStreamCxt->param.pktBuff = pDataBuffer;

    memset(&StatPacket, 0, sizeof(StatPacket));

    /* Prepare IP address & port */
    memset(&fromAddr, 0, sizeof(struct sockaddr_in));
    fromSize = sizeof(struct sockaddr_in);

    /* Init fd_set */
    swat_fd_zero(&master);
    swat_fd_set(pStreamCxt->socketLocal, &master);
    pStreamCxt->pfd_set = (void *)&master;
    tmo.tv_sec  = 1;
    tmo.tv_usec = 0;

    while(1)
    {
        if (swat_bench_quit())
        {

            if(!isFirst)
                swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
            break;
        }

        sockSet = master;

        /* Wait for Input */
        fdAct = swat_select(pStreamCxt->socketLocal + 1, &sockSet, NULL, NULL, &tmo); //k_select()
        if (0 != fdAct)
        {
            if (swat_fd_isset(pStreamCxt->socketLocal, &sockSet))
            {
                recvBytes = swat_recvfrom(pStreamCxt->socketLocal, (CHAR*)pDataBuffer, 
                        CFG_PACKET_SIZE_MAX_RX, 0, (struct sockaddr *)&fromAddr, &fromSize);
                if (recvBytes <= 0)
                {
                    SWAT_PTF("RAW Socket receive is error %d, sumBytes = %d\r\n", recvBytes, sumBytes);
                    continue;
                }
                else
                {
                    sumBytes += recvBytes;
                    //SWAT_PTF("RAW Socket receive %d, sumBytes = %d\r\n", recvBytes, sumBytes);
                }
            if(pStreamCxt->param.rawmode == ETH_RAW)
            {
                    qcom_printf("Received: %d\n",recvBytes);
                    int i;
                    for(i=0; i<recvBytes; i++){
                        qcom_printf("%02x ",*((CHAR*)pDataBuffer+i));
                        if( (i + 1)%16 == 0) //each line prints 16 numbers
                            qcom_printf("\n");
                    }
                    qcom_printf("\n");

                    char eapol_pkt[] = {
                        0x00, 0x21, 0x70, 0xaa, 0x87, 0x1a, 0x00, 0x03,  0x7F, 0x00, 0x40, 0x89, 0x88, 0x8E, 0x01, 0x00,
                        0x00, 0x05, 0x01, 0x01, 0x00, 0x05, 0x01, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
                    };
                    A_UINT8* pkt_buffer = NULL;
                    int data_len = sizeof(eapol_pkt);
                    int send_result;
                    pkt_buffer = swat_mem_malloc(data_len);
                    if (NULL == pkt_buffer)
                    {
                        SWAT_PTF("pkt buffer malloc error for ETH RAW\r\n");
                        return;
                    }
                    memcpy(ip_str, &pDataBuffer[6], 6);
                    memcpy(pkt_buffer, eapol_pkt, data_len);
                    memcpy(pkt_buffer, ip_str, 6);
                    send_result = swat_sendto(pStreamCxt->socketLocal, 
                                  (CHAR*)(&pkt_buffer), data_len, 0, (struct sockaddr*)&fromAddr, sizeof(struct sockaddr_in));

                    if(send_result < 0)
                        SWAT_PTF("Eapol send error \n");
                    else
                        SWAT_PTF("Eapol sent: %d \n",data_len);
                    
                    swat_buffer_free(&(pkt_buffer));
           }
           else{    

                if (recvBytes >= sizeof(EOT_PACKET_t))
                {
                    if (isFirst)
                    {
                        if(recvBytes > sizeof(EOT_PACKET_t))
                        {
                            clientIp   = ntohl(fromAddr.sin_addr.s_addr);
                            clientPort = ntohs(fromAddr.sin_port);
                            SWAT_PTF("RAW receving from 0x%x port:%d \r\n", 
                                    clientIp, clientPort);
                            isFirst = 0;        

                            pStreamCxt->calc[DEFAULT_FD_INDEX].firstTime.milliseconds = CALC_TIME_DEF;
                            pStreamCxt->calc[DEFAULT_FD_INDEX].lastTime.milliseconds  = CALC_TIME_DEF;
                            pStreamCxt->calc[DEFAULT_FD_INDEX].bytes  = CALC_BYTES_DEF;
                            pStreamCxt->calc[DEFAULT_FD_INDEX].kbytes = CALC_KBYTES_DEF;
                            swat_get_first_time(pStreamCxt, DEFAULT_FD_INDEX);
                            swat_bytes_calc(pStreamCxt, recvBytes, DEFAULT_FD_INDEX);											
                        }
                    }
                    else
                    {
                        A_UINT8* bf;
                        fromAddr.sin_family = AF_INET;                    /* End Of Transfer */
                        if (recvBytes == sizeof(EOT_PACKET_t))
                        {
                            swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);

                            totalInterval = (pStreamCxt->calc[DEFAULT_FD_INDEX].lastTime.milliseconds - pStreamCxt->calc[DEFAULT_FD_INDEX].firstTime.milliseconds);
                            StatPacket.kbytes = pStreamCxt->calc[DEFAULT_FD_INDEX].kbytes;
                            StatPacket.bytes  = pStreamCxt->calc[DEFAULT_FD_INDEX].bytes;
                            StatPacket.msec   = totalInterval;
                            bf = (A_UINT8*)&StatPacket;
                            /* Tell ath_console TX received end mark AABBCCDD with throughput value */
                            while(sendTerminalCount <= 10)
                            {
                                 size = sizeof(STAT_PACKET_t);

                                /* Add IP header */
                                if(pStreamCxt->param.ip_hdr_inc == 1) //if raw_mode == iphdr
                                {
                                  size += sizeof(ip_header);
                                  iphdr = (ip_header *)pStreamCxt->param.pktBuff;
                                  iphdr->iph_ihl = IPV4_HEADER_LENGTH / sizeof(A_UINT32);
                                  iphdr->iph_ver = 4; /* IPv4 */
                                  iphdr->iph_tos = 0;
                                  iphdr->iph_len = htons(size);
                                  iphdr->iph_ident = htons(0);        
                                  /* Zero (1 bit) */
                                  ip_flags[0] = 0;
                                  /* Do not fragment flag (1 bit) */
                                  ip_flags[1] = 0;
                                  /* More fragments following flag * (1 bit) */
                                  ip_flags[2] = 0;
                                  /* Fragmentation * offset (13 bits)* */
                                  ip_flags[3] = 0;      
                                  iphdr->iph_offset = htons((ip_flags[0] << 15) | (ip_flags[1] << 14)| (ip_flags[2] << 13) | ip_flags[3]);
                                  /* Time-to-Live (8 bits): default to maximum value */
                                  iphdr->iph_ttl = 255;
                                  /* Transport layer protocol (8 bits): 17 for UDP ,255 for raw */
                                  iphdr->iph_protocol  = pStreamCxt->param.port; //ATH_IP_PROTO_RAW;
                                  iphdr->iph_sourceip  = NTOHL(pStreamCxt->param.local_ipAddress);
                                  iphdr->iph_destip    = fromAddr.sin_addr.s_addr;
                                  memcpy((A_UINT8*)iphdr + sizeof(ip_header),&StatPacket, sizeof(STAT_PACKET_t));
                                  bf = (A_UINT8*)iphdr;
                                }
                                                                
                                 /* Clean */
                                SWAT_PTF("SENDING\n");
			                    sendBytes = swat_sendto(pStreamCxt->socketLocal, 
			                                    (CHAR*)(bf/*&StatPacket*/), size, 0, 
			                                    (struct sockaddr *)&fromAddr, sizeof(struct sockaddr_in));
				                if (sendBytes < 0)
				                {
				                         SWAT_PTF("RAW send throughput info packet error %d , retry %d \r\n", sendBytes, sendTerminalCount);
				                         qcom_thread_msleep(100);
										 sendTerminalCount++;
				                }
								else
								{
                                    /* Clean */
                                    tmo.tv_sec = 2;
                                    tmo.tv_usec = 0;
					                fdAct = swat_select(pStreamCxt->socketLocal + 1, &sockSet, NULL, NULL, &tmo);   //k_select()
                                    if (0 == fdAct) {
                                       SWAT_PTF("RAW break\n");
                                       break;
                                    }
						            if (swat_fd_isset(pStreamCxt->socketLocal, &sockSet)) {
                                                recvBytes = swat_recvfrom(pStreamCxt->socketLocal, (CHAR*)pDataBuffer, 
                                       CFG_PACKET_SIZE_MAX_RX, 0, (struct sockaddr *)&fromAddr, &fromSize);
						            }
									sendTerminalCount++;
                               }
                            }	
                            break;
                        } /* EOT packet is received */
						else
						{								
							swat_bytes_calc(pStreamCxt, recvBytes, DEFAULT_FD_INDEX);										
						}
                    }     /* not the first packet   */
                }         /* received pkt >= EOT    */
              }           /* IP_RAW                 */
           }              /* swatfdisset            */
        }                 /* swat select !=0        */
    }                     /* end of while(1)        */

    swat_test_result_print(pStreamCxt, DEFAULT_FD_INDEX);

    SWAT_PTF("Waiting.\n");
    /* Free Buffer */
    swat_buffer_free(&(pStreamCxt->param.pktBuff));

}
#endif

void
swat_udp_rx_handle(STREAM_CXT_t * pStreamCxt)
{
    q_fd_set sockSet;
    struct timeval tmo;
#ifdef SWAT_SSL    
    A_INT32 result = 0;
    SSL_INST *ssl = NULL;
#endif

    /* Initial Bench Value */
    swat_bench_quit_init();

#ifdef SWAT_SSL
    if (pStreamCxt->param.protocol == TEST_PROT_DTLS){
        if(NULL == (ssl = swat_find_ssl_inst(pStreamCxt->param.ssl_inst_index))){
            goto QUIT;
        }
    
        if (ssl->ssl == NULL)
        {
                    // Create SSL connection object
            ssl->ssl = qcom_DTLS_new(ssl->sslCtx);
            if (ssl->ssl == NULL)
            {
                printf("ERROR: Unable to create SSL context\n");
                goto QUIT;
            }
         }
                
         // Add socket handle to SSL connection
         result = qcom_SSL_set_fd(ssl->ssl, pStreamCxt->socketLocal);
         if (result < 0)
         {
            printf("ERROR: Unable to add socket handle to SSL (%d)\n", result);
            goto  QUIT;
         }
     }
#endif


    /* Init fd_set */
    swat_fd_zero(&sockSet);
    swat_fd_set(pStreamCxt->socketLocal, &sockSet);
    tmo.tv_sec = 1;
    tmo.tv_usec = 0;
    pStreamCxt->pfd_set = (void *) &sockSet;

    while (1) {
        if (swat_bench_quit()) {
            goto QUIT;
        }
        swat_udp_rx_data(pStreamCxt);
    }
  QUIT:
  if (!pStreamCxt->param.iperf_mode) {
      swat_test_result_print(pStreamCxt, DEFAULT_FD_INDEX);
      SWAT_PTF("*************IOT Throughput Test Completed **************\n");
  }
    SWAT_PTF("Shell> ");
    swat_fd_zero(&sockSet);
    pStreamCxt->pfd_set = NULL;
#ifdef SWAT_SSL
    if ((ssl != NULL) && (ssl->role == SSL_SERVER) && (ssl->ssl != NULL))
    {
        qcom_SSL_shutdown(ssl->ssl);
         ssl->ssl = NULL;
    }
#endif
    /* Close Socket */
    swat_socket_close(pStreamCxt);
}

#ifdef SWAT_BENCH_RAW
void swat_raw_rx_handle(STREAM_CXT_t* pStreamCxt)
{
    q_fd_set sockSet;
    struct timeval tmo;
    
    /* Initial Bench Value */
    swat_bench_quit_init();

    /* Init fd_set */
    swat_fd_zero(&sockSet);
    swat_fd_set(pStreamCxt->socketLocal, &sockSet);    
    tmo.tv_sec = 2;   
    tmo.tv_usec = 0;
    pStreamCxt->pfd_set = (void *)&sockSet;
    
    while (1)
    {
            if (swat_bench_quit())   {
                goto QUIT;
            }            
        swat_raw_rx_data(pStreamCxt);
    }
QUIT:
    swat_test_result_print(pStreamCxt, DEFAULT_FD_INDEX);
    SWAT_PTF("*************IOT Throughput Test Completed **************\n");
    SWAT_PTF("Shell> ");
    swat_fd_zero(&sockSet);
    pStreamCxt->pfd_set = NULL;
    /* Close Socket */
    swat_socket_close(pStreamCxt);
}
#endif
static A_UINT32 iperf_stream_id = 1;
void
swat_bench_tcp_tx_task(STREAM_CXT_t * pStreamCxt)
{
    A_INT32 ret = 0;
    struct sockaddr_in remoteAddr;
    struct sockaddr_in6 remote6Addr;
    A_UINT32 active_sock_close = 1;
#ifdef SWAT_SSL
    SSL_INST *ssl = NULL;
#endif    

    SWAT_PTR_NULL_CHK(pStreamCxt);
    if(pStreamCxt->param.is_v6_enabled){ 
        pStreamCxt->socketLocal = swat_socket(AF_INET6, SOCK_STREAM, 0);
    }else{
        pStreamCxt->socketLocal = swat_socket(AF_INET, SOCK_STREAM, 0);
    }
    if (pStreamCxt->socketLocal < 0) {
        SWAT_PTF("Open socket error...\r\n");
        goto QUIT;
    } 
    if (config_socket_active_close)
        swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, SO_ACTIVE_CLOSE, (char *)&active_sock_close,
                        sizeof(active_sock_close));
	
    pStreamCxt->param.iperf_time_sec = 0;
    pStreamCxt->param.iperf_udp_buf_size = 1452;
    pStreamCxt->param.iperf_stream_id = iperf_stream_id;
    iperf_stream_id++;

    /* Connect Socket */
    if(pStreamCxt->param.is_v6_enabled){
        swat_mem_set(&remote6Addr, 0, sizeof (struct sockaddr_in6));
        memcpy(&remote6Addr.sin6_addr, pStreamCxt->param.ip6Address.addr,sizeof(IP6_ADDR_T));
        remote6Addr.sin6_port = htons(pStreamCxt->param.port);
        remote6Addr.sin6_family = AF_INET6;
        ret = swat_connect(pStreamCxt->socketLocal, (struct sockaddr *) &remote6Addr,
                     sizeof (struct  sockaddr_in6));

    }else{
        swat_mem_set(&remoteAddr, 0, sizeof (struct sockaddr_in));
        remoteAddr.sin_addr.s_addr = htonl(pStreamCxt->param.ipAddress);
        remoteAddr.sin_port = htons(pStreamCxt->param.port);
        remoteAddr.sin_family = AF_INET;
        ret = swat_connect(pStreamCxt->socketLocal, (struct sockaddr *) &remoteAddr,
                     sizeof (struct  sockaddr_in));

    }
    if (pStreamCxt->param.iperf_mode) {

    	app_printf("------------------------------------------------------------\n");
		app_printf("Client connecting to %d.%d.%d.%d, TCP port %d\n",
				(pStreamCxt->param.ipAddress) >> 24 & 0xFF, (pStreamCxt->param.ipAddress) >> 16 & 0xFF,
				(pStreamCxt->param.ipAddress) >> 8 & 0xF, (pStreamCxt->param.ipAddress) & 0xFF,
				pStreamCxt->param.port);
		app_printf("TCP window size: 8 KByte (default)\n");
		app_printf("------------------------------------------------------------\n");
    }
    else {
    	SWAT_PTF("Connecting from socket %d.\n", pStreamCxt->socketLocal);
    }
    if (ret < 0) {
        /* Close Socket */
        SWAT_PTF("Connect Failed\r\n");
        swat_socket_close(pStreamCxt);
        goto QUIT;
    }

    if (!pStreamCxt->param.iperf_mode) {
    SWAT_PTF("Connect %lu.%lu.%lu.%lu OK\r\n",
             (remoteAddr.sin_addr.s_addr) & 0xFF, (remoteAddr.sin_addr.s_addr) >> 8 & 0xFF,
             (remoteAddr.sin_addr.s_addr) >> 16 & 0xF, (remoteAddr.sin_addr.s_addr) >> 24 & 0xFF);
    } else {
        A_UINT32 ipv4_addr;

    	if((swat_getsockopt(pStreamCxt->socketLocal, SOL_SOCKET, SO_MYADDR, (A_UINT8*) &ipv4_addr,
                  sizeof (ipv4_addr))) < 0) {
    		ipv4_addr = 0;
    	}
    	ipv4_addr = htonl(ipv4_addr);

    	app_printf("[%3d] local %d.%d.%d.%d port %d connected with %d.%d.%d.%d port %d\n",
    			pStreamCxt->param.iperf_stream_id,
                (ipv4_addr) >> 24 & 0xFF, (ipv4_addr) >> 16 & 0xFF,
                (ipv4_addr) >> 8 & 0xF, (ipv4_addr) & 0xFF,
    			0,
				(pStreamCxt->param.ipAddress) >> 24 & 0xFF, (pStreamCxt->param.ipAddress) >> 16 & 0xFF,
				(pStreamCxt->param.ipAddress) >> 8 & 0xF, (pStreamCxt->param.ipAddress) & 0xFF,
				pStreamCxt->param.port);
		app_printf("[ ID] Interval       Transfer     Bandwidth\n");
    }
    /* Packet Handle */
    swat_tcp_tx_handle(pStreamCxt);
  QUIT:
#ifdef SWAT_SSL
    if (pStreamCxt->param.protocol == TEST_PROT_SSL) {
        if(NULL != (ssl = swat_find_ssl_inst(pStreamCxt->param.ssl_inst_index))){
            ssl->state = SSL_FREE;
        }
    }
#endif    
    /* Free Index */
    swat_cxt_index_free(&tcpTxIndex[pStreamCxt->index]);
    swat_task_delete();
}

void
swat_bench_tcp_rx_task(STREAM_CXT_t * pStreamCxt)
{
    A_INT32 ret = 0;
    struct sockaddr_in remoteAddr;
    struct sockaddr_in6 remote6Addr;
    A_UINT32 active_sock_close = 1;

    swat_bench_pwrmode_maxperf();

    SWAT_PTR_NULL_CHK(pStreamCxt);
    if(pStreamCxt->param.is_v6_enabled){
        pStreamCxt->socketLocal = swat_socket(AF_INET6, SOCK_STREAM, 0);
    }else{
        pStreamCxt->socketLocal = swat_socket(AF_INET, SOCK_STREAM, 0);
    }
    if (pStreamCxt->socketLocal < 0) {
        SWAT_PTF("Open socket error...\r\n");
        goto QUIT;
    }
    if (config_socket_active_close) 
        swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, SO_ACTIVE_CLOSE, (char *)&active_sock_close,
                       sizeof(active_sock_close));
    
        

    if(pStreamCxt->param.is_v6_enabled){
        swat_mem_set(&remote6Addr, 0, sizeof (struct sockaddr_in6));
        memcpy(&remote6Addr.sin6_addr, pStreamCxt->param.ip6Address.addr,sizeof(IP6_ADDR_T));
        remote6Addr.sin6_port = htons(pStreamCxt->param.port);
        remote6Addr.sin6_family = AF_INET6;
        ret = swat_bind(pStreamCxt->socketLocal, (struct sockaddr *) &remote6Addr,
                  sizeof (struct sockaddr_in6));
   
    }else{
        /* Connect Socket */
        swat_mem_set(&remoteAddr, 0, sizeof (struct sockaddr_in));
        remoteAddr.sin_addr.s_addr = htonl(pStreamCxt->param.ipAddress);
        remoteAddr.sin_port = htons(pStreamCxt->param.port);
        remoteAddr.sin_family = AF_INET;
        ret = swat_bind(pStreamCxt->socketLocal, (struct sockaddr *) &remoteAddr,
                  sizeof (struct sockaddr_in));
    }
     if (ret < 0) {
        /* Close Socket */
        SWAT_PTF("Bind Failed %d.\n", pStreamCxt->socketLocal);
        swat_socket_close(pStreamCxt);
        goto QUIT;
    }

    if (pStreamCxt->param.iperf_mode) {
    	A_UINT32 rcv_buf;
    	if((swat_getsockopt(pStreamCxt->socketLocal, SOL_SOCKET, SO_RCVBUF, (A_UINT8*) &rcv_buf,
                  sizeof (rcv_buf))) < 0) {
    		rcv_buf = 8192;
    	}

    	app_printf("------------------------------------------------------------\n");
		app_printf("Server listening on TCP port %d\n",	pStreamCxt->param.port);
		app_printf("TCP window size: %d KByte (default)\n", rcv_buf / 1024);
		app_printf("------------------------------------------------------------\n");

    } else {
        SWAT_PTF("****************************************************\n");
        if (pStreamCxt->param.protocol == TEST_PROT_SSL){
            SWAT_PTF(" SSL RX Test\n"); 
        } else{
            SWAT_PTF(" TCP RX Test\n");
        }
        SWAT_PTF("****************************************************\n");
        SWAT_PTF("Local port %d\n", pStreamCxt->param.port);
        SWAT_PTF("Type benchquit to terminate test\n");
        SWAT_PTF("****************************************************\n");

        SWAT_PTF("Waiting.\n");
   }

    /* Packet Handle */
    swat_tcp_rx_handle(pStreamCxt); 
QUIT:
    /* Free Index */
    swat_cxt_index_free(&tcpRxIndex[pStreamCxt->index]);
    swat_bench_restore_pwrmode();

    /* Thread Delete */
    swat_task_delete();
}

void
swat_bench_udp_tx_task(STREAM_CXT_t * pStreamCxt)
{
    A_INT32 ret = 0;
    A_UINT32 local_ip = 0;
    SWAT_PTR_NULL_CHK(pStreamCxt);
    struct sockaddr_in localAddr;
    A_UINT32 ipAddress;
    A_UINT32 submask;
    A_UINT32 gateway;
#ifdef SWAT_SSL
    SSL_INST *ssl = NULL;
#endif  

    pStreamCxt->param.iperf_time_sec = 0;
    pStreamCxt->param.iperf_udp_buf_size = 1400;
    pStreamCxt->param.iperf_stream_id = iperf_stream_id;
    iperf_stream_id++;
    if(pStreamCxt->param.is_v6_enabled){
        pStreamCxt->socketLocal = swat_socket(PF_INET6, SOCK_DGRAM, 0);
    }else{
        pStreamCxt->socketLocal = swat_socket(PF_INET, SOCK_DGRAM, 0);
    }
    if (pStreamCxt->socketLocal < 0) {
        SWAT_PTF("Open socket error...\r\n");
        goto QUIT;
    }
    if(!pStreamCxt->param.is_v6_enabled){
        memset(&localAddr, 0, sizeof(struct sockaddr_in));
        qcom_ipconfig(currentDeviceId, IP_CONFIG_QUERY, &ipAddress, &submask, &gateway);
        localAddr.sin_addr.s_addr = htonl(ipAddress);
        localAddr.sin_family      = AF_INET;
		/*bind addr when in concurrency mode*/
        if((swat_bind(pStreamCxt->socketLocal, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in))) < 0){         
            /* Close Socket */
            SWAT_PTF("Bind Failed\n");
            swat_socket_close(pStreamCxt);
	        goto QUIT;
        }
        if(pStreamCxt->param.mcast_enabled){
            local_ip = HTONL(pStreamCxt->param.local_ipAddress);

            if (local_ip != 0) {
                if((ret = swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, IP_MULTICAST_IF, (A_UINT8*) &local_ip,
                          sizeof (local_ip))) < 0) {
                    /* Close Socket */
                    SWAT_PTF("Failed to set socket option %d.\n", pStreamCxt->socketLocal);
                    swat_socket_close(pStreamCxt);
                    goto QUIT;
                }
            }
         }
	}
    if (pStreamCxt->param.iperf_mode) {

    	app_printf("------------------------------------------------------------\n");
		app_printf("Client connecting to %d.%d.%d.%d, UDP port %d\n",
				(pStreamCxt->param.ipAddress) >> 24 & 0xFF, (pStreamCxt->param.ipAddress) >> 16 & 0xFF,
				(pStreamCxt->param.ipAddress) >> 8 & 0xF, (pStreamCxt->param.ipAddress) & 0xFF,
				pStreamCxt->param.port);
		app_printf("Sending %d byte datagrams\n", pStreamCxt->param.pktSize);
		app_printf("UDP buffer size:  %d Byte (default)\n", pStreamCxt->param.iperf_udp_buf_size);
		app_printf("------------------------------------------------------------\n");
	}
    /* Packet Handle */
    swat_udp_tx_handle(pStreamCxt);
QUIT:
#ifdef SWAT_SSL
    if (pStreamCxt->param.protocol == TEST_PROT_DTLS) {
        if(NULL != (ssl = swat_find_ssl_inst(pStreamCxt->param.ssl_inst_index))){
            ssl->state = SSL_FREE;
        }
    }
#endif      
    /* Free Index */
    swat_cxt_index_free(&udpTxIndex[pStreamCxt->index]);
    /* Thread Delete */
    swat_task_delete();
}

void
swat_bench_udp_rx_task(STREAM_CXT_t * pStreamCxt)
{
    A_INT32 ret = 0;
    A_INT32 i=-1;
    struct sockaddr_in remoteAddr;
    struct sockaddr_in6 remoteAddr6;
    struct ip_mreq {
        A_UINT32 imr_multiaddr;   /* IP multicast address of group */
        A_UINT32 imr_interface;   /* local IP address of interface */
    } group;
    struct ipv6_mreq {
       IP6_ADDR_T ipv6mr_multiaddr; /* IPv6 multicast addr */
       IP6_ADDR_T ipv6mr_interface; /* IPv6 interface address */
    } group6;

    SWAT_PTR_NULL_CHK(pStreamCxt);
    swat_bench_pwrmode_maxperf();
    while(++i<CALC_STREAM_NUMBER){
        if((pStreamCxt->param.port==cxtUdpRxPara[i].param.port)&&(i!=pStreamCxt->index)){
            SWAT_PTF("Bind Failed!\n");
            swat_socket_close(pStreamCxt);
            goto QUIT;
        }
    }
    if(pStreamCxt->param.is_v6_enabled){
        pStreamCxt->socketLocal = swat_socket(PF_INET6, SOCK_DGRAM, 0);
        SWAT_PTF("SOCKET %d\n",pStreamCxt->socketLocal);
        if(pStreamCxt->socketLocal < 0)
            goto QUIT;

         /* Bind Socket */
        swat_mem_set(&remoteAddr6, 0, sizeof (struct sockaddr_in6));
        A_MEMCPY(&remoteAddr6.sin6_addr, pStreamCxt->param.ip6Address.addr, sizeof(IP6_ADDR_T));
        remoteAddr6.sin6_port = htons(pStreamCxt->param.port);
        remoteAddr6.sin6_family = AF_INET6;

        if((ret = swat_bind(pStreamCxt->socketLocal, (struct sockaddr *) &remoteAddr6,sizeof (struct sockaddr_in6))) < 0){         
            /* Close Socket */
            SWAT_PTF("Bind Failed\n");
            swat_socket_close(pStreamCxt);
            goto QUIT;
        }

        if(pStreamCxt->param.mcast_enabled){
            memcpy(&group6.ipv6mr_multiaddr,pStreamCxt->param.mcastIp6.addr, sizeof(IP6_ADDR_T));
            memcpy(&group6.ipv6mr_interface,pStreamCxt->param.localIp6.addr, sizeof(IP6_ADDR_T));

            if((ret = swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, IPV6_JOIN_GROUP,
			     (void*)(&group6),sizeof(struct ipv6_mreq))) < 0)
			{
				SWAT_PTF("SetsockOPT error : unable to add to multicast group\r\n");
                swat_socket_close(pStreamCxt);
	            goto QUIT;
			}
        }
    }else{
        if((pStreamCxt->socketLocal = swat_socket(PF_INET, SOCK_DGRAM, 0)) < 0){
            SWAT_PTF("Open socket error...\r\n");
            goto QUIT;
        }

        /* Connect Socket */
        swat_mem_set(&remoteAddr, 0, sizeof (struct sockaddr_in));
        remoteAddr.sin_addr.s_addr = htonl(pStreamCxt->param.ipAddress);
        remoteAddr.sin_port = htons(pStreamCxt->param.port);
        remoteAddr.sin_family = AF_INET;

        if((ret = swat_bind(pStreamCxt->socketLocal, (struct sockaddr *) &remoteAddr,sizeof (struct sockaddr_in))) < 0){         
            /* Close Socket */
            SWAT_PTF("Failed to bind socket %d.\n", pStreamCxt->socketLocal);
            swat_socket_close(pStreamCxt);
            goto QUIT;
        }
        if(pStreamCxt->param.mcast_enabled){
            group.imr_multiaddr = HTONL(pStreamCxt->param.mcAddress);
            group.imr_interface = HTONL(pStreamCxt->param.local_ipAddress);

            if (group.imr_multiaddr != 0) {
                if((ret = swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, IP_ADD_MEMBERSHIP, (void *) &group,
                          sizeof (group))) < 0) {
                    /* Close Socket */
                    SWAT_PTF("Failed to set socket option %d.\n", pStreamCxt->socketLocal);
                    swat_socket_close(pStreamCxt);
                    goto QUIT;
                }
            }
        }
    }
  
    if (pStreamCxt->param.iperf_mode) {
    	app_printf("------------------------------------------------------------\n");
		app_printf("Server listening on UDP port %d\n",	pStreamCxt->param.port);
		app_printf("Receiving %d byte datagrams\n", 1470);
		app_printf("UDP buffer size:  %d KByte (default)\n", pStreamCxt->param.iperf_udp_buf_size);
		app_printf("------------------------------------------------------------\n");

    } else {
        SWAT_PTF("****************************************************\n");
        SWAT_PTF(" UDP RX Test\n");
        SWAT_PTF("****************************************************\n");

        SWAT_PTF("Local port %d\n", pStreamCxt->param.port);
        SWAT_PTF("Type benchquit to termintate test\n");
        SWAT_PTF("****************************************************\n");

        SWAT_PTF("Waiting.\n");
    }
    /* Packet Handle */
    swat_udp_rx_handle(pStreamCxt);
  QUIT:
    /* Free Index */
    swat_cxt_index_free(&udpRxIndex[pStreamCxt->index]);
    
    swat_bench_restore_pwrmode();

    /* Thread Delete */
    swat_task_delete();
}
#ifdef SWAT_BENCH_RAW
void swat_bench_raw_rx_task(STREAM_CXT_t* pStreamCxt)
{
    A_INT32 ret = 0;
    struct sockaddr_in localAddr;
    struct ip_mreq
    {
       A_UINT32 imr_multiaddr; /* IP multicast address of group */
       A_UINT32 imr_interface; /* local IP address of interface */
    } group;
    swat_bench_pwrmode_maxperf();
    SWAT_PTR_NULL_CHK(pStreamCxt);

    pStreamCxt->socketLocal = swat_socket(AF_INET, SOCK_RAW, pStreamCxt->param.port);
    if (pStreamCxt->socketLocal < 0)

    {
        SWAT_PTF("Open raw socket error...\r\n");
        goto QUIT;
    }

    /* Connect Socket */
    swat_mem_set(&localAddr, 0, sizeof(struct sockaddr_in));
    localAddr.sin_addr.s_addr = htonl(pStreamCxt->param.local_ipAddress);
    localAddr.sin_family      = AF_INET;

    SWAT_PTF("****************************************************\n");
    SWAT_PTF(" RAW RX Test\n\n");
    SWAT_PTF("Type benchquit to termintate test\n");
    SWAT_PTF("****************************************************\n");

    SWAT_PTF("Waiting....\n");

    ret = swat_bind(pStreamCxt->socketLocal, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in));
    if (ret < 0)
    {
        /* Close Socket */
        SWAT_PTF("Failed to bind socket %d.\n", pStreamCxt->socketLocal);
        swat_socket_close(pStreamCxt);
        goto QUIT;
    }

    group.imr_multiaddr = (pStreamCxt->param.mcAddress);
    group.imr_interface = (pStreamCxt->param.ipAddress);

    if (group.imr_multiaddr != 0) {
        ret = swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, IP_ADD_MEMBERSHIP, (void *)&group, sizeof(group));
        if (ret < 0) {
            /* Close Socket */
            SWAT_PTF("Failed to set socket option %d.\n", pStreamCxt->socketLocal);
            swat_socket_close(pStreamCxt);
            goto QUIT;
        }
    }
   #define  IP_HDRINCL        2  /* int; header is included with data */
   if(pStreamCxt->param.ip_hdr_inc == 1) // if raw mode == iphdr
   {
        ret = swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, IP_HDRINCL, (A_UINT8 *)(&pStreamCxt->param.ip_hdr_inc), sizeof(int));
        if (ret < 0) {
            /* Close Socket */
            SWAT_PTF("Failed to set socket option %d.\n", pStreamCxt->socketLocal);
            swat_socket_close(pStreamCxt);
            goto QUIT;
        }
   }
    SWAT_PTF("CALLING HANDLE\n");
    /* Packet Handle */
    swat_raw_rx_handle(pStreamCxt);
QUIT:    
    /* Free Index */
    swat_cxt_index_free(&rawRxIndex[pStreamCxt->index]);
    swat_bench_restore_pwrmode();

    /* Thread Delete */
    swat_task_delete();
}
#endif

void
swat_bench_tcp_tx(A_UINT32 index)
{
    swat_bench_tcp_tx_task(&cxtTcpTxPara[index]);
}

void
swat_bench_udp_tx(A_UINT32 index)
{
    swat_bench_udp_tx_task(&cxtUdpTxPara[index]);
}

void
swat_bench_tcp_rx(A_UINT32 index)
{
    swat_bench_tcp_rx_task(&cxtTcpRxPara[index]);
}

void
swat_bench_udp_rx(A_UINT32 index)
{
    swat_bench_udp_rx_task(&cxtUdpRxPara[index]);
}

#ifdef SWAT_BENCH_RAW
void swat_bench_raw_tx(A_UINT32 index)
{
    swat_bench_raw_tx_task(&cxtRawTxPara[index]);
}

void swat_bench_raw_rx(A_UINT32 index)
{
    swat_bench_raw_rx_task(&cxtRawRxPara[index]);
}
#endif

void
swat_bench_tx_test(A_UINT32 ipAddress, IP6_ADDR_T ip6Address, A_UINT32 port, A_UINT32 protocol, A_UINT32 ssl_inst_index, 
                   A_UINT32 pktSize, A_UINT32 mode, A_UINT32 seconds, A_UINT32 numOfPkts,
	               A_UINT32 local_ipAddress, A_UINT32 ip_hdr_inc, A_UINT32 delay, A_UINT32 iperf_mode, 
	               A_UINT32 interval, A_UINT32 udpRate)
{

    A_UINT8 index = 0;
    A_UINT8 ret = 0;
#ifdef SWAT_SSL
    SSL_INST *ssl = NULL;
#endif

    /* TCP */
    if (TEST_PROT_TCP == protocol || TEST_PROT_SSL == protocol) {
        /*Calc Handle */
        for (index = 0; index < CALC_STREAM_NUMBER; index++) {
            ret = swat_cxt_index_find(&tcpTxIndex[index]);

            if (CALC_STREAM_NUMBER_INVALID != ret) {
                swat_cxt_index_configure(&tcpTxIndex[index]);
                break;
            }
        }
        if (CALC_STREAM_NUMBER_INVALID == ret) {
            SWAT_PTF("Warning tcpTxIndex is full\n");
            return;
        }
        /* Database Initial */
        swat_database_initial(&cxtTcpTxPara[index]);
        /* Update Database */
        swat_database_set(&cxtTcpTxPara[index], ipAddress, &ip6Address, INADDR_ANY, NULL, NULL, port, protocol, ssl_inst_index,
                          pktSize, mode, seconds, numOfPkts, TEST_DIR_TX, local_ipAddress,ip_hdr_inc,0,delay,iperf_mode,interval, 0);

        /* Client */
        cxtTcpTxPara[index].index = index;
        ret = swat_task_create(swat_bench_tcp_tx, index, 2048, 50);
        if (0 == ret) {
            SWAT_PTF("TCP TX session %d Success\n", index);
        } else {
            swat_cxt_index_free(&tcpTxIndex[index]);
        }
        /* Thread Delete */
    }
    /* UDP */
    if (TEST_PROT_UDP == protocol || TEST_PROT_DTLS == protocol) {
        /*Calc Handle */
        for (index = 0; index < CALC_STREAM_NUMBER; index++) {
            ret = swat_cxt_index_find(&udpTxIndex[index]);

            if (CALC_STREAM_NUMBER_INVALID != ret) {
                swat_cxt_index_configure(&udpTxIndex[index]);
                break;
            }
        }
        if (CALC_STREAM_NUMBER_INVALID == ret) {
            SWAT_PTF("Warning udpTxIndex is full\n");
            return;
        }

        /* DataBase Initial */
        swat_database_initial(&cxtUdpTxPara[index]);

        /* Update Database */
	    /* for multicast udp tx, set mcAddress to dest ip so that mcast_enable is set*/
	    if (!v6_enabled &&((ipAddress & 0xf0000000) == 0xe0000000)){
            if (local_ipAddress == 0){
                SWAT_PTF("Multicast need lolca IP Address\n");
                swat_cxt_index_free(&udpTxIndex[index]);
                return;
            }
	        swat_database_set(&cxtUdpTxPara[index], ipAddress, &ip6Address, ipAddress, NULL, NULL, port, protocol, ssl_inst_index,
                          pktSize, mode, seconds, numOfPkts, TEST_DIR_TX,local_ipAddress,ip_hdr_inc,0,delay,iperf_mode,
                          interval, udpRate);
        } else { 
            swat_database_set(&cxtUdpTxPara[index], ipAddress, &ip6Address, INADDR_ANY, NULL, NULL, port, protocol, ssl_inst_index,
                          pktSize, mode, seconds, numOfPkts, TEST_DIR_TX,local_ipAddress,ip_hdr_inc,0,delay,iperf_mode,
                          interval, udpRate);
        }
        /* Client */
        cxtUdpTxPara[index].index = index;
        /* Client */
        ret = swat_task_create(swat_bench_udp_tx, index, 2048, 50);
        if (0 == ret) {
            SWAT_PTF("UDP TX session %d Success\n", index);
        } else {
            swat_cxt_index_free(&udpTxIndex[index]);
        }
    }
 /* RAW */
#ifdef SWAT_BENCH_RAW

    if (TEST_PROT_RAW == protocol)
    {
    	// Check Raw Protocol
    	for (index = 0; index < CALC_STREAM_NUMBER; index ++)
		{
            ret = swat_cxt_index_find(&rawRxIndex[index]);
			if (CALC_STREAM_NUMBER_INVALID == ret)
			{
				if (cxtRawRxPara[index].param.port == port)
				{
					SWAT_PTF("This port is in use, use another Port.\n");
					return;
				}
			}
			
            ret = swat_cxt_index_find(&rawTxIndex[index]);
			if (CALC_STREAM_NUMBER_INVALID == ret)
			{
				if (cxtRawTxPara[index].param.port == port)
				{
					SWAT_PTF("This port is in use, use another Port.\n");
					return;
				}
			}
		}
		
        /*Calc Handle */
        for (index = 0; index < CALC_STREAM_NUMBER; index ++)
        {
            ret = swat_cxt_index_find(&rawTxIndex[index]);

            if (CALC_STREAM_NUMBER_INVALID != ret)
            {
                swat_cxt_index_configure(&rawTxIndex[index]);
                break;
            }
        }
        if (CALC_STREAM_NUMBER_INVALID == ret)
        {
            SWAT_PTF("Warning rawTxIndex is full\n");
            return;
        }

        /* DataBase Initial */
        swat_database_initial(&cxtRawTxPara[index]);

        /* Update Database*/
        swat_database_set(&cxtRawTxPara[index], ipAddress, &ip6Address, INADDR_ANY, NULL, NULL, port, protocol, 0,
            pktSize, mode, seconds, numOfPkts, TEST_DIR_TX,local_ipAddress,ip_hdr_inc,1,delay,0,0, 0);

        /* Client */
        cxtRawTxPara[index].index = index;
        /* Client */
        SWAT_PTF("Creating task swat_bench_raw_tx \n");
        ret = swat_task_create(swat_bench_raw_tx, index, 2048, 50);
        if (0 == ret)  
        {
            SWAT_PTF("RAW TX session %d Success\n",index);        

        }
        else
        {
            swat_cxt_index_free(&rawTxIndex[index]);
        }
    }

#ifdef SWAT_SSL
    //To avoid creating more than one SSL connection in one SSL instance in SSL MTBF test.
    if ((0 == ret) && ((TEST_PROT_SSL == protocol) ||(TEST_PROT_DTLS == protocol)))
    {
        if(NULL != (ssl = swat_find_ssl_inst(ssl_inst_index)))
        {
            ssl->state = SSL_PRE_HANDSHAKE;
        }
    }    
#endif        
#endif /*SWAT_BENCH_RAW*/
}

void
swat_bench_rx_test(A_UINT32 protocol, A_UINT32 ssl_inst_index, A_UINT32 port, A_UINT32 localIpAddress, A_UINT32 mcastIpAddress, IP6_ADDR_T* lIp6, IP6_ADDR_T* mIp6, A_UINT32 ip_hdr_inc, A_UINT32 rawmode, A_UINT32 iperf_mode, A_UINT32 interval)
{
    A_UINT8 index = 0;
    A_UINT8 ret = 0;

    /* TCP */
    if (TEST_PROT_TCP == protocol || TEST_PROT_SSL == protocol) {
        /*Calc Handle */
        for (index = 0; index < CALC_STREAM_NUMBER; index++) {
            ret = swat_cxt_index_find(&tcpRxIndex[index]);

            if (CALC_STREAM_NUMBER_INVALID != ret) {
                swat_cxt_index_configure(&tcpRxIndex[index]);
                break;
            }
        }
        if (CALC_STREAM_NUMBER_INVALID == ret) {
            SWAT_PTF("Warning tcpRxIndex is full\n");
            return;
        }

        /* DataBase Initial */
        swat_database_initial(&cxtTcpRxPara[index]);

        /* Update DataBase */
        swat_database_set(&cxtTcpRxPara[index], INADDR_ANY, NULL, INADDR_ANY, NULL, NULL, port, protocol, ssl_inst_index,
                          1500, 0, 0, 0, TEST_DIR_RX, 0, 0,0,0, iperf_mode, interval, 0);

        /* Server */
        cxtTcpRxPara[index].index = index;
        ret = swat_task_create(swat_bench_tcp_rx, index, 2048, 50);
        if (0 == ret) {
            SWAT_PTF("TCP RX session %d Success\n", index);
        } else {
            swat_cxt_index_free(&tcpRxIndex[index]);
        }
    }
    /* UDP */
    if (TEST_PROT_UDP == protocol || TEST_PROT_DTLS == protocol) {
        /*Calc Handle */
        for (index = 0; index < CALC_STREAM_NUMBER; index++) {
            ret = swat_cxt_index_find(&udpRxIndex[index]);

            if (CALC_STREAM_NUMBER_INVALID != ret) {
                swat_cxt_index_configure(&udpRxIndex[index]);
                break;
            }else 			
			{
				if (cxtUdpRxPara[index].param.port == port)
				{
					SWAT_PTF("This port is in use, use another Port.\n");
					return;
				}
            }
        }

        if (CALC_STREAM_NUMBER_INVALID == ret) {
            //SWAT_PTF("Warning udpRxIndex is full\n");
            return;
        }

        /* DataBase Initial */
        swat_database_initial(&cxtUdpRxPara[index]);

        /* Update DataBase */
        swat_database_set(&cxtUdpRxPara[index], INADDR_ANY, NULL, mcastIpAddress, lIp6, mIp6, port,
                          protocol, ssl_inst_index, 1500, 0, 0, 0, TEST_DIR_RX,localIpAddress,0,0,0,iperf_mode,interval, 0);

        /* Server */
        cxtUdpRxPara[index].index = index;
        /* Server */
        ret = swat_task_create(swat_bench_udp_rx, index, 2048+512, 50);
        if (0 == ret) {
            //SWAT_PTF("UDP RX session %d Success\n", index);
        } else {
            swat_cxt_index_free(&udpRxIndex[index]);
        }
    }
#ifdef SWAT_BENCH_RAW
    if (TEST_PROT_RAW == protocol)
    {
    	// Check Raw Protocol
    	for (index = 0; index < CALC_STREAM_NUMBER; index ++)
		{
            ret = swat_cxt_index_find(&rawRxIndex[index]);
			if (CALC_STREAM_NUMBER_INVALID == ret)
			{
				if (cxtRawRxPara[index].param.port == port)
				{
					SWAT_PTF("This port is in use, use another Port.\n");
					return;
				}
			}
			
            ret = swat_cxt_index_find(&rawTxIndex[index]);
			if (CALC_STREAM_NUMBER_INVALID == ret)
			{
				if (cxtRawTxPara[index].param.port == port)
				{
					SWAT_PTF("This port is in use, use another Port.\n");
					return;
				}
			}
		}
		
        /*Calc Handle */
        for (index = 0; index < CALC_STREAM_NUMBER; index ++)
        {
            ret = swat_cxt_index_find(&rawRxIndex[index]);

            if (CALC_STREAM_NUMBER_INVALID != ret)
            {
                swat_cxt_index_configure(&rawRxIndex[index]);
                break;
            }
        }

        if (CALC_STREAM_NUMBER_INVALID == ret)
        {
            //SWAT_PTF("Warning rawRxIndex is full\n");
            return;
        }

        /* DataBase Initial */
        swat_database_initial(&cxtRawRxPara[index]);

        /* Update DataBase */
        swat_database_set(&cxtRawRxPara[index], INADDR_ANY, NULL, mcastIpAddress, NULL, NULL, port, 
            protocol, 0, 1500, 0, 0, 0, TEST_DIR_RX,localIpAddress,ip_hdr_inc,rawmode,0,0,0, 0);

        /* Server */
        cxtRawRxPara[index].index =index;
        /* Server */
	ret = swat_task_create(swat_bench_raw_rx, index, 2048, 50);
	if (0 == ret)
	{
		SWAT_PTF("RAW RX session %d Success\n",index);
	}
    else
       {
           swat_cxt_index_free(&rawRxIndex[index]);
       }
    }

#endif 
}

void swat_socket_close_all(void)
{
    int i;

    for (i = 0; i < CALC_STREAM_NUMBER; i++)
    {
        if (TRUE == tcpTxIndex[i].isUsed)
        {
            swat_cxt_index_free(&tcpTxIndex[i]);
            swat_socket_close(&cxtTcpTxPara[i]);
            swat_buffer_free(&(cxtTcpTxPara[i].param.pktBuff));
        }

        if (TRUE == tcpRxIndex[i].isUsed)
        {
            swat_cxt_index_free(&tcpRxIndex[i]);
            swat_socket_close(&cxtTcpRxPara[i]);
            swat_buffer_free(&(cxtTcpRxPara[i].param.pktBuff));
        }

        if (TRUE == udpTxIndex[i].isUsed)
        {
            swat_cxt_index_free(&udpTxIndex[i]);
            swat_socket_close(&cxtUdpTxPara[i]);
            swat_buffer_free(&(cxtUdpTxPara[i].param.pktBuff));
        }

        if (TRUE == udpRxIndex[i].isUsed)
        {
            swat_cxt_index_free(&udpRxIndex[i]);
            swat_socket_close(&cxtUdpRxPara[i]);
            swat_buffer_free(&(cxtUdpRxPara[i].param.pktBuff));
        }

#ifdef SWAT_BENCH_RAW
        if (TRUE == rawTxIndex[i].isUsed)
        {
            swat_cxt_index_free(&rawTxIndex[i]);
            swat_socket_close(&cxtRawTxPara[i]);
            swat_buffer_free(&(cxtRawTxPara[i].param.pktBuff));
        }

        if (TRUE == rawRxIndex[i].isUsed)
        {
            swat_cxt_index_free(&rawRxIndex[i]);
            swat_socket_close(&cxtRawRxPara[i]);
            swat_buffer_free(&(cxtRawRxPara[i].param.pktBuff));
        }
#endif
    }
    

    return ;
}

A_BOOL swat_sock_close_all_status()
{
    int i;
    A_BOOL ret = TRUE;

    for (i = 0; i < CALC_STREAM_NUMBER; i++)
    {
        if (TRUE == tcpTxIndex[i].isUsed)
        {
	    ret = FALSE;
	    break;
        }

        if (TRUE == tcpRxIndex[i].isUsed)
        {
	    ret = FALSE;
	    break;
        }

        if (TRUE == udpTxIndex[i].isUsed)
        {
	    ret = FALSE;
	    break;
        }

        if (TRUE == udpRxIndex[i].isUsed)
        {
	    ret = FALSE;
	    break;
        }

#ifdef SWAT_BENCH_RAW
        if (TRUE == rawTxIndex[i].isUsed)
        {
	    ret = FALSE;
	    break;
        }

        if (TRUE == rawRxIndex[i].isUsed)
        {
	    ret = FALSE;
	    break;
        }
#endif
    }
    return ret;
    
}
#ifdef SWAT_BENCH_RAW
void swat_raw_tx_handle(STREAM_CXT_t* pStreamCxt)
{
    A_INT32  sendBytes = 0;
    A_UINT32 sumBytes = 0;
    A_UINT32 currentPackets = 0;
    A_UINT8* pDataBuffer = NULL;
    struct sockaddr_in remoteAddr;
    A_UINT32    temp_addr;
    ip_header *iphdr;
    unsigned short int ip_flags[4];
    EOT_PACKET_t eotPacket;
    A_INT32 ret;

    A_INT32 sendTerminalCount = 0;
    A_UINT32 packet_size = pStreamCxt->param.pktSize;

    SWAT_PTR_NULL_CHK(pStreamCxt);

    /* Initial Bench Value */
    swat_bench_quit_init();

    /* Initial Calc & Time */
    pStreamCxt->calc[DEFAULT_FD_INDEX].firstTime.milliseconds = CALC_TIME_DEF;
    pStreamCxt->calc[DEFAULT_FD_INDEX].lastTime.milliseconds  = CALC_TIME_DEF;
    pStreamCxt->calc[DEFAULT_FD_INDEX].bytes  = CALC_BYTES_DEF;
    pStreamCxt->calc[DEFAULT_FD_INDEX].kbytes = CALC_KBYTES_DEF;
    
    /* Malloc Packet Buffer Size */
    pDataBuffer = swat_mem_malloc(pStreamCxt->param.pktSize);
    if (NULL == pDataBuffer)
    {
        SWAT_PTF("malloc error\r\n");
        /* Close Socket */
        swat_socket_close(pStreamCxt);
        return;
    }
    pStreamCxt->param.pktBuff = pDataBuffer;

    temp_addr = pStreamCxt->param.ipAddress; 
    /* Add IP header */
    if(pStreamCxt->param.ip_hdr_inc == 1) 
    {
        pStreamCxt->param.pktSize += 20;
        packet_size += 20;
        if(pStreamCxt->param.pktSize > CFG_PACKET_SIZE_MAX_TX)
        {
                packet_size = CFG_PACKET_SIZE_MAX_TX;
        }
        temp_addr = pStreamCxt->param.local_ipAddress;

    }

    /* Prepare IP address & port */
    memset(&remoteAddr, 0, sizeof( struct sockaddr_in));
    remoteAddr.sin_addr.s_addr = NTOHL(pStreamCxt->param.ipAddress);
    remoteAddr.sin_port        = NTOHS(pStreamCxt->param.port);    
    remoteAddr.sin_family      =  AF_INET;

    /* Initial Packet */
    SWAT_PTF("RAW Remote IP %d.%d.%d.%d prepare OK\r\n",
            (pStreamCxt->param.ipAddress) >> 24 & 0xFF, (pStreamCxt->param.ipAddress) >> 16 & 0xFF, 
            (pStreamCxt->param.ipAddress) >> 8 & 0xFF, (pStreamCxt->param.ipAddress) & 0xFF);
   SWAT_PTF("Sending...\r\n");
    
    /* Get First Time */   
    swat_get_first_time(pStreamCxt, DEFAULT_FD_INDEX);
    if (TEST_MODE_TIME == pStreamCxt->param.mode)
    {
        swat_bench_timer_init(pStreamCxt->param.seconds, pStreamCxt->param.protocol, pStreamCxt->index);
    }
    while(1)
    {
        if (swat_bench_quit())
        {
            //SWAT_PTF("Bench quit!!\r\n");  
            swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
            break;
        }
        
        /* Add IP header */
            if(pStreamCxt->param.ip_hdr_inc == 1) //if raw_mode == iphdr
            {
                  iphdr = (ip_header *)pStreamCxt->param.pktBuff;
                  iphdr->iph_ihl = IPV4_HEADER_LENGTH / sizeof(A_UINT32);
                  iphdr->iph_ver = 4; /* IPv4 */
                  iphdr->iph_tos = 0;
                  iphdr->iph_len = htons(packet_size);
                  iphdr->iph_ident = htons(0);        
                  /* Zero (1 bit) */
                  ip_flags[0] = 0;
                  /* Do not fragment flag (1 bit) */
                  ip_flags[1] = 0;
                  /* More fragments following flag * (1 bit) */
                  ip_flags[2] = 0;
                  /* Fragmentation * offset (13 bits)* */
                  ip_flags[3] = 0;      
                  iphdr->iph_offset = htons((ip_flags[0] << 15) | (ip_flags[1] << 14)| (ip_flags[2] << 13) | ip_flags[3]);
                  /* Time-to-Live (8 bits): default to maximum value */
                  iphdr->iph_ttl = 255;
                  /* Transport layer protocol (8 bits): 17 for UDP ,255 for raw */
                  iphdr->iph_protocol  = pStreamCxt->param.port;//ATH_IP_PROTO_RAW;
                  iphdr->iph_sourceip  = NTOHL(pStreamCxt->param.local_ipAddress);
                  iphdr->iph_destip    = NTOHL(pStreamCxt->param.ipAddress);
                      
              }/*end of ip_hdr_inc == 1 */

                sendBytes = swat_sendto(pStreamCxt->socketLocal, (CHAR*)pDataBuffer, pStreamCxt->param.pktSize,
                                 0, (struct sockaddr *)&remoteAddr, sizeof(struct sockaddr_in));

                if (sendBytes < 0)
                {
                    SWAT_PTF("RAW TX Error %d sumBytes = %d\r\n", sendBytes, sumBytes);
                    /* Free Buffer */
                    qcom_thread_msleep(100);
                }
                else
                {
                    /* bytes & kbytes */
                    sumBytes += sendBytes;
                }

								if (sendBytes > 0){
									swat_bytes_calc(pStreamCxt,sendBytes, DEFAULT_FD_INDEX);
								}

                /* Packets Mode */
                if (TEST_MODE_PACKETS == pStreamCxt->param.mode)
                {      currentPackets++; 
                    if (0 != (sumBytes / pStreamCxt->param.pktSize))                
                    {
                        //currentPackets += (sumBytes / pStreamCxt->param.pktSize);
                        sumBytes = sumBytes % pStreamCxt->param.pktSize;
                    }

                    if (currentPackets >= pStreamCxt->param.numOfPkts)
                    {           
                        break;
                    }
                }
                
                /* Time Mode */
                if (TEST_MODE_TIME == pStreamCxt->param.mode)
                {   
                    /* Get Last Time */   
                    if (0 != benchRawTxTimerStop[pStreamCxt->index])
                    {
                        swat_get_last_time(pStreamCxt, DEFAULT_FD_INDEX);
                        break;
                    }
                }
                      
    } /* end of while(1) */
        
    pStreamCxt->param.ip_hdr_inc = 0;
    #define  IP_HDRINCL        2  /* int; header is included with data */
    ret = swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, IP_HDRINCL, (A_UINT8 *)(&pStreamCxt->param.ip_hdr_inc), sizeof(int));
    if (ret < 0) {
        /* Close Socket */
        SWAT_PTF("SETSOCKOPT err %d.\n", pStreamCxt->socketLocal);
        swat_socket_close(pStreamCxt);
    }
    
    /* Tell ath_console TX is complete end mark is AABBCCDD */
    while(sendTerminalCount <= 10)
    {
        eotPacket.code = 0xAABBCCDD;
        eotPacket.packetCount = currentPackets;
        /* Wait for Output */
        sendBytes = swat_sendto(pStreamCxt->socketLocal, (CHAR*)&eotPacket, 
                  sizeof(EOT_PACKET_t), 0, (struct sockaddr *)&remoteAddr, 
                  sizeof(struct sockaddr_in));
        if (sendBytes < 0)
        {
             SWAT_PTF("RAW TX error %d , retry %d \r\n", sendBytes,sendTerminalCount);
             qcom_thread_msleep(100);
             sendTerminalCount++;
        }
        else
        {
            break;
        }
    }
		swat_test_result_print(pStreamCxt, DEFAULT_FD_INDEX);
    SWAT_PTF("*************IOT Throughput Test Completed **************\n");

    /* Free Buffer */
    swat_buffer_free(&(pStreamCxt->param.pktBuff));
    /*Close socket*/
    swat_socket_close(pStreamCxt);
}
#endif

#ifdef SWAT_BENCH_RAW
void swat_bench_raw_tx_task(STREAM_CXT_t* pStreamCxt)
{

    A_INT32 ret = 0;
    SWAT_PTR_NULL_CHK(pStreamCxt);

    pStreamCxt->socketLocal = swat_socket(AF_INET, SOCK_RAW, pStreamCxt->param.port);
    if (pStreamCxt->socketLocal < 0)
    {
        SWAT_PTF("Open socket error...\r\n");
        goto QUIT;
    }

    #define  IP_HDRINCL        2  /* int; header is included with data */
    /* Set IP_HDRINCL socket option if we need to receive the packet with IP header */ 
   if(pStreamCxt->param.ip_hdr_inc == 1) // if raw mode == iphdr
   {
        ret = swat_setsockopt(pStreamCxt->socketLocal, SOL_SOCKET, IP_HDRINCL, (A_UINT8 *)(&pStreamCxt->param.ip_hdr_inc), sizeof(int));
        if (ret < 0) {
            /* Close Socket */
            SWAT_PTF("Failed to set socket option %d.\n", pStreamCxt->socketLocal);
            swat_socket_close(pStreamCxt);
            goto QUIT;
        }
   }
    /* Packet Handle */
    swat_raw_tx_handle(pStreamCxt);
QUIT:
    /* Free Index */
    swat_cxt_index_free(&rawTxIndex[pStreamCxt->index]);
   // swat_bench_raw_timer_clean(pStreamCxt->index);
    /* Thread Delete */
    swat_task_delete();
}
#endif /* swat_bench_raw_tx_task */

int swat_unblock_socket(int handle)
{
   qcom_unblock_socket(handle);
}


#endif    


